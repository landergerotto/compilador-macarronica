#!/usr/bin/env bash
# Rota Arduino da placa DOIT ESP32 DevKit V1 (make placa, make placa-gravar,
# make placa-monitor, make placa-pc).
#
# O firmware sempre sai do compilador Macaronica: o C99 vem de
# `macaronica --alvo=esp32`; este script so monta o sketch (o C gerado, o
# runtime da placa e um .ino de 2 funcoes) e chama o arduino-cli. Roda no
# Windows (Git Bash, chamado pelo mingw32-make) e no Linux.
#
# Uso: tools/placa.sh gerar    PROG.mac [PILHA] [COMPILADOR]
#      tools/placa.sh compilar PROG.mac [PILHA] [COMPILADOR]
#      tools/placa.sh gravar   PROG.mac PORTA
#      tools/placa.sh monitor  PORTA
#      tools/placa.sh pc       PROG.mac [PILHA] [COMPILADOR] [CC]
# PILHA: bytes da pilha da tarefa que executa zl_principal (padrao 65536).
# O mesmo numero vai para o --ram-limite do compilador e para o
# -DZL_PILHA_BYTES do runtime, entao a verificacao de memoria e a tarefa da
# placa usam a mesma conta. Variaveis opcionais: ARDUINO_CLI, FQBN.
set -euo pipefail

raiz="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$raiz"

FQBN="${FQBN:-esp32:esp32:esp32doit-devkit-v1}"
BAUD=115200
# -ffp-contract=off: no modo gnu17 do nucleo Arduino o gcc da placa funde
# a*b+c numa instrucao madd.s, com um arredondamento so, e um flut poderia
# sair diferente do PC. Valor fixo de proposito: mudar estas opcoes recompila
# o nucleo inteiro (por isso a pilha vai em zl_config.h, nao aqui).
FLAGS_EXTRA="-ffp-contract=off"

falha() {
    printf 'placa: %s\n' "$*" >&2
    exit 1
}

uso() {
    sed -n '10,14p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//' >&2
    exit 2
}

# O bash do MSYS2, que o Makefile prefere no Windows, nao repassa USERPROFILE,
# LOCALAPPDATA, TEMP e TMP aos programas que chama; sem eles o arduino-cli nao
# acha o nucleo esp32 (em Arduino15) nem cria temporarios. Recupera as pastas
# pela API do Windows (cygpath -F: 40 perfil, 28 dados locais, 26 dados moveis).
prepara_windows() {
    command -v cygpath >/dev/null 2>&1 || return 0
    if [ -z "${USERPROFILE:-}" ]; then USERPROFILE="$(cygpath -w -F 40)"; export USERPROFILE; fi
    if [ -z "${LOCALAPPDATA:-}" ]; then LOCALAPPDATA="$(cygpath -w -F 28)"; export LOCALAPPDATA; fi
    if [ -z "${APPDATA:-}" ]; then APPDATA="$(cygpath -w -F 26)"; export APPDATA; fi
    if [ -z "${TEMP:-}" ]; then TEMP="$(cygpath -w "$(cygpath -u "$LOCALAPPDATA")/Temp")"; export TEMP; fi
    if [ -z "${TMP:-}" ]; then TMP="$TEMP"; export TMP; fi
}

acha_arduino_cli() {
    local ide="/c/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
    if [ -n "${ARDUINO_CLI:-}" ]; then
        printf '%s\n' "$ARDUINO_CLI"
    elif command -v arduino-cli >/dev/null 2>&1; then
        command -v arduino-cli
    elif [ -x "$ide" ]; then
        printf '%s\n' "$ide"
    else
        falha "arduino-cli nao encontrado: instale-o ou defina ARDUINO_CLI"
    fi
}

