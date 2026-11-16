#!/bin/sh

set -u

compiler=${1:-build/debug/macaronica}
test_cc=${2:-${CC:-gcc}}
test_scope=${3:-all}
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
# GCC do Windows consulta TMP/TEMP antes do temporario do shell.
if command -v cygpath >/dev/null 2>&1; then
    native_temporary_dir=$(cygpath -w "$temporary_dir")
else
    native_temporary_dir=$temporary_dir
fi
TMPDIR=$native_temporary_dir
TMP=$native_temporary_dir
TEMP=$native_temporary_dir
export TMPDIR TMP TEMP

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

run_program()
{
    if [ -n "$timeout_command" ]; then
        "$timeout_command" 10 "$@"
    else
        "$@"
    fi
}

run_lli()
{
    source_ir=$1
    linked_ir=$2
    if [ "$lli_runtime_mode" = dll ]; then
        run_program "$llvm_lli" --entry-function=zl_principal \
            "--dlopen=$runtime_dll_native" "$source_ir"
    else
        run_program "$llvm_lli" "$linked_ir"
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

# run_source_case CAMINHO aceito|rejeitado FASE [ERROS]
# ERROS e a quantidade exata de diagnosticos de erro esperada (padrao 1 para
# rejeitado). Conferir a quantidade, e nao so a existencia, e o que pega
# erros em cascata: um erro real deve gerar um diagnostico (D-018).
run_source_case()
{
    relative_path=$1
    expectation=$2
    expected_phase=$3
    expected_errors=${4:-1}
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
    error_count=$(grep -Ec ': (lexico|sintatico|semantico) E-' "$stderr_file")
    if [ "$error_count" -ne "$expected_errors" ]; then
        record_failure "$relative_path" \
            "esperava $expected_errors diagnostico(s) de erro, recebeu $error_count"
        sed 's/^/       /' "$stderr_file" >&2
        return
    fi
    record_pass "$relative_path"
}

# Colunas do manifesto: arquivo, esperado, fase, erros, categoria.
while IFS="$(printf '\t')" read -r relative_path expectation phase errors category; do
    [ "$relative_path" = arquivo ] && continue
    [ -z "$relative_path" ] && continue
    run_source_case "$relative_path" "$expectation" "$phase" "$errors"
done < "$test_root/manifest.tsv"

run_source_case 'integration/valido_minimo.mac' aceito '-'
for source_path in "$test_root"/lexer/invalido_*.mac; do
    run_source_case "lexer/$(basename -- "$source_path")" rejeitado lexica
done
for source_path in "$test_root"/parser/invalido_*.mac; do
    run_source_case "parser/$(basename -- "$source_path")" rejeitado sintatica
done

for source_path in "$test_root"/semantic/valido_*.mac; do
    run_source_case "semantic/$(basename -- "$source_path")" aceito '-'
done

for source_path in "$test_root"/semantic/invalido_*.mac; do
    run_source_case "semantic/$(basename -- "$source_path")" rejeitado semantica
done

# Casos distribuidos pelo professor (extensao .mcn): todos devem ser aceitos.
for source_path in "$test_root"/professor/*.mcn; do
    run_source_case "professor/$(basename -- "$source_path")" aceito '-'
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

chain_source="$temporary_dir/cadeia_operadores.mac"
{
    printf 'vazio principal() @\n[\n    inteira x;\n    x receba 1'
    link=0
    while [ "$link" -le 512 ]; do
        printf '+1'
        link=$((link + 1))
    done
    printf ';\n    respost;\n]\n'
} > "$chain_source"
run_compiler "$chain_source" >"$temporary_dir/chain.txt" 2>"$temporary_dir/chain.err"
status=$?
if [ "$status" -eq 1 ] && grep -q ': sintatico E-SIN-009:' "$temporary_dir/chain.err"; then
    record_pass 'parser limita cadeia de operadores'
else
    record_failure 'parser limita cadeia de operadores' \
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

# Regressao: no Windows o tmpfile() do msvcrt criava o temporario na raiz do
# drive do diretorio atual (C:\ sem permissao de escrita) e a compilacao
# falhava fora do D:. Compila a partir de / e confere que nada sobra ao lado.
(cd / && run_compiler "$project_root/tests/professor/nivel1_soma.mcn" \
    -o "$temporary_dir/outro-cwd.ll") >"$temporary_dir/outro-cwd.txt" 2>"$temporary_dir/outro-cwd.err"
status=$?
if [ "$status" -eq 0 ] && [ -s "$temporary_dir/outro-cwd.ll" ] && [ ! -e "$temporary_dir/outro-cwd.ll.tmp" ]; then
    record_pass 'CLI compila a partir de outro diretorio'
else
    record_failure 'CLI compila a partir de outro diretorio' "codigo $status: $(head -n 1 "$temporary_dir/outro-cwd.err")"
fi

# Erro de fonte nao pode destruir uma saida que ja existia nem deixar .tmp.
printf 'saida anterior\n' >"$temporary_dir/preserva.ll"
run_compiler "$project_root/tests/invalidos/lexico_igual_isolado.mac" -o "$temporary_dir/preserva.ll" \
    >"$temporary_dir/preserva.txt" 2>"$temporary_dir/preserva.err"
status=$?
if [ "$status" -eq 1 ] && [ "$(cat "$temporary_dir/preserva.ll")" = 'saida anterior' ] \
    && [ ! -e "$temporary_dir/preserva.ll.tmp" ]; then
    record_pass 'CLI erro de fonte preserva saida existente'
else
    record_failure 'CLI erro de fonte preserva saida existente' "codigo $status"
fi

if [ "$test_scope" != frontend ]; then
    llvm_bindir=${LLVM_BINDIR:-}
    if [ -n "$llvm_bindir" ]; then
        llvm_as=${LLVM_AS:-$llvm_bindir/llvm-as}
        llvm_opt=${LLVM_OPT:-$llvm_bindir/opt}
        llvm_lli=${LLVM_LLI:-$llvm_bindir/lli}
        llvm_clang=${LLVM_CLANG:-$llvm_bindir/clang}
        llvm_link=${LLVM_LINK:-$llvm_bindir/llvm-link}
    else
        llvm_as=${LLVM_AS:-llvm-as}
        llvm_opt=${LLVM_OPT:-opt}
        llvm_lli=${LLVM_LLI:-lli}
        llvm_clang=${LLVM_CLANG:-clang}
        llvm_link=${LLVM_LINK:-llvm-link}
    fi

    tools_ready=1
    for tool in "$llvm_as" "$llvm_opt" "$llvm_lli" "$llvm_clang" "$llvm_link"; do
        if ! command -v "$tool" >/dev/null 2>&1; then
            record_failure 'dependencias LLVM' "ferramenta indisponivel: $tool (configure LLVM_BINDIR ou LLVM_AS/LLVM_OPT/LLVM_LLI/LLVM_CLANG)"
            tools_ready=0
        fi
    done

    # Falhas anteriores de qualquer fase nao podem deixar um .ll final.
    for invalid in invalidos/lexico_simbolo_invalido.mac \
                   invalidos/sintatico_ponto_virgula_ausente.mac \
                   invalidos/semantico_nao_declarado.mac; do
        invalid_ir="$temporary_dir/invalid.ll"
        run_compiler "$test_root/$invalid" -o "$invalid_ir" \
            >"$temporary_dir/invalid.out" 2>"$temporary_dir/invalid.err"
        invalid_status=$?
        if [ "$invalid_status" -eq 1 ] && [ ! -e "$invalid_ir" ] &&
           grep -Eq ':[0-9]+:[0-9]+: (lexico|sintatico|semantico) E-' "$temporary_dir/invalid.err"; then
            record_pass "-o rejeita $invalid sem .ll"
        else
            record_failure "-o rejeita $invalid sem .ll" "codigo $invalid_status ou arquivo residual/diagnostico ausente"
        fi
    done

    if [ "$tools_ready" -eq 1 ]; then
        runtime_bc="$temporary_dir/zl_runtime_posix.bc"
        if ! "$llvm_clang" -std=c99 -O0 -emit-llvm -c \
            "$project_root/runtime/zl_runtime_posix.c" -o "$runtime_bc" \
            >"$temporary_dir/runtime-build.out" 2>"$temporary_dir/runtime-build.err"; then
            record_failure 'runtime bitcode' 'clang nao compilou runtime POSIX para bitcode'
            sed 's/^/       /' "$temporary_dir/runtime-build.err" >&2
            tools_ready=0
        fi
    fi
    lli_runtime_mode=bitcode
    if [ "$tools_ready" -eq 1 ] && command -v cygpath >/dev/null 2>&1; then
        lli_runtime_mode=dll
        runtime_dll="$temporary_dir/zl_runtime_posix.dll"
        if ! "$llvm_clang" -std=c99 -O0 -shared -DZL_RUNTIME_NO_MAIN \
            "$project_root/runtime/zl_runtime_posix.c" -lm \
            '-Wl,--export-all-symbols' -o "$runtime_dll" \
            >"$temporary_dir/runtime-dll.out" 2>"$temporary_dir/runtime-dll.err"; then
            record_failure 'runtime DLL para lli' 'clang nao compilou runtime compartilhado'
            sed 's/^/       /' "$temporary_dir/runtime-dll.err" >&2
            tools_ready=0
        fi
        runtime_dll_native=$(cygpath -m "$runtime_dll")
    fi
    if [ "$tools_ready" -eq 1 ]; then
        for source_path in "$test_root"/execucao/*.mac; do
            case_name=$(basename -- "$source_path" .mac)
            label="execucao/$case_name"
            ir="$temporary_dir/$case_name.ll"
            bc="$temporary_dir/$case_name.bc"
            linked_bc="$temporary_dir/$case_name-linked.bc"
            native="$temporary_dir/$case_name.exe"
            expected="$test_root/execucao/$case_name.esperado"
            input="$test_root/execucao/$case_name.in"
            status_file="$test_root/execucao/$case_name.status"
            expected_status=0
            if [ -f "$status_file" ]; then expected_status=$(cat "$status_file"); fi
            if [ ! -f "$expected" ]; then
                record_failure "$label" 'arquivo .esperado ausente'
                continue
            fi
            run_compiler "$source_path" -o "$ir" >"$temporary_dir/codegen.out" 2>"$temporary_dir/codegen.err"
            if [ "$?" -ne 0 ] || [ ! -s "$ir" ]; then
                record_failure "$label" 'compilador falhou ou nao gerou .ll'
                sed 's/^/       /' "$temporary_dir/codegen.err" >&2
                continue
            fi
            if ! grep -q 'define i32 @zl_principal(' "$ir" ||
               grep -Eq '@(printf|scanf)\(|^(target triple|target datalayout)' "$ir"; then
                record_failure "$label" 'IR sem @zl_principal ou acoplada a libc/target'
                continue
            fi
            if ! "$llvm_as" "$ir" -o "$bc" >"$temporary_dir/as.out" 2>"$temporary_dir/as.err" ||
               ! "$llvm_opt" -passes=verify "$ir" -disable-output \
                   >"$temporary_dir/verify.out" 2>"$temporary_dir/verify.err" ||
               ! "$llvm_link" "$bc" "$runtime_bc" -o "$linked_bc" \
                   >"$temporary_dir/link.out" 2>"$temporary_dir/link.err"; then
                record_failure "$label" 'llvm-as ou opt verify rejeitou a IR'
                sed 's/^/       /' "$temporary_dir/as.err" "$temporary_dir/verify.err" "$temporary_dir/link.err" >&2
                continue
            fi
            if ! "$llvm_clang" -Wno-override-module "$ir" "$project_root/runtime/zl_runtime_posix.c" -lm -o "$native" \
                >"$temporary_dir/clang.out" 2>"$temporary_dir/clang.err"; then
                record_failure "$label" 'clang + runtime falhou'
                sed 's/^/       /' "$temporary_dir/clang.err" >&2
                continue
            fi
            if [ -f "$input" ]; then
                run_lli "$ir" "$linked_bc" \
                    <"$input" >"$temporary_dir/lli.out" 2>"$temporary_dir/lli.err"
            else
                run_lli "$ir" "$linked_bc" \
                    </dev/null >"$temporary_dir/lli.out" 2>"$temporary_dir/lli.err"
            fi
            lli_status=$?
            if [ -f "$input" ]; then
                run_program "$native" <"$input" >"$temporary_dir/native.out" 2>"$temporary_dir/native.err"
            else
                run_program "$native" </dev/null >"$temporary_dir/native.out" 2>"$temporary_dir/native.err"
            fi
            native_status=$?
            sed 's/\r$//' "$temporary_dir/lli.out" >"$temporary_dir/lli-normalized.out"
            sed 's/\r$//' "$temporary_dir/native.out" >"$temporary_dir/native-normalized.out"
            if [ "$lli_status" -ne "$expected_status" ] || [ "$native_status" -ne "$expected_status" ] ||
               ! cmp -s "$expected" "$temporary_dir/lli-normalized.out" ||
               ! cmp -s "$expected" "$temporary_dir/native-normalized.out"; then
                record_failure "$label" "lli=$lli_status nativo=$native_status esperado=$expected_status ou stdout difere"
                diff -u "$expected" "$temporary_dir/lli-normalized.out" >&2 || :
                diff -u "$expected" "$temporary_dir/native-normalized.out" >&2 || :
                sed 's/^/       lli: /' "$temporary_dir/lli.err" >&2
                sed 's/^/       nativo: /' "$temporary_dir/native.err" >&2
            else
                record_pass "$label (llvm-as, verify, lli, clang)"
            fi
        done

        for source_path in "$test_root"/execucao/erros/*.mac; do
            case_name=$(basename -- "$source_path" .mac)
            error_code=$(printf '%s' "$case_name" | cut -c2)
            label="execucao/erros/$case_name"
            ir="$temporary_dir/erro-$case_name.ll"
            bc="$temporary_dir/erro-$case_name.bc"
            linked_bc="$temporary_dir/erro-$case_name-linked.bc"
            native="$temporary_dir/erro-$case_name.exe"
            run_compiler "$source_path" -o "$ir" >"$temporary_dir/error-build.out" 2>"$temporary_dir/error-build.err"
            if [ "$?" -ne 0 ] || [ ! -s "$ir" ] ||
               ! "$llvm_as" "$ir" -o "$bc" ||
               ! "$llvm_opt" -passes=verify "$ir" -disable-output ||
               ! "$llvm_link" "$bc" "$runtime_bc" -o "$linked_bc" ||
               ! "$llvm_clang" -Wno-override-module "$ir" "$project_root/runtime/zl_runtime_posix.c" -lm -o "$native"; then
                record_failure "$label" 'compilacao, verificacao ou link falhou'
                sed 's/^/       /' "$temporary_dir/error-build.err" >&2
                continue
            fi
            run_lli "$ir" "$linked_bc" </dev/null \
                >"$temporary_dir/lli-error.out" 2>"$temporary_dir/lli-error.err"
            lli_status=$?
            run_program "$native" </dev/null \
                >"$temporary_dir/native-error.out" 2>"$temporary_dir/native-error.err"
            native_status=$?
            if [ "$lli_status" -eq 1 ] && [ "$native_status" -eq 1 ] &&
               [ ! -s "$temporary_dir/lli-error.out" ] &&
               [ ! -s "$temporary_dir/native-error.out" ] &&
               grep -Eq "linha [1-9][0-9]*: erro de execucao $error_code:" "$temporary_dir/lli-error.err" &&
               grep -Eq "linha [1-9][0-9]*: erro de execucao $error_code:" "$temporary_dir/native-error.err"; then
                record_pass "$label (erro $error_code, lli e clang)"
            else
                record_failure "$label" "lli=$lli_status nativo=$native_status; esperado erro $error_code com linha"
                sed 's/^/       /' "$temporary_dir/lli-error.err" "$temporary_dir/native-error.err" >&2
            fi
        done

        # Regressao: um NUL no token nao pode transformar 12\0abc em 12.
        malicious_input="$temporary_dir/entrada-nul.in"
        printf '12\000abc\n' >"$malicious_input"
        input_ir="$temporary_dir/erro-06_entrada.ll"
        input_bc="$temporary_dir/erro-06_entrada-linked.bc"
        input_native="$temporary_dir/erro-06_entrada.exe"
        if [ -s "$input_ir" ] && [ -s "$input_bc" ] && [ -x "$input_native" ]; then
            run_lli "$input_ir" "$input_bc" <"$malicious_input" \
                >"$temporary_dir/lli-nul.out" 2>"$temporary_dir/lli-nul.err"
            lli_status=$?
            run_program "$input_native" <"$malicious_input" \
                >"$temporary_dir/native-nul.out" 2>"$temporary_dir/native-nul.err"
            native_status=$?
            if [ "$lli_status" -eq 1 ] && [ "$native_status" -eq 1 ] &&
               grep -q 'erro de execucao 6:' "$temporary_dir/lli-nul.err" &&
               grep -q 'erro de execucao 6:' "$temporary_dir/native-nul.err"; then
                record_pass 'entrada com NUL rejeitada por lli e clang'
            else
                record_failure 'entrada com NUL' "lli=$lli_status nativo=$native_status; esperava erro 6"
                sed 's/^/       /' "$temporary_dir/lli-nul.err" "$temporary_dir/native-nul.err" >&2
            fi
        fi

        # SSA didatica: mem2reg precisa introduzir phi e preservar resultados.
        ssa_source="$project_root/examples/ssa.mac"
        if [ -f "$ssa_source" ]; then
            ssa_ir="$temporary_dir/ssa.ll"
            ssa_mem="$temporary_dir/ssa-mem2reg.ll"
            ssa_o2="$temporary_dir/ssa-O2.ll"
            run_compiler "$ssa_source" -o "$ssa_ir" >"$temporary_dir/ssa-build.out" 2>"$temporary_dir/ssa-build.err"
            if [ "$?" -ne 0 ] ||
               ! "$llvm_opt" -S -passes=mem2reg "$ssa_ir" -o "$ssa_mem" ||
               ! "$llvm_opt" -S -O2 "$ssa_ir" -o "$ssa_o2" ||
               ! "$llvm_opt" -passes=verify "$ssa_mem" -disable-output ||
               ! "$llvm_opt" -passes=verify "$ssa_o2" -disable-output ||
               ! grep -Eq ' = phi ' "$ssa_mem"; then
                record_failure 'mem2reg e O2' 'passagens falharam, verify rejeitou IR ou phi ausente'
            else
                ssa_ready=1
                for variant in original mem2reg O2; do
                    case "$variant" in
                        original) variant_ir=$ssa_ir ;;
                        mem2reg) variant_ir=$ssa_mem ;;
                        O2) variant_ir=$ssa_o2 ;;
                    esac
                    if ! "$llvm_clang" -Wno-override-module "$variant_ir" \
                        "$project_root/runtime/zl_runtime_posix.c" -lm \
                        -o "$temporary_dir/ssa-$variant.exe" \
                        >"$temporary_dir/ssa-$variant-build.out" \
                        2>"$temporary_dir/ssa-$variant-build.err"; then
                        record_failure "SSA $variant" 'clang nao compilou variante'
                        ssa_ready=0
                    fi
                done
                if [ "$ssa_ready" -eq 1 ]; then
                    for scenario in 0:0 1:1 2:2 5:11 8:29; do
                        input_value=${scenario%%:*}
                        expected_value=${scenario#*:}
                        for variant in original mem2reg O2; do
                            printf '%s\n' "$input_value" | run_program "$temporary_dir/ssa-$variant.exe" \
                                >"$temporary_dir/ssa-$variant.out" 2>"$temporary_dir/ssa-$variant.err"
                            variant_status=$?
                            actual_value=$(tr -d '\r\n' <"$temporary_dir/ssa-$variant.out")
                            if [ "$variant_status" -eq 0 ] && [ "$actual_value" = "$expected_value" ]; then
                                record_pass "SSA $variant entrada=$input_value"
                            else
                                record_failure "SSA $variant entrada=$input_value" \
                                    "status=$variant_status saida=$actual_value esperado=$expected_value"
                            fi
                        done
                    done
                fi
            fi
        else
            record_failure 'mem2reg e O2' 'examples/ssa.mac ausente'
        fi
    fi
fi

printf '\nResultado: %d passou/passaram; %d falhou/falharam.\n' "$passed" "$failed"
[ "$failed" -eq 0 ]
