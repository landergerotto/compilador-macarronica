#ifndef MACARONICA_PARSER_AST_H
#define MACARONICA_PARSER_AST_H

/*
 * A AST e a forma organizada do programa. Em vez de uma lista de tokens, ela
 * mostra o que e declaracao, comando, operacao e filho de quem.
 */

#include "../common/source.h"

#include <stddef.h>
#include <stdio.h>

/* Usado enquanto a semantica ainda nao ligou o no a nada. */
#define AST_ID_NONE ((size_t)-1)

/*
 * Tipos de no que podem aparecer na arvore. Pontuacao como `;` nao precisa
 * virar no porque ela so serviu para o parser entender a estrutura.
 */
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
    AST_IDENTIFIER_EXPR,
    AST_FIELD_EXPR
} AstKind;

/* Qual operacao um no de expressao representa. */
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

/* Diz qual campo do valor literal esta valendo. */
typedef enum {
    AST_LITERAL_INTEGER,
    AST_LITERAL_REAL,
    AST_LITERAL_STRING,
    AST_LITERAL_BOOLEAN
} AstLiteralKind;

/*
 * Nome do tipo pertencente ao parser. A semântica preenche resolved_type_id
 * sem acoplar a AST às estruturas internas da tabela de tipos.
 */
typedef struct {
    char *name;
    SourceSpan span;
    size_t resolved_type_id;
} AstTypeRef;

/* Uma entrada para cada nome de uma declaração como `inteira a, b;`. */
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

/* Lista usada quando um no pode ter varios filhos. */
typedef struct {
    AstNode **items;
    size_t count;
    size_t capacity;
} AstNodeList;

/* Junta a condicao com o bloco daquele ramo. */
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

/*
 * O parser deixa esses campos vazios. Depois a semantica coloca aqui qual
 * simbolo e qual tipo o no representa, alem das flags de uso.
 */
typedef struct {
    size_t symbol_id;
    size_t type_id;
    unsigned int flags;
    /*
     * Expressao mista (D-012): tipo para o qual os operandos sao convertidos
     * antes da conta. AST_ID_NONE quando nenhum operando precisa de conversao.
     * Os back-ends so emitem as conversoes anotadas aqui.
     */
    size_t conversion_type_id;
} AstSemanticInfo;

/*
 * Todos os nos usam essa mesma struct. O kind diz qual parte da union `as`
 * pode ser acessada. Por exemplo, AST_BINARY_EXPR usa as.binary_expr.
 *
 * Cada no e dono dos seus filhos. Por isso liberar a raiz libera a arvore toda.
 */
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
            AstNodeList parameters; /* AST_VAR_DECL, um nome por nó */
            AstNode *body;           /* AST_BLOCK */
        } function_decl;
        struct {
            char *name;
            SourceSpan name_span;
            AstNodeList fields; /* AST_VAR_DECL, um nome por campo */
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
            AstNode *else_body; /* AST_BLOCK ou NULL */
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
            AstNode *value; /* Pode ser NULL. */
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
            AstNode *base;
            char *field_name;
            SourceSpan field_span;
        } field_expr;
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

/* Cria um no novo e deixa a parte semantica como ainda nao resolvida. */
AstNode *ast_node_create(AstKind kind, SourceSpan span, size_t node_id);

/*
 * Colocam itens nas listas dinamicas da AST. Se retornar 1, a lista passa a ser
 * dona do item. Se retornar 0, quem chamou ainda precisa liberar o item.
 */
int ast_node_list_append(AstNodeList *list, AstNode *node);
int ast_declared_name_list_append(AstDeclaredNameList *list,
                                  char *owned_name,
                                  SourceSpan span);
int ast_if_branch_list_append(AstIfBranchList *list,
                              AstNode *condition,
                              AstNode *body,
                              SourceSpan span);

/* Limpa uma referencia de tipo e o nome guardado nela. */
void ast_type_ref_destroy(AstTypeRef *type);

/* Libera um no e vai descendo nos filhos ate limpar a arvore toda. */
void ast_node_destroy(AstNode *node);

/* Nomes usados no dump e nas mensagens de erro. */
const char *ast_kind_name(AstKind kind);
const char *ast_operator_name(AstOperator operator_kind);

/* Mostra a arvore no formato do --dump-ast. */
void ast_dump(const AstNode *node, FILE *stream);

#endif
