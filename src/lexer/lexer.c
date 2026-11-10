#include "lexer.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const SourceFile *source;
    DiagnosticBag *diagnostics;
    TokenBuffer *tokens;
    size_t current;
    size_t line;
    size_t column;
    int failed;
} Lexer;

typedef struct {
    const char *text;
    size_t length;
    TokenKind kind;
} Keyword;

static const Keyword KEYWORDS[] = {
    {"bool", 4U, TOKEN_KW_BOOL}, {"inteira", 7U, TOKEN_KW_INTEIRA},
    {"flut", 4U, TOKEN_KW_FLUT}, {"palavra", 7U, TOKEN_KW_PALAVRA},
    {"duplocarpado", 12U, TOKEN_KW_DUPLOCARPADO}, {"vazio", 5U, TOKEN_KW_VAZIO},
    {"receba", 6U, TOKEN_KW_RECEBA}, {"repete", 6U, TOKEN_KW_REPETE},
    {"durante", 7U, TOKEN_KW_DURANTE}, {"cond", 4U, TOKEN_KW_COND},
    {"casocontrario", 13U, TOKEN_KW_CASOCONTRARIO}, {"principal", 9U, TOKEN_KW_PRINCIPAL},
    {"respost", 7U, TOKEN_KW_RESPOST}, {"registro", 8U, TOKEN_KW_REGISTRO},
    {"outrafuncao", 11U, TOKEN_KW_OUTRAFUNCAO}, {"vet", 3U, TOKEN_KW_VET},
    {"edai", 4U, TOKEN_KW_EDAI}, {"func", 4U, TOKEN_KW_FUNC},
    {"raiz", 4U, TOKEN_KW_RAIZ}, {"VER", 3U, TOKEN_TRUE},
    {"FAL", 3U, TOKEN_FALSE}, {"OU", 2U, TOKEN_LOGICAL_OR},
    {"E", 1U, TOKEN_LOGICAL_AND}, {"XOR", 3U, TOKEN_LOGICAL_XOR}
};

static SourcePosition position(const Lexer *lexer)
{
    return source_position(lexer->current, lexer->line, lexer->column);
}

static int at_end(const Lexer *lexer)
{
    return lexer->current >= lexer->source->length;
}

static unsigned char peek_byte(const Lexer *lexer, size_t lookahead)
{
    size_t offset = lexer->current + lookahead;
    return offset < lexer->source->length
               ? (unsigned char)lexer->source->buffer[offset]
               : 0U;
}

static int is_ascii_letter(unsigned char byte)
{
    return (byte >= (unsigned char)'a' && byte <= (unsigned char)'z') ||
           (byte >= (unsigned char)'A' && byte <= (unsigned char)'Z');
}

static int is_ascii_digit(unsigned char byte)
{
    return byte >= (unsigned char)'0' && byte <= (unsigned char)'9';
}

static int is_identifier_continue(unsigned char byte)
{
    return is_ascii_letter(byte) || is_ascii_digit(byte);
}

static size_t utf8_length(const unsigned char *bytes, size_t remaining, int *valid)
{
    unsigned char first;
    size_t length;
    size_t index;

    *valid = 0;
    if (remaining == 0U) {
        return 0U;
    }
    first = bytes[0];
    if (first < 0x80U) {
        *valid = 1;
        return 1U;
    }
    if (first >= 0xC2U && first <= 0xDFU) {
        length = 2U;
    } else if (first >= 0xE0U && first <= 0xEFU) {
        length = 3U;
    } else if (first >= 0xF0U && first <= 0xF4U) {
        length = 4U;
    } else {
        return 1U;
    }
    if (remaining < length) {
        return 1U;
    }
    for (index = 1U; index < length; ++index) {
        if ((bytes[index] & 0xC0U) != 0x80U) {
            return 1U;
        }
    }
    if ((first == 0xE0U && bytes[1] < 0xA0U) ||
        (first == 0xEDU && bytes[1] >= 0xA0U) ||
        (first == 0xF0U && bytes[1] < 0x90U) ||
        (first == 0xF4U && bytes[1] >= 0x90U)) {
        return 1U;
    }
    *valid = 1;
    return length;
}

