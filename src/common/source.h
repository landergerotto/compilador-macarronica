#ifndef MACARONICA_COMMON_SOURCE_H
#define MACARONICA_COMMON_SOURCE_H

#include <stddef.h>

typedef struct {
    size_t offset; /* Deslocamento em bytes na fonte UTF-8. */
    size_t line;   /* Linha, iniciada em 1. */
    size_t column; /* Coluna em pontos de codigo Unicode, iniciada em 1. */
} SourcePosition;

typedef struct {
    SourcePosition start;
    SourcePosition end; /* Posicao final exclusiva. */
} SourceSpan;

typedef struct {
    const char *path;
    const char *buffer;
    size_t length;
    int owns_path;
    int owns_buffer;
} SourceFile;

/* Cria uma visao sem posse; path e buffer devem sobreviver ao SourceFile. */
void source_file_from_buffer(SourceFile *source,
                             const char *path,
                             const char *buffer,
                             size_t length);

/*
 * Carrega uma copia terminada em NUL e transfere ao SourceFile a posse de path
 * e buffer. source deve estar vazio ou inicializado; antes de reutiliza-lo, o
 * chamador deve destruir seu conteudo anterior. Em falha, preenche error_message
 * quando esse buffer for fornecido.
 */
int source_file_load(SourceFile *source,
                     const char *path,
                     char *error_message,
                     size_t error_message_capacity);

void source_file_destroy(SourceFile *source);
SourcePosition source_position(size_t offset, size_t line, size_t column);
SourceSpan source_span(SourcePosition start, SourcePosition end);

#endif
