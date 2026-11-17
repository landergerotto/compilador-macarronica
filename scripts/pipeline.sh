#!/usr/bin/env bash
# Lowering dos nucleos DSL; o modulo LLVM host e ligado separadamente.
# Uso: pipeline.sh entrada.mlir saida.ll [scalar|vector|parallel]
set -euo pipefail
if [[ $# -lt 2 || $# -gt 3 ]]; then
    echo 'uso: pipeline.sh entrada.mlir saida.ll [scalar|vector|parallel]' >&2
    exit 2
fi
input=$1
output=$2
mode=${3:-scalar}
case "$mode" in scalar|vector|parallel) ;; *) echo "modo invalido: $mode" >&2; exit 2;; esac
mlir_opt=${MLIR_OPT:-${MLIR_BINDIR:+$MLIR_BINDIR/}mlir-opt}
mlir_translate=${MLIR_TRANSLATE:-${MLIR_BINDIR:+$MLIR_BINDIR/}mlir-translate}
llvm_as=${LLVM_AS:-${LLVM_BINDIR:+$LLVM_BINDIR/}llvm-as}
llvm_opt=${LLVM_OPT:-${LLVM_BINDIR:+$LLVM_BINDIR/}opt}
for tool in "$mlir_opt" "$mlir_translate" "$llvm_as" "$llvm_opt"; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "ferramenta ausente: $tool (configure MLIR_BINDIR/LLVM_BINDIR ou variaveis individuais)" >&2
        exit 1
    fi
done
mkdir -p -- "$(dirname -- "$output")"
stages="${output%.ll}.stages"
mkdir -p -- "$stages"
{
    "$mlir_opt" --version
    "$mlir_translate" --version
    "$llvm_opt" --version
} > "$stages/versions.txt"

# 1. Verifica o modulo de alto nivel. Preserva o primeiro nivel para arguição.
"$mlir_opt" "$input" --verify-each -o "$stages/01-affine.mlir"

# 2. C2 vetoriza loops affine e reducoes inteiras modulares (associativas).
#    Reducoes reais sao emitidas como scf.for: nao sao tocadas pelos passes
#    affine nem recebem fastmath. C3 paraleliza loops/reducoes legais affine.
case "$mode" in
    scalar)
        "$mlir_opt" "$stages/01-affine.mlir" --lower-affine \
            --verify-each -o "$stages/02-scf.mlir"
        ;;
    vector)
        "$mlir_opt" "$stages/01-affine.mlir" \
            --affine-super-vectorize='virtual-vector-size=8 vectorize-reductions=true' \
            --lower-affine --verify-each -o "$stages/02-vector-scf.mlir"
        cp -- "$stages/02-vector-scf.mlir" "$stages/02-scf.mlir"
        ;;
    parallel)
        "$mlir_opt" "$stages/01-affine.mlir" \
            --affine-parallelize='max-nested=1 parallel-reductions=true' --lower-affine \
            --verify-each -o "$stages/02-scf.mlir"
        ;;
esac

# 3. Transferencias vectoriais sao materializadas e os loops SCF paralelos
#    tornam-se OpenMP. canonicalize limpa a forma de suporte do OpenMP.
if [[ "$mode" == parallel ]]; then
    "$mlir_opt" "$stages/02-scf.mlir" --convert-scf-to-openmp --canonicalize \
        --verify-each -o "$stages/03-openmp.mlir"
    lowered_input="$stages/03-openmp.mlir"
else
    lowered_input="$stages/02-scf.mlir"
fi

# 4. Lowering estrutural para o dialeto LLVM. Memrefs estaticos chegam ao ABI
#    como ponteiros simples; tamanho/stride sao reconstruidos do tipo estatico.
#    Isso corresponde aos argumentos ptr emitidos pelo backend LLVM host.
extra=()
if [[ "$mode" == parallel ]]; then extra+=(--convert-openmp-to-llvm); fi
"$mlir_opt" "$lowered_input" --convert-vector-to-scf --convert-scf-to-cf \
    --convert-vector-to-llvm --convert-arith-to-llvm --convert-index-to-llvm \
    --finalize-memref-to-llvm \
    --convert-func-to-llvm='use-bare-ptr-memref-call-conv=1' \
    --convert-cf-to-llvm "${extra[@]}" --reconcile-unrealized-casts \
    --canonicalize --verify-each -o "$stages/04-llvm.mlir"

# 5. Traduz dialeto LLVM em LLVM IR; valida antes de publicar a saida final.
"$mlir_translate" --mlir-to-llvmir "$stages/04-llvm.mlir" -o "$stages/final.ll"
"$llvm_as" "$stages/final.ll" -o "$stages/final.bc"
"$llvm_opt" -passes=verify "$stages/final.bc" -disable-output
cp -- "$stages/final.ll" "$output"
echo "MLIR $mode -> $output (niveis e versoes: $stages)"
