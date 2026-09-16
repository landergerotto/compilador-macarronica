#ifndef MACARONICA_COMPILER_H
#define MACARONICA_COMPILER_H

typedef struct {
    int dump_tokens;
    int dump_ast;
    int dump_symbols;
} CompilerOptions;

typedef enum {
    COMPILER_EXIT_SUCCESS = 0,
    COMPILER_EXIT_SOURCE_ERROR = 1,
    COMPILER_EXIT_USAGE_ERROR = 2,
    COMPILER_EXIT_IO_ERROR = 3,
    COMPILER_EXIT_INTERNAL_ERROR = 4
} CompilerExitCode;

CompilerExitCode compiler_compile_file(const char *path,
                                       const CompilerOptions *options);

#endif
