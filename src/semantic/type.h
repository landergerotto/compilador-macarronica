#ifndef MACARONICA_SEMANTIC_TYPE_H
#define MACARONICA_SEMANTIC_TYPE_H

#include <stddef.h>

/*
 * Os tipos nativos sempre usam estes IDs. TYPE_ID_ERROR deixa a analise seguir
 * depois de um erro sem ficar reclamando varias vezes da mesma coisa.
 */
#define TYPE_ID_NONE ((size_t)-1)
#define TYPE_ID_ERROR ((size_t)0)
#define TYPE_ID_BOOL ((size_t)1)
#define TYPE_ID_INTEGER ((size_t)2)
#define TYPE_ID_FLOAT ((size_t)3)
#define TYPE_ID_WORD ((size_t)4)
#define TYPE_ID_DOUBLE ((size_t)5)
#define TYPE_ID_VOID ((size_t)6)

/* Tipos que a nossa tabela sabe representar. */
typedef enum {
    TYPE_ERROR,
    TYPE_BOOL,
    TYPE_INTEGER,
    TYPE_FLOAT,
    TYPE_WORD,
    TYPE_DOUBLE,
    TYPE_VOID,
    TYPE_RECORD,
    TYPE_ARRAY
} TypeKind;

/*
 * Uma entrada da tabela. Os campos extras sao usados por vetor e registro.
 */
typedef struct {
    size_t id;
    TypeKind kind;
    char *name;
    size_t element_type_id;
    long long array_size;
    size_t declaration_symbol_id;
} Type;

/* A lista central de tipos conhecidos pela semantica. */
typedef struct {
    Type *items;
    size_t count;
    size_t capacity;
} TypeTable;

/* Comeca a tabela ja colocando os tipos nativos nela. */
int type_table_init(TypeTable *table);

/* Libera os tipos e nomes guardados pela tabela. */
void type_table_destroy(TypeTable *table);

/* Buscas por ID ou pelo nome escrito no codigo. */
const Type *type_table_get(const TypeTable *table, size_t type_id);
size_t type_table_find_named(const TypeTable *table, const char *name);

/*
 * Colocam registro ou vetor na tabela. Se um vetor igual ja existe, reutiliza o
 * mesmo ID em vez de criar outra entrada.
 */
int type_table_add_record(TypeTable *table, const char *name, size_t *out_type_id);
int type_table_add_array(TypeTable *table,
                         size_t element_type_id,
                         long long size,
                         size_t *out_type_id);

/* Liga o tipo de registro com a declaracao dele na tabela de simbolos. */
void type_table_set_declaration_symbol(TypeTable *table,
                                       size_t type_id,
                                       size_t symbol_id);

/* Traduz tipo e ID para os nomes usados nos dumps e erros. */
const char *type_kind_name(TypeKind kind);
const char *type_name(const TypeTable *table, size_t type_id);

/* Perguntas prontas usadas durante a checagem de expressoes. */
int type_is_numeric(const TypeTable *table, size_t type_id);
int type_is_scalar(const TypeTable *table, size_t type_id);

/*
 * Aceita tipos iguais e inteira -> flut/duplocarpado, flut -> duplocarpado.
 * Usado em atribuicoes, argumentos e retornos; operadores usam
 * type_numeric_common.
 * TYPE_ID_ERROR passa de proposito, porque o erro
 * verdadeiro ja foi mostrado antes e nao queremos criar mensagens em cascata.
 */
int type_is_assignable(size_t target_type_id, size_t value_type_id);

/*
 * Tipo comum de uma expressao mista (D-012): o mais largo dos dois tipos
 * numericos, na ordem inteira < flut < duplocarpado. Devolve TYPE_ID_ERROR
 * se algum dos dois nao for numerico.
 */
size_t type_numeric_common(size_t left_type_id, size_t right_type_id);

#endif
