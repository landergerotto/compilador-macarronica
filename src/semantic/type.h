#ifndef MACARONICA_SEMANTIC_TYPE_H
#define MACARONICA_SEMANTIC_TYPE_H

#include <stddef.h>

#define TYPE_ID_NONE ((size_t)-1)
#define TYPE_ID_ERROR ((size_t)0)
#define TYPE_ID_BOOL ((size_t)1)
#define TYPE_ID_INTEGER ((size_t)2)
#define TYPE_ID_FLOAT ((size_t)3)
#define TYPE_ID_WORD ((size_t)4)
#define TYPE_ID_DOUBLE ((size_t)5)
#define TYPE_ID_VOID ((size_t)6)

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

typedef struct {
    size_t id;
    TypeKind kind;
    char *name;
    size_t element_type_id;
    long long array_size;
    size_t declaration_symbol_id;
} Type;

typedef struct {
    Type *items;
    size_t count;
    size_t capacity;
} TypeTable;

int type_table_init(TypeTable *table);
void type_table_destroy(TypeTable *table);
const Type *type_table_get(const TypeTable *table, size_t type_id);
size_t type_table_find_named(const TypeTable *table, const char *name);
int type_table_add_record(TypeTable *table, const char *name, size_t *out_type_id);
int type_table_add_array(TypeTable *table,
                         size_t element_type_id,
                         long long size,
                         size_t *out_type_id);
void type_table_set_declaration_symbol(TypeTable *table,
                                       size_t type_id,
                                       size_t symbol_id);
const char *type_kind_name(TypeKind kind);
const char *type_name(const TypeTable *table, size_t type_id);
int type_is_numeric(const TypeTable *table, size_t type_id);
int type_is_scalar(const TypeTable *table, size_t type_id);
int type_is_assignable(size_t target_type_id, size_t value_type_id);

#endif
