#!/usr/bin/env bash
# Teste diferencial da frente qa.
#
# Cada programa Macaronica passa por todas as rotas de execucao disponiveis, e
# todas precisam concordar entre si e com o resultado conferido a mao. Assim um
# erro de um back-end aparece como divergencia, mesmo sem .esperado.
#
# Rotas:
#   lli      .ll do compilador -> llvm-as, opt -passes=verify, llvm-link com o
#            runtime POSIX em bitcode -> lli               (precisa do LLVM 18)
#   clang    o mesmo .ll + runtime POSIX, compilados pelo clang (nativo)
#   c-esp32  --alvo=esp32 (o mesmo C que vai para a placa) + runtime POSIX +
#            stub de GPIO (scripts/qa/gpio_host.c), compilados pelo cc do PC
#   c-host   --emit-c (rota C do PC) + runtime POSIX
#
# Arquivos de cada programa P.mcn (ou P.mac):
#   P.esperado  stdout esperado, conferido a mao. Sem .esperado, .status e .erro
#               o teste e diferencial puro: so as rotas entre si.
#   P.in        stdin (opcional; sem ele, /dev/null)
#   P.status    codigo de saida esperado (padrao 0). O shell so ve os 8 bits
#               baixos: respost 302 na principal aparece como 46.
#   P.erro      prefixo esperado do erro de execucao no stderr, por exemplo
#               "linha 7: erro de execucao 1:" (sem ele, o stderr tem de ser vazio)
#   P.rotas     rotas em que o programa vale (padrao: todas), ex.: "c-esp32 c-host"
#   P.rejeita   uma linha "rota CODIGO" por rota que deve RECUSAR o programa na
#               compilacao com esse diagnostico (ex.: "c-esp32 E-TGT-002")
#
# Uso:
#   scripts/qa/diferencial.sh [-c compilador] [--cc gcc] [--rotas "lli clang"]
#                             [--exigir-llvm] [--manter pasta] [arquivo|pasta ...]
# Sem arquivos: tests/professor, tests/qa, tests/qa/ensaio, tests/execucao e
# tests/execucao/erros.
# Saida: uma linha OK/FALHA por programa, diffs das falhas e um resumo final.
# Codigo de saida: 0 se tudo passou, 1 se algo falhou, 2 se o uso estiver errado.

set -u

qa_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
raiz=$(CDPATH= cd -- "$qa_dir/../.." && pwd)

compilador=
cc=${CC:-}
rotas_pedidas="lli clang c-esp32 c-host"
exigir_llvm=0
manter=
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
        --rotas) rotas_pedidas=${2:?}; shift 2 ;;
        --exigir-llvm) exigir_llvm=1; shift ;;
        --manter) manter=${2:?}; shift 2 ;;
        -h|--help) uso ;;
        -*) printf 'opcao desconhecida: %s\n' "$1" >&2; uso ;;
        *) alvos+=("$1"); shift ;;
    esac
done

# No WSL o build/ tambem guarda o .exe do Windows (que o WSL consegue
# executar): escolher pelo sistema evita rodar o binario errado.
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) windows=1; exe=.exe ;;
    *) windows=0; exe= ;;
esac

if [ -z "$compilador" ]; then
    for candidato in build/debug/macaronica$exe build/release/macaronica$exe; do
        if [ -f "$raiz/$candidato" ]; then compilador="$raiz/$candidato"; break; fi
    done
