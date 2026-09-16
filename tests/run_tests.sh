#!/bin/sh

set -u

compiler=${1:-build/debug/macaronica}
test_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(CDPATH= cd -- "$test_root/.." && pwd)

case "$compiler" in
    /* | [A-Za-z]:/*) ;;
    *) compiler="$project_root/$compiler" ;;
esac

if [ ! -x "$compiler" ]; then
    printf 'ERRO: compilador nao encontrado ou nao executavel: %s\n' "$compiler" >&2
    exit 1
fi

temporary_base="$project_root/build"
mkdir -p -- "$temporary_base" || exit 1
temporary_dir=$(mktemp -d "$temporary_base/test-tmp.XXXXXX") || exit 1
trap 'rm -rf -- "$temporary_dir"' EXIT HUP INT TERM

passed=0
failed=0

run_compiler()
{
    if command -v timeout >/dev/null 2>&1; then
        timeout 10 "$compiler" "$@"
    else
        "$compiler" "$@"
    fi
}

record_pass()
{
    passed=$((passed + 1))
    printf 'OK   %s\n' "$1"
}

record_failure()
{
    failed=$((failed + 1))
    printf 'FALHA %s: %s\n' "$1" "$2" >&2
}

run_source_case()
{
    relative_path=$1
    expectation=$2
    expected_phase=$3
    source_path="$test_root/$relative_path"
    stdout_file="$temporary_dir/stdout.txt"
    stderr_file="$temporary_dir/stderr.txt"

    run_compiler "$source_path" >"$stdout_file" 2>"$stderr_file"
    status=$?

    if [ "$expectation" = aceito ]; then
        if [ "$status" -ne 0 ]; then
            record_failure "$relative_path" "esperava codigo 0, recebeu $status"
            sed 's/^/       /' "$stderr_file" >&2
            return
        fi
        if grep -Eq ': (lexico|sintatico|semantico) E-' "$stderr_file"; then
            record_failure "$relative_path" 'fonte aceita produziu diagnostico de erro'
            return
        fi
        record_pass "$relative_path"
        return
    fi

    if [ "$status" -ne 1 ]; then
        record_failure "$relative_path" "esperava codigo 1, recebeu $status"
        sed 's/^/       /' "$stderr_file" >&2
        return
    fi

    case "$expected_phase" in
        lexica) diagnostic_phase=lexico ;;
        sintatica) diagnostic_phase=sintatico ;;
        semantica) diagnostic_phase=semantico ;;
        *) diagnostic_phase=$expected_phase ;;
    esac

    if ! grep -Eq ": ${diagnostic_phase} E-" "$stderr_file"; then
        record_failure "$relative_path" "nao produziu diagnostico da fase $diagnostic_phase"
        sed 's/^/       /' "$stderr_file" >&2
        return
    fi
    record_pass "$relative_path"
}

while IFS="$(printf '\t')" read -r relative_path expectation phase category; do
    [ "$relative_path" = arquivo ] && continue
    [ -z "$relative_path" ] && continue
    run_source_case "$relative_path" "$expectation" "$phase"
done < "$test_root/manifest.tsv"

run_source_case 'integration/valido_minimo.mac' aceito '-'
run_source_case 'lexer/invalido_real_truncado.mac' rejeitado lexica
run_source_case 'parser/invalido_truncado_bloco.mac' rejeitado sintatica

for source_path in "$test_root"/semantic/valido_*.mac; do
    run_source_case "semantic/$(basename -- "$source_path")" aceito '-'
done

for source_path in "$test_root"/semantic/invalido_*.mac; do
    run_source_case "semantic/$(basename -- "$source_path")" rejeitado semantica
done

deep_source="$temporary_dir/profundidade_blocos.mac"
{
    printf 'vazio principal() @\n[\n'
    depth=0
    while [ "$depth" -le 512 ]; do
        printf 'cond (1 < 2) edai\n[\n'
        depth=$((depth + 1))
    done
    printf 'respost;\n'
    while [ "$depth" -gt 0 ]; do
        printf ']\n'
        depth=$((depth - 1))
    done
    printf ']\n'
} > "$deep_source"
run_compiler "$deep_source" >"$temporary_dir/deep.txt" 2>"$temporary_dir/deep.err"
status=$?
if [ "$status" -eq 1 ] && grep -q ': sintatico E-SIN-009:' "$temporary_dir/deep.err"; then
    record_pass 'parser limita profundidade de blocos'
else
    record_failure 'parser limita profundidade de blocos' \
        "esperava codigo 1 e E-SIN-009, recebeu $status"
fi

sample="$project_root/examples/algo.mac"
for option in --dump-tokens --dump-ast --dump-symbols; do
    run_compiler "$option" "$sample" >"$temporary_dir/dump-1.txt" 2>"$temporary_dir/dump-1.err"
    first_status=$?
    run_compiler "$option" "$sample" >"$temporary_dir/dump-2.txt" 2>"$temporary_dir/dump-2.err"
    second_status=$?
    first_dump=$(cat "$temporary_dir/dump-1.txt")
    second_dump=$(cat "$temporary_dir/dump-2.txt")
    if [ "$first_status" -eq 0 ] && [ "$second_status" -eq 0 ] &&
       [ -s "$temporary_dir/dump-1.txt" ] &&
       [ "$first_dump" = "$second_dump" ]; then
        record_pass "dump deterministico $option"
    else
        record_failure "dump $option" 'falhou ou mudou entre duas execucoes'
    fi
done

run_compiler --help >"$temporary_dir/help.txt" 2>"$temporary_dir/help.err"
status=$?
if [ "$status" -eq 0 ] && grep -q '^Uso:' "$temporary_dir/help.txt"; then
    record_pass 'CLI --help'
else
    record_failure 'CLI --help' "codigo $status ou texto inesperado"
fi

run_compiler --version >"$temporary_dir/version.txt" 2>"$temporary_dir/version.err"
status=$?
if [ "$status" -eq 0 ] && grep -q '^Macaronica ' "$temporary_dir/version.txt"; then
    record_pass 'CLI --version'
else
    record_failure 'CLI --version' "codigo $status ou texto inesperado"
fi

run_compiler >"$temporary_dir/no-args.txt" 2>"$temporary_dir/no-args.err"
status=$?
if [ "$status" -eq 2 ]; then
    record_pass 'CLI sem arquivo'
else
    record_failure 'CLI sem arquivo' "esperava codigo 2, recebeu $status"
fi

run_compiler "$temporary_dir/ausente.mac" >"$temporary_dir/missing.txt" 2>"$temporary_dir/missing.err"
status=$?
if [ "$status" -eq 3 ]; then
    record_pass 'CLI arquivo ausente'
else
    record_failure 'CLI arquivo ausente' "esperava codigo 3, recebeu $status"
fi

printf '\nResultado: %d passou/passaram; %d falhou/falharam.\n' "$passed" "$failed"
[ "$failed" -eq 0 ]
