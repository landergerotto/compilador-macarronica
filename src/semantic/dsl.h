#ifndef MACARONICA_SEMANTIC_DSL_H
#define MACARONICA_SEMANTIC_DSL_H

/*
 * DSL vetorial do PjBL 4 (docs/dsl.md). Este modulo e da frente dsl-mlir:
 * reconhece `c receba a + b;`, `c receba k * a;`, somavet, maxvet e prodesc,
 * confere tipos e tamanhos e descreve cada operacao para os back-ends.
 *
 * O semantic.c so chama os ganchos abaixo; a regra da DSL mora aqui.
 */

#include "semantic.h"

/* Marca os nos aceitos pela DSL em node->semantic.flags. O bit fica longe dos
 * AST_SEMANTIC_* de semantic.h para nao colidir com flags futuras. */
#define DSL_SEMANTIC_FLAG 0x8000u

/* O que o gancho precisa do analisador comum, sem ver a struct dele. */
typedef struct {
    const SourceFile *source;
    DiagnosticBag *diagnostics;
    SemanticResult *result;
    size_t scope_id;
    int *allocation_failed;
    void *analyzer;
    /* Analisa uma expressao escalar com as regras normais e devolve o tipo. */
    size_t (*analyze_expression)(void *analyzer, AstNode *node);
} DslContext;

typedef enum {
    DSL_FORM_NONE,
    DSL_FORM_VECTOR_VECTOR, /* c receba a op b */
    DSL_FORM_SCALAR_VECTOR, /* c receba k op a */
    DSL_FORM_VECTOR_SCALAR, /* c receba a op k */
    DSL_FORM_SUM,           /* somavet(a) */
    DSL_FORM_MAX,           /* maxvet(a) */
    DSL_FORM_DOT            /* prodesc(a, b) */
} DslForm;

/* Uma operacao da DSL ja conferida pela semantica. */
typedef struct {
    DslForm form;
    AstOperator op;             /* ADD, SUBTRACT ou MULTIPLY nas atribuicoes */
    long long length;           /* N, o mesmo em todos os vetores */
    size_t vector_ids[2];       /* vetores operandos, na ordem do texto */
    size_t vector_count;
    size_t target_id;           /* vetor destino; SYMBOL_ID_NONE nas reducoes */
    const AstNode *scalar;      /* expressao escalar das formas com escalar */
    size_t element_type_ids[2]; /* elemento de cada vetor operando */
    size_t scalar_type_id;      /* tipo do escalar antes de virar R */
    size_t result_type_id;      /* R: tipo em que a conta e feita */
    size_t target_type_id;      /* C: elemento do destino; nas reducoes, R */
    char kernel[48];            /* nome do nucleo (docs/dsl.md, 6.1) */
} DslOperation;

/* Um argumento do nucleo, na ordem da assinatura. */
typedef struct {
    int is_scalar;
    size_t symbol_id; /* vetor; SYMBOL_ID_NONE quando is_scalar */
} DslArgument;

/* Nucleos distintos que um programa usa, na ordem em que aparecem. */
typedef struct {
    DslOperation *items;
    size_t count;
    size_t capacity;
} DslKernelList;

/* Ganchos da analise semantica. */
int dsl_declare_builtins(SemanticResult *result);
int dsl_claims_assignment(const DslContext *context, const AstNode *assignment);
void dsl_analyze_assignment(DslContext *context, AstNode *assignment);
int dsl_is_reduction_call(const SemanticResult *result, const AstNode *call);
size_t dsl_analyze_reduction(DslContext *context, AstNode *call,
                             int used_as_statement);

/* Consultas dos back-ends sobre a AST ja anotada. */
int dsl_is_operation(const AstNode *node);
int dsl_describe(const SemanticResult *semantic, const AstNode *node,
                 DslOperation *operation);
size_t dsl_kernel_arguments(const DslOperation *operation,
                            DslArgument arguments[3]);
int dsl_collect_kernels(const AstNode *program, const SemanticResult *semantic,
                        DslKernelList *list);
void dsl_kernel_list_destroy(DslKernelList *list);

#endif
