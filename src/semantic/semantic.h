#ifndef MACARONICA_SEMANTIC_SEMANTIC_H
#define MACARONICA_SEMANTIC_SEMANTIC_H

#include "../common/diagnostic.h"
#include "../common/source.h"
#include "../common/status.h"
#include "../parser/ast.h"
#include "symbol.h"
#include "type.h"

#include <stdio.h>

#ifndef MACARONICA_SEMANTIC_DEBUG
#define MACARONICA_SEMANTIC_DEBUG 1
#endif

#define AST_SEMANTIC_LVALUE 0x01U
#define AST_SEMANTIC_RVALUE 0x02U
#define AST_SEMANTIC_CONSTANT 0x04U
#define AST_SEMANTIC_ERROR 0x08U
#define AST_SEMANTIC_ARRAY 0x10U

typedef struct {
    TypeTable types;
    SymbolTable symbols;
    size_t global_scope_id;
    size_t principal_symbol_id;
    size_t semantic_error_count;
    int analyzed;
} SemanticResult;

int semantic_result_init(SemanticResult *result);
void semantic_result_destroy(SemanticResult *result);
FrontendStatus semantic_analyze(const SourceFile *source,
                                AstNode *root,
                                DiagnosticBag *diagnostics,
                                SemanticResult *result);
void semantic_dump_symbols(const SemanticResult *result, FILE *stream);

#endif
