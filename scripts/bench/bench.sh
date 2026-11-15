#!/usr/bin/env bash
# bench.sh - roda o estudo da Parte C de ponta a ponta (alvos make bench*).
#
#   bash scripts/bench/bench.sh            completo: N = 10^6 e 10^7, mediana de
#                                          5 execuções; grava CSV, tabelas,
#                                          gráficos e evidências em docs/bench/
#   bash scripts/bench/bench.sh --rapido   só N = 10^6 e 1 compilação; o
#                                          resultado fica na pasta temporária
#                                          (docs/bench/ não muda)
#   bash scripts/bench/bench.sh --teste    N = 4096 e 1 execução: confere que
#                                          todo pipeline compila, roda e acerta
#                                          o resultado (entra no make test)
#
# A medição roda numa cópia de scripts/bench/ dentro de $BENCH_TMP (padrão
# /tmp/macaronica-bench), no sistema de arquivos do Linux. No WSL, compilar e
# ligar dentro de /mnt/d passa pelo 9P, que é lento, e o tempo de compilação
# mediria o disco, não o compilador.
#
# Variáveis: BENCH_TMP, BENCH_MARCH (padrão native), BENCH_OMP_PROC_BIND,
# BENCH_OMP_PLACES, LLVM_BINDIR (padrão /usr/lib/llvm-18/bin) e PYTHON
# (padrão python3).
set -eu

modo=completo
case "${1:-}" in
    "") ;;
    --rapido) modo=rapido ;;
    --teste) modo=teste ;;
    *) echo "uso: $0 [--rapido | --teste]" >&2; exit 2 ;;
esac

raiz=$(cd -- "$(dirname -- "$0")/../.." && pwd)
tmp=${BENCH_TMP:-/tmp/macaronica-bench}
# A pasta do modo é apagada a cada rodada: só aceita um caminho dentro de /tmp.
case "$tmp" in
    /tmp/?*) ;;
    *) echo "bench.sh: BENCH_TMP precisa ficar dentro de /tmp: $tmp" >&2; exit 2 ;;
esac
python=${PYTHON:-python3}
base="$tmp/$modo"
inicio=$(date +%s)

rm -rf -- "$base"
mkdir -p -- "$base/trabalho" "$base/saida"
cp -R -- "$raiz/scripts/bench/." "$base/scripts"
echo "bench.sh: modo $modo, cópia de trabalho em $base"

status=0
"$python" "$base/scripts/medir.py" --modo "$modo" --trabalho "$base/trabalho" \
    --saida "$base/saida" || status=$?

if [ "$modo" = teste ]; then
    [ "$status" -eq 0 ] && echo "bench.sh: teste OK em $(( $(date +%s) - inicio )) s"
    exit "$status"
fi

# Gráficos e tabelas saem mesmo se alguma conferência falhou, para inspeção.
if [ -f "$base/saida/execucao.csv" ]; then
    "$python" "$base/scripts/graficos.py" --saida "$base/saida" || status=$?
fi

if [ "$status" -ne 0 ]; then
    echo "bench.sh: falhou (status $status); docs/bench/ não foi alterado." \
         "Resultado parcial em $base/saida" >&2
    exit "$status"
fi

if [ "$modo" = completo ]; then
    destino="$raiz/docs/bench"
    rm -rf -- "$destino/evidencias"
    mkdir -p -- "$destino"
    cp -R -- "$base/saida/." "$destino/"
    echo "bench.sh: resultados em docs/bench/ ($(( $(date +%s) - inicio )) s)"
else
    echo "bench.sh: resultados em $base/saida ($(( $(date +%s) - inicio )) s)"
fi
