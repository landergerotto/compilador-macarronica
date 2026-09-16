#ifndef MACARONICA_COMMON_SOURCE_H
#define MACARONICA_COMMON_SOURCE_H

#include <stddef.h>

typedef struct {
    size_t offset; /* byte offset in the UTF-8 source */
    size_t line;   /* one-based */
    size_t column; /* one-based, counted in Unicode code points */
} SourcePosition;

typedef struct {
    SourcePosition start;
    SourcePosition end; /* exclusive */
} SourceSpan;

typedef struct {
    const char *path;
    const char *buffer;
    size_t length;
    int owns_path;
    int owns_buffer;
} SourceFile;

/* Initializes a non-owning view. path and buffer must outlive the SourceFile. */
void source_file_from_buffer(SourceFile *source,
                             const char *path,
                             const char *buffer,
                             size_t length);

/*
 * Loads a NUL-terminated copy of path's bytes. On failure, error_message is
 * filled when supplied. The returned SourceFile owns path and buffer.
 *
 * source must already be an empty/initialized SourceFile (for example, one
 * initialized with source_file_from_buffer(source, NULL, NULL, 0)). This
 * function does not call source_file_destroy automatically. The caller must
 * destroy a previously populated SourceFile before reusing it here; otherwise
 * its owned pointers would be lost.
 */
int source_file_load(SourceFile *source,
                     const char *path,
                     char *error_message,
                     size_t error_message_capacity);

void source_file_destroy(SourceFile *source);
SourcePosition source_position(size_t offset, size_t line, size_t column);
SourceSpan source_span(SourcePosition start, SourcePosition end);

#endif
