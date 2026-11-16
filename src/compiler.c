#include "compiler.h"

#include "common/diagnostic.h"
#include "common/source.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "ir/codegen_llvm.h"
#include "codegen_c.h"
#include "target.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void dump_tokens(const TokenBuffer *tokens, const char *path)
{
    size_t index;

    (void)fputs("tokens:\n", stdout);
    for (index = 0U; index < tokens->count; ++index) {
        token_print(&tokens->items[index], path, stdout);
    }
}

/*
 * A saida e gerada primeiro em "<saida>.tmp", ao lado dela, e so substitui a
 * saida real (rename) depois de toda a IR ter sido gerada e gravada com
 * sucesso. Em qualquer erro o temporario e apagado, entao nada parcial fica
 * para tras e uma saida preexistente e preservada. Nao usamos tmpfile(): no
 * Windows (msvcrt) ele cria o arquivo na raiz do drive do diretorio atual e
 * falha quando ali nao ha permissao de escrita (por exemplo, C:\).
 */
static CompilerExitCode write_generated_output(const SourceFile *source,
                                          const AstNode *program,
                                          const SemanticResult *semantic,
                                          DiagnosticBag *diagnostics,
                                          const char *output_path,
                                          int emit_c)
{
    static const char suffix[] = ".tmp";
    size_t path_length = strlen(output_path);
    char *temporary_path = malloc(path_length + sizeof(suffix));
    FILE *temporary;
    FrontendStatus status;
    CompilerExitCode result = COMPILER_EXIT_IO_ERROR;
    int write_failed;

    if (temporary_path == NULL) {
        (void)fputs("erro interno: memoria insuficiente\n", stderr);
        return COMPILER_EXIT_INTERNAL_ERROR;
    }
    memcpy(temporary_path, output_path, path_length);
    memcpy(temporary_path + path_length, suffix, sizeof(suffix));

    temporary = fopen(temporary_path, "wb");
    if (temporary == NULL) {
        (void)fprintf(stderr, "erro de E/S: nao foi possivel abrir `%s` para escrita: %s\n",
                      temporary_path, strerror(errno));
        free(temporary_path);
        return COMPILER_EXIT_IO_ERROR;
    }
    status = emit_c ? codegen_c_emit(source, program, semantic, diagnostics, temporary)
                    : codegen_llvm_emit(source, program, semantic, diagnostics, temporary);
    write_failed = ferror(temporary) != 0;
    if (fclose(temporary) != 0) write_failed = 1;
    if (status != FRONTEND_STATUS_OK) {
        result = write_failed ? COMPILER_EXIT_IO_ERROR :
                 status == FRONTEND_STATUS_SOURCE_ERROR ? COMPILER_EXIT_SOURCE_ERROR :
                 COMPILER_EXIT_INTERNAL_ERROR;
        goto cleanup;
    }
    if (write_failed) {
        (void)fprintf(stderr, "erro de E/S: falha ao gravar `%s`\n", temporary_path);
        goto cleanup;
    }
    if (rename(temporary_path, output_path) != 0) {
        /* No Windows rename nao substitui um destino que ja existe. */
        (void)remove(output_path);
        if (rename(temporary_path, output_path) != 0) {
            (void)fprintf(stderr, "erro de E/S: nao foi possivel publicar `%s`: %s\n",
                          output_path, strerror(errno));
            goto cleanup;
        }
    }
    result = COMPILER_EXIT_SUCCESS;

cleanup:
    if (result != COMPILER_EXIT_SUCCESS) (void)remove(temporary_path);
    free(temporary_path);
    return result;
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
        dump_tokens(&tokens, path);
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

    phase_status = target_validate(&source, program, &semantic, &diagnostics,
                                   options->target_esp32, options->ram_limit);
    if (phase_status != FRONTEND_STATUS_OK) {
        exit_code = phase_status == FRONTEND_STATUS_SOURCE_ERROR
                        ? COMPILER_EXIT_SOURCE_ERROR : COMPILER_EXIT_INTERNAL_ERROR;
        goto diagnostics_and_cleanup;
    }
    if (options->output_path != NULL) {
        exit_code = write_generated_output(&source, program, &semantic, &diagnostics,
                                           options->output_path, options->emit_c);
        if (exit_code != COMPILER_EXIT_SUCCESS) goto diagnostics_and_cleanup;
    }
    exit_code = COMPILER_EXIT_SUCCESS;
    if (!options->dump_tokens && !options->dump_ast && !options->dump_symbols) {
        if (options->output_path != NULL)
            (void)printf("%s: %s gerado em %s\n", path,
                          options->emit_c ? "C99" : "LLVM IR", options->output_path);
        else
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
