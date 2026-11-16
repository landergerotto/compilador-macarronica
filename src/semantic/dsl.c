#include "dsl.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* As reducoes sao intrinsecas pre-declaradas, como imprimeInteira. */
typedef struct {
    const char *name;
    DslForm form;
    size_t arity;
} DslBuiltin;

static const DslBuiltin dsl_builtins[] = {
    { "somavet", DSL_FORM_SUM, 1U },
    { "maxvet", DSL_FORM_MAX, 1U },
    { "prodesc", DSL_FORM_DOT, 2U }
};

static const DslBuiltin *find_builtin(const char *name)
{
    size_t index;
    if (name == NULL) return NULL;
    for (index = 0U; index < sizeof(dsl_builtins) / sizeof(dsl_builtins[0]); ++index)
        if (strcmp(dsl_builtins[index].name, name) == 0) return &dsl_builtins[index];
    return NULL;
}

static void dsl_error(DslContext *context, SourceSpan span, const char *code,
                      const char *format, ...)
{
    char message[320];
    va_list arguments;
    va_start(arguments, format);
    (void)vsnprintf(message, sizeof(message), format, arguments);
    va_end(arguments);
    if (diagnostic_bag_add(context->diagnostics, context->source, span,
                           DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SEMANTIC,
                           code, "%s", message))
        ++context->result->semantic_error_count;
    else if (context->allocation_failed != NULL)
        *context->allocation_failed = 1;
}

static void mark(AstNode *node, size_t type_id, size_t symbol_id,
                 unsigned int flags)
{
    node->semantic.type_id = type_id;
    node->semantic.symbol_id = symbol_id;
    node->semantic.flags = flags;
}

/*
 * Promocao elemento a elemento: a mesma ordem das expressoes mistas do
 * PjBL 3 (inteira < flut < duplocarpado). Quando a promocao da frente da
 * linguagem entrar, esta funcao passa a chamar a dela.
 */
static size_t promote(size_t left, size_t right)
{
    if (left == TYPE_ID_DOUBLE || right == TYPE_ID_DOUBLE) return TYPE_ID_DOUBLE;
    if (left == TYPE_ID_FLOAT || right == TYPE_ID_FLOAT) return TYPE_ID_FLOAT;
    return TYPE_ID_INTEGER;
}

/* Nome solto que se refere a um vetor inteiro (nao a um elemento). */
static const Symbol *vector_symbol(const SemanticResult *result, size_t scope_id,
                                   const AstNode *node)
{
    const Symbol *symbol;
    const Type *type;
    if (node == NULL || node->kind != AST_IDENTIFIER_EXPR) return NULL;
    symbol = symbol_table_get(&result->symbols,
                              symbol_table_lookup(&result->symbols, scope_id,
                                                  SYMBOL_NAMESPACE_VALUE,
                                                  node->as.identifier_expr.name));
    if (symbol == NULL) return NULL;
    type = type_table_get(&result->types, symbol->type_id);
    return type != NULL && type->kind == TYPE_ARRAY ? symbol : NULL;
}

/* `a + b` dentro de outra operacao vetorial: a DSL faz uma por comando.
 * a{i} e um elemento (escalar); chamadas ficam com a analise comum. */
static int contains_vector(const DslContext *context, const AstNode *node)
{
    if (node == NULL) return 0;
    switch (node->kind) {
    case AST_IDENTIFIER_EXPR:
        return vector_symbol(context->result, context->scope_id, node) != NULL;
    case AST_INDEX_EXPR: return contains_vector(context, node->as.index_expr.index);
    case AST_FIELD_EXPR: return contains_vector(context, node->as.field_expr.base);
    case AST_BINARY_EXPR:
        return contains_vector(context, node->as.binary_expr.left) ||
               contains_vector(context, node->as.binary_expr.right);
    case AST_UNARY_EXPR: return contains_vector(context, node->as.unary_expr.operand);
    default: return 0;
    }
}

static int numeric_element(DslContext *context, const Symbol *vector,
                           SourceSpan span)
{
    if (type_is_numeric(&context->result->types, vector->element_type_id)) return 1;
    dsl_error(context, span, "E-DSL-002",
              "vetor '%s' tem elemento %s; a DSL opera vetores inteira, flut ou duplocarpado",
              vector->name, type_name(&context->result->types, vector->element_type_id));
    return 0;
}