static size_t advance_character(Lexer *lexer, int *valid)
{
    const unsigned char *bytes;
    size_t length;

    if (at_end(lexer)) {
        *valid = 1;
        return 0U;
    }
    bytes = (const unsigned char *)lexer->source->buffer + lexer->current;
    if (bytes[0] == (unsigned char)'\r') {
        length = 1U;
        if (lexer->current + 1U < lexer->source->length && bytes[1] == (unsigned char)'\n') {
            length = 2U;
        }
        lexer->current += length;
        ++lexer->line;
        lexer->column = 1U;
        *valid = 1;
        return length;
    }
    if (bytes[0] == (unsigned char)'\n') {
        ++lexer->current;
        ++lexer->line;
        lexer->column = 1U;
        *valid = 1;
        return 1U;
    }
    length = utf8_length(bytes, lexer->source->length - lexer->current, valid);
    lexer->current += length;
    ++lexer->column;
    return length;
}

static void add_lexical_error(Lexer *lexer,
                              SourcePosition start,
                              const char *code,
                              const char *message)
{
    SourceSpan span = source_span(start, position(lexer));
    if (!diagnostic_bag_add(lexer->diagnostics, lexer->source, span,
                            DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_LEXICAL,
                            code, "%s", message)) {
        lexer->failed = 1;
    }
}

static int emit_token(Lexer *lexer, TokenKind kind, SourcePosition start, TokenLiteral literal)
{
    Token token;
    token.kind = kind;
    token.span = source_span(start, position(lexer));
    token.start = lexer->source->buffer + start.offset;
    token.length = lexer->current - start.offset;
    token.literal = literal;
    if (!token_buffer_push(lexer->tokens, token)) {
        lexer->failed = 1;
        return 0;
    }
#if MACARONICA_LEXER_DEBUG
    (void)fprintf(stderr, "[debug][lexer] %zu:%zu %-18s `", // debug - remover
                  start.line, start.column, token_kind_name(kind));
    if (token.start != NULL && token.length > 0U) {
        (void)fwrite(token.start, 1U, token.length, stderr); // debug - remover
    }
    (void)fputs("`\n", stderr); // debug - remover
#endif
    return 1;
}

static TokenLiteral empty_literal(void)
{
    TokenLiteral literal;
    literal.inteira = 0;
    return literal;
}

static void skip_whitespace(Lexer *lexer)
{
    int valid;
    unsigned char byte;
    while (!at_end(lexer)) {
        byte = peek_byte(lexer, 0U);
        if (byte != (unsigned char)' ' && byte != (unsigned char)'\t' &&
            byte != (unsigned char)'\r' && byte != (unsigned char)'\n') {
            break;
        }
        (void)advance_character(lexer, &valid);
    }
}

static TokenKind identifier_kind(const char *start, size_t length)
{
    size_t index;
    for (index = 0U; index < sizeof(KEYWORDS) / sizeof(KEYWORDS[0]); ++index) {
        if (KEYWORDS[index].length == length &&
            memcmp(KEYWORDS[index].text, start, length) == 0) {
            return KEYWORDS[index].kind;
        }
    }
    return TOKEN_IDENTIFIER;
}

static void scan_identifier(Lexer *lexer, SourcePosition start)
{
    TokenLiteral literal = empty_literal();
    TokenKind kind;
    int valid;
    while (!at_end(lexer) && is_identifier_continue(peek_byte(lexer, 0U))) {
        (void)advance_character(lexer, &valid);
    }
    kind = identifier_kind(lexer->source->buffer + start.offset,
                           lexer->current - start.offset);
    if (kind == TOKEN_TRUE || kind == TOKEN_FALSE) {
        literal.booleano = kind == TOKEN_TRUE;
    }
    (void)emit_token(lexer, kind, start, literal);
}

