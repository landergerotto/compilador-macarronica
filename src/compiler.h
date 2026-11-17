#ifndef MACARONICA_COMPILER_H
#define MACARONICA_COMPILER_H

#include <stddef.h>

/*
 * Os dumps mostram as fases. output_path solicita LLVM IR depois da semantica;
 * NULL mantem o modo de validacao. A string pertence ao chamador.
 */
typedef struct {
    int dump_tokens;
    int dump_ast;
    int dump_symbols;
    const char *output_path;
    int emit_c;
    int target_esp32;
    size_t ram_limit;
    int emit_mlir;
} CompilerOptions;

/* O numero que o programa devolve quando termina. */
typedef enum {
    COMPILER_EXIT_SUCCESS = 0,       /* Deu tudo certo. */
    COMPILER_EXIT_SOURCE_ERROR = 1,  /* O arquivo tem algum erro. */
    COMPILER_EXIT_USAGE_ERROR = 2,   /* O comando foi usado errado. */
    COMPILER_EXIT_IO_ERROR = 3,      /* Falha de leitura ou escrita. */
    COMPILER_EXIT_INTERNAL_ERROR = 4 /* O compilador falhou internamente. */
} CompilerExitCode;

/*
 * Essa e a funcao que junta tudo: carrega o arquivo, chama lexer, parser e
 * semantica, emite IR quando solicitada e limpa os recursos. path/options sao
 * somente lidos. Entrada invalida nunca abre ou trunca a saida solicitada.
 */
CompilerExitCode compiler_compile_file(const char *path,
                                       const CompilerOptions *options);

#endif