static int is_unknown_name(const DslContext *context, const AstNode *node)
{
    return node->kind == AST_IDENTIFIER_EXPR &&
           symbol_table_lookup(&context->result->symbols, context->scope_id,
                               SYMBOL_NAMESPACE_VALUE,
                               node->as.identifier_expr.name) == SYMBOL_ID_NONE;
}

int dsl_declare_builtins(SemanticResult *result)
{
    static const size_t parameters[2] = { TYPE_ID_NONE, TYPE_ID_NONE };
    SourceSpan span = source_span(source_position(0U, 0U, 0U),
                                  source_position(0U, 0U, 0U));
    size_t index;
    if (result == NULL) return 0;
    for (index = 0U; index < sizeof(dsl_builtins) / sizeof(dsl_builtins[0]); ++index) {
        size_t symbol_id;
        /* O tipo de retorno depende do argumento; a chamada o resolve. */
        if (!symbol_table_declare(&result->symbols, dsl_builtins[index].name,
                                  SYMBOL_NAMESPACE_FUNCTION, SYMBOL_BUILTIN_FUNCTION,
                                  TYPE_ID_ERROR, result->global_scope_id, span,
                                  &symbol_id) ||
            !symbol_set_parameters(&result->symbols, symbol_id, parameters,
                                   dsl_builtins[index].arity))
            return 0;
    }
    return 1;
}

int dsl_claims_assignment(const DslContext *context, const AstNode *assignment)
{
    const AstNode *value;
    AstOperator op;
    if (context == NULL || assignment == NULL || assignment->kind != AST_ASSIGNMENT)
        return 0;
    value = assignment->as.assignment.value;
    if (value == NULL || value->kind != AST_BINARY_EXPR) return 0;
    op = value->as.binary_expr.operator_kind;
    if (op != AST_OP_ADD && op != AST_OP_SUBTRACT && op != AST_OP_MULTIPLY) return 0;
    return vector_symbol(context->result, context->scope_id,
                         assignment->as.assignment.target) != NULL ||
           vector_symbol(context->result, context->scope_id,
                         value->as.binary_expr.left) != NULL ||
           vector_symbol(context->result, context->scope_id,
                         value->as.binary_expr.right) != NULL;
}

