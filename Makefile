# Compilador Macaronica (LFC, PUCPR 2026.2)
#
#   make                  compila o compilador (release, sem mensagens de depuracao)
#   make debug            build de depuracao (-O0 -g3)
#   make test             todos os testes: PjBL 1 (front-end), PjBL 3 (LLVM IR),
#                         rota C da placa no PC e os testes extras de mk/*.mk
#   make test-frontend    so os testes do PjBL 1
#   make test-c           so a rota C (back-end da placa) executada no PC
#   make sanitize         testes com AddressSanitizer e UBSan (Linux/WSL)
#   make placa PROG=...   executavel para a placa (alvos em mk/placa.mk)
#
# Os testes de LLVM IR usam o LLVM 18 (llvm-as, opt, lli, clang). No Linux/WSL a
# pasta /usr/lib/llvm-18/bin e detectada sozinha; fora dela, defina LLVM_BINDIR.

CC = gcc
CPPFLAGS = -Isrc
WARNINGS = -Wall -Wextra -pedantic
CFLAGS = -std=c99 $(WARNINGS) -O0 -g3
LDFLAGS =
LDLIBS = -lm

ifneq ($(wildcard C:/Windows/System32/cmd.exe),)
EXEEXT = .exe
ifneq ($(wildcard C:/msys64/usr/bin/bash.exe),)
BASH = C:/msys64/usr/bin/bash.exe
else
BASH = C:/Program Files/Git/usr/bin/bash.exe
endif
else
EXEEXT =
BASH = bash
# LLVM/MLIR 18 do apt (pacotes llvm-18, clang-18, mlir-18-tools).
LLVM_BINDIR ?= $(wildcard /usr/lib/llvm-18/bin)
endif
export LLVM_BINDIR

# Todos os .c de src/: um modulo novo entra no build sem editar este arquivo.
SOURCES = $(sort $(wildcard src/*.c src/*/*.c))
HEADERS = $(wildcard src/*.h src/*/*.h)

DEBUG_TARGET = build/debug/macaronica$(EXEEXT)
RELEASE_TARGET = build/release/macaronica$(EXEEXT)
SANITIZE_TARGET = build/sanitize/macaronica$(EXEEXT)

.DEFAULT_GOAL := all

# Cada parte do PjBL 4 tem seus alvos em mk/<parte>.mk (placa, dsl, bench).
# Um desses arquivos registra testes com "TESTES_EXTRAS += <alvo>", e o
# make test os executa depois dos testes do PjBL 1 e do PjBL 3.
TESTES_EXTRAS =
-include $(sort $(wildcard mk/*.mk))

.PHONY: all debug release test test-llvm test-frontend test-c sanitize clean

all: $(RELEASE_TARGET)

debug: $(DEBUG_TARGET)

release: $(RELEASE_TARGET)

test: test-llvm test-c $(TESTES_EXTRAS)

# PjBL 1 (validos e invalidos) e PjBL 3 (llvm-as, opt -passes=verify, lli, clang).
test-llvm: $(DEBUG_TARGET)
	"$(BASH)" tests/run_tests.sh "$(DEBUG_TARGET)" "$(CC)"

test-frontend: $(DEBUG_TARGET)
	"$(BASH)" tests/run_tests.sh "$(DEBUG_TARGET)" "$(CC)" frontend

# Rota C da placa (--alvo=esp32) executada no PC com o runtime POSIX.
test-c: $(DEBUG_TARGET)
	"$(BASH)" tests/run_c_tests.sh "$(DEBUG_TARGET)" "$(CC)"

sanitize: $(SANITIZE_TARGET)
	"$(BASH)" tests/run_tests.sh "$(SANITIZE_TARGET)" "$(CC)"

$(DEBUG_TARGET): $(SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

$(RELEASE_TARGET): $(SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -std=c99 $(WARNINGS) -O2 \
		$(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

$(SANITIZE_TARGET): $(SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -std=c99 $(WARNINGS) -O1 -g3 \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(LDFLAGS) -fsanitize=address,undefined -o $@ $(SOURCES) $(LDLIBS)

clean:
	rm -rf build
