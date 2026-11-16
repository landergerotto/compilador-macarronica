# Parte qa: alvos make test-qa e make robustez-qa*. Dono: frente qa (luiz/qa).
# Incluido pelo Makefile principal; usa $(DEBUG_TARGET), $(SANITIZE_TARGET),
# $(EXEEXT), $(BASH) e $(CC).
#
# test-qa               teste diferencial de tests/professor, tests/qa,
#                       tests/qa/ensaio, tests/execucao e tests/execucao/erros:
#                       todas as rotas disponiveis (lli, clang, --alvo=esp32 e
#                       --emit-c) concordam entre si e com a saida conferida a
#                       mao. No Linux exige o LLVM 18; no Windows roda so as
#                       rotas C, porque la nao ha LLVM.
# robustez-qa           entradas mutadas e sinteticas no compilador de debug:
#                       nenhuma pode derrubar o compilador.
# robustez-qa-sanitize  o mesmo no build com ASan e UBSan (Linux ou WSL).
#
# test-qa so entra em TESTES_EXTRAS (make test) quando estiver todo verde.

QA_MUTANTES ?= 24

ifeq ($(EXEEXT),.exe)
QA_EXIGIR_LLVM =
else
QA_EXIGIR_LLVM = --exigir-llvm
endif

.PHONY: test-qa robustez-qa robustez-qa-sanitize

test-qa: $(DEBUG_TARGET)
	"$(BASH)" scripts/qa/diferencial.sh -c "$(DEBUG_TARGET)" --cc "$(CC)" $(QA_EXIGIR_LLVM)

robustez-qa: $(DEBUG_TARGET)
	"$(BASH)" scripts/qa/mutacao.sh -c "$(DEBUG_TARGET)" --cc "$(CC)" -n $(QA_MUTANTES)

robustez-qa-sanitize: $(SANITIZE_TARGET)
	"$(BASH)" scripts/qa/mutacao.sh -c "$(SANITIZE_TARGET)" --cc "$(CC)" -n $(QA_MUTANTES)