void dsl_analyze_assignment(DslContext *context, AstNode *assignment)
{
    AstNode *target = assignment->as.assignment.target;
    AstNode *value = assignment->as.assignment.value;
    AstNode *operands[2];
    const Symbol *vectors[2];
    const Symbol *destination = vector_symbol(context->result, context->scope_id, target);
    size_t types[2];
    size_t result_type;
    size_t index;
    int ok = 1;
    operands[0] = value->as.binary_expr.left;
    operands[1] = value->as.binary_expr.right;
    mark(assignment, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);

    /* D-020: o destino e analisado antes do valor. */
    if (destination == NULL) {
        if (context->analyze_expression(context->analyzer, target) != TYPE_ID_ERROR)
            dsl_error(context, target->span, "E-DSL-001",
                      "destino de operacao vetorial deve ser o nome de um vetor");
        ok = 0;
    } else {
        mark(target, destination->type_id, destination->id,
             AST_SEMANTIC_LVALUE | AST_SEMANTIC_ARRAY | DSL_SEMANTIC_FLAG);
        if (!numeric_element(context, destination, target->span)) ok = 0;
    }

    for (index = 0U; index < 2U; ++index) {
        AstNode *operand = operands[index];
        vectors[index] = vector_symbol(context->result, context->scope_id, operand);
        types[index] = TYPE_ID_ERROR;
        if (vectors[index] != NULL) {
            mark(operand, vectors[index]->type_id, vectors[index]->id,
                 AST_SEMANTIC_RVALUE | AST_SEMANTIC_ARRAY | DSL_SEMANTIC_FLAG);
            if (numeric_element(context, vectors[index], operand->span))
                types[index] = vectors[index]->element_type_id;
            else ok = 0;
        } else if (contains_vector(context, operand)) {
            dsl_error(context, operand->span, "E-DSL-005",
                      "expressao vetorial composta: a DSL faz uma operacao por comando");
            ok = 0;
        } else {
            size_t type = context->analyze_expression(context->analyzer, operand);
            if (type == TYPE_ID_ERROR) ok = 0;
            else if (!type_is_numeric(&context->result->types, type)) {
                dsl_error(context, operand->span, "E-DSL-006",
                          "escalar de operacao vetorial deve ser numerico, recebeu %s",
                          type_name(&context->result->types, type));
                ok = 0;
            } else types[index] = type;
        }
    }
    if (vectors[0] == NULL && vectors[1] == NULL) {
        if (ok)
            dsl_error(context, value->span, "E-DSL-005",
                      "operacao vetorial precisa de ao menos um vetor como operando");
        return;
    }
    if (!ok) return;

    for (index = 0U; index < 2U; ++index) {
        if (vectors[index] != NULL &&
            vectors[index]->array_size != destination->array_size) {
            dsl_error(context, operands[index]->span, "E-DSL-003",
                      "vetor '%s' tem %lld elemento(s) e o destino '%s' tem %lld",
                      vectors[index]->name, vectors[index]->array_size,
                      destination->name, destination->array_size);
            ok = 0;
        }
    }
    if (!ok) return;

    result_type = promote(types[0], types[1]);
    if (!type_is_assignable(destination->element_type_id, result_type)) {
        dsl_error(context, value->span, "E-DSL-004",
                  "operacao vetorial resulta em %s, que nao cabe no elemento %s de '%s'",
                  type_name(&context->result->types, result_type),
                  type_name(&context->result->types, destination->element_type_id),
                  destination->name);
        return;
    }
    /* O no binario guarda R, o tipo em que cada elemento e calculado. */
    mark(value, result_type, SYMBOL_ID_NONE,
         AST_SEMANTIC_RVALUE | AST_SEMANTIC_ARRAY | DSL_SEMANTIC_FLAG);
    mark(assignment, destination->type_id, destination->id,
         AST_SEMANTIC_ARRAY | DSL_SEMANTIC_FLAG);
}

int dsl_is_reduction_call(const SemanticResult *result, const AstNode *call)
{
    const Symbol *symbol;
    if (result == NULL || call == NULL || call->kind != AST_CALL_EXPR ||
        find_builtin(call->as.call_expr.callee) == NULL)
        return 0;
    symbol = symbol_table_get(&result->symbols,
                              symbol_table_lookup(&result->symbols,
                                                  result->global_scope_id,
                                                  SYMBOL_NAMESPACE_FUNCTION,
                                                  call->as.call_expr.callee));
    return symbol != NULL && symbol->category == SYMBOL_BUILTIN_FUNCTION;
}

