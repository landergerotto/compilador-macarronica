#include "compiler.h"

#include <stdio.h>
#include <string.h>

#define MACARONICA_VERSION "0.1.0"

static void print_usage(FILE *stream, const char *program_name)
{
    (void)fprintf(stream,
                  "Uso: %s [--dump-tokens] [--dump-ast] [--dump-symbols] arquivo.mac\n"
                  "     %s --help\n"
                  "     %s --version\n",
                  program_name, program_name, program_name);
}

static int has_mac_extension(const char *path)
{
    size_t length;

    if (path == NULL) {
        return 0;
    }
    length = strlen(path);
    return length >= 4U && strcmp(path + length - 4U, ".mac") == 0;
}

int main(int argc, char **argv)
{
    CompilerOptions options = {0, 0, 0};
    const char *input_path = NULL;
    const char *program_name = argc > 0 && argv[0] != NULL ? argv[0] : "macaronica";
    int index;

    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        print_usage(stdout, program_name);
        return COMPILER_EXIT_SUCCESS;
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        (void)printf("Macaronica %s\n", MACARONICA_VERSION);
        return COMPILER_EXIT_SUCCESS;
    }

    for (index = 1; index < argc; ++index) {
        const char *argument = argv[index];
        if (strcmp(argument, "--dump-tokens") == 0) {
            options.dump_tokens = 1;
        } else if (strcmp(argument, "--dump-ast") == 0) {
            options.dump_ast = 1;
        } else if (strcmp(argument, "--dump-symbols") == 0) {
            options.dump_symbols = 1;
        } else if (argument[0] == '-') {
            (void)fprintf(stderr, "opcao desconhecida: %s\n", argument);
            print_usage(stderr, program_name);
            return COMPILER_EXIT_USAGE_ERROR;
        } else if (input_path != NULL) {
            (void)fputs("erro: informe exatamente um arquivo .mac\n", stderr);
            print_usage(stderr, program_name);
            return COMPILER_EXIT_USAGE_ERROR;
        } else {
            input_path = argument;
        }
    }

    if (input_path == NULL) {
        (void)fputs("erro: arquivo .mac nao informado\n", stderr);
        print_usage(stderr, program_name);
        return COMPILER_EXIT_USAGE_ERROR;
    }
    if (!has_mac_extension(input_path)) {
        (void)fprintf(stderr, "erro: o arquivo deve ter extensao .mac: %s\n", input_path);
        return COMPILER_EXIT_USAGE_ERROR;
    }
    return (int)compiler_compile_file(input_path, &options);
}
