#!/bin/sh
# Rota C (back-end da placa) executada no PC: gera C99 com --emit-c, liga com o
# runtime POSIX e compara a saida com o .esperado. Nao comprova LED fisico.
set -u
compiler=${1:-build/debug/macaronica}
test_cc=${2:-${CC:-gcc}}
test_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
project_root=$(CDPATH= cd -- "$test_root/.." && pwd)
case "$compiler" in /* | [A-Za-z]:/*) ;; *) compiler="$project_root/$compiler" ;; esac
[ -x "$compiler" ] || { printf 'ERRO: compilador ausente: %s\n' "$compiler" >&2; exit 1; }
mkdir -p "$project_root/build" || exit 1
tmp=$(mktemp -d "$project_root/build/test-c.XXXXXX") || exit 1
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM
if command -v cygpath >/dev/null 2>&1; then native_tmp=$(cygpath -w "$tmp"); else native_tmp=$tmp; fi
TMPDIR=$native_tmp TMP=$native_tmp TEMP=$native_tmp
export TMPDIR TMP TEMP
passed=0
failed=0
pass() { passed=$((passed + 1)); printf 'OK   C99 %s\n' "$1"; }
fail() { failed=$((failed + 1)); printf 'FALHA C99 %s\n' "$1" >&2; }
run() {
    if [ -x /usr/bin/timeout ]; then /usr/bin/timeout 10 "$@"; else "$@"; fi
}
# Compila com nova tentativa. No Windows o executavel recem-escrito pode ficar
# preso por instantes (antivirus ou processo anterior) e o ld falha com
# "reopening ...: Permission denied"; so esse erro e repetido, ate 5 vezes.
compile_c() {
    saida=$1
    shift
    tentativa=1
    while :; do
        if "$test_cc" -std=c99 -Wall -Wextra -pedantic "-I$project_root/runtime" \
            "$@" -o "$saida" >"$tmp/build.out" 2>"$tmp/build.err"; then
            return 0
        fi
        grep -q 'Permission denied' "$tmp/build.err" || return 1
        [ "$tentativa" -lt 5 ] || return 1
        tentativa=$((tentativa + 1))
        sleep 1
    done
}

# O runtime POSIX e compilado uma vez; cada teste so gera e liga o seu C.
if ! compile_c "$tmp/zl_runtime_posix.o" -c "$project_root/runtime/zl_runtime_posix.c"; then
    cat "$tmp/build.err" >&2
    printf 'ERRO: runtime POSIX nao compilou\n' >&2
    exit 1
fi

# Cada teste tem arquivos com nome proprio ($tmp/<nome>.c e .exe): um
# executavel ainda preso nunca e reescrito pelo teste seguinte.
build_host() {
    "$compiler" --emit-c "$1" -o "$tmp/$2.c" >"$tmp/geracao.out" 2>"$tmp/geracao.err" &&
    compile_c "$tmp/$2.exe" "$tmp/$2.c" "$tmp/zl_runtime_posix.o" -lm
}
show_build_errors() { cat "$tmp/geracao.err" "$tmp/build.err" >&2; }

for source in "$test_root"/execucao/*.mac "$test_root"/esp32/ordem_wrap.mac; do
    name=$(basename "$source" .mac)
    if ! build_host "$source" "$name"; then fail "$name geracao/compilacao"; show_build_errors; continue; fi
    input=${source%.mac}.in
    [ -f "$input" ] || input=/dev/null
    expected_status=0
    [ ! -f "${source%.mac}.status" ] || expected_status=$(tr -d '\r\n' <"${source%.mac}.status")
    run "$tmp/$name.exe" <"$input" >"$tmp/run.out" 2>"$tmp/run.err"
    status=$?
    tr -d '\r' <"$tmp/run.out" >"$tmp/actual"
    tr -d '\r' <"${source%.mac}.esperado" >"$tmp/expected"
    if [ "$status" -eq "$expected_status" ] && [ ! -s "$tmp/run.err" ] && cmp -s "$tmp/actual" "$tmp/expected"; then
        pass "$name"
    else
        fail "$name status=$status esperado=$expected_status"
        diff -u "$tmp/expected" "$tmp/actual" >&2 || :
        cat "$tmp/run.err" >&2
    fi
done
for source in "$test_root"/execucao/erros/*.mac; do
    name=$(basename "$source" .mac)
    code=$(printf '%s' "$name" | cut -c2)
    if ! build_host "$source" "erro_$name"; then fail "$name geracao/compilacao"; show_build_errors; continue; fi
    run "$tmp/erro_$name.exe" </dev/null >"$tmp/run.out" 2>"$tmp/run.err"
    status=$?
    if [ "$status" -eq 1 ] && [ ! -s "$tmp/run.out" ] && grep -Eq "linha [1-9][0-9]*: erro de execucao $code:" "$tmp/run.err"; then
        pass "erro $name"
    else
        fail "erro $name status=$status"
        cat "$tmp/run.out" "$tmp/run.err" >&2
    fi
done
blink="$project_root/examples/esp32_blink.mac"
if [ ! -f "$blink" ]; then
    fail 'fonte examples/esp32_blink.mac ausente'
else
    if "$compiler" --emit-c "$blink" -o "$tmp/rejeitado.c" >"$tmp/gpio.out" 2>"$tmp/gpio.err"; then
        fail 'GPIO deve ser rejeitado sem --alvo=esp32'
    elif grep -q E-TGT-001 "$tmp/gpio.err" && [ ! -e "$tmp/rejeitado.c" ]; then
        pass 'GPIO rejeitado no alvo padrao'
    else
        fail 'diagnostico/saida de GPIO no alvo padrao'
        cat "$tmp/gpio.err" >&2
    fi
    if "$compiler" --alvo=esp32 "$blink" -o "$tmp/blink.c" >"$tmp/geracao.out" 2>"$tmp/geracao.err" &&
       compile_c "$tmp/blink.exe" "$tmp/blink.c" "$test_root/esp32/runtime_gpio_stub.c" -lm; then
        mode_line=$(awk '/^[[:space:]]*pinoModo[(]/ { print NR; exit }' "$blink")
        on_line=$(awk '/^[[:space:]]*pinoEscreve[(].*VER/ { print NR; exit }' "$blink")
        off_line=$(awk '/^[[:space:]]*pinoEscreve[(].*FAL/ { print NR; exit }' "$blink")
        waits=$(awk '/^[[:space:]]*espera[(]/ { print NR }' "$blink")
        on_wait=$(printf '%s\n' "$waits" | sed -n '1p')
        off_wait=$(printf '%s\n' "$waits" | sed -n '2p')
        {
            printf 'modo 2 1 %s\n' "$mode_line"
            for cycle in 1 2 3; do
                printf 'escreve 2 1 %s\nespera 1000 %s\nescreve 2 0 %s\nespera 1000 %s\n' \
                    "$on_line" "$on_wait" "$off_line" "$off_wait"
            done
        } >"$tmp/blink.expected"
        run "$tmp/blink.exe" >"$tmp/blink.out" 2>"$tmp/blink.err"
        status=$?
        tr -d '\r' <"$tmp/blink.out" >"$tmp/blink.actual"
        if [ "$status" -eq 0 ] && [ ! -s "$tmp/blink.err" ] && cmp -s "$tmp/blink.expected" "$tmp/blink.actual"; then
            pass 'blink: tres ciclos GPIO2 1/0, esperas1000, linhas e laco continuo (stub host)'
        else
            fail "blink stub status=$status"
            diff -u "$tmp/blink.expected" "$tmp/blink.actual" >&2 || :
            cat "$tmp/blink.err" >&2
        fi
    else
        fail 'blink geracao/compilacao'
        show_build_errors
    fi
fi
printf '\nResultado C99: %d passou/passaram; %d falhou/falharam.\n' "$passed" "$failed"
[ "$failed" -eq 0 ]
