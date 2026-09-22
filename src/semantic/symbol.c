#include "symbol.h"

#include <stdlib.h>
#include <string.h>

static char *copy_string(const char *text)
{
    size_t length;
    char *copy;
    if (text == NULL) return NULL;
    length = strlen(text);
    if (length == (size_t)-1) return NULL;
    copy = (char *)malloc(length + 1U);
    if (copy != NULL) memcpy(copy, text, length + 1U);
    return copy;
}

static int reserve_symbols(SymbolTable *table, size_t needed)
{
    Symbol *items;
    size_t capacity = table->capacity == 0U ? 16U : table->capacity;
    if (needed <= table->capacity) return 1;
    while (capacity < needed) {
        if (capacity > ((size_t)-1) / 2U) return 0;
        capacity *= 2U;
    }
    if (capacity > ((size_t)-1) / sizeof(*items)) return 0;
    items = (Symbol *)realloc(table->items, capacity * sizeof(*items));
    if (items == NULL) return 0;
    table->items = items;
    table->capacity = capacity;
    return 1;
}

static int reserve_scopes(SymbolTable *table, size_t needed)
{
    Scope *items;
    size_t capacity = table->scope_capacity == 0U ? 8U : table->scope_capacity;
    if (needed <= table->scope_capacity) return 1;
    while (capacity < needed) {
        if (capacity > ((size_t)-1) / 2U) return 0;
        capacity *= 2U;
    }
    if (capacity > ((size_t)-1) / sizeof(*items)) return 0;
    items = (Scope *)realloc(table->scopes, capacity * sizeof(*items));
    if (items == NULL) return 0;
    table->scopes = items;
    table->scope_capacity = capacity;
    return 1;
}

int symbol_table_init(SymbolTable *table)
{
    size_t global_id;
    if (table == NULL) return 0;
    memset(table, 0, sizeof(*table));
    return symbol_table_add_scope(table, SCOPE_ID_NONE, SCOPE_GLOBAL,
                                  "<global>", SYMBOL_ID_NONE, &global_id);
}

void symbol_table_destroy(SymbolTable *table)
{
    size_t index;
    if (table == NULL) return;
    for (index = 0U; index < table->count; ++index) {
        free(table->items[index].name);
        free(table->items[index].parameter_type_ids);
    }
    for (index = 0U; index < table->scope_count; ++index) {
        free(table->scopes[index].name);
    }
    free(table->items);
    free(table->scopes);
    memset(table, 0, sizeof(*table));
}

int symbol_table_add_scope(SymbolTable *table,
                           size_t parent_id,
                           ScopeKind kind,
                           const char *name,
                           size_t owner_symbol_id,
                           size_t *out_scope_id)
{
    Scope *scope;
    if (table == NULL || !reserve_scopes(table, table->scope_count + 1U)) return 0;
    scope = &table->scopes[table->scope_count];
    scope->name = copy_string(name != NULL ? name : "<anonimo>");
    if (scope->name == NULL) return 0;
    scope->id = table->scope_count;
    scope->parent_id = parent_id;
    scope->kind = kind;
    scope->owner_symbol_id = owner_symbol_id;
    if (out_scope_id != NULL) *out_scope_id = scope->id;
    ++table->scope_count;
    return 1;
}

int symbol_table_declare(SymbolTable *table,
                         const char *name,
                         SymbolNamespace name_space,
                         SymbolCategory category,
                         size_t type_id,
                         size_t scope_id,
                         SourceSpan declaration_span,
                         size_t *out_symbol_id)
{
    Symbol *symbol;
    if (table == NULL || name == NULL || scope_id >= table->scope_count ||
        !reserve_symbols(table, table->count + 1U)) return 0;
    symbol = &table->items[table->count];
    memset(symbol, 0, sizeof(*symbol));
    symbol->name = copy_string(name);
    if (symbol->name == NULL) return 0;
    symbol->id = table->count;
    symbol->name_space = name_space;
    symbol->category = category;
    symbol->type_id = type_id;
    symbol->scope_id = scope_id;
    symbol->declaration_span = declaration_span;
    symbol->element_type_id = (size_t)-1;
    symbol->array_size = 0;
    symbol->is_principal = category == SYMBOL_PRINCIPAL;
    if (out_symbol_id != NULL) *out_symbol_id = symbol->id;
    ++table->count;
    return 1;
}

