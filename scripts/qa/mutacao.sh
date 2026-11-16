#!/usr/bin/env bash
# Robustez da frente qa: nenhuma entrada, por pior que seja, derruba o compilador.
#
# Cada programa-semente gera mutantes (corte do arquivo, troca, duplicacao,
# remocao e substituicao de tokens, e remocao de um caractere dentro de um
# token), e o script acrescenta entradas sinteticas extremas (arquivo vazio,
# aninhamento profundo, cadeias enormes, literais fora da faixa, UTF-8 invalido,
# byte NUL, CRLF). Cada entrada passa pelo compilador nos tres modos: .ll,
# --emit-c e --alvo=esp32. Uma entrada falha quando o compilador:
#   - termina por sinal, estoura o tempo ou devolve codigo fora de 0, 1 e 4;
#   - recusa a entrada (codigo 1) sem nenhum diagnostico no stderr;
#   - recusa a entrada e mesmo assim deixa o arquivo de saida;
#   - deixa relatorio de ASan, UBSan ou LSan no stderr (build `make sanitize`);
#   - aceita a entrada (codigo 0) e gera C que o cc nao compila, ou .ll que o
#     llvm-as nao aceita (quando o LLVM 18 existe).
# As entradas que falham ficam em build/qa-mutacao/falhas para reproduzir.
#
# Uso:
#   scripts/qa/mutacao.sh [-c compilador] [--cc gcc] [-n mutantes] [--semente S]
#                         [--sem-sinteticos] [arquivo|pasta ...]
# Sem arquivos: tests/professor, tests/qa e tests/execucao.
# -n e o numero de mutantes por semente (padrao 24); a mesma --semente com o
# mesmo awk gera os mesmos mutantes.
# Codigo de saida: 0 se nada falhou, 1 se algo falhou, 2 se o uso estiver errado.

set -u

qa_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
raiz=$(CDPATH= cd -- "$qa_dir/../.." && pwd)

compilador=
cc=${CC:-}
mutantes=24
semente_base=2026
sinteticos=1
tempo_limite=${QA_TEMPO_LIMITE:-20}
alvos=()

uso()
{
    sed -n '2,/^set -u$/p' "$0" | sed '$d' | sed 's/^# \{0,1\}//' >&2
    exit 2
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        -c) compilador=${2:?}; shift 2 ;;
        --cc) cc=${2:?}; shift 2 ;;
        -n) mutantes=${2:?}; shift 2 ;;
        --semente) semente_base=${2:?}; shift 2 ;;
        --sem-sinteticos) sinteticos=0; shift ;;
        -h|--help) uso ;;
        -*) printf 'opcao desconhecida: %s\n' "$1" >&2; uso ;;
        *) alvos+=("$1"); shift ;;
    esac
done
case "$mutantes" in ''|*[!0-9]*) printf 'numero de mutantes invalido: %s\n' "$mutantes" >&2; exit 2 ;; esac

case "$(uname -s)" in MINGW*|MSYS*|CYGWIN*) exe=.exe ;; *) exe= ;; esac
if [ -z "$compilador" ]; then
    compilador="$raiz/build/debug/macaronica$exe"
    [ -x "$compilador" ] || compilador="$raiz/build/release/macaronica$exe"