static int matches_logical_not(const Lexer *lexer)
{
    static const unsigned char spelling[] = {'N', 0xC3U, 0x83U, 'O'};
    size_t remaining = lexer->source->length - lexer->current;
    if (remaining < sizeof(spelling) ||
        memcmp(lexer->source->buffer + lexer->current, spelling, sizeof(spelling)) != 0) {
        return 0;
    }
    return remaining == sizeof(spelling) ||
           !is_identifier_continue((unsigned char)lexer->source->buffer[lexer->current + sizeof(spelling)]);
}

static void scan_number(Lexer *lexer, SourcePosition start)
{
    TokenLiteral literal = empty_literal();
    unsigned long long integer = 0U;
    int overflow = 0;
    int valid;
    size_t index;

    while (!at_end(lexer) && is_ascii_digit(peek_byte(lexer, 0U))) {
        (void)advance_character(lexer, &valid);
    }
    if (!at_end(lexer) && peek_byte(lexer, 0U) == (unsigned char)'.') {
        size_t length;
        char *text;
        char *end;
        double real;

        (void)advance_character(lexer, &valid);
        if (at_end(lexer) || !is_ascii_digit(peek_byte(lexer, 0U))) {
            add_lexical_error(lexer, start, "E-LEX-004", "literal real incompleto; esperado digito apos '.'");
            (void)emit_token(lexer, TOKEN_INVALID, start, literal);
            return;
        }
        while (!at_end(lexer) && is_ascii_digit(peek_byte(lexer, 0U))) {
            (void)advance_character(lexer, &valid);
        }
        /* SourceFile tambem aceita buffers sem terminador; copie apenas o lexema. */
        length = lexer->current - start.offset;
        if (length == (size_t)-1 || (text = (char *)malloc(length + 1U)) == NULL) {
            lexer->failed = 1;
            return;
        }
        memcpy(text, lexer->source->buffer + start.offset, length);
        text[length] = '\0';
        real = strtod(text, &end);
        if (end != text + length) {
            /* A gramatica ja validou os caracteres; uma falha aqui e interna. */
            free(text);
            lexer->failed = 1;
            return;
        }
        if (!isfinite(real)) {
            add_lexical_error(lexer, start, "E-LEX-006", "literal real fora do intervalo representavel");
            (void)emit_token(lexer, TOKEN_INVALID, start, literal);
        } else {
            /* Converter direto para float evita arredondar duas vezes: decimal ->
             * double -> float pode alterar um valor proximo de um empate. Valores
             * acima de FLT_MAX ficam em double para o diagnostico semantico. */
            literal.real = real > (double)FLT_MAX ? real : (double)strtof(text, NULL);
            (void)emit_token(lexer, TOKEN_LITERAL_REAL, start, literal);
        }
        free(text);
        return;
    }

    for (index = start.offset; index < lexer->current; ++index) {
        unsigned int digit = (unsigned int)((unsigned char)lexer->source->buffer[index] - (unsigned char)'0');
        if (integer > ((unsigned long long)LLONG_MAX - digit) / 10U) {
            overflow = 1;
            break;
        }
        integer = integer * 10U + digit;
    }
    if (overflow) {
        add_lexical_error(lexer, start, "E-LEX-006", "literal inteiro fora do intervalo representavel");
        (void)emit_token(lexer, TOKEN_INVALID, start, literal);
    } else {
        literal.inteira = (long long)integer;
        (void)emit_token(lexer, TOKEN_LITERAL_INTEGER, start, literal);
    }
}

static int is_known_escape(unsigned char byte)
{
    return byte == (unsigned char)'\\' || byte == (unsigned char)'"' ||
           byte == (unsigned char)'n' || byte == (unsigned char)'r' ||
           byte == (unsigned char)'t';
}

