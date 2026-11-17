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
endif

SOURCES = \
	src/main.c \
	src/compiler.c \
	src/common/source.c \
	src/common/diagnostic.c \
	src/lexer/token.c \
	src/lexer/lexer.c \
	src/parser/ast.c \
	src/parser/parser.c \
	src/semantic/type.c \
	src/semantic/symbol.c \
	src/semantic/semantic.c

SOURCES += $(wildcard src/ir/codegen_llvm.c src/ir/codegen_mlir.c src/ir/dsl.c src/codegen_c.c src/target.c)

HEADERS = $(wildcard src/*.h src/*/*.h)

DEBUG_TARGET = build/debug/macaronica$(EXEEXT)
RELEASE_TARGET = build/release/macaronica$(EXEEXT)
SANITIZE_TARGET = build/sanitize/macaronica$(EXEEXT)

.PHONY: all debug release test test-c test-frontend sanitize clean placa placa-gerar placa-gravar placa-monitor

all: $(DEBUG_TARGET)

debug: $(DEBUG_TARGET)

release: $(RELEASE_TARGET)

test: $(DEBUG_TARGET)
	"$(BASH)" tests/run_tests.sh "$(DEBUG_TARGET)" "$(CC)"
	"$(BASH)" tests/run_c_tests.sh "$(DEBUG_TARGET)" "$(CC)"

test-frontend: $(DEBUG_TARGET)
	"$(BASH)" tests/run_tests.sh "$(DEBUG_TARGET)" "$(CC)" frontend

sanitize: $(SANITIZE_TARGET)
	"$(BASH)" tests/run_tests.sh "$(SANITIZE_TARGET)" "$(CC)"

$(DEBUG_TARGET): $(SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

$(RELEASE_TARGET): $(SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -std=c99 $(WARNINGS) -O2 \
		-DMACARONICA_COMPILER_DEBUG=0 \
		-DMACARONICA_LEXER_DEBUG=0 \
		-DMACARONICA_PARSER_DEBUG=0 \
		-DMACARONICA_SEMANTIC_DEBUG=0 \
		$(LDFLAGS) -o $@ $(SOURCES) $(LDLIBS)

$(SANITIZE_TARGET): $(SOURCES) $(HEADERS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -std=c99 $(WARNINGS) -O1 -g3 \
		-fsanitize=address,undefined -fno-omit-frame-pointer \
		$(LDFLAGS) -fsanitize=address,undefined -o $@ $(SOURCES) $(LDLIBS)

clean:
	rm -rf build

# Rota C99 -> ESP-IDF; o firmware sempre usa a saida do compilador Macaronica.
PROG ?= examples/esp32_blink.mac
PORT ?= COM4
IDF_PY ?= idf.py

test-c: $(DEBUG_TARGET)
	"$(BASH)" tests/run_c_tests.sh "$(DEBUG_TARGET)" "$(CC)"

ifeq ($(EXEEXT),.exe)
placa:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action build -Program "$(PROG)" -Port "$(PORT)"

placa-gerar:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action generate -Program "$(PROG)"

placa-gravar:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action flash -Program "$(PROG)" -Port "$(PORT)"

placa-monitor:
	powershell -NoProfile -ExecutionPolicy Bypass -File tools/esp32.ps1 -Action monitor -Port "$(PORT)"
else
placa-gerar: $(RELEASE_TARGET)
	@mkdir -p build/esp32-generated
	$(RELEASE_TARGET) --alvo=esp32 "$(PROG)" -o build/esp32-generated/prog_gerado.c

placa: placa-gerar
	$(IDF_PY) -C esp32 -B ../build/esp32 -D SDKCONFIG="$(CURDIR)/build/esp32/sdkconfig" -D IDF_TARGET=esp32 build

placa-gravar: placa
	$(IDF_PY) -C esp32 -B ../build/esp32 -p "$(PORT)" -b 115200 flash

placa-monitor:
	$(IDF_PY) -C esp32 -B ../build/esp32 -p "$(PORT)" monitor
endif
