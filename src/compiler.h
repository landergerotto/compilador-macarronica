#ifndef MACARONICA_COMPILER_H
#define MACARONICA_COMPILER_H

/*
 * Opcoes pra quando a gente quer enxergar o que cada parte do compilador fez.
 * Elas so ligam os dumps, nao mudam a analise do programa.
 */
typedef struct {
    int dump_tokens;
    int dump_ast;
    int dump_symbols;
} CompilerOptions;

/* O numero que o programa devolve quando termina. */
typedef enum {
    COMPILER_EXIT_SUCCESS = 0,       /* Deu tudo certo. */
    COMPILER_EXIT_SOURCE_ERROR = 1,  /* O arquivo tem algum erro. */
    COMPILER_EXIT_USAGE_ERROR = 2,   /* O comando foi usado errado. */
    COMPILER_EXIT_IO_ERROR = 3,      /* Nao deu pra ler o arquivo. */
    COMPILER_EXIT_INTERNAL_ERROR = 4 /* O compilador falhou internamente. */
} CompilerExitCode;

/*
 * Essa e a funcao que junta tudo: carrega o arquivo, chama lexer, parser e
 * semantica e depois limpa o que foi criado. path e options so sao lidos aqui.
 */
CompilerExitCode compiler_compile_file(const char *path,
                                       const CompilerOptions *options);

#endif
