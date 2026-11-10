#ifndef MACARONICA_COMMON_SOURCE_H
#define MACARONICA_COMMON_SOURCE_H

#include <stddef.h>

/* Onde estamos dentro do arquivo. */
typedef struct {
    size_t offset; /* Deslocamento em bytes na fonte UTF-8. */
    size_t line;   /* Linha, iniciada em 1. */
    size_t column; /* Coluna em pontos de codigo Unicode, iniciada em 1. */
} SourcePosition;

/* Um pedaco da fonte. O inicio entra no intervalo, mas o fim nao. */
typedef struct {
    SourcePosition start;
    SourcePosition end; /* Posicao final exclusiva. */
} SourceSpan;

/*
 * O arquivo inteiro carregado na memoria. Os campos owns_* dizem se o texto e
 * o caminho foram alocados aqui e, por isso, precisam ser liberados depois.
 */
typedef struct {
    const char *path;
    const char *buffer;
    size_t length;
    int owns_path;
    int owns_buffer;
} SourceFile;

/*
 * Usa um texto que ja existe, sem copiar. Por isso path e buffer precisam
 * continuar vivos enquanto o SourceFile estiver sendo usado.
 */
void source_file_from_buffer(SourceFile *source,
                             const char *path,
                             const char *buffer,
                             size_t length);

/*
 * Le o arquivo do disco e guarda uma copia do caminho e do conteudo. Retorna 1
 * se conseguiu e 0 se falhou. Nesse caso, error_message explica o motivo.
 */
int source_file_load(SourceFile *source,
                     const char *path,
                     char *error_message,
                     size_t error_message_capacity);

/* Limpa o SourceFile. So da free no que realmente pertence a ele. */
void source_file_destroy(SourceFile *source);

/* Atalhos pra montar posicoes e intervalos sem repetir a struct toda. */
SourcePosition source_position(size_t offset, size_t line, size_t column);
SourceSpan source_span(SourcePosition start, SourcePosition end);

#endif
