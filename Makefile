CC = gcc
CPPFLAGS = -Isrc
WARNINGS = -Wall -Wextra -pedantic
CFLAGS = -std=c99 $(WARNINGS) -O0 -g3
LDFLAGS =
LDLIBS = -lm

ifeq ($(OS),Windows_NT)
EXEEXT = .exe
BASH = C:/msys64/usr/bin/bash.exe
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

HEADERS = $(wildcard src/*.h src/*/*.h)

DEBUG_TARGET = build/debug/macaronica$(EXEEXT)
RELEASE_TARGET = build/release/macaronica$(EXEEXT)
SANITIZE_TARGET = build/sanitize/macaronica$(EXEEXT)

.PHONY: all debug release test sanitize clean

all: $(DEBUG_TARGET)

debug: $(DEBUG_TARGET)

release: $(RELEASE_TARGET)

test: $(DEBUG_TARGET)
	$(BASH) tests/run_tests.sh "$(DEBUG_TARGET)"

sanitize: $(SANITIZE_TARGET)
	$(BASH) tests/run_tests.sh "$(SANITIZE_TARGET)"

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