static void scan_string(Lexer *lexer, SourcePosition start)
{
    int token_invalid = 0;
    int valid;

    (void)advance_character(lexer, &valid);
    while (!at_end(lexer)) {
        unsigned char byte = peek_byte(lexer, 0U);
        if (byte == (unsigned char)'"') {
            (void)advance_character(lexer, &valid);
            (void)emit_token(lexer, token_invalid ? TOKEN_INVALID : TOKEN_LITERAL_STRING,
                             start, empty_literal());
            return;
        }
        if (byte == (unsigned char)'\r' || byte == (unsigned char)'\n') {
            add_lexical_error(lexer, start, "E-LEX-002", "literal de palavra nao terminado antes da quebra de linha");
            (void)emit_token(lexer, TOKEN_INVALID, start, empty_literal());
            return;
        }
        if (byte == (unsigned char)'\\') {
            SourcePosition escape_start = position(lexer);
            (void)advance_character(lexer, &valid);
            if (at_end(lexer)) {
                break;
            }
            if (peek_byte(lexer, 0U) == (unsigned char)'\r' ||
                peek_byte(lexer, 0U) == (unsigned char)'\n') {
                add_lexical_error(lexer, start, "E-LEX-002",
                                  "literal de palavra nao terminado antes da quebra de linha");
                (void)emit_token(lexer, TOKEN_INVALID, start, empty_literal());
                return;
            }
            if (!is_known_escape(peek_byte(lexer, 0U))) {
                (void)advance_character(lexer, &valid);
                if (valid) {
                    add_lexical_error(lexer, escape_start, "E-LEX-003",
                                      "escape desconhecido em literal de palavra");
                } else {
                    add_lexical_error(lexer, escape_start, "E-LEX-005",
                                      "sequencia UTF-8 invalida em literal de palavra");
                }
                token_invalid = 1;
            } else {
                (void)advance_character(lexer, &valid);
            }
            continue;
        }
        {
            SourcePosition character_start = position(lexer);
            (void)advance_character(lexer, &valid);
            if (!valid) {
                add_lexical_error(lexer, character_start, "E-LEX-005", "sequencia UTF-8 invalida");
                token_invalid = 1;
            }
        }
    }
    add_lexical_error(lexer, start, "E-LEX-002", "literal de palavra nao terminado antes do fim do arquivo");
    (void)emit_token(lexer, TOKEN_INVALID, start, empty_literal());
}

static void emit_single_or_pair(Lexer *lexer, SourcePosition start,
                                unsigned char second,
                                TokenKind single_kind, TokenKind pair_kind)
{
    int valid;
    (void)advance_character(lexer, &valid);
    if (!at_end(lexer) && peek_byte(lexer, 0U) == second) {
        (void)advance_character(lexer, &valid);
        (void)emit_token(lexer, pair_kind, start, empty_literal());
    } else {
        (void)emit_token(lexer, single_kind, start, empty_literal());
    }
}

static void scan_invalid_character(Lexer *lexer, SourcePosition start)
{
    int valid;
    size_t byte_length = advance_character(lexer, &valid);
    char message[96];
    if (!valid) {
        add_lexical_error(lexer, start, "E-LEX-005", "sequencia UTF-8 invalida");
    } else if (byte_length == 1U) {
        unsigned char byte = (unsigned char)lexer->source->buffer[start.offset];
        if (byte >= 32U && byte <= 126U) {
            (void)snprintf(message, sizeof(message), "caractere '%c' nao pertence a linguagem", (int)byte);
        } else {
            (void)snprintf(message, sizeof(message), "byte 0x%02X nao pertence a linguagem", (unsigned int)byte);
        }
        add_lexical_error(lexer, start, "E-LEX-001", message);
    } else {
        (void)snprintf(message, sizeof(message), "caractere UTF-8 nao pertence a linguagem");
        add_lexical_error(lexer, start, "E-LEX-001", message);
    }
    (void)emit_token(lexer, TOKEN_INVALID, start, empty_literal());
}

