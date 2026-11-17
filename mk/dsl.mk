# Partes A e B (DSL vetorial e saida MLIR): make dsl-teste. Dono: frente dsl-mlir.
# Incluido pelo Makefile principal; usa $(BASH), $(RELEASE_TARGET) e LLVM_BINDIR.
#
#   make dsl-teste   cada recusa da DSL sai com o codigo E-DSL esperado;
#                    tests/dsl/16_dsl_vetorial.mac roda pela rota C99 e, com
#                    LLVM/MLIR 18, o .mlir dos nucleos passa no mlir-opt, desce
#                    ate LLVM IR e, ligado ao .ll, roda no lli e no clang;
#                    entra no make test

DSL_TMP ?= build/dsl

TESTES_EXTRAS += dsl-teste

.PHONY: dsl-teste

dsl-teste: $(RELEASE_TARGET)
	"$(BASH)" scripts/dsl/teste.sh "$(RELEASE_TARGET)" "$(DSL_TMP)" "$(CC)"
