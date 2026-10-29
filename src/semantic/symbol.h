#ifndef MACARONICA_SEMANTIC_SYMBOL_H
#define MACARONICA_SEMANTIC_SYMBOL_H

#include "../common/source.h"

#include <stddef.h>

/* Usados quando ainda nao existe simbolo ou escopo ligado ao item. */
#define SYMBOL_ID_NONE ((size_t)-1)
#define SCOPE_ID_NONE ((size_t)-1)

/*
 * Separa nomes de tipos, funcoes e valores. Assim procurar uma variavel nao
 * corre o risco de devolver um registro ou uma funcao com o mesmo nome.
 */
typedef enum {
    SYMBOL_NAMESPACE_TYPE,
    SYMBOL_NAMESPACE_FUNCTION,
    SYMBOL_NAMESPACE_VALUE
} SymbolNamespace;

/* O que aquele nome representa no programa. */
typedef enum {
    SYMBOL_RECORD,
    SYMBOL_FUNCTION,
    SYMBOL_PRINCIPAL,
    SYMBOL_GLOBAL_VARIABLE,
    SYMBOL_LOCAL_VARIABLE,
    SYMBOL_PARAMETER,
    SYMBOL_VECTOR,
    SYMBOL_FIELD,
    SYMBOL_BUILTIN_FUNCTION
} SymbolCategory;

/* Identidade das intrinsecas: o codegen nao precisa comparar nomes da fonte. */
typedef enum {
    BUILTIN_NONE,
    BUILTIN_PRINT_INTEGER,
    BUILTIN_PRINT_FLOAT,
    BUILTIN_PRINT_DOUBLE,
    BUILTIN_PRINT_WORD,
    BUILTIN_PRINT_BOOL,
    BUILTIN_PRINT_NEWLINE,
    BUILTIN_READ_INTEGER,
    BUILTIN_READ_FLOAT,
    BUILTIN_READ_DOUBLE
} BuiltinKind;

/* Escopos que existem hoje. cond, repete e durante nao criam um novo. */
typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_RECORD
} ScopeKind;

/* Cada escopo sabe quem e seu pai e qual simbolo criou ele. */
typedef struct {
    size_t id;
    size_t parent_id;
    ScopeKind kind;
    char *name;
    size_t owner_symbol_id;
} Scope;

/*
 * Tudo que precisamos lembrar sobre um nome declarado. Os ultimos campos
 * guardam detalhes extras de funcoes e vetores.
 */
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
    BuiltinKind builtin_kind;
    size_t field_index; /* Ordinal textual; SYMBOL_ID_NONE para outros simbolos. */
} Symbol;

/* Lista de simbolos junto com a arvore de escopos. */
typedef struct {
    Symbol *items;
    size_t count;
    size_t capacity;
    Scope *scopes;
    size_t scope_count;
    size_t scope_capacity;
} SymbolTable;

/* Comeca a tabela e ja cria o escopo global. */
int symbol_table_init(SymbolTable *table);

/* Libera simbolos, escopos e nomes guardados pela tabela. */
void symbol_table_destroy(SymbolTable *table);

/*
 * Cria um escopo novo. parent_id diz para onde a busca sobe quando um nome nao
 * foi encontrado aqui.
 */
int symbol_table_add_scope(SymbolTable *table,
                           size_t parent_id,
                           ScopeKind kind,
                           const char *name,
                           size_t owner_symbol_id,
                           size_t *out_scope_id);

/*
 * Guarda uma declaracao na tabela. A checagem de nome repetido acontece antes,
 * na analise semantica.
 */
int symbol_table_declare(SymbolTable *table,
                         const char *name,
                         SymbolNamespace name_space,
                         SymbolCategory category,
                         size_t type_id,
                         size_t scope_id,
                         SourceSpan declaration_span,
                         size_t *out_symbol_id);

/* Guarda os tipos dos parametros de uma funcao. */
int symbol_set_parameters(SymbolTable *table,
                          size_t symbol_id,
                          const size_t *parameter_type_ids,
                          size_t parameter_count);

/* Guarda o tipo do elemento e o tamanho de um vetor. */
void symbol_set_vector(SymbolTable *table,
                       size_t symbol_id,
                       size_t element_type_id,
                       long long array_size);

/*
 * Busca direta pelo ID. O ponteiro devolvido pertence a tabela e pode mudar
 * depois que novos simbolos forem adicionados.
 */
Symbol *symbol_table_get_mut(SymbolTable *table, size_t symbol_id);
const Symbol *symbol_table_get(const SymbolTable *table, size_t symbol_id);
const Scope *symbol_table_get_scope(const SymbolTable *table, size_t scope_id);

/* Procura so naquele escopo. Usado para detectar nome repetido. */
size_t symbol_table_lookup_current(const SymbolTable *table,
                                   size_t scope_id,
                                   SymbolNamespace name_space,
                                   const char *name);

/* Procura aqui e depois vai subindo pelos pais ate chegar no global. */
size_t symbol_table_lookup(const SymbolTable *table,
                           size_t scope_id,
                           SymbolNamespace name_space,
                           const char *name);

/* Traduz os enums pra nomes usados nos dumps e erros. */
const char *symbol_category_name(SymbolCategory category);
const char *symbol_namespace_name(SymbolNamespace name_space);
const char *scope_kind_name(ScopeKind kind);

#endif
