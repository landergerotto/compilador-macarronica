#ifndef MACARONICA_SEMANTIC_SYMBOL_H
#define MACARONICA_SEMANTIC_SYMBOL_H

#include "../common/source.h"

#include <stddef.h>

#define SYMBOL_ID_NONE ((size_t)-1)
#define SCOPE_ID_NONE ((size_t)-1)

typedef enum {
    SYMBOL_NAMESPACE_TYPE,
    SYMBOL_NAMESPACE_FUNCTION,
    SYMBOL_NAMESPACE_VALUE
} SymbolNamespace;

typedef enum {
    SYMBOL_RECORD,
    SYMBOL_FUNCTION,
    SYMBOL_PRINCIPAL,
    SYMBOL_GLOBAL_VARIABLE,
    SYMBOL_LOCAL_VARIABLE,
    SYMBOL_PARAMETER,
    SYMBOL_VECTOR,
    SYMBOL_FIELD
} SymbolCategory;

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_RECORD
} ScopeKind;

typedef struct {
    size_t id;
    size_t parent_id;
    ScopeKind kind;
    char *name;
    size_t owner_symbol_id;
} Scope;

typedef struct {
    size_t id;
    char *name;
    SymbolNamespace name_space;
    SymbolCategory category;
    size_t type_id;
    size_t scope_id;
    SourceSpan declaration_span;
    size_t *parameter_type_ids;
    size_t parameter_count;
    size_t element_type_id;
    long long array_size;
    int is_principal;
} Symbol;

typedef struct {
    Symbol *items;
    size_t count;
    size_t capacity;
    Scope *scopes;
    size_t scope_count;
    size_t scope_capacity;
} SymbolTable;

int symbol_table_init(SymbolTable *table);
void symbol_table_destroy(SymbolTable *table);
int symbol_table_add_scope(SymbolTable *table,
                           size_t parent_id,
                           ScopeKind kind,
                           const char *name,
                           size_t owner_symbol_id,
                           size_t *out_scope_id);
int symbol_table_declare(SymbolTable *table,
                         const char *name,
                         SymbolNamespace name_space,
                         SymbolCategory category,
                         size_t type_id,
                         size_t scope_id,
                         SourceSpan declaration_span,
                         size_t *out_symbol_id);
int symbol_set_parameters(SymbolTable *table,
                          size_t symbol_id,
                          const size_t *parameter_type_ids,
                          size_t parameter_count);
void symbol_set_vector(SymbolTable *table,
                       size_t symbol_id,
                       size_t element_type_id,
                       long long array_size);
Symbol *symbol_table_get_mut(SymbolTable *table, size_t symbol_id);
const Symbol *symbol_table_get(const SymbolTable *table, size_t symbol_id);
const Scope *symbol_table_get_scope(const SymbolTable *table, size_t scope_id);
size_t symbol_table_lookup_current(const SymbolTable *table,
                                   size_t scope_id,
                                   SymbolNamespace name_space,
                                   const char *name);
size_t symbol_table_lookup(const SymbolTable *table,
                           size_t scope_id,
                           SymbolNamespace name_space,
                           const char *name);
const char *symbol_category_name(SymbolCategory category);
const char *symbol_namespace_name(SymbolNamespace name_space);
const char *scope_kind_name(ScopeKind kind);

#endif
