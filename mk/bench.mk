# Parte C (estudo de otimizacao): alvos make bench*. Dono: frente bench (luiz/bench).
# Incluido pelo Makefile principal; usa $(BASH) e LLVM_BINDIR.
#
#   make bench          estudo completo (N = 10^6 e 10^7, mediana de 5 execucoes):
#                       grava CSV, tabelas, graficos e evidencias em docs/bench/
#   make bench-rapido   so N = 10^6; o resultado fica em $(BENCH_TMP)/rapido/saida
#                       e docs/bench/ nao muda
#   make bench-teste    N = 4096: confere que todo pipeline (C0 a C3 e as variantes
#                       da pergunta obrigatoria) compila, roda e acerta; entra no
#                       make test
#
# So Linux/WSL (ou o Dockerfile). Precisa de LLVM/MLIR 18 com libomp e python3;
# os graficos usam python3-matplotlib. No Ubuntu 26.04: build-essential llvm-18
# clang-18 mlir-18-tools libomp-18-dev python3-matplotlib.
# A medicao roda numa copia em $(BENCH_TMP), fora de /mnt/d (veja bench.sh).
BENCH_TMP ?= /tmp/macaronica-bench
BENCH_MARCH ?= native
export BENCH_TMP BENCH_MARCH

TESTES_EXTRAS += bench-teste

.PHONY: bench bench-rapido bench-teste

bench:
	"$(BASH)" scripts/bench/bench.sh

bench-rapido:
	"$(BASH)" scripts/bench/bench.sh --rapido

bench-teste:
	"$(BASH)" scripts/bench/bench.sh --teste
