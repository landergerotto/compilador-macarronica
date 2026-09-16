#include "source.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *text)
{
    size_t length;
    char *copy;

    if (text == NULL) {
        return NULL;
    }
    length = strlen(text);
    if (length == (size_t)-1) {
        errno = ENOMEM;
        return NULL;
    }
    copy = (char *)malloc(length + 1U);
    if (copy != NULL) {
        memcpy(copy, text, length + 1U);
    }
    return copy;
}

static void set_error(char *message, size_t capacity, const char *path)
{
    if (message != NULL && capacity > 0U) {
        (void)snprintf(message, capacity, "nao foi possivel ler '%s': %s",
                       path != NULL ? path : "<fonte>", strerror(errno));
    }
}

void source_file_from_buffer(SourceFile *source,
                             const char *path,
                             const char *buffer,
                             size_t length)
{
    if (source == NULL) {
        return;
    }
    source->path = path != NULL ? path : "<memoria>";
    source->buffer = buffer != NULL ? buffer : "";
    source->length = buffer != NULL ? length : 0U;
    source->owns_path = 0;
    source->owns_buffer = 0;
}

int source_file_load(SourceFile *source,
                     const char *path,
                     char *error_message,
                     size_t error_message_capacity)
{
    FILE *file;
    long file_size;
    size_t file_length;
    char *buffer;
    char *path_copy;
    size_t bytes_read;

    if (source == NULL || path == NULL) {
        errno = EINVAL;
        set_error(error_message, error_message_capacity, path);
        return 0;
    }
    source_file_from_buffer(source, NULL, NULL, 0U);

    file = fopen(path, "rb");
    if (file == NULL) {
        set_error(error_message, error_message_capacity, path);
        return 0;
    }
    if (fseek(file, 0L, SEEK_END) != 0 || (file_size = ftell(file)) < 0L ||
        fseek(file, 0L, SEEK_SET) != 0) {
        set_error(error_message, error_message_capacity, path);
        (void)fclose(file);
        return 0;
    }

    if ((unsigned long long)file_size >
        (unsigned long long)((size_t)-1) - 1ULL) {
        (void)fclose(file);
        errno = EFBIG;
        set_error(error_message, error_message_capacity, path);
        return 0;
    }
    file_length = (size_t)file_size;
    buffer = (char *)malloc(file_length + 1U);
    path_copy = copy_string(path);
    if (buffer == NULL || path_copy == NULL) {
        free(buffer);
        free(path_copy);
        (void)fclose(file);
        errno = ENOMEM;
        set_error(error_message, error_message_capacity, path);
        return 0;
    }
    errno = 0;
    bytes_read = fread(buffer, 1U, file_length, file);
    if (bytes_read != file_length || ferror(file)) {
        free(buffer);
        free(path_copy);
        if (errno == 0) {
            errno = EIO;
        }
        set_error(error_message, error_message_capacity, path);
        (void)fclose(file);
        return 0;
    }
    buffer[bytes_read] = '\0';
    (void)fclose(file);

    source->path = path_copy;
    source->buffer = buffer;
    source->length = bytes_read;
    source->owns_path = 1;
    source->owns_buffer = 1;
    if (error_message != NULL && error_message_capacity > 0U) {
        error_message[0] = '\0';
    }
    return 1;
}

void source_file_destroy(SourceFile *source)
{
    if (source == NULL) {
        return;
    }
    if (source->owns_buffer) {
        free((void *)source->buffer);
    }
    if (source->owns_path) {
        free((void *)source->path);
    }
    source_file_from_buffer(source, NULL, NULL, 0U);
}

SourcePosition source_position(size_t offset, size_t line, size_t column)
{
    SourcePosition position;
    position.offset = offset;
    position.line = line;
    position.column = column;
    return position;
}

SourceSpan source_span(SourcePosition start, SourcePosition end)
{
    SourceSpan span;
    span.start = start;
    span.end = end;
    return span;
}