fi
case "$compilador" in
    '') printf 'ERRO: compilador nao encontrado em build/; rode make antes\n' >&2; exit 2 ;;
    /* | [A-Za-z]:/*) ;;
    *) compilador="$raiz/$compilador" ;;
esac
if [ ! -x "$compilador" ]; then
    printf 'ERRO: compilador nao executavel: %s\n' "$compilador" >&2
    exit 2
fi
if [ -z "$cc" ]; then
    if command -v gcc >/dev/null 2>&1; then cc=gcc; else cc=cc; fi
fi

# --- pasta temporaria (o gcc do Windows consulta TMP/TEMP) -------------------
mkdir -p "$raiz/build" || exit 2
tmp=$(mktemp -d "$raiz/build/qa-dif.XXXXXX") || exit 2
if [ -n "$manter" ]; then
    mkdir -p "$manter" || exit 2
    trap 'cp -R "$tmp"/. "$manter"/ 2>/dev/null; rm -rf -- "$tmp"' EXIT HUP INT TERM
else
    trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM
fi
if command -v cygpath >/dev/null 2>&1; then nativo_tmp=$(cygpath -w "$tmp"); else nativo_tmp=$tmp; fi
TMPDIR=$nativo_tmp TMP=$nativo_tmp TEMP=$nativo_tmp
export TMPDIR TMP TEMP

timeout_cmd=
if command -v timeout >/dev/null 2>&1 && timeout 1 true >/dev/null 2>&1; then timeout_cmd=timeout; fi
limitado()
{
    if [ -n "$timeout_cmd" ]; then "$timeout_cmd" "$tempo_limite" "$@"; else "$@"; fi
}

# --- ferramentas LLVM -----------------------------------------------------------
llvm_bindir=${LLVM_BINDIR:-}
if [ -z "$llvm_bindir" ] && [ -d /usr/lib/llvm-18/bin ]; then llvm_bindir=/usr/lib/llvm-18/bin; fi
ferramenta()
{
    if [ -n "$llvm_bindir" ]; then printf '%s/%s' "$llvm_bindir" "$1"; else printf '%s' "$1"; fi
}
llvm_as=$(ferramenta llvm-as)
llvm_opt=$(ferramenta opt)
llvm_link=$(ferramenta llvm-link)
llvm_lli=$(ferramenta lli)
llvm_clang=$(ferramenta clang)

llvm_ok=1
for f in "$llvm_as" "$llvm_opt" "$llvm_link" "$llvm_lli" "$llvm_clang"; do
    command -v "$f" >/dev/null 2>&1 || llvm_ok=0
done
runtime_c="$raiz/runtime/zl_runtime_posix.c"
gpio_stub="$qa_dir/gpio_host.c"
runtime_bc="$tmp/zl_runtime_posix.bc"
if [ "$llvm_ok" -eq 1 ] && [ "$windows" -eq 0 ]; then
    if ! "$llvm_clang" -std=c99 -O0 -emit-llvm -c "$runtime_c" -o "$runtime_bc" 2>"$tmp/rt.err"; then
        printf 'ERRO: clang nao compilou o runtime POSIX para bitcode:\n' >&2
        cat "$tmp/rt.err" >&2
        llvm_ok=0
    fi
elif [ "$windows" -eq 1 ]; then
    # O lli do Windows precisa do runtime como DLL (ver decisoes_backend.md);
    # a frente qa executa as rotas LLVM so no WSL.
    llvm_ok=0
fi

rotas_ativas=
for rota in $rotas_pedidas; do
    case "$rota" in
        lli|clang)
            if [ "$llvm_ok" -eq 1 ]; then rotas_ativas="$rotas_ativas $rota"; fi ;;
        c-esp32|c-host) rotas_ativas="$rotas_ativas $rota" ;;
        *) printf 'rota desconhecida: %s\n' "$rota" >&2; exit 2 ;;
    esac
done
rotas_ativas=${rotas_ativas# }

falhas_globais=0
if [ "$llvm_ok" -eq 0 ]; then
    case " $rotas_pedidas " in
        *" lli "*|*" clang "*)
            if [ "$exigir_llvm" -eq 1 ]; then
                printf 'FALHA LLVM 18 indisponivel (LLVM_BINDIR=%s) e --exigir-llvm foi pedido\n' "$llvm_bindir" >&2
                falhas_globais=1
            else
                printf 'AVISO: LLVM 18 indisponivel aqui; rotas lli e clang puladas\n'
            fi ;;
    esac
fi
printf 'compilador: %s\ncc: %s\nrotas: %s\n\n' "$compilador" "$cc" "${rotas_ativas:-nenhuma}"

# --- lista de programas -----------------------------------------------------------
if [ "${#alvos[@]}" -eq 0 ]; then
    alvos=("$raiz/tests/professor" "$raiz/tests/qa" "$raiz/tests/qa/ensaio"
           "$raiz/tests/execucao" "$raiz/tests/execucao/erros")
fi
programas=()
for alvo in "${alvos[@]}"; do
    if [ -d "$alvo" ]; then
        for f in "$alvo"/*.mcn "$alvo"/*.mac; do
            [ -f "$f" ] && programas+=("$f")
        done
    elif [ -f "$alvo" ]; then
        programas+=("$alvo")
    else
        printf 'AVISO: %s nao existe\n' "$alvo"
    fi
done

# Primeiras linhas uteis do log de construcao (sem rastros [debug] do build
# de depuracao nem o aviso "gerado em").
resumo_build()
{
    grep -v -e '^\[debug\]' -e 'gerado em' "$1" | head -n 3 | tr '\n' ' '
}

relativo()
{
    case "$1" in "$raiz"/*) printf '%s' "${1#"$raiz"/}" ;; *) printf '%s' "$1" ;; esac
}

# --- construcao e execucao de uma rota ------------------------------------------
# constroi ROTA FONTE PASTA -> PASTA/ROTA.prog (ou .bc no lli); erros em PASTA/ROTA.build
constroi()
{
    local rota=$1 fonte=$2 pasta=$3 log opcao
    log="$pasta/$rota.build"
    : >"$log"
    case "$rota" in
        lli|clang)
            if [ ! -s "$pasta/prog.ll" ]; then
                limitado "$compilador" "$fonte" -o "$pasta/prog.ll" >>"$log" 2>&1 || return 1
                [ -s "$pasta/prog.ll" ] || { echo 'compilador nao gerou .ll' >>"$log"; return 1; }
            fi
            if [ "$rota" = lli ]; then
                "$llvm_as" "$pasta/prog.ll" -o "$pasta/prog.bc" >>"$log" 2>&1 &&
                "$llvm_opt" -passes=verify "$pasta/prog.ll" -disable-output >>"$log" 2>&1 &&
                "$llvm_link" "$pasta/prog.bc" "$runtime_bc" -o "$pasta/lli.prog" >>"$log" 2>&1
            else
                "$llvm_clang" -Wno-override-module "$pasta/prog.ll" "$runtime_c" -lm \
                    -o "$pasta/clang.prog" >>"$log" 2>&1
            fi ;;
        c-esp32|c-host)
            if [ "$rota" = c-esp32 ]; then opcao=--alvo=esp32; else opcao=--emit-c; fi
            limitado "$compilador" "$opcao" "$fonte" -o "$pasta/$rota.c" >>"$log" 2>&1 || return 1
            [ -s "$pasta/$rota.c" ] || { echo 'compilador nao gerou .c' >>"$log"; return 1; }
            "$cc" -std=c99 -Wall -Wextra -pedantic "-I$raiz/runtime" "$pasta/$rota.c" \
                "$runtime_c" "$gpio_stub" -lm -o "$pasta/$rota.prog$exe" >>"$log" 2>&1 ;;
    esac
}

executa()
{
    local rota=$1 entrada=$2 pasta=$3 status
    if [ "$rota" = lli ]; then
        limitado "$llvm_lli" "$pasta/lli.prog" <"$entrada" >"$pasta/$rota.out" 2>"$pasta/$rota.err"
    else
        limitado "$pasta/$rota.prog$exe" <"$entrada" >"$pasta/$rota.out" 2>"$pasta/$rota.err"
    fi
    status=$?
    # CR do Windows e linhas do stub de GPIO nao fazem parte do contrato.
    tr -d '\r' <"$pasta/$rota.out" >"$pasta/$rota.saida"
    tr -d '\r' <"$pasta/$rota.err" | grep -v '^gpio: ' >"$pasta/$rota.erros"
    printf '%s' "$((status & 255))"
}

total=0
aprovados=0
reprovados=0
lista_falhas=()

for fonte in "${programas[@]}"; do
    total=$((total + 1))
    base=${fonte%.*}
    nome=$(relativo "$fonte")
    pasta="$tmp/$total"
    mkdir -p "$pasta"

    esperado="$base.esperado"
    entrada="$base.in"; [ -f "$entrada" ] || entrada=/dev/null
    status_esperado=0
    [ ! -f "$base.status" ] || status_esperado=$(tr -d ' \r\n' <"$base.status")
    erro_esperado=
    [ ! -f "$base.erro" ] || erro_esperado=$(head -n 1 "$base.erro" | tr -d '\r')
    # Sem nenhum arquivo de expectativa: diferencial puro (so rotas entre si).
    puro=0
    if [ ! -f "$esperado" ] && [ ! -f "$base.status" ] && [ ! -f "$base.erro" ]; then puro=1; fi
    rotas_do_programa=$rotas_ativas
    if [ -f "$base.rotas" ]; then
        rotas_do_programa=
        for rota in $(tr -d '\r' <"$base.rotas"); do
            case " $rotas_ativas " in *" $rota "*) rotas_do_programa="$rotas_do_programa $rota" ;; esac
        done
        rotas_do_programa=${rotas_do_programa# }
    fi

    problemas=()
    rodadas=
    ref_rota= ref_status=
    for rota in $rotas_do_programa; do
        recusa=
        [ ! -f "$base.rejeita" ] || recusa=$(tr -d '\r' <"$base.rejeita" | awk -v r="$rota" '$1 == r { print $2 }')
        if [ -n "$recusa" ]; then
            if constroi "$rota" "$fonte" "$pasta"; then
                problemas+=("$rota: devia recusar com $recusa, mas compilou")
            elif ! grep -q -- "$recusa" "$pasta/$rota.build"; then
                problemas+=("$rota: recusou sem o diagnostico $recusa: $(resumo_build "$pasta/$rota.build")")
            else
                rodadas="$rodadas $rota(recusa)"
            fi
            continue
        fi
        if ! constroi "$rota" "$fonte" "$pasta"; then
            problemas+=("$rota: compilacao falhou: $(resumo_build "$pasta/$rota.build")")
            continue
        fi
        status=$(executa "$rota" "$entrada" "$pasta")
        rodadas="$rodadas $rota"
        if [ "$puro" -eq 0 ] && [ "$status" != "$status_esperado" ]; then
            problemas+=("$rota: status $status, esperado $status_esperado")
        fi
        if [ -f "$esperado" ]; then
            tr -d '\r' <"$esperado" >"$pasta/esperado.saida"
            if ! cmp -s "$pasta/esperado.saida" "$pasta/$rota.saida"; then
                problemas+=("$rota: stdout difere do .esperado")
                diff -u "$pasta/esperado.saida" "$pasta/$rota.saida" | head -n 30 >"$pasta/$rota.diff"
            fi
        fi
        if [ -n "$erro_esperado" ]; then
            if ! grep -qF -- "$erro_esperado" "$pasta/$rota.erros"; then
                problemas+=("$rota: stderr sem '$erro_esperado': $(head -n 2 "$pasta/$rota.erros" | tr '\n' ' ')")
            fi
        elif [ "$puro" -eq 0 ] && [ -s "$pasta/$rota.erros" ]; then
            problemas+=("$rota: stderr inesperado: $(head -n 2 "$pasta/$rota.erros" | tr '\n' ' ')")
        fi
        # Diferencial: todas as rotas contra a primeira que executou.
        if [ -z "$ref_rota" ]; then
            ref_rota=$rota ref_status=$status
        else
            if [ "$status" != "$ref_status" ]; then
                problemas+=("$rota diverge de $ref_rota: status $status x $ref_status")
            fi
            if ! cmp -s "$pasta/$ref_rota.saida" "$pasta/$rota.saida"; then
                problemas+=("$rota diverge de $ref_rota no stdout")
                [ -f "$pasta/$rota.diff" ] ||
                    diff -u "$pasta/$ref_rota.saida" "$pasta/$rota.saida" | head -n 30 >"$pasta/$rota.diff"
            fi
            if [ "$(head -n 1 "$pasta/$ref_rota.erros")" != "$(head -n 1 "$pasta/$rota.erros")" ]; then
                problemas+=("$rota diverge de $ref_rota na 1a linha do stderr")
            fi
        fi
    done

    if [ -z "$rotas_do_programa" ]; then
        problemas+=("nenhuma rota aplicavel neste ambiente")
    fi
    if [ "${#problemas[@]}" -eq 0 ]; then
        aprovados=$((aprovados + 1))
        printf 'OK    %s [%s] status=%s\n' "$nome" "${rodadas# }" "$status_esperado"
    else
        reprovados=$((reprovados + 1))
        lista_falhas+=("$nome")
        printf 'FALHA %s [%s]\n' "$nome" "${rodadas# }"
        for p in "${problemas[@]}"; do printf '      - %s\n' "$p"; done
        for d in "$pasta"/*.diff; do
            [ -f "$d" ] || continue
            printf '      diff (%s):\n' "$(basename "$d" .diff)"
            sed 's/^/        /' "$d"
        done
    fi
done

printf '\nResultado diferencial: %d programa(s); %d passou/passaram; %d falhou/falharam.\n' \
    "$total" "$aprovados" "$reprovados"
if [ "$reprovados" -gt 0 ]; then
    printf 'Falhas:\n'
    printf '  %s\n' "${lista_falhas[@]}"
fi
[ "$reprovados" -eq 0 ] && [ "$falhas_globais" -eq 0 ]
