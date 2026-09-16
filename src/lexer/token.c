#include "token.h"

#include <stdlib.h>
#include <string.h>

void token_buffer_init(TokenBuffer *buffer)
{
    if (buffer == NULL) {
        return;
    }
    buffer->items = NULL;
    buffer->count = 0U;
    buffer->capacity = 0U;
}

void token_buffer_destroy(TokenBuffer *buffer)
{
    if (buffer == NULL) {
        return;
    }
    free(buffer->items);
    token_buffer_init(buffer);
}

int token_buffer_push(TokenBuffer *buffer, Token token)
{
    Token *items;
    size_t capacity;

    if (buffer == NULL) {
        return 0;
    }
    if (buffer->count == buffer->capacity) {
        if (buffer->capacity == 0U) {
            capacity = 32U;
        } else {
            if (buffer->capacity > ((size_t)-1) / 2U) {
                return 0;
            }
            capacity = buffer->capacity * 2U;
        }
        if (capacity > ((size_t)-1) / sizeof(*items)) {
            return 0;
        }
        items = (Token *)realloc(buffer->items, capacity * sizeof(*items));
        if (items == NULL) {
            return 0;
        }
        buffer->items = items;
        buffer->capacity = capacity;
    }
    buffer->items[buffer->count++] = token;
    return 1;
}

const char *token_kind_name(TokenKind kind)
{
    static const char *const names[] = {
        "INVALIDO", "EOF", "IDENT", "LIT_INTEIRA", "LIT_REAL", "LIT_PALAVRA",
        "bool", "inteira", "flut", "palavra", "duplocarpado", "vazio",
        "receba", "repete", "durante", "cond", "casocontrario", "principal",
        "respost", "registro", "outrafuncao", "vet", "edai", "func", "raiz",
        "VER", "FAL", "OU", "E", "N\xC3\x83O", "XOR", "+", "-", "*", "/", "//",
        "^", "<", ">", "==", "!", ";", ",", "@", "(", ")", "[", "]", "{", "}"
    };
    size_t count = sizeof(names) / sizeof(names[0]);
    return (int)kind >= 0 && (size_t)kind < count ? names[(size_t)kind] : "<token-desconhecido>";
}

int token_lexeme_equals(const Token *token, const char *text)
{
    size_t length;
    if (token == NULL || text == NULL) {
        return 0;
    }
    length = strlen(text);
    return token->length == length && memcmp(token->start, text, length) == 0;
}

void token_print(const Token *token, FILE *stream)
{
    if (token == NULL) {
        return;
    }
    if (stream == NULL) {
        stream = stdout;
    }
    (void)fprintf(stream, "%s:%zu:%zu %-18s `",
                  "token",
                  token->span.start.line,
                  token->span.start.column,
                  token_kind_name(token->kind));
    if (token->start != NULL && token->length > 0U) {
        (void)fwrite(token->start, 1U, token->length, stream);
    }
    (void)fputs("`\n", stream);
}
