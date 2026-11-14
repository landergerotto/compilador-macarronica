#include "compiler.h"

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define MACARONICA_VERSION "0.3.0"

static void print_usage(FILE *stream, const char *program_name)
{
    (void)fprintf(stream,
                  "Uso: %s [--dump-tokens] [--dump-ast] [--dump-symbols] arquivo.mac [-o saida.ll]\n"
                  "     --emit-c -o saida.c       gera C99 da AST validada\n"
                  "     --alvo=esp32             gera C para ESP-IDF\n"
                  "     --ram-limite=BYTES       orcamento fonte ESP32 (padrao: 65536)\n"
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
    /* .mac e a extensao do grupo; .mcn, a dos casos de teste do professor. */
    return length >= 4U && (strcmp(path + length - 4U, ".mac") == 0 ||
                            strcmp(path + length - 4U, ".mcn") == 0);
}

int main(int argc, char **argv)
{
    CompilerOptions options = {0, 0, 0, NULL, 0, 0, 65536U};
    int ram_limit_given = 0;
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
        } else if (strcmp(argument, "--emit-c") == 0) {
            options.emit_c = 1;
        } else if (strcmp(argument, "--alvo=esp32") == 0) {
            options.target_esp32 = 1;
            options.emit_c = 1;
        } else if (strncmp(argument, "--ram-limite=", 13U) == 0) {
            const char *digits = argument + 13U;
            char *end;
            unsigned long long limit;
            errno = 0;
            limit = strtoull(digits, &end, 10);
            if (*digits < '0' || *digits > '9' || *end != '\0' ||
                errno == ERANGE || limit == 0U || limit > SIZE_MAX) {
                (void)fputs("erro: --ram-limite exige numero de bytes positivo representavel\n", stderr);
                return COMPILER_EXIT_USAGE_ERROR;
            }
            options.ram_limit = (size_t)limit;
            ram_limit_given = 1;
        } else if (strcmp(argument, "-o") == 0) {
            if (options.output_path != NULL || index + 1 >= argc ||
                argv[index + 1][0] == '\0' || argv[index + 1][0] == '-') {
                (void)fputs("erro: use -o uma unica vez, seguido do caminho de saida\n", stderr);
                print_usage(stderr, program_name);
                return COMPILER_EXIT_USAGE_ERROR;
            }
            options.output_path = argv[++index];
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
        (void)fprintf(stderr, "erro: o arquivo deve ter extensao .mac ou .mcn: %s\n", input_path);
        return COMPILER_EXIT_USAGE_ERROR;
    }
    if (options.output_path != NULL && strcmp(input_path, options.output_path) == 0) {
        (void)fputs("erro: entrada e saida devem ter caminhos diferentes\n", stderr);
        return COMPILER_EXIT_USAGE_ERROR;
    }
    if (ram_limit_given && !options.target_esp32) {
        (void)fputs("erro: --ram-limite exige --alvo=esp32\n", stderr);
        return COMPILER_EXIT_USAGE_ERROR;
    }
    if (options.emit_c && options.output_path == NULL) {
        (void)fputs("erro: --emit-c e --alvo=esp32 exigem -o saida.c\n", stderr);
        return COMPILER_EXIT_USAGE_ERROR;
    }
    return (int)compiler_compile_file(input_path, &options);
}
