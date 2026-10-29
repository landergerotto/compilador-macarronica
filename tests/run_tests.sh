#!/bin/sh

set -u

compiler=${1:-build/debug/macaronica}
test_cc=${2:-${CC:-gcc}}
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

timeout_command=
if [ -x /usr/bin/timeout ] && /usr/bin/timeout 1 true >/dev/null 2>&1; then
    timeout_command=/usr/bin/timeout
elif command -v timeout >/dev/null 2>&1 && timeout 1 true >/dev/null 2>&1; then
    timeout_command=timeout
fi

run_compiler()
{
    if [ -n "$timeout_command" ]; then
        "$timeout_command" 10 "$compiler" "$@"
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

# O teste de arredondamento precisa inspecionar o valor do token do lexer.
# O executavel fica no diretorio temporario removido pelo trap acima.
rounding_test="$temporary_dir/real_arredondamento.exe"
if "$test_cc" -std=c99 -Wall -Wextra -pedantic -DMACARONICA_LEXER_DEBUG=0 \
    "-I$project_root/src" \
    "$test_root/lexer/real_arredondamento.c" \
    "$project_root/src/common/source.c" "$project_root/src/common/diagnostic.c" \
    "$project_root/src/lexer/token.c" "$project_root/src/lexer/lexer.c" \
    -lm -o "$rounding_test" >"$temporary_dir/rounding-build.out" 2>"$temporary_dir/rounding-build.err"; then
    if [ -s "$temporary_dir/rounding-build.err" ]; then
        sed 's/^/       /' "$temporary_dir/rounding-build.err" >&2
    fi
    if "$rounding_test" >"$temporary_dir/rounding.out" 2>"$temporary_dir/rounding.err"; then
        record_pass 'lexer arredonda literais diretamente para flut'
    else
        record_failure 'lexer arredonda literais diretamente para flut' 'teste C falhou'
        sed 's/^/       /' "$temporary_dir/rounding.err" >&2
    fi
else
    record_failure 'lexer arredonda literais diretamente para flut' 'compilacao do teste C falhou'
    sed 's/^/       /' "$temporary_dir/rounding-build.err" >&2
fi

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

    if ! grep -Eq ":[0-9]+:[0-9]+: ${diagnostic_phase} E-" "$stderr_file"; then
        record_failure "$relative_path" "nao produziu diagnostico localizado da fase $diagnostic_phase"
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
for source_path in "$test_root"/parser/invalido_*.mac; do
    run_source_case "parser/$(basename -- "$source_path")" rejeitado sintatica
done

for source_path in "$test_root"/semantic/valido_*.mac; do
    run_source_case "semantic/$(basename -- "$source_path")" aceito '-'
done

for source_path in "$test_root"/semantic/invalido_*.mac; do
    run_source_case "semantic/$(basename -- "$source_path")" rejeitado semantica
done

precedence_source="$test_root/parser/valido_precedencia_condicao.mac"
run_compiler --dump-ast "$precedence_source" \
    >"$temporary_dir/precedence-ast.txt" 2>"$temporary_dir/precedence-ast.err"
status=$?
if [ "$status" -eq 0 ] && awk '
    /operator=OU / { ou_indent = index($0, "operator=OU"); in_ou = 1; next }
    in_ou && /operator=E / && index($0, "operator=E") > ou_indent { found = 1 }
    END { exit found ? 0 : 1 }
' "$temporary_dir/precedence-ast.txt"; then
    record_pass 'AST preserva precedencia de E sobre OU em condicao'
else
    record_failure 'AST preserva precedencia de E sobre OU em condicao' \
        "codigo $status ou arvore sem OU contendo E"
fi

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

run_compiler --dump-tokens "$sample" >"$temporary_dir/dump-tokens-path.txt" 2>"$temporary_dir/dump-tokens-path.err"
if grep -q "algo.mac:" "$temporary_dir/dump-tokens-path.txt"; then
    record_pass 'dump-tokens usa caminho real do arquivo'
else
    record_failure 'dump-tokens usa caminho real do arquivo' \
        'linhas do dump nao contem o caminho do arquivo-fonte'
fi

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
