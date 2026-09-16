#ifndef MACARONICA_PARSER_AST_H
#define MACARONICA_PARSER_AST_H

#include "../common/source.h"

#include <stddef.h>
#include <stdio.h>

#define AST_ID_NONE ((size_t)-1)

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION_DECL,
    AST_STRUCT_DECL,
    AST_VAR_DECL,
    AST_VECTOR_DECL,
    AST_BLOCK,
    AST_ASSIGNMENT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    AST_EXPR_STMT,
    AST_CALL_EXPR,
    AST_INDEX_EXPR,
    AST_BINARY_EXPR,
    AST_UNARY_EXPR,
    AST_LITERAL_EXPR,
    AST_IDENTIFIER_EXPR
} AstKind;

typedef enum {
    AST_OP_NONE,
    AST_OP_ADD,
    AST_OP_SUBTRACT,
    AST_OP_MULTIPLY,
    AST_OP_DIVIDE,
    AST_OP_INTEGER_DIVIDE,
    AST_OP_POWER,
    AST_OP_LESS,
    AST_OP_GREATER,
    AST_OP_EQUAL,
    AST_OP_DIFFERENT,
    AST_OP_LOGICAL_NOT,
    AST_OP_LOGICAL_AND,
    AST_OP_LOGICAL_XOR,
    AST_OP_LOGICAL_OR,
    AST_OP_ROOT,
    AST_OP_UNARY_PLUS,
    AST_OP_UNARY_MINUS
} AstOperator;

typedef enum {
    AST_LITERAL_INTEGER,
    AST_LITERAL_REAL,
    AST_LITERAL_STRING,
    AST_LITERAL_BOOLEAN
} AstLiteralKind;

/*
 * Parser-owned spelling of a type. resolved_type_id is available to the
 * semantic phase without making the AST depend on its symbol-table types.
 */
typedef struct {
    char *name;
    SourceSpan span;
    size_t resolved_type_id;
} AstTypeRef;

/* One entry for every name in a declaration such as `inteira a, b;`. */
typedef struct {
    char *name;
    SourceSpan span;
    size_t symbol_id;
} AstDeclaredName;

typedef struct {
    AstDeclaredName *items;
    size_t count;
    size_t capacity;
} AstDeclaredNameList;

typedef struct AstNode AstNode;

typedef struct {
    AstNode **items;
    size_t count;
    size_t capacity;
} AstNodeList;

typedef struct {
    AstNode *condition;
    AstNode *body;
    SourceSpan span;
} AstIfBranch;

typedef struct {
    AstIfBranch *items;
    size_t count;
    size_t capacity;
} AstIfBranchList;

/* Fields intentionally opaque to the parser and owned by later phases. */
typedef struct {
    size_t symbol_id;
    size_t type_id;
    unsigned int flags;
} AstSemanticInfo;

struct AstNode {
    AstKind kind;
    SourceSpan span;
    size_t node_id;
    AstSemanticInfo semantic;
    union {
        struct {
            AstNodeList declarations;
        } program;
        struct {
            AstTypeRef return_type;
            char *name;
            SourceSpan name_span;
            int is_principal;
            AstNodeList parameters; /* AST_VAR_DECL, one name per node */
            AstNode *body;           /* AST_BLOCK */
        } function_decl;
        struct {
            char *name;
            SourceSpan name_span;
            AstNodeList fields; /* AST_VAR_DECL, one name per field */
        } struct_decl;
        struct {
            AstTypeRef type;
            AstDeclaredNameList names;
        } var_decl;
        struct {
            AstTypeRef type;
            long long size;
            SourceSpan size_span;
            AstDeclaredNameList names;
        } vector_decl;
        struct {
            AstNodeList statements;
        } block;
        struct {
            AstNode *target;
            AstNode *value;
        } assignment;
        struct {
            AstIfBranchList branches;
            AstNode *else_body; /* nullable AST_BLOCK */
        } if_stmt;
        struct {
            AstNode *condition;
            AstNode *body;
        } while_stmt;
        struct {
            AstNode *initializer;
            AstNode *condition;
            AstNode *iteration;
            AstNode *body;
        } for_stmt;
        struct {
            AstNode *value; /* nullable */
        } return_stmt;
        struct {
            AstNode *expression;
        } expr_stmt;
        struct {
            char *callee;
            SourceSpan callee_span;
            AstNodeList arguments;
        } call_expr;
        struct {
            AstNode *base;
            AstNode *index;
        } index_expr;
        struct {
            AstOperator operator_kind;
            SourceSpan operator_span;
            AstNode *left;
            AstNode *right;
        } binary_expr;
        struct {
            AstOperator operator_kind;
            SourceSpan operator_span;
            AstNode *operand;
        } unary_expr;
        struct {
            AstLiteralKind literal_kind;
            union {
                long long integer;
                double real;
                int boolean;
                char *string;
            } value;
        } literal_expr;
        struct {
            char *name;
        } identifier_expr;
    } as;
};

AstNode *ast_node_create(AstKind kind, SourceSpan span, size_t node_id);
int ast_node_list_append(AstNodeList *list, AstNode *node);
int ast_declared_name_list_append(AstDeclaredNameList *list,
                                  char *owned_name,
                                  SourceSpan span);
int ast_if_branch_list_append(AstIfBranchList *list,
                              AstNode *condition,
                              AstNode *body,
                              SourceSpan span);
void ast_type_ref_destroy(AstTypeRef *type);
void ast_node_destroy(AstNode *node);
const char *ast_kind_name(AstKind kind);
const char *ast_operator_name(AstOperator operator_kind);
void ast_dump(const AstNode *node, FILE *stream);

#endif
