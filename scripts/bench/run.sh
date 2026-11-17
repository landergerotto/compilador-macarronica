#!/usr/bin/env bash
# Kernel-only measurements; kernels must originate in the Macaronica compiler.
set -u
set -o pipefail
root=$(CDPATH= cd -- "$(dirname -- "$0")/../.." && pwd)
compiler=${1:-"$root/build/release/macaronica"}
out=${2:-"$root/build/bench"}
repeats=${BENCH_REPEATS:-5}
sizes=${BENCH_SIZES:-"1000000 10000000"}
clang=${LLVM_CLANG:-clang}
pipeline=${BENCH_PIPELINE:-"$root/scripts/pipeline.sh"}
case "$repeats" in ''|*[!0-9]*) echo 'BENCH_REPEATS must be an integer >= 5' >&2; exit 2;; esac
if [ "$repeats" -lt 5 ]; then echo 'At least five measured repetitions required' >&2; exit 2; fi
mkdir -p "$out" || exit 2
out=$(CDPATH= cd -- "$out" && pwd)
case "$compiler" in /*|[A-Za-z]:/*) ;; *) compiler="$(pwd)/$compiler";; esac
printf 'operation,n,variant,threads,status,reason\n' >"$out/status.csv"
printf 'operation,n,variant,threads,sample,seconds,checksum\n' >"$out/samples.csv"
printf 'operation,n,variant,threads,median_seconds,speedup_vs_C0,compile_seconds\n' >"$out/summary.csv"
printf 'operation,n,variant,compile_seconds\n' >"$out/compile.csv"
printf 'operation,n,variant,vector_ir,openmp_ir\n' >"$out/transformations.csv"
{
    date -u '+UTC=%Y-%m-%dT%H:%M:%SZ'
    uname -a
    printf 'PROCESSOR_IDENTIFIER=%s\nNUMBER_OF_PROCESSORS=%s\n' "${PROCESSOR_IDENTIFIER:-unknown}" "${NUMBER_OF_PROCESSORS:-unknown}"
    printf 'compiler=%s\nclang=%s\npipeline=%s\nrepeats=%s\nsizes=%s\n' "$compiler" "$clang" "$pipeline" "$repeats" "$sizes"
    "$clang" --version 2>&1 || :
    command -v mlir-opt >/dev/null 2>&1 && mlir-opt --version
    command -v lscpu >/dev/null 2>&1 && lscpu
    for level in LEVEL1_DCACHE_SIZE LEVEL2_CACHE_SIZE LEVEL3_CACHE_SIZE; do
        printf '%s=' "$level"; getconf "$level" 2>/dev/null || printf 'unknown\n'
    done
} >"$out/environment.txt"
cores=$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')
case "$cores" in ''|*[!0-9]*) cores=1;; esac
thread_counts=()
t=1
while [ "$t" -le "$cores" ]; do thread_counts+=("$t"); t=$((t * 2)); done
# Permit an explicit smaller reproducible subset, never oversubscribe by default.
if [ -n "${BENCH_THREADS:-}" ]; then read -r -a thread_counts <<<"$BENCH_THREADS"; fi
for t in "${thread_counts[@]}"; do
    case "$t" in ''|*[!0-9]*) echo 'Invalid BENCH_THREADS' >&2; exit 2;; esac
    if [ "$t" -lt 1 ] || [ "$t" -gt "$cores" ]; then echo 'Thread count outside logical CPU range' >&2; exit 2; fi
done
stamp() { date +%s%N; }
skipped=0
skip() { skipped=$((skipped + 1)); printf '%s,%s,%s,%s,skipped,%s\n' "$1" "$2" "$3" "$4" "$5" >>"$out/status.csv"; }
failed=0
for operation in add dot; do
  for n in $sizes; do
    case "$n" in 1000000|10000000) ;; *) echo 'Supported sizes: 1000000 10000000' >&2; exit 2;; esac
    folder="$out/$operation-$n"
    mkdir -p "$folder"
    sed "s/BENCHSIZE/$n/g" "$root/scripts/bench/fixtures/$operation.mac.in" >"$folder/input.mac"
    available=1
    if [ ! -x "$compiler" ]; then reason=compiler_unavailable; available=0
    elif ! command -v "$clang" >/dev/null 2>&1; then reason=clang_unavailable; available=0
    elif [ ! -f "$pipeline" ]; then reason=mlir_pipeline_unavailable; available=0
    elif ! "$compiler" --emit-mlir "$folder/input.mac" -o "$folder/kernels.mlir" >"$folder/frontend.stdout" 2>"$folder/frontend.stderr"; then
        reason=compiler_emit_mlir_failed; available=0; failed=1
    fi
    if [ "$available" = 0 ]; then
        for variant in C0 C1 C2 C3; do skip "$operation" "$n" "$variant" 1 "$reason"; done
        continue
    fi
    # Each fixture contains exactly one DSL kernel. A bare-pointer ABI symbol
    # must be an ordinary C identifier. Do not rewrite or reimplement its body.
    kernel=$(sed -n 's/.*func\.func[[:space:]]\+@\(mac_vec_[A-Za-z0-9_]*\).*/\1/p' "$folder/kernels.mlir")
    if ! [[ "$kernel" =~ ^mac_vec_[A-Za-z0-9_]+$ ]]; then
        for variant in C0 C1 C2 C3; do skip "$operation" "$n" "$variant" 1 ambiguous_kernel_symbol; done
        failed=1; continue
    fi
    baseline=''
    for variant in C0 C1 C2 C3; do
        mode=scalar
        flags=(-O0 -fno-vectorize -fno-slp-vectorize)
        threads=(1)
        case "$variant" in
          C1) flags=(-O2 -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize);;
          C2) mode=vector; flags=(-O2 -fno-vectorize -fno-slp-vectorize);;
          C3) mode=parallel; flags=(-O2 -fopenmp); threads=("${thread_counts[@]}");;
        esac
        begin=$(stamp)
        printf '%q ' bash "$pipeline" "$folder/kernels.mlir" "$folder/$variant.ll" "$mode" >"$folder/$variant.command.txt"
        printf '\n' >>"$folder/$variant.command.txt"
        if ! bash "$pipeline" "$folder/kernels.mlir" "$folder/$variant.ll" "$mode" >"$folder/$variant.pipeline.stdout" 2>"$folder/$variant.pipeline.stderr"; then
            for t in "${threads[@]}"; do skip "$operation" "$n" "$variant" "$t" pipeline_failed_see_log; done
            continue
        fi
        vector_ir=no
        openmp_ir=no
        if grep -Eq '<[[:space:]]*[0-9]+[[:space:]]+x[[:space:]]' "$folder/$variant.ll"; then vector_ir=yes; fi
        if grep -Eq '@(__kmpc_|GOMP_)' "$folder/$variant.ll"; then openmp_ir=yes; fi
        printf '%s,%s,%s,%s,%s\n' "$operation" "$n" "$variant" "$vector_ir" "$openmp_ir" >>"$out/transformations.csv"
        if { [ "$variant" = C2 ] && [ "$vector_ir" != yes ]; } ||
           { [ "$variant" = C3 ] && [ "$openmp_ir" != yes ]; }; then
            for t in "${threads[@]}"; do skip "$operation" "$n" "$variant" "$t" requested_transformation_not_observed; done
            failed=1; continue
        fi
        defines=(-DBENCH_N="$n" -DBENCH_KERNEL="$kernel")
        [ "$operation" != add ] || defines+=(-DBENCH_ADD)
        printf '%q ' "$clang" "${flags[@]}" -c "$folder/$variant.ll" -o "$folder/$variant.o" >>"$folder/$variant.command.txt"
        printf '\n' >>"$folder/$variant.command.txt"
        printf '%q ' "$clang" -std=c99 -Wall -Wextra -pedantic -O2 "${defines[@]}" -c "$root/scripts/bench/driver.c" -o "$folder/driver.o" >>"$folder/$variant.command.txt"
        printf '\n' >>"$folder/$variant.command.txt"
        printf '%q ' "$clang" "${flags[@]}" "$folder/$variant.o" "$folder/driver.o" -o "$folder/$variant.exe" >>"$folder/$variant.command.txt"
        printf '\n' >>"$folder/$variant.command.txt"
        if ! "$clang" "${flags[@]}" -c "$folder/$variant.ll" -o "$folder/$variant.o" >"$folder/$variant.build.stdout" 2>"$folder/$variant.build.stderr" ||
           ! "$clang" -std=c99 -Wall -Wextra -pedantic -O2 "${defines[@]}" -c "$root/scripts/bench/driver.c" -o "$folder/driver.o" >>"$folder/$variant.build.stdout" 2>>"$folder/$variant.build.stderr" ||
           ! "$clang" "${flags[@]}" "$folder/$variant.o" "$folder/driver.o" -o "$folder/$variant.exe" >>"$folder/$variant.build.stdout" 2>>"$folder/$variant.build.stderr"; then
            for t in "${threads[@]}"; do skip "$operation" "$n" "$variant" "$t" clang_or_openmp_link_failed_see_log; done
            continue
        fi
        end=$(stamp)
        compile_seconds=$(awk -v a="$begin" -v b="$end" 'BEGIN { printf "%.9f", (b-a)/1e9 }')
        printf '%s,%s,%s,%s\n' "$operation" "$n" "$variant" "$compile_seconds" >>"$out/compile.csv"
        for t in "${threads[@]}"; do
            raw="$folder/$variant-t$t.csv"
            if ! OMP_NUM_THREADS="$t" OMP_DYNAMIC=FALSE "$folder/$variant.exe" "$repeats" >"$raw" 2>"$folder/$variant-t$t.stderr"; then
                printf '%s,%s,%s,%s,failed,execution_or_correctness_failed\n' "$operation" "$n" "$variant" "$t" >>"$out/status.csv"
                failed=1; continue
            fi
            median=$(tail -n +2 "$raw" | cut -d, -f2 | sort -g | awk '{ a[NR]=$1 } END { if (NR%2) printf "%.12f", a[(NR+1)/2]; else printf "%.12f", (a[NR/2]+a[NR/2+1])/2 }')
            if [ "$variant" = C0 ]; then baseline=$median; fi
            speedup=$(awk -v b="$baseline" -v m="$median" 'BEGIN { if (b!="" && m>0) printf "%.6f", b/m; else printf "NA" }')
            tail -n +2 "$raw" | awk -v op="$operation" -v n="$n" -v v="$variant" -v t="$t" '{ print op "," n "," v "," t "," $0 }' >>"$out/samples.csv"
            printf '%s,%s,%s,%s,%s,%s,%s\n' "$operation" "$n" "$variant" "$t" "$median" "$speedup" "$compile_seconds" >>"$out/summary.csv"
            printf '%s,%s,%s,%s,measured,correct\n' "$operation" "$n" "$variant" "$t" >>"$out/status.csv"
            printf '%s N=%s %s threads=%s median=%ss speedup=%s\n' "$operation" "$n" "$variant" "$t" "$median" "$speedup"
        done
    done
  done
done
awk -F, -f "$root/scripts/bench/chart.awk" "$out/summary.csv" >"$out/medians.svg"
printf 'Results: %s\n' "$out"
if [ "$(wc -l <"$out/summary.csv")" -le 1 ]; then
    echo 'No measurements available; inspect status.csv and logs.' >&2
    exit 3
fi
if [ "$skipped" -gt 0 ]; then exit 1; fi
exit "$failed"