size_t dsl_analyze_reduction(DslContext *context, AstNode *call,
                             int used_as_statement)
{
    const DslBuiltin *builtin = find_builtin(call->as.call_expr.callee);
    const AstNodeList *arguments = &call->as.call_expr.arguments;
    size_t symbol_id = symbol_table_lookup(&context->result->symbols,
                                           context->result->global_scope_id,
                                           SYMBOL_NAMESPACE_FUNCTION,
                                           call->as.call_expr.callee);
    const Symbol *vectors[2] = { NULL, NULL };
    size_t result_type;
    size_t index;
    int ok = 1;
    mark(call, TYPE_ID_ERROR, symbol_id, AST_SEMANTIC_ERROR);
    if (builtin == NULL) return TYPE_ID_ERROR;
    if (arguments->count != builtin->arity) {
        dsl_error(context, call->span, "E-DSL-008",
                  "%s espera %zu vetor(es), recebeu %zu argumento(s)",
                  builtin->name, builtin->arity, arguments->count);
        ok = 0;
    }
    for (index = 0U; index < arguments->count; ++index) {
        AstNode *argument = arguments->items[index];
        const Symbol *vector = vector_symbol(context->result, context->scope_id, argument);
        if (vector == NULL) {
            /* Nome desconhecido: a analise comum da o E-SEM-007 de sempre. */
            if (is_unknown_name(context, argument))
                (void)context->analyze_expression(context->analyzer, argument);
            else
                dsl_error(context, argument->span, "E-DSL-007",
                          "argumento %zu de %s deve ser o nome de um vetor",
                          index + 1U, builtin->name);
            ok = 0;
            continue;
        }
        mark(argument, vector->type_id, vector->id,
             AST_SEMANTIC_RVALUE | AST_SEMANTIC_ARRAY | DSL_SEMANTIC_FLAG);
        if (!numeric_element(context, vector, argument->span)) ok = 0;
        else if (index < 2U) vectors[index] = vector;
    }
    if (!ok) return TYPE_ID_ERROR;
    if (builtin->arity == 2U && vectors[0]->array_size != vectors[1]->array_size) {
        dsl_error(context, arguments->items[1]->span, "E-DSL-003",
                  "prodesc com tamanhos diferentes: '%s' tem %lld elemento(s) e '%s' tem %lld",
                  vectors[0]->name, vectors[0]->array_size,
                  vectors[1]->name, vectors[1]->array_size);
        return TYPE_ID_ERROR;
    }
    result_type = builtin->arity == 2U
                      ? promote(vectors[0]->element_type_id, vectors[1]->element_type_id)
                      : vectors[0]->element_type_id;
    if (used_as_statement) {
        dsl_error(context, call->span, "E-SEM-027",
                  "chamada usada como comando deve retornar vazio; '%s' retorna %s",
                  builtin->name, type_name(&context->result->types, result_type));
        return TYPE_ID_ERROR;
    }
    mark(call, result_type, symbol_id, AST_SEMANTIC_RVALUE | DSL_SEMANTIC_FLAG);
    return result_type;
}

int dsl_is_operation(const AstNode *node)
{
    return node != NULL &&
           (node->kind == AST_ASSIGNMENT || node->kind == AST_CALL_EXPR) &&
           (node->semantic.flags & DSL_SEMANTIC_FLAG) != 0U &&
           (node->semantic.flags & AST_SEMANTIC_ERROR) == 0U;
}

static const char *type_tag(size_t type_id)
{
    switch (type_id) {
    case TYPE_ID_INTEGER: return "i64";
    case TYPE_ID_FLOAT: return "f32";
    case TYPE_ID_DOUBLE: return "f64";
    default: return "x";
    }
}

static const char *operator_tag(AstOperator op)
{
    switch (op) {
    case AST_OP_ADD: return "soma";
    case AST_OP_SUBTRACT: return "sub";
    case AST_OP_MULTIPLY: return "mul";
    default: return "op";
    }
}

static void name_kernel(DslOperation *operation)
{
    const char *op = operator_tag(operation->op);
    const char *first = type_tag(operation->element_type_ids[0]);
    const char *result = type_tag(operation->result_type_id);
    const char *target = type_tag(operation->target_type_id);
    char *name = operation->kernel;
    size_t size = sizeof(operation->kernel);
    switch (operation->form) {
    case DSL_FORM_VECTOR_VECTOR:
        (void)snprintf(name, size, "mcn_vv_%s_%s_%s_%s", op, first,
                       type_tag(operation->element_type_ids[1]), target);
        break;
    case DSL_FORM_SCALAR_VECTOR:
        (void)snprintf(name, size, "mcn_ev_%s_%s_%s_%s", op, result, first, target);
        break;
    case DSL_FORM_VECTOR_SCALAR:
        (void)snprintf(name, size, "mcn_ve_%s_%s_%s_%s", op, first, result, target);
        break;
    case DSL_FORM_SUM: (void)snprintf(name, size, "mcn_somavet_%s", first); break;
    case DSL_FORM_MAX: (void)snprintf(name, size, "mcn_maxvet_%s", first); break;
    case DSL_FORM_DOT:
        (void)snprintf(name, size, "mcn_prodesc_%s_%s", first,
                       type_tag(operation->element_type_ids[1]));
        break;
    default: (void)snprintf(name, size, "mcn_invalido"); break;
    }
}

