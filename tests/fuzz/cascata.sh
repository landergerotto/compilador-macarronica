#!/bin/sh
# Mini-fuzzer de cascatas de erro (D-018: um diagnostico por erro real).
#
# Uso: sh tests/fuzz/cascata.sh COMPILADOR [FONTE...]
#      (sem FONTE, usa tests/fuzz/base_cascata.mac)
#
# Cada FONTE deve ser valida e ter um espaco entre tokens (como a base). Para
# cada token, o script gera duas mutacoes: sem o token e com o token
# duplicado. Cada mutacao e um unico erro real, entao o compilador deve dar no
# maximo um diagnostico e nunca travar. Sai com codigo 1 se alguma mutacao der
# mais de um diagnostico (cascata), travar (codigo acima de 1) ou falhar sem
# diagnostico.
compiler=$1
shift
[ -n "$compiler" ] || { echo "uso: $0 COMPILADOR [FONTE...]" >&2; exit 2; }
[ $# -gt 0 ] || set -- "$(dirname -- "$0")/base_cascata.mac"
work=$(mktemp -d) || exit 2
trap 'rm -rf "$work"' EXIT HUP INT TERM
problems=0
for base in "$@"; do
    if ! "$compiler" "$base" >/dev/null 2>&1; then
        echo "BASE INVALIDA: $base" >&2; problems=$((problems + 1)); continue
    fi
    total=$(awk '{ n += NF } END { print n + 0 }' "$base")
    mutations=0
    for mode in del dup; do
        k=1
        while [ "$k" -le "$total" ]; do
            # Reescreve a base removendo (del) ou duplicando (dup) o k-esimo token.
            awk -v k="$k" -v m="$mode" '{
                line = ""
                for (i = 1; i <= NF; i++) {
                    c++; w = $i
                    if (c == k) w = (m == "del") ? "" : $i " " $i
                    if (w != "") line = line (line == "" ? "" : " ") w
                }
                print line
            }' "$base" > "$work/m.mac"
            "$compiler" "$work/m.mac" >/dev/null 2>"$work/err"
            status=$?
            count=$(grep -Ec ': (lexico|sintatico|semantico) E-' "$work/err")
            token=$(awk -v k="$k" '{ for (i = 1; i <= NF; i++) if (++c == k) { print $i " (linha " NR ")"; exit } }' "$base")
            if [ "$status" -gt 1 ]; then
                echo "TRAVOU (codigo $status): $mode $token em $base"; problems=$((problems + 1))
            elif [ "$count" -gt 1 ]; then
                echo "CASCATA ($count diagnosticos): $mode $token em $base"
                grep -E ': (lexico|sintatico|semantico) E-' "$work/err" | sed 's/^/    /'
                problems=$((problems + 1))
            elif [ "$status" -eq 1 ] && [ "$count" -eq 0 ]; then
                echo "SEM DIAGNOSTICO: $mode $token em $base"; problems=$((problems + 1))
            fi
            mutations=$((mutations + 1))
            k=$((k + 1))
        done
    done
    echo "$base: $mutations mutacoes"
done
echo "problemas: $problems"
[ "$problems" -eq 0 ]