int symbol_set_parameters(SymbolTable *table,
                          size_t symbol_id,
                          const size_t *parameter_type_ids,
                          size_t parameter_count)
{
    Symbol *symbol = symbol_table_get_mut(table, symbol_id);
    size_t *copy = NULL;
    if (symbol == NULL) return 0;
    if (parameter_count != 0U) {
        if (parameter_count > ((size_t)-1) / sizeof(*copy)) return 0;
        copy = (size_t *)malloc(parameter_count * sizeof(*copy));
        if (copy == NULL) return 0;
        memcpy(copy, parameter_type_ids, parameter_count * sizeof(*copy));
    }
    free(symbol->parameter_type_ids);
    symbol->parameter_type_ids = copy;
    symbol->parameter_count = parameter_count;
    return 1;
}

void symbol_set_vector(SymbolTable *table,
                       size_t symbol_id,
                       size_t element_type_id,
                       long long array_size)
{
    Symbol *symbol = symbol_table_get_mut(table, symbol_id);
    if (symbol == NULL) return;
    symbol->element_type_id = element_type_id;
    symbol->array_size = array_size;
}

Symbol *symbol_table_get_mut(SymbolTable *table, size_t symbol_id)
{
    if (table == NULL || symbol_id >= table->count) return NULL;
    return &table->items[symbol_id];
}

const Symbol *symbol_table_get(const SymbolTable *table, size_t symbol_id)
{
    if (table == NULL || symbol_id >= table->count) return NULL;
    return &table->items[symbol_id];
}

const Scope *symbol_table_get_scope(const SymbolTable *table, size_t scope_id)
{
    if (table == NULL || scope_id >= table->scope_count) return NULL;
    return &table->scopes[scope_id];
}

size_t symbol_table_lookup_current(const SymbolTable *table,
                                   size_t scope_id,
                                   SymbolNamespace name_space,
                                   const char *name)
{
    size_t index;
    if (table == NULL || name == NULL) return SYMBOL_ID_NONE;
    for (index = table->count; index > 0U; --index) {
        const Symbol *symbol = &table->items[index - 1U];
        if (symbol->scope_id == scope_id && symbol->name_space == name_space &&
            strcmp(symbol->name, name) == 0) return symbol->id;
    }
    return SYMBOL_ID_NONE;
}

size_t symbol_table_lookup(const SymbolTable *table,
                           size_t scope_id,
                           SymbolNamespace name_space,
                           const char *name)
{
    while (scope_id != SCOPE_ID_NONE) {
        size_t found = symbol_table_lookup_current(table, scope_id, name_space, name);
        const Scope *scope;
        if (found != SYMBOL_ID_NONE) return found;
        scope = symbol_table_get_scope(table, scope_id);
        if (scope == NULL) break;
        scope_id = scope->parent_id;
    }
    return SYMBOL_ID_NONE;
}

const char *symbol_category_name(SymbolCategory category)
{
    static const char *const names[] = {
        "registro", "funcao", "principal", "variavel global",
        "variavel local", "parametro", "vetor", "campo"
    };
    return (size_t)category < sizeof(names) / sizeof(names[0])
               ? names[(size_t)category] : "simbolo desconhecido";
}

const char *symbol_namespace_name(SymbolNamespace name_space)
{
    static const char *const names[] = { "tipos", "funcoes", "valores" };
    return (size_t)name_space < sizeof(names) / sizeof(names[0])
               ? names[(size_t)name_space] : "desconhecido";
}

const char *scope_kind_name(ScopeKind kind)
{
    static const char *const names[] = { "global", "funcao", "registro" };
    return (size_t)kind < sizeof(names) / sizeof(names[0])
               ? names[(size_t)kind] : "desconhecido";
}