acha_compilador() {
    local candidato
    if [ -n "${1:-}" ]; then
        [ -x "$1" ] || falha "compilador nao encontrado: $1 (rode make antes)"
        printf '%s\n' "$1"
        return
    fi
    for candidato in build/release/macaronica.exe build/release/macaronica; do
        if [ -x "$candidato" ]; then
            printf '%s\n' "$candidato"
            return
        fi
    done
    falha "compilador ausente em build/release: rode make antes"
}

confere_pilha() {
    case "$1" in
        '' | *[!0-9]*) falha "PILHA deve ser um numero de bytes, recebido: '$1'" ;;
    esac
}

nome_do_programa() {
    local base
    base="$(basename "$1")"
    printf '%s\n' "${base%.mac}"
}

# Gera o C e monta o sketch em build/placa/<nome>/macaronica/.
gerar() {
    local prog="$1" pilha="$2" compilador="$3" nome sketch
    [ -f "$prog" ] || falha "programa nao encontrado: $prog"
    nome="$(nome_do_programa "$prog")"
    sketch="build/placa/$nome/macaronica"
    rm -rf "$sketch"
    mkdir -p "$sketch"
    "$compilador" --alvo=esp32 --ram-limite="$pilha" "$prog" -o "$sketch/programa.c"
    printf '/* Gerado por tools/placa.sh: pilha da tarefa do programa, em bytes. */\n#define ZL_PILHA_BYTES %s\n' \
        "$pilha" >"$sketch/zl_config.h"
    cp runtime/zl_runtime.h runtime/zl_esp32.h runtime/zl_formato.h runtime/zl_placa.h \
        runtime/zl_runtime_esp32.c runtime/zl_plataforma_arduino.cpp "$sketch/"
    cat >"$sketch/macaronica.ino" <<'FIM'
// Sketch montado por tools/placa.sh (make placa); refeito a cada build.
// O programa Macaronica esta em programa.c, gerado pelo compilador com
// --alvo=esp32; o runtime da placa esta nos arquivos zl_*.
#include "zl_esp32.h"

void setup()
{
    // Abre a serial e cria a tarefa "macaronica" (nucleo 1, pilha de
    // ZL_PILHA_BYTES), que executa zl_principal. A tarefa do loop() tem
    // so 8 KB de pilha e nao roda o programa.
    zl_esp32_inicia();
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
FIM
    printf 'placa: sketch montado em %s\n' "$sketch"
}

compilar() {
    local prog="$1" pilha="$2" compilador="$3" nome sketch build saida ac versao_nucleo commit
    nome="$(nome_do_programa "$prog")"
    sketch="build/placa/$nome/macaronica"
    build="build/placa/$nome/build"
    saida="placa/$nome"
    ac="$(acha_arduino_cli)"
    gerar "$prog" "$pilha" "$compilador"
    mkdir -p "$build" "$saida"
    if ! "$ac" compile -b "$FQBN" --build-path "$build" \
        --build-property "compiler.c.extra_flags=$FLAGS_EXTRA" \
        --build-property "compiler.cpp.extra_flags=$FLAGS_EXTRA" \
        "$sketch" 2>&1 | tee "build/placa/$nome/compilacao.log"; then
        falha "arduino-cli nao compilou $prog (log em build/placa/$nome/compilacao.log)"
    fi
    cp "$build/macaronica.ino.bin" "$saida/$nome.bin"
    cp "$build/macaronica.ino.elf" "$saida/$nome.elf"
    cp "$build/macaronica.ino.bootloader.bin" "$saida/$nome.bootloader.bin"
    cp "$build/macaronica.ino.partitions.bin" "$saida/$nome.partitions.bin"
    cp "$sketch/programa.c" "$saida/$nome.c"
    versao_nucleo="$("$ac" core list 2>/dev/null | awk '$1 == "esp32:esp32" { print $2 }')"
    commit="$(git rev-parse --short HEAD 2>/dev/null || printf '?')"
    if ! git diff --quiet HEAD 2>/dev/null; then commit="$commit, com alteracoes locais"; fi
    {
        printf 'programa: %s\n' "$prog"
        printf 'compilador: %s (commit %s)\n' "$compilador" "$commit"
        printf 'geracao: macaronica --alvo=esp32 --ram-limite=%s %s -o programa.c\n' "$pilha" "$prog"
        printf 'placa: %s, nucleo esp32:esp32 %s\n' "$FQBN" "${versao_nucleo:-?}"
        printf 'arduino-cli: %s\n' "$("$ac" version 2>/dev/null | head -n 1)"
        printf 'pilha da tarefa macaronica: %s bytes (zl_config.h); opcoes extras: %s\n' \
            "$pilha" "$FLAGS_EXTRA"
        grep -E 'Sketch uses|Global variables' "build/placa/$nome/compilacao.log" || true
        printf 'sha256 %s.bin: ' "$nome"
        sha256sum "$saida/$nome.bin" | cut -d ' ' -f 1
        printf 'data: %s\n' "$(date '+%Y-%m-%d %H:%M:%S')"
    } >"$saida/proveniencia.txt"
    printf 'placa: firmware em %s/ (%s.bin, %s.elf, proveniencia.txt)\n' "$saida" "$nome" "$nome"
}

gravar() {
    local prog="$1" porta="$2" nome ac
    nome="$(nome_do_programa "$prog")"
    ac="$(acha_arduino_cli)"
    [ -f "placa/$nome/$nome.bin" ] || falha "placa/$nome/$nome.bin ausente: rode make placa PROG=$prog"
    # Grava bootloader, tabela de particoes e programa (o mesmo da IDE).
    "$ac" upload -b "$FQBN" -p "$porta" --input-file "placa/$nome/$nome.bin"
}

monitor() {
    local porta="$1" ac
    ac="$(acha_arduino_cli)"
    printf 'placa: monitor em %s a %s baud; aperte EN na placa para reiniciar o programa.\n' \
        "$porta" "$BAUD" >&2
    "$ac" monitor -p "$porta" -b "$FQBN" --config "baudrate=$BAUD"
}

# Roda no PC o mesmo C gerado para a placa, com o runtime POSIX e a simulacao
# dos pinos (runtime/zl_placa_pc.c). A saida deve ser a do .esperado.
pc() {
    local prog="$1" pilha="$2" compilador="$3" cc="$4" nome dir exe
    [ -f "$prog" ] || falha "programa nao encontrado: $prog"
    nome="$(nome_do_programa "$prog")"
    dir="build/placa/$nome/pc"
    exe="$dir/programa"
    case "$(uname -s)" in MINGW* | MSYS* | CYGWIN*) exe="$exe.exe" ;; esac
    mkdir -p "$dir"
    "$compilador" --alvo=esp32 --ram-limite="$pilha" "$prog" -o "$dir/programa.c"
    "$cc" -std=c99 -O2 -Iruntime -o "$exe" "$dir/programa.c" runtime/zl_runtime_posix.c \
        runtime/zl_placa_pc.c -lm
    "$exe"
}

[ $# -ge 1 ] || uso
prepara_windows
acao="$1"
shift
case "$acao" in
    gerar | compilar)
        [ $# -ge 1 ] || uso
        pilha="${2:-65536}"
        confere_pilha "$pilha"
        "$acao" "$1" "$pilha" "$(acha_compilador "${3:-}")"
        ;;
    gravar)
        [ $# -ge 2 ] || uso
        gravar "$1" "$2"
        ;;
    monitor)
        [ $# -ge 1 ] || uso
        monitor "$1"
        ;;
    pc)
        [ $# -ge 1 ] || uso
        pilha="${2:-65536}"
        confere_pilha "$pilha"
        pc "$1" "$pilha" "$(acha_compilador "${3:-}")" "${4:-gcc}"
        ;;
    *) uso ;;
esac