int dsl_describe(const SemanticResult *semantic, const AstNode *node,
                 DslOperation *operation)
{
    const Symbol *vectors[2] = { NULL, NULL };
    size_t index;
    if (semantic == NULL || operation == NULL || !dsl_is_operation(node)) return 0;
    (void)memset(operation, 0, sizeof(*operation));
    operation->op = AST_OP_NONE;
    operation->vector_ids[0] = operation->vector_ids[1] = SYMBOL_ID_NONE;
    operation->target_id = SYMBOL_ID_NONE;
    operation->element_type_ids[0] = operation->element_type_ids[1] = TYPE_ID_NONE;
    operation->scalar_type_id = TYPE_ID_NONE;
    if (node->kind == AST_CALL_EXPR) {
        const DslBuiltin *builtin = find_builtin(node->as.call_expr.callee);
        if (builtin == NULL || node->as.call_expr.arguments.count != builtin->arity)
            return 0;
        operation->form = builtin->form;
        for (index = 0U; index < builtin->arity; ++index) {
            vectors[index] = symbol_table_get(
                &semantic->symbols,
                node->as.call_expr.arguments.items[index]->semantic.symbol_id);
            if (vectors[index] == NULL) return 0;
        }
        operation->vector_count = builtin->arity;
    } else {
        const AstNode *value = node->as.assignment.value;
        const Symbol *target = symbol_table_get(&semantic->symbols,
                                                node->as.assignment.target->semantic.symbol_id);
        const AstNode *operands[2];
        if (target == NULL || value == NULL || value->kind != AST_BINARY_EXPR) return 0;
        operation->op = value->as.binary_expr.operator_kind;
        operation->target_id = target->id;
        operation->target_type_id = target->element_type_id;
        operands[0] = value->as.binary_expr.left;
        operands[1] = value->as.binary_expr.right;
        for (index = 0U; index < 2U; ++index) {
            if (operands[index]->kind == AST_IDENTIFIER_EXPR &&
                (operands[index]->semantic.flags & DSL_SEMANTIC_FLAG) != 0U) {
                vectors[operation->vector_count] =
                    symbol_table_get(&semantic->symbols, operands[index]->semantic.symbol_id);
                if (vectors[operation->vector_count] == NULL) return 0;
                ++operation->vector_count;
            } else {
                operation->scalar = operands[index];
                operation->scalar_type_id = operands[index]->semantic.type_id;
            }
        }
        if (operation->vector_count == 0U) return 0;
        operation->form = operation->vector_count == 2U ? DSL_FORM_VECTOR_VECTOR
                          : operation->scalar == operands[0] ? DSL_FORM_SCALAR_VECTOR
                                                             : DSL_FORM_VECTOR_SCALAR;
    }
    for (index = 0U; index < operation->vector_count; ++index) {
        operation->vector_ids[index] = vectors[index]->id;
        operation->element_type_ids[index] = vectors[index]->element_type_id;
    }
    operation->length = vectors[0]->array_size;
    switch (operation->form) {
    case DSL_FORM_VECTOR_VECTOR:
    case DSL_FORM_DOT:
        operation->result_type_id = promote(operation->element_type_ids[0],
                                            operation->element_type_ids[1]);
        break;
    case DSL_FORM_SCALAR_VECTOR:
    case DSL_FORM_VECTOR_SCALAR:
        operation->result_type_id = promote(operation->scalar_type_id,
                                            operation->element_type_ids[0]);
        break;
    default:
        operation->result_type_id = operation->element_type_ids[0];
        break;
    }
    if (operation->target_id == SYMBOL_ID_NONE)
        operation->target_type_id = operation->result_type_id;
    name_kernel(operation);
    return 1;
}

size_t dsl_kernel_arguments(const DslOperation *operation, DslArgument arguments[3])
{
    size_t count = 0U;
    size_t index;
    for (index = 0U; index < 3U; ++index) {
        arguments[index].is_scalar = 0;
        arguments[index].symbol_id = SYMBOL_ID_NONE;
    }
    if (operation->form == DSL_FORM_SCALAR_VECTOR) arguments[count++].is_scalar = 1;
    arguments[count++].symbol_id = operation->vector_ids[0];
    if (operation->form == DSL_FORM_VECTOR_SCALAR) arguments[count++].is_scalar = 1;
    if (operation->vector_count == 2U) arguments[count++].symbol_id = operation->vector_ids[1];
    if (operation->target_id != SYMBOL_ID_NONE) arguments[count++].symbol_id = operation->target_id;
    return count;
}

