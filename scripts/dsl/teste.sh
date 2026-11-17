#!/usr/bin/env bash
# Teste das Partes A e B do PjBL 4 (make dsl-teste, dentro do make test).
#
# 1. Cada recusa da DSL vetorial sai com o codigo da tabela de docs/dsl.md.
# 2. Rota C99: tests/dsl/16_dsl_vetorial.mac vira C (a DSL sai como lacos
#    inline), liga com o runtime POSIX e imprime o .esperado.
# 3. Rota MLIR, com LLVM/MLIR 18 em LLVM_BINDIR: o compilador grava o .ll e,
#    ao lado, o .mlir dos nucleos mcn_* que o .ll chama. O .mlir passa no
#    mlir-opt e desce ate LLVM IR (o mesmo rebaixamento da Parte C). O
#    programa ligado (.ll do compilador + nucleos + runtime) roda no lli e
#    no clang e imprime o mesmo .esperado.
#
# O 16_dsl_vetorial fica em tests/dsl/ (e nao em tests/execucao/) porque o
# .ll dele sozinho nao liga: os nucleos vem do .mlir.
#
# Uso: scripts/dsl/teste.sh COMPILADOR PASTA_TEMPORARIA [CC]
set -u
compilador="$1"
pasta="$2"
cc="${3:-cc}"
mkdir -p "$pasta"
falhas=0

ok() { printf 'OK   dsl %s\n' "$1"; }
falha() { printf 'FALHA dsl %s\n' "$1"; falhas=$((falhas + 1)); }

fonte=tests/dsl/16_dsl_vetorial.mac
esperado=tests/dsl/16_dsl_vetorial.esperado
sed 's/\r$//' "$esperado" >"$pasta/esperado.txt"

# confere ROTULO SAIDA: a saida (sem \r) tem de ser igual ao .esperado
confere() {
    sed 's/\r$//' "$2" >"$2.txt"
    if cmp -s "$2.txt" "$pasta/esperado.txt"; then
        ok "$1"
    else
        falha "$1: saida difere de $esperado ($2.txt)"
    fi
}

while read -r caso codigo; do
    recusa="tests/semantic/$caso.mac"
    if "$compilador" "$recusa" >"$pasta/$caso.log" 2>&1; then
        falha "$caso: aceito, esperava $codigo"
    elif grep -q ": semantico $codigo:" "$pasta/$caso.log"; then
        ok "$caso ($codigo)"
    else
        falha "$caso: recusado sem $codigo ($pasta/$caso.log)"
    fi
done <<'CASOS'
invalido_dsl_destino_escalar E-DSL-001
invalido_dsl_tamanhos E-DSL-003
invalido_dsl_estreita E-DSL-004
invalido_dsl_forma E-DSL-005
invalido_dsl_reducao E-DSL-007
CASOS

# Rota C99 (lacos inline)
if ! "$compilador" --emit-c "$fonte" -o "$pasta/16.c" >"$pasta/c.log" 2>&1 ||
   ! "$cc" -std=c99 -Wall -Wextra -pedantic -Iruntime "$pasta/16.c" \
       runtime/zl_runtime_posix.c -lm -o "$pasta/16_c.exe" >>"$pasta/c.log" 2>&1; then
    falha "16_dsl_vetorial C99: nao gerou ou nao ligou ($pasta/c.log)"
else
    "$pasta/16_c.exe" </dev/null >"$pasta/16_c.out" 2>>"$pasta/c.log"
    confere "16_dsl_vetorial C99 (lacos inline)" "$pasta/16_c.out"
fi

# Rota MLIR (nucleos mcn_* ligados ao .ll)
bindir="${LLVM_BINDIR:-}"
if [ -z "$bindir" ] || [ ! -x "$bindir/mlir-opt" ]; then
    printf 'PULADO dsl mlir: sem LLVM/MLIR 18 em LLVM_BINDIR\n'
elif ! "$compilador" "$fonte" -o "$pasta/16.ll" >"$pasta/compilar.log" 2>&1 ||
     [ ! -s "$pasta/16.mlir" ]; then
    falha "16_dsl_vetorial: nao gerou o .mlir ($pasta/compilar.log)"
elif ! "$bindir/mlir-opt" "$pasta/16.mlir" -o /dev/null 2>"$pasta/verifica.log"; then
    falha "16_dsl_vetorial: o mlir-opt recusou o .mlir ($pasta/verifica.log)"
elif ! "$bindir/mlir-opt" "$pasta/16.mlir" --lower-affine --convert-scf-to-cf \
        --convert-arith-to-llvm --finalize-memref-to-llvm --convert-func-to-llvm \
        --convert-cf-to-llvm --reconcile-unrealized-casts \
        -o "$pasta/16_llvm.mlir" 2>"$pasta/desce.log" ||
     ! "$bindir/mlir-translate" --mlir-to-llvmir "$pasta/16_llvm.mlir" \
        -o "$pasta/16_nucleos.ll" 2>>"$pasta/desce.log" ||
     ! "$bindir/opt" -passes=verify -disable-output "$pasta/16_nucleos.ll" 2>>"$pasta/desce.log"; then
    falha "16_dsl_vetorial: o .mlir nao desceu ate LLVM IR ($pasta/desce.log)"
else
    ok "16_dsl_vetorial: .mlir verificado e descido ate LLVM IR ($(grep -c '^define' "$pasta/16_nucleos.ll") nucleos)"
    if ! "$bindir/clang" -std=c99 -O0 -emit-llvm -c runtime/zl_runtime_posix.c \
           -o "$pasta/runtime.bc" >"$pasta/liga.log" 2>&1 ||
       ! "$bindir/llvm-link" "$pasta/16.ll" "$pasta/16_nucleos.ll" "$pasta/runtime.bc" \
           -o "$pasta/16_ligado.bc" >>"$pasta/liga.log" 2>&1; then
        falha "16_dsl_vetorial MLIR: llvm-link do .ll com os nucleos falhou ($pasta/liga.log)"
    else
        "$bindir/lli" "$pasta/16_ligado.bc" </dev/null >"$pasta/16_lli.out" 2>>"$pasta/liga.log"
        confere "16_dsl_vetorial MLIR + lli (.ll + nucleos + runtime)" "$pasta/16_lli.out"
    fi
    if ! "$bindir/clang" -Wno-override-module "$pasta/16.ll" "$pasta/16_nucleos.ll" \
           runtime/zl_runtime_posix.c -lm -o "$pasta/16_nativo" >"$pasta/clang.log" 2>&1; then
        falha "16_dsl_vetorial MLIR: clang nao ligou .ll + nucleos + runtime ($pasta/clang.log)"
    else
        "$pasta/16_nativo" </dev/null >"$pasta/16_nativo.out" 2>>"$pasta/clang.log"
        confere "16_dsl_vetorial MLIR + clang (nativo)" "$pasta/16_nativo.out"
    fi
fi

printf 'Resultado DSL: %s falha(s)\n' "$falhas"
[ "$falhas" -eq 0 ]
