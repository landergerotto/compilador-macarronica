#include "type.h"

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
        return NULL;
    }
    copy = (char *)malloc(length + 1U);
    if (copy != NULL) {
        memcpy(copy, text, length + 1U);
    }
    return copy;
}

static int reserve_types(TypeTable *table, size_t needed)
{
    Type *items;
    size_t capacity;
    if (needed <= table->capacity) {
        return 1;
    }
    capacity = table->capacity == 0U ? 16U : table->capacity;
    while (capacity < needed) {
        if (capacity > ((size_t)-1) / 2U) {
            return 0;
        }
        capacity *= 2U;
    }
    if (capacity > ((size_t)-1) / sizeof(*items)) {
        return 0;
    }
    items = (Type *)realloc(table->items, capacity * sizeof(*items));
    if (items == NULL) {
        return 0;
    }
    table->items = items;
    table->capacity = capacity;
    return 1;
}

static int append_type(TypeTable *table,
                       TypeKind kind,
                       const char *name,
                       size_t element_type_id,
                       long long array_size,
                       size_t *out_type_id)
{
    Type *type;
    if (!reserve_types(table, table->count + 1U)) {
        return 0;
    }
    type = &table->items[table->count];
    type->id = table->count;
    type->kind = kind;
    type->name = copy_string(name);
    if (name != NULL && type->name == NULL) {
        return 0;
    }
    type->element_type_id = element_type_id;
    type->array_size = array_size;
    type->declaration_symbol_id = TYPE_ID_NONE;
    if (out_type_id != NULL) {
        *out_type_id = type->id;
    }
    ++table->count;
    return 1;
}

int type_table_init(TypeTable *table)
{
    static const struct {
        TypeKind kind;
        const char *name;
    } builtins[] = {
        { TYPE_ERROR, "<erro>" },
        { TYPE_BOOL, "bool" },
        { TYPE_INTEGER, "inteira" },
        { TYPE_FLOAT, "flut" },
        { TYPE_WORD, "palavra" },
        { TYPE_DOUBLE, "duplocarpado" },
        { TYPE_VOID, "vazio" }
    };
    size_t index;
    if (table == NULL) {
        return 0;
    }
    table->items = NULL;
    table->count = 0U;
    table->capacity = 0U;
    for (index = 0U; index < sizeof(builtins) / sizeof(builtins[0]); ++index) {
        if (!append_type(table, builtins[index].kind, builtins[index].name,
                         TYPE_ID_NONE, 0, NULL)) {
            type_table_destroy(table);
            return 0;
        }
    }
    return 1;
}

void type_table_destroy(TypeTable *table)
{
    size_t index;
    if (table == NULL) {
        return;
    }
    for (index = 0U; index < table->count; ++index) {
        free(table->items[index].name);
    }
    free(table->items);
    table->items = NULL;
    table->count = 0U;
    table->capacity = 0U;
}

const Type *type_table_get(const TypeTable *table, size_t type_id)
{
    if (table == NULL || type_id >= table->count) {
        return NULL;
    }
    return &table->items[type_id];
}

size_t type_table_find_named(const TypeTable *table, const char *name)
{
    size_t index;
    if (table == NULL || name == NULL) {
        return TYPE_ID_NONE;
    }
    for (index = 0U; index < table->count; ++index) {
        if (table->items[index].name != NULL &&
            strcmp(table->items[index].name, name) == 0) {
            return table->items[index].id;
        }
    }
    return TYPE_ID_NONE;
}

int type_table_add_record(TypeTable *table, const char *name, size_t *out_type_id)
{
    return append_type(table, TYPE_RECORD, name, TYPE_ID_NONE, 0, out_type_id);
}

int type_table_add_array(TypeTable *table,
                         size_t element_type_id,
                         long long size,
                         size_t *out_type_id)
{
    size_t index;
    if (table == NULL) {
        return 0;
    }
    for (index = 0U; index < table->count; ++index) {
        if (table->items[index].kind == TYPE_ARRAY &&
            table->items[index].element_type_id == element_type_id &&
            table->items[index].array_size == size) {
            if (out_type_id != NULL) {
                *out_type_id = table->items[index].id;
            }
            return 1;
        }
    }
    return append_type(table, TYPE_ARRAY, NULL, element_type_id, size, out_type_id);
}

void type_table_set_declaration_symbol(TypeTable *table,
                                       size_t type_id,
                                       size_t symbol_id)
{
    if (table != NULL && type_id < table->count) {
        table->items[type_id].declaration_symbol_id = symbol_id;
    }
}

const char *type_kind_name(TypeKind kind)
{
    static const char *const names[] = {
        "erro", "bool", "inteira", "flut", "palavra", "duplocarpado",
        "vazio", "registro", "vetor"
    };
    if ((size_t)kind >= sizeof(names) / sizeof(names[0])) {
        return "desconhecido";
    }
    return names[(size_t)kind];
}

const char *type_name(const TypeTable *table, size_t type_id)
{
    const Type *type = type_table_get(table, type_id);
    if (type == NULL) {
        return "<tipo desconhecido>";
    }
    return type->name != NULL ? type->name : type_kind_name(type->kind);
}

int type_is_numeric(const TypeTable *table, size_t type_id)
{
    const Type *type = type_table_get(table, type_id);
    return type != NULL && (type->kind == TYPE_INTEGER ||
                            type->kind == TYPE_FLOAT ||
                            type->kind == TYPE_DOUBLE);
}

int type_is_scalar(const TypeTable *table, size_t type_id)
{
    const Type *type = type_table_get(table, type_id);
    return type != NULL && (type->kind == TYPE_BOOL ||
                            type->kind == TYPE_INTEGER ||
                            type->kind == TYPE_FLOAT ||
                            type->kind == TYPE_WORD ||
                            type->kind == TYPE_DOUBLE);
}

int type_is_assignable(size_t target_type_id, size_t value_type_id)
{
    if (target_type_id == TYPE_ID_ERROR || value_type_id == TYPE_ID_ERROR) {
        return 1;
    }
    return target_type_id == value_type_id;
}