fi
case "$compilador" in /* | [A-Za-z]:/*) ;; *) compilador="$raiz/$compilador" ;; esac
[ -x "$compilador" ] || { printf 'ERRO: compilador ausente: %s\n' "$compilador" >&2; exit 2; }
if [ -z "$cc" ]; then
    if command -v gcc >/dev/null 2>&1; then cc=gcc; else cc=cc; fi
fi
command -v "$cc" >/dev/null 2>&1 || { printf 'ERRO: compilador C ausente: %s\n' "$cc" >&2; exit 2; }
llvm_as=
for candidato in "${LLVM_BINDIR:-/usr/lib/llvm-18/bin}/llvm-as" llvm-as-18; do
    if command -v "$candidato" >/dev/null 2>&1; then llvm_as=$candidato; break; fi
done

mkdir -p "$raiz/build" || exit 2
tmp=$(mktemp -d "$raiz/build/qa-mut.XXXXXX") || exit 2
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM
falhas_dir="$raiz/build/qa-mutacao/falhas"
rm -rf -- "$falhas_dir"
mkdir -p "$falhas_dir" "$tmp/entradas" || exit 2

limitado()
{
    if command -v timeout >/dev/null 2>&1; then timeout "$tempo_limite" "$@"; else "$@"; fi
}

# muta OPERACAO SEMENTE ORIGEM DESTINO
muta()
{
    LC_ALL=C awk -v op="$1" -v semente="$2" '
        BEGIN {
            srand(semente)
            nv = split("[ ] @ ( ) { } ; , . receba edai cond casocontrario durante repete respost principal outrafuncao registro vet inteira flut palavra bool vazio duplocarpado VER FAL E OU XOR NÃO raiz // / ^ - + * < > == ! = \" \\ 0 1. 0.0 9223372036854775807 9223372036854775808 imprimeInteira leInteira pinoModo espera", vocab, " ")
        }
        { for (i = 1; i <= NF; i++) { n++; tok[n] = $i; lin[n] = NR } }
        END {
            if (n == 0) exit
            k = int(rand() * n) + 1
            for (i = 1; i <= n; i++) {
                t = tok[i]
                if (i == k) {
                    if (op == "corta") { printf "%s", substr(t, 1, int(rand() * length(t))); exit }
                    else if (op == "apaga") t = ""
                    else if (op == "duplica") t = t " " t
                    else if (op == "troca" && i < n) { t = tok[i + 1]; tok[i + 1] = tok[i] }
                    else if (op == "substitui") t = vocab[int(rand() * nv) + 1]
                    else if (op == "caractere") { p = int(rand() * length(t)) + 1; t = substr(t, 1, p - 1) substr(t, p + 1) }
                }
                if (i > 1) printf "%s", (lin[i] != lin[i - 1]) ? "\n" : " "
                printf "%s", t
            }
            print ""
        }' "$3" >"$4"
}

# repete_texto N TEXTO: TEXTO repetido N vezes, sem quebra de linha
repete_texto()
{
    awk -v n="$1" -v t="$2" 'BEGIN { for (i = 0; i < n; i++) printf "%s", t }'
}

gera_sinteticos()
{
    local d=$tmp/entradas p='inteira principal() @ ['
    : >"$d/s01_vazio.mcn"
    printf '   \n\t\n' >"$d/s02_so_espacos.mcn"
    { printf '%s respost ' "$p"; repete_texto 5000 '('; printf 1; repete_texto 5000 ')'; printf '; ]\n'; } >"$d/s03_parenteses_profundos.mcn"
    { printf '%s\n' "$p"; repete_texto 2000 'cond (VER) edai [ '; repete_texto 2000 '] '; printf 'respost 0; ]\n'; } >"$d/s04_blocos_profundos.mcn"
    { printf '%s respost ' "$p"; repete_texto 100000 '- '; printf '1; ]\n'; } >"$d/s05_unarios_profundos.mcn"
    { printf '%s respost 1' "$p"; repete_texto 10000 ' ^ 1'; printf '; ]\n'; } >"$d/s06_potencia_longa.mcn"
    { printf '%s respost 1' "$p"; repete_texto 100000 ' + 1'; printf '; ]\n'; } >"$d/s07_soma_longa.mcn"
    { printf '%s cond (VER' "$p"; repete_texto 100000 ' E VER'; printf ') edai [ respost 1; ] respost 0; ]\n'; } >"$d/s08_e_longo.mcn"
    printf '%s respost 99999999999999999999999999; ]\n' "$p" >"$d/s09_literal_enorme.mcn"
    printf '%s respost 1.%s; ]\n' "$p" "$(repete_texto 400 9)" >"$d/s10_real_longo.mcn"
    { printf '%s inteira ' "$p"; repete_texto 100000 a; printf '; respost 0; ]\n'; } >"$d/s11_identificador_longo.mcn"
    { printf '%s imprimePalavra("' "$p"; repete_texto 100000 x; printf '"); respost 0; ]\n'; } >"$d/s12_texto_longo.mcn"
    printf '%s imprimePalavra("abc\n respost 0; ]\n' "$p" >"$d/s13_texto_sem_fim.mcn"
    printf '%s imprimePalavra("\\q"); respost 0; ]\n' "$p" >"$d/s14_escape_desconhecido.mcn"
    printf '%s inteira a\377b; respost 0; ]\n' "$p" >"$d/s15_utf8_invalido.mcn"
    printf '%s imprimePalavra("\303"); respost 0; ]\n' "$p" >"$d/s16_utf8_cortado_no_texto.mcn"
    printf '%s respost\000 0; ]\n' "$p" >"$d/s17_byte_nul.mcn"
    printf 'inteira principal() @\r\n[\r\n    imprimeInteira(1);\r\n    respost 0;\r\n]\r\n' >"$d/s18_crlf.mcn"
    printf '\357\273\277%s respost 0; ]\n' "$p" >"$d/s19_bom.mcn"
    { awk 'BEGIN { for (i = 0; i < 3000; i++) printf "outrafuncao inteira f%d(inteira x) @ [ respost x + %d; ]\n", i, i }'; printf '%s respost f2999(1); ]\n' "$p"; } >"$d/s20_muitas_funcoes.mcn"
    printf '%s inteira vet{2147483647} v; v{2147483646} receba 1; respost 0; ]\n' "$p" >"$d/s21_vetor_maximo.mcn"
    printf '%s inteira vet{2147483648} v; respost 0; ]\n' "$p" >"$d/s22_vetor_acima_do_maximo.mcn"
    printf '%s inteira vet{0} v; respost 0; ]\n' "$p" >"$d/s23_vetor_zero.mcn"
    printf 'registro A @ [ B b; ]\nregistro B @ [ A a; ]\n%s respost 0; ]\n' "$p" >"$d/s24_registros_ciclicos.mcn"
    printf '%s respost 0; ]\n%s respost 1; ]\n' "$p" "$p" >"$d/s25_principal_duplicada.mcn"
    printf 'outrafuncao inteira f() @ [ respost f(); ]\n' >"$d/s26_sem_principal.mcn"
    printf '%s respost principal(); ]\n' "$p" >"$d/s27_chama_principal.mcn"
    printf '%s inteira x; x receba x; x receba -9223372036854775807 - 1; respost x // -1; ]\n' "$p" >"$d/s28_overflow_estatico.mcn"
    printf '%s respost 1 // 0; ]\n' "$p" >"$d/s29_divisao_zero_literal.mcn"
    printf '%s inteira vet{3} v; respost v{3}; ]\n' "$p" >"$d/s30_indice_literal_fora.mcn"
}

# confere ROTULO ENTRADA: roda os tres modos e anota as falhas
total=0
falharam=0
lista_falhas=()
confere()
{
    local rotulo=$1 entrada=$2 modo saida codigo problemas=()
    for modo in ll emit-c esp32; do
        case "$modo" in ll) saida=$tmp/saida.ll ;; *) saida=$tmp/saida.c ;; esac
        rm -f -- "$tmp/saida.ll" "$tmp/saida.c"
        case "$modo" in
            ll) limitado "$compilador" "$entrada" -o "$saida" >"$tmp/out" 2>"$tmp/err" ;;
            emit-c) limitado "$compilador" --emit-c "$entrada" -o "$saida" >"$tmp/out" 2>"$tmp/err" ;;
            esp32) limitado "$compilador" --alvo=esp32 "$entrada" -o "$saida" >"$tmp/out" 2>"$tmp/err" ;;
        esac
        codigo=$?
        case "$codigo" in
            0|1|4) ;;
            124) problemas+=("$modo: estourou ${tempo_limite}s") ;;
            *) problemas+=("$modo: codigo $codigo (sinal ou falha inesperada)") ;;
        esac
        if grep -E -q 'ERROR: (Address|Leak)Sanitizer|runtime error:|SUMMARY: (Address|Undefined|Leak)Sanitizer' "$tmp/err"; then
            problemas+=("$modo: relatorio de sanitizer: $(grep -E -m 1 'Sanitizer|runtime error:' "$tmp/err")")
        fi
        if [ "$codigo" -eq 1 ] && ! grep -q . "$tmp/err"; then
            problemas+=("$modo: recusou sem diagnostico")
        fi
        if [ "$codigo" -ne 0 ] && [ -e "$saida" ]; then
            problemas+=("$modo: recusou e deixou $(basename "$saida")")
        fi
        if [ "$codigo" -eq 0 ]; then
            if [ ! -s "$saida" ]; then
                problemas+=("$modo: aceitou sem gerar saida")
            elif [ "$modo" = ll ]; then
                if [ -n "$llvm_as" ] && ! "$llvm_as" "$saida" -o "$tmp/saida.bc" 2>"$tmp/as.err"; then
                    problemas+=("ll: llvm-as recusou o .ll: $(head -n 1 "$tmp/as.err")")
                fi
            elif ! "$cc" -std=c99 -fsyntax-only "-I$raiz/runtime" "$saida" 2>"$tmp/cc.err"; then
                problemas+=("$modo: o cc recusou o C gerado: $(grep -m 1 'error' "$tmp/cc.err")")
            fi
        fi
    done
    total=$((total + 1))
    if [ "${#problemas[@]}" -gt 0 ]; then
        falharam=$((falharam + 1))
        lista_falhas+=("$rotulo")
        cp -- "$entrada" "$falhas_dir/$rotulo.mcn"
        cp -- "$tmp/err" "$falhas_dir/$rotulo.stderr"
        printf 'FALHA %s\n' "$rotulo"
        printf '      - %s\n' "${problemas[@]}"
    fi
}

sementes=()
[ "${#alvos[@]}" -gt 0 ] || alvos=("$raiz/tests/professor" "$raiz/tests/qa" "$raiz/tests/execucao")
for alvo in "${alvos[@]}"; do
    if [ -d "$alvo" ]; then
        for f in "$alvo"/*.mcn "$alvo"/*.mac; do [ -f "$f" ] && sementes+=("$f"); done
    elif [ -f "$alvo" ]; then
        sementes+=("$alvo")
    else
        printf 'AVISO: %s nao existe\n' "$alvo" >&2
    fi
done

printf 'compilador: %s\ncc: %s\nllvm-as: %s\nsementes: %d, %d mutantes cada, semente base %s\n\n' \
    "$compilador" "$cc" "${llvm_as:-indisponivel}" "${#sementes[@]}" "$mutantes" "$semente_base"

if [ "$sinteticos" -eq 1 ]; then
    gera_sinteticos
    for f in "$tmp"/entradas/s*.mcn; do confere "sintetico_$(basename "$f" .mcn)" "$f"; done
fi

operacoes=(corta troca duplica apaga substitui caractere)
for f in "${sementes[@]}"; do
    nome=$(basename "$f"); nome=${nome%.*}
    for ((m = 1; m <= mutantes; m++)); do
        op=${operacoes[$((m % ${#operacoes[@]}))]}
        s=$((semente_base * 1000 + m))
        muta "$op" "$s" "$f" "$tmp/mutante.mcn"
        confere "${nome}_${op}_${s}" "$tmp/mutante.mcn"
    done
done

printf '\nResultado mutacao: %d entrada(s); %d sem falha; %d falhou/falharam.\n' \
    "$total" "$((total - falharam))" "$falharam"
if [ "$falharam" -gt 0 ]; then
    printf 'Entradas que falharam (copiadas em %s):\n' "$falhas_dir"
    printf '  %s\n' "${lista_falhas[@]}"
    exit 1
fi
exit 0
