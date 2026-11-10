#ifndef MACARONICA_LEXER_TOKEN_H
#define MACARONICA_LEXER_TOKEN_H

#include "../common/source.h"

#include <stddef.h>
#include <stdio.h>

/*
 * Todos os tipos de pedacos que o lexer consegue reconhecer: identificadores,
 * literais, palavras reservadas, operadores e pontuacao.
 */
typedef enum {
    TOKEN_INVALID,
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_LITERAL_INTEGER,
    TOKEN_LITERAL_REAL,
    TOKEN_LITERAL_STRING,

    TOKEN_KW_BOOL,
    TOKEN_KW_INTEIRA,
    TOKEN_KW_FLUT,
    TOKEN_KW_PALAVRA,
    TOKEN_KW_DUPLOCARPADO,
    TOKEN_KW_VAZIO,
    TOKEN_KW_RECEBA,
    TOKEN_KW_REPETE,
    TOKEN_KW_DURANTE,
    TOKEN_KW_COND,
    TOKEN_KW_CASOCONTRARIO,
    TOKEN_KW_PRINCIPAL,
    TOKEN_KW_RESPOST,
    TOKEN_KW_REGISTRO,
    TOKEN_KW_OUTRAFUNCAO,
    TOKEN_KW_VET,
    TOKEN_KW_EDAI,
    TOKEN_KW_FUNC,
    TOKEN_KW_RAIZ,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_LOGICAL_OR,
    TOKEN_LOGICAL_AND,
    TOKEN_LOGICAL_NOT,
    TOKEN_LOGICAL_XOR,

    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_INTEGER_DIVIDE,
    TOKEN_CARET,
    TOKEN_LESS,
    TOKEN_GREATER,
    TOKEN_EQUAL_EQUAL,
    TOKEN_DIFFERENT,

    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_AT,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_DOT
} TokenKind;

/* Guarda o valor do literal depois da conversao. O kind diz qual campo usar. */
typedef union {
    long long inteira;
    double real;
    int booleano;
} TokenLiteral;

/*
 * Um token guarda seu tipo, onde apareceu e o trecho original do texto.
 * start aponta pra dentro do SourceFile, entao o token nao pode viver sozinho.
 */
typedef struct {
    TokenKind kind;
    SourceSpan span;
    const char *start; /* Fatia sem posse de SourceFile.buffer. */
    size_t length;
    TokenLiteral literal;
} Token;

/* Lista de tokens. Ela guarda os tokens, mas o texto ainda pertence a fonte. */
typedef struct {
    Token *items;
    size_t count;
    size_t capacity;
} TokenBuffer;

/* Comeca uma lista de tokens vazia. */
void token_buffer_init(TokenBuffer *buffer);

/* Libera a lista, mas nao o texto da fonte. */
void token_buffer_destroy(TokenBuffer *buffer);

/* Coloca mais um token na lista. Retorna 0 se nao conseguiu aumentar o vetor. */
int token_buffer_push(TokenBuffer *buffer, Token token);

/* Traduz o enum pra um nome que da pra mostrar no dump. */
const char *token_kind_name(TokenKind kind);

/* Compara o texto original do token com uma string comum de C. */
int token_lexeme_equals(const Token *token, const char *text);

/* Mostra o token no formato usado por --dump-tokens. */
void token_print(const Token *token, const char *path, FILE *stream);

#endif