static void scan_token(Lexer *lexer)
{
    SourcePosition start;
    unsigned char byte;
    int valid;

    skip_whitespace(lexer);
    if (at_end(lexer)) {
        return;
    }
    start = position(lexer);
    byte = peek_byte(lexer, 0U);

    if (matches_logical_not(lexer)) {
        (void)advance_character(lexer, &valid);
        (void)advance_character(lexer, &valid);
        (void)advance_character(lexer, &valid);
        (void)emit_token(lexer, TOKEN_LOGICAL_NOT, start, empty_literal());
        return;
    }
    if (is_ascii_letter(byte)) {
        scan_identifier(lexer, start);
        return;
    }
    if (is_ascii_digit(byte)) {
        scan_number(lexer, start);
        return;
    }
    if (byte == (unsigned char)'"') {
        scan_string(lexer, start);
        return;
    }

    switch (byte) {
    case '+': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_PLUS, start, empty_literal()); break;
    case '-': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_MINUS, start, empty_literal()); break;
    case '*': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_STAR, start, empty_literal()); break;
    case '/': emit_single_or_pair(lexer, start, '/', TOKEN_SLASH, TOKEN_INTEGER_DIVIDE); break;
    case '^': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_CARET, start, empty_literal()); break;
    case '<': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_LESS, start, empty_literal()); break;
    case '>': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_GREATER, start, empty_literal()); break;
    case '!': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_DIFFERENT, start, empty_literal()); break;
    case ';': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_SEMICOLON, start, empty_literal()); break;
    case ',': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_COMMA, start, empty_literal()); break;
    case '.': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_DOT, start, empty_literal()); break;
    case '@': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_AT, start, empty_literal()); break;
    case '(': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_LEFT_PAREN, start, empty_literal()); break;
    case ')': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_RIGHT_PAREN, start, empty_literal()); break;
    case '[': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_LEFT_BRACKET, start, empty_literal()); break;
    case ']': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_RIGHT_BRACKET, start, empty_literal()); break;
    case '{': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_LEFT_BRACE, start, empty_literal()); break;
    case '}': (void)advance_character(lexer, &valid); (void)emit_token(lexer, TOKEN_RIGHT_BRACE, start, empty_literal()); break;
    case '=':
        (void)advance_character(lexer, &valid);
        if (!at_end(lexer) && peek_byte(lexer, 0U) == (unsigned char)'=') {
            (void)advance_character(lexer, &valid);
            (void)emit_token(lexer, TOKEN_EQUAL_EQUAL, start, empty_literal());
        } else {
            add_lexical_error(lexer, start, "E-LEX-001", "'=' isolado nao pertence a linguagem; use '==' para igualdade");
            (void)emit_token(lexer, TOKEN_INVALID, start, empty_literal());
        }
        break;
    default: scan_invalid_character(lexer, start); break;
    }
}

FrontendStatus lexer_scan_all(const SourceFile *source,
                              DiagnosticBag *diagnostics,
                              TokenBuffer *out_tokens)
{
    Lexer lexer;
    TokenLiteral literal = empty_literal();
    size_t errors_before;

    if (source == NULL || diagnostics == NULL || out_tokens == NULL || source->buffer == NULL) {
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }
    lexer.source = source;
    lexer.diagnostics = diagnostics;
    lexer.tokens = out_tokens;
    lexer.current = 0U;
    lexer.line = 1U;
    lexer.column = 1U;
    lexer.failed = 0;
    errors_before = diagnostics->error_count;

#if MACARONICA_LEXER_DEBUG
    (void)fprintf(stderr, "[debug][lexer] iniciando `%s` (%zu bytes)\n", // debug - remover
                  source->path != NULL ? source->path : "<fonte>",
                  source->length);
#endif
    while (!at_end(&lexer) && !lexer.failed) {
        scan_token(&lexer);
    }
    (void)emit_token(&lexer, TOKEN_EOF, position(&lexer), literal);
#if MACARONICA_LEXER_DEBUG
    (void)fprintf(stderr, "[debug][lexer] fim: %zu tokens, %zu erro(s) novo(s)\n", // debug - remover
                  out_tokens->count,
                  diagnostics->error_count - errors_before);
#endif
    if (lexer.failed) {
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }
    return diagnostics->error_count == errors_before
               ? FRONTEND_STATUS_OK
               : FRONTEND_STATUS_SOURCE_ERROR;
}