static int add_kernel(DslKernelList *list, const DslOperation *operation)
{
    size_t index;
    for (index = 0U; index < list->count; ++index)
        if (strcmp(list->items[index].kernel, operation->kernel) == 0) return 1;
    if (list->count == list->capacity) {
        size_t capacity = list->capacity == 0U ? 8U : list->capacity * 2U;
        DslOperation *items;
        if (capacity > (size_t)-1 / sizeof(*items)) return 0;
        items = (DslOperation *)realloc(list->items, capacity * sizeof(*items));
        if (items == NULL) return 0;
        list->items = items;
        list->capacity = capacity;
    }
    list->items[list->count++] = *operation;
    return 1;
}

static int collect(const AstNode *node, const SemanticResult *semantic,
                   DslKernelList *list);

static int collect_list(const AstNodeList *nodes, const SemanticResult *semantic,
                        DslKernelList *list)
{
    size_t index;
    for (index = 0U; index < nodes->count; ++index)
        if (!collect(nodes->items[index], semantic, list)) return 0;
    return 1;
}

static int collect(const AstNode *node, const SemanticResult *semantic,
                   DslKernelList *list)
{
    size_t index;
    if (node == NULL) return 1;
    if (dsl_is_operation(node)) {
        DslOperation operation;
        if (dsl_describe(semantic, node, &operation) && !add_kernel(list, &operation))
            return 0;
    }
    switch (node->kind) {
    case AST_PROGRAM: return collect_list(&node->as.program.declarations, semantic, list);
    case AST_FUNCTION_DECL: return collect(node->as.function_decl.body, semantic, list);
    case AST_BLOCK: return collect_list(&node->as.block.statements, semantic, list);
    case AST_ASSIGNMENT:
        return collect(node->as.assignment.target, semantic, list) &&
               collect(node->as.assignment.value, semantic, list);
    case AST_IF_STMT:
        for (index = 0U; index < node->as.if_stmt.branches.count; ++index)
            if (!collect(node->as.if_stmt.branches.items[index].condition, semantic, list) ||
                !collect(node->as.if_stmt.branches.items[index].body, semantic, list))
                return 0;
        return collect(node->as.if_stmt.else_body, semantic, list);
    case AST_WHILE_STMT:
        return collect(node->as.while_stmt.condition, semantic, list) &&
               collect(node->as.while_stmt.body, semantic, list);
    case AST_FOR_STMT:
        return collect(node->as.for_stmt.initializer, semantic, list) &&
               collect(node->as.for_stmt.condition, semantic, list) &&
               collect(node->as.for_stmt.iteration, semantic, list) &&
               collect(node->as.for_stmt.body, semantic, list);
    case AST_RETURN_STMT: return collect(node->as.return_stmt.value, semantic, list);
    case AST_EXPR_STMT: return collect(node->as.expr_stmt.expression, semantic, list);
    case AST_CALL_EXPR: return collect_list(&node->as.call_expr.arguments, semantic, list);
    case AST_INDEX_EXPR:
        return collect(node->as.index_expr.base, semantic, list) &&
               collect(node->as.index_expr.index, semantic, list);
    case AST_FIELD_EXPR: return collect(node->as.field_expr.base, semantic, list);
    case AST_BINARY_EXPR:
        return collect(node->as.binary_expr.left, semantic, list) &&
               collect(node->as.binary_expr.right, semantic, list);
    case AST_UNARY_EXPR: return collect(node->as.unary_expr.operand, semantic, list);
    default: return 1;
    }
}

int dsl_collect_kernels(const AstNode *program, const SemanticResult *semantic,
                        DslKernelList *list)
{
    if (program == NULL || semantic == NULL || list == NULL) return 0;
    list->items = NULL;
    list->count = 0U;
    list->capacity = 0U;
    if (collect(program, semantic, list)) return 1;
    dsl_kernel_list_destroy(list);
    return 0;
}

void dsl_kernel_list_destroy(DslKernelList *list)
{
    if (list == NULL) return;
    free(list->items);
    list->items = NULL;
    list->count = 0U;
    list->capacity = 0U;
}
