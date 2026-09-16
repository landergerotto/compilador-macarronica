#include "compiler.h"

#include "common/diagnostic.h"
#include "common/source.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "semantic/semantic.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef MACARONICA_COMPILER_DEBUG
#define MACARONICA_COMPILER_DEBUG 1
#endif

static void dump_tokens(const TokenBuffer *tokens)
{
    size_t index;

    (void)fputs("tokens:\n", stdout);
    for (index = 0U; index < tokens->count; ++index) {
        token_print(&tokens->items[index], stdout);
    }
}

CompilerExitCode compiler_compile_file(const char *path,
                                       const CompilerOptions *options)
{
    SourceFile source;
    DiagnosticBag diagnostics;
    TokenBuffer tokens;
    AstNode *program = NULL;
    SemanticResult semantic;
    CompilerExitCode exit_code = COMPILER_EXIT_INTERNAL_ERROR;
    char io_error[512];
    int semantic_initialized = 0;
    FrontendStatus phase_status;

    source_file_from_buffer(&source, NULL, NULL, 0U);
    diagnostic_bag_init(&diagnostics);
    token_buffer_init(&tokens);
    (void)memset(&semantic, 0, sizeof(semantic));

    if (path == NULL || options == NULL) {
        (void)fprintf(stderr, "erro interno: parametros invalidos do compilador\n");
        goto cleanup;
    }

#if MACARONICA_COMPILER_DEBUG
    (void)fprintf(stderr, "[debug][compiler] carregando `%s`\n", path); // debug - remover
#endif

    if (!source_file_load(&source, path, io_error, sizeof(io_error))) {
        if (errno == ENOMEM) {
            (void)fprintf(stderr, "erro interno: %s\n", io_error);
            exit_code = COMPILER_EXIT_INTERNAL_ERROR;
        } else {
            (void)fprintf(stderr, "erro de E/S: %s\n", io_error);
            exit_code = COMPILER_EXIT_IO_ERROR;
        }
        goto cleanup;
    }

    phase_status = lexer_scan_all(&source, &diagnostics, &tokens);
    if (phase_status != FRONTEND_STATUS_OK) {
        exit_code = phase_status == FRONTEND_STATUS_SOURCE_ERROR
                        ? COMPILER_EXIT_SOURCE_ERROR
                        : COMPILER_EXIT_INTERNAL_ERROR;
        goto diagnostics_and_cleanup;
    }
    if (options->dump_tokens) {
        dump_tokens(&tokens);
    }

    phase_status = parser_parse(&source, &tokens, &diagnostics, &program);
    if (phase_status != FRONTEND_STATUS_OK) {
        exit_code = phase_status == FRONTEND_STATUS_SOURCE_ERROR
                        ? COMPILER_EXIT_SOURCE_ERROR
                        : COMPILER_EXIT_INTERNAL_ERROR;
        goto diagnostics_and_cleanup;
    }
    if (program == NULL) {
        (void)fprintf(stderr, "erro interno: parser nao produziu uma AST\n");
        exit_code = COMPILER_EXIT_INTERNAL_ERROR;
        goto cleanup;
    }
    if (options->dump_ast) {
        ast_dump(program, stdout);
    }

    if (!semantic_result_init(&semantic)) {
        (void)fprintf(stderr, "erro interno: nao foi possivel inicializar a semantica\n");
        exit_code = COMPILER_EXIT_INTERNAL_ERROR;
        goto cleanup;
    }
    semantic_initialized = 1;
    phase_status = semantic_analyze(&source, program, &diagnostics, &semantic);
    if (options->dump_symbols && semantic.analyzed) {
        semantic_dump_symbols(&semantic, stdout);
    }
    if (phase_status != FRONTEND_STATUS_OK) {
        exit_code = phase_status == FRONTEND_STATUS_SOURCE_ERROR
                        ? COMPILER_EXIT_SOURCE_ERROR
                        : COMPILER_EXIT_INTERNAL_ERROR;
        goto diagnostics_and_cleanup;
    }

    exit_code = COMPILER_EXIT_SUCCESS;
    if (!options->dump_tokens && !options->dump_ast && !options->dump_symbols) {
        (void)printf("%s: arquivo lexica, sintatica e semanticamente valido\n", path);
    }
    goto cleanup;

diagnostics_and_cleanup:
    diagnostic_bag_print(&diagnostics, stderr);
    if (exit_code == COMPILER_EXIT_INTERNAL_ERROR && diagnostics.count == 0U) {
        (void)fputs("erro interno: a fase falhou sem produzir diagnostico\n", stderr);
    }

cleanup:
    if (semantic_initialized) {
        semantic_result_destroy(&semantic);
    }
    ast_node_destroy(program);
    token_buffer_destroy(&tokens);
    diagnostic_bag_destroy(&diagnostics);
    source_file_destroy(&source);
    return exit_code;
}
