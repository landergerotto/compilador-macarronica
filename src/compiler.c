#include "compiler.h"

#include "common/diagnostic.h"
#include "common/source.h"
#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/ast.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "ir/codegen_llvm.h"
#include "ir/codegen_mlir.h"
#include "ir/dsl.h"
#include "codegen_c.h"
#include "target.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#ifndef MACARONICA_COMPILER_DEBUG
#define MACARONICA_COMPILER_DEBUG 1
#endif

static void dump_tokens(const TokenBuffer *tokens, const char *path)
{
    size_t index;

    (void)fputs("tokens:\n", stdout);
    for (index = 0U; index < tokens->count; ++index) {
        token_print(&tokens->items[index], path, stdout);
    }
}

/*
 * tmpfile e C99 e apaga seu armazenamento ao fechar. So abrimos a saida real
 * depois de toda a IR ter sido gerada e descarregada com sucesso. Assim um
 * erro de fonte ou de codegen preserva ate uma saida preexistente.
 */
static CompilerExitCode write_generated_output(const SourceFile *source,
                                          const AstNode *program,
                                          const SemanticResult *semantic,
                                          DiagnosticBag *diagnostics,
                                          const char *output_path,
                                          int emit_c, int emit_mlir)
{
    FILE *temporary = tmpfile();
    FILE *output = NULL;
    FrontendStatus status;
    CompilerExitCode result = COMPILER_EXIT_IO_ERROR;
    unsigned char buffer[8192];
    size_t count;
    int io_failed = 0;
    int output_created = 0;

    if (temporary == NULL) {
        (void)fprintf(stderr, "erro de E/S: nao foi possivel criar temporario para `%s`: %s\n",
                      output_path, strerror(errno));
        return COMPILER_EXIT_IO_ERROR;
    }
    status = emit_mlir ? codegen_mlir_emit(source, program, semantic, diagnostics, temporary)
                    : emit_c ? codegen_c_emit(source, program, semantic, diagnostics, temporary)
                    : codegen_llvm_emit(source, program, semantic, diagnostics, temporary);
    if (status != FRONTEND_STATUS_OK) {
        result = ferror(temporary) ? COMPILER_EXIT_IO_ERROR :
                 status == FRONTEND_STATUS_SOURCE_ERROR ? COMPILER_EXIT_SOURCE_ERROR :
                 COMPILER_EXIT_INTERNAL_ERROR;
        goto cleanup;
    }
    if (fflush(temporary) != 0 || fseek(temporary, 0L, SEEK_SET) != 0) {
        (void)fprintf(stderr, "erro de E/S: nao foi possivel finalizar IR temporaria: %s\n", strerror(errno));
        goto cleanup;
    }
    output = fopen(output_path, "wb");
    if (output == NULL) {
        (void)fprintf(stderr, "erro de E/S: nao foi possivel abrir `%s` para escrita: %s\n",
                      output_path, strerror(errno));
        goto cleanup;
    }
    output_created = 1;
    while ((count = fread(buffer, 1U, sizeof(buffer), temporary)) != 0U) {
        if (fwrite(buffer, 1U, count, output) != count) {
            io_failed = 1;
            break;
        }
    }
    if (ferror(temporary)) io_failed = 1;
    if (fclose(output) != 0) io_failed = 1;
    output = NULL;
    if (io_failed) {
        (void)fprintf(stderr, "erro de E/S: falha ao publicar `%s`\n", output_path);
        goto cleanup;
    }
    result = COMPILER_EXIT_SUCCESS;

cleanup:
    if (output != NULL && fclose(output) != 0) result = COMPILER_EXIT_IO_ERROR;
    if (fclose(temporary) != 0 && result == COMPILER_EXIT_SUCCESS) {
        (void)fputs("erro de E/S: falha ao fechar IR temporaria\n", stderr);
        result = COMPILER_EXIT_IO_ERROR;
    }
    if (result != COMPILER_EXIT_SUCCESS && output_created && remove(output_path) != 0) {
        (void)fprintf(stderr, "erro de E/S: nao foi possivel remover saida incompleta `%s`\n", output_path);
    }
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
                                           options->output_path, options->emit_c, options->emit_mlir);
        if (exit_code != COMPILER_EXIT_SUCCESS) goto diagnostics_and_cleanup;
    }
    exit_code = COMPILER_EXIT_SUCCESS;
    if (!options->dump_tokens && !options->dump_ast && !options->dump_symbols) {
        if (options->output_path != NULL)
            (void)printf("%s: %s gerado em %s\n", path,
                          options->emit_mlir ? "MLIR" : options->emit_c ? "C99" : "LLVM IR", options->output_path);
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
