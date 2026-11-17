#include "semantic.h"
#include "dsl.h" /* Frente dsl-mlir */

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const SourceFile *source;
    DiagnosticBag *diagnostics;
    SemanticResult *result;
    size_t current_scope_id;
    size_t current_function_id;
    size_t current_return_type_id;
    int inside_function;
    int allocation_failed;
} SemanticAnalyzer;


static void annotate(AstNode *node, size_t type_id, size_t symbol_id,
                     unsigned int flags)
{
    if (node == NULL) return;
    node->semantic.type_id = type_id;
    node->semantic.symbol_id = symbol_id;
    node->semantic.flags = flags;
}

static void semantic_error(SemanticAnalyzer *analyzer,
                           SourceSpan span,
                           const char *code,
                           const char *format,
                           ...)
{
    va_list arguments;
    va_list copy;
    int required;
    char *message;
    if (analyzer == NULL || analyzer->diagnostics == NULL || format == NULL) return;
    va_start(arguments, format);
    va_copy(copy, arguments);
    required = vsnprintf(NULL, 0U, format, copy);
    va_end(copy);
    if (required < 0) {
        va_end(arguments);
        return;
    }
    message = (char *)malloc((size_t)required + 1U);
    if (message == NULL) {
        va_end(arguments);
        analyzer->allocation_failed = 1;
        return;
    }
    (void)vsnprintf(message, (size_t)required + 1U, format, arguments);
    va_end(arguments);
    if (diagnostic_bag_add(analyzer->diagnostics, analyzer->source, span,
                           DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SEMANTIC,
                           code, "%s", message)) {
        ++analyzer->result->semantic_error_count;
    } else {
        analyzer->allocation_failed = 1;
    }
    free(message);
}

static void allocation_error(SemanticAnalyzer *analyzer, SourceSpan span)
{
    if (analyzer->allocation_failed) return;
    analyzer->allocation_failed = 1;
    semantic_error(analyzer, span, "E-SEM-000",
                   "memoria insuficiente durante a analise semantica");
}

static size_t resolve_type_ref(SemanticAnalyzer *analyzer,
                               AstTypeRef *reference,
                               int allow_void,
                               const char *context)
{
    size_t type_id;
    if (reference == NULL || reference->name == NULL) return TYPE_ID_ERROR;
    type_id = type_table_find_named(&analyzer->result->types, reference->name);
    if (type_id == TYPE_ID_NONE) {
        semantic_error(analyzer, reference->span, "E-SEM-001",
                       "tipo '%s' nao foi declarado", reference->name);
        type_id = TYPE_ID_ERROR;
    } else if (!allow_void && type_id == TYPE_ID_VOID) {
        semantic_error(analyzer, reference->span, "E-SEM-002",
                       "tipo vazio nao pode ser usado em %s", context);
        type_id = TYPE_ID_ERROR;
    }
    reference->resolved_type_id = type_id;
    return type_id;
}

static int declare_symbol(SemanticAnalyzer *analyzer,
                          const char *name,
                          SourceSpan span,
                          SymbolNamespace name_space,
                          SymbolCategory category,
                          size_t type_id,
                          size_t scope_id,
                          size_t *out_symbol_id)
{
    size_t previous = symbol_table_lookup_current(&analyzer->result->symbols,
                                                  scope_id, name_space, name);
    if (previous != SYMBOL_ID_NONE) {
        const Symbol *old = symbol_table_get(&analyzer->result->symbols, previous);
        semantic_error(analyzer, span, "E-SEM-003",
                       "redefinicao de '%s' no mesmo escopo (declaracao anterior em %zu:%zu)",
                       name,
                       old->declaration_span.start.line,
                       old->declaration_span.start.column);
        if (out_symbol_id != NULL) *out_symbol_id = SYMBOL_ID_NONE;
        return 0;
    }
    if (!symbol_table_declare(&analyzer->result->symbols, name, name_space,
                              category, type_id, scope_id, span, out_symbol_id)) {
        allocation_error(analyzer, span);
        return 0;
    }
    return 1;
}

static int collect_record_names(SemanticAnalyzer *analyzer, AstNode *root)
{
    size_t index;
    if (root == NULL || root->kind != AST_PROGRAM) return 0;
    for (index = 0U; index < root->as.program.declarations.count; ++index) {
        AstNode *node = root->as.program.declarations.items[index];
        size_t type_id;
        size_t symbol_id;
        if (node == NULL || node->kind != AST_STRUCT_DECL) continue;
        if (type_table_find_named(&analyzer->result->types,
                                  node->as.struct_decl.name) != TYPE_ID_NONE ||
            symbol_table_lookup_current(&analyzer->result->symbols,
                                        analyzer->result->global_scope_id,
                                        SYMBOL_NAMESPACE_TYPE,
                                        node->as.struct_decl.name) != SYMBOL_ID_NONE) {
            semantic_error(analyzer, node->as.struct_decl.name_span, "E-SEM-003",
                           "redefinicao do tipo '%s'", node->as.struct_decl.name);
            annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
            continue;
        }
        if (!type_table_add_record(&analyzer->result->types,
                                   node->as.struct_decl.name, &type_id)) {
            allocation_error(analyzer, node->span);
            return 0;
        }
        if (!declare_symbol(analyzer, node->as.struct_decl.name,
                            node->as.struct_decl.name_span, SYMBOL_NAMESPACE_TYPE,
                            SYMBOL_RECORD, type_id,
                            analyzer->result->global_scope_id, &symbol_id)) continue;
        type_table_set_declaration_symbol(&analyzer->result->types, type_id, symbol_id);
        annotate(node, type_id, symbol_id, 0U);
    }
    return !analyzer->allocation_failed;
}

/* Chamadas comuns, com assinaturas fixas e identidade preservada para a IR. */
static int declare_builtins(SemanticAnalyzer *analyzer)
{
    static const struct {
        const char *name;
        BuiltinKind kind;
        size_t return_type;
        size_t parameter_types[2];
        size_t parameter_count;
    } builtins[] = {
        { "imprimeInteira", BUILTIN_PRINT_INTEGER, TYPE_ID_VOID, { TYPE_ID_INTEGER }, 1U },
        { "imprimeFlut", BUILTIN_PRINT_FLOAT, TYPE_ID_VOID, { TYPE_ID_FLOAT }, 1U },
        { "imprimeDuplocarpado", BUILTIN_PRINT_DOUBLE, TYPE_ID_VOID, { TYPE_ID_DOUBLE }, 1U },
        { "imprimePalavra", BUILTIN_PRINT_WORD, TYPE_ID_VOID, { TYPE_ID_WORD }, 1U },
        { "imprimeBool", BUILTIN_PRINT_BOOL, TYPE_ID_VOID, { TYPE_ID_BOOL }, 1U },
        { "imprimeNl", BUILTIN_PRINT_NEWLINE, TYPE_ID_VOID, { TYPE_ID_NONE }, 0U },
        { "leInteira", BUILTIN_READ_INTEGER, TYPE_ID_INTEGER, { TYPE_ID_NONE }, 0U },
        { "leFlut", BUILTIN_READ_FLOAT, TYPE_ID_FLOAT, { TYPE_ID_NONE }, 0U },
        { "leDuplocarpado", BUILTIN_READ_DOUBLE, TYPE_ID_DOUBLE, { TYPE_ID_NONE }, 0U },
        { "pinoModo", BUILTIN_PIN_MODE, TYPE_ID_VOID, { TYPE_ID_INTEGER, TYPE_ID_INTEGER }, 2U },
        { "pinoEscreve", BUILTIN_PIN_WRITE, TYPE_ID_VOID, { TYPE_ID_INTEGER, TYPE_ID_BOOL }, 2U },
        { "pinoLe", BUILTIN_PIN_READ, TYPE_ID_BOOL, { TYPE_ID_INTEGER }, 1U },
        { "espera", BUILTIN_DELAY_MS, TYPE_ID_VOID, { TYPE_ID_INTEGER }, 1U }
    };
    SourceSpan span = source_span(source_position(0U, 0U, 0U),
                                  source_position(0U, 0U, 0U));
    size_t index;
    for (index = 0U; index < sizeof(builtins) / sizeof(builtins[0]); ++index) {
        size_t symbol_id;
        Symbol *symbol;
        if (!declare_symbol(analyzer, builtins[index].name, span,
                            SYMBOL_NAMESPACE_FUNCTION, SYMBOL_BUILTIN_FUNCTION,
                            builtins[index].return_type,
                            analyzer->result->global_scope_id, &symbol_id) ||
            !symbol_set_parameters(&analyzer->result->symbols, symbol_id,
                                   builtins[index].parameter_types,
                                   builtins[index].parameter_count)) {
            allocation_error(analyzer, span);
            return 0;
        }
        symbol = symbol_table_get_mut(&analyzer->result->symbols, symbol_id);
        symbol->builtin_kind = builtins[index].kind;
    }
    return 1;
}

static int collect_function_signatures(SemanticAnalyzer *analyzer, AstNode *root)
{
    size_t index;
    size_t principal_count = 0U;
    SourceSpan program_span = root->span;
    for (index = 0U; index < root->as.program.declarations.count; ++index) {
        AstNode *node = root->as.program.declarations.items[index];
        size_t *parameter_types = NULL;
        size_t parameter_count;
        size_t parameter_index;
        size_t return_type;
        size_t symbol_id;
        if (node == NULL || node->kind != AST_FUNCTION_DECL) continue;
        if (node->as.function_decl.is_principal) ++principal_count;
        return_type = resolve_type_ref(analyzer, &node->as.function_decl.return_type,
                                       1, "retorno de funcao");
        parameter_count = node->as.function_decl.parameters.count;
        if (node->as.function_decl.is_principal &&
            (parameter_count != 0U ||
             (return_type != TYPE_ID_INTEGER && return_type != TYPE_ID_VOID &&
              return_type != TYPE_ID_ERROR))) {
            semantic_error(analyzer, node->as.function_decl.name_span,
                           "E-SEM-034",
                           "principal deve retornar inteira ou vazio e nao receber parametros");
        }
        if (parameter_count != 0U) {
            if (parameter_count > ((size_t)-1) / sizeof(*parameter_types)) {
                allocation_error(analyzer, node->span);
                return 0;
            }
            parameter_types = (size_t *)malloc(parameter_count * sizeof(*parameter_types));
            if (parameter_types == NULL) {
                allocation_error(analyzer, node->span);
                return 0;
            }
        }
        for (parameter_index = 0U; parameter_index < parameter_count; ++parameter_index) {
            AstNode *parameter = node->as.function_decl.parameters.items[parameter_index];
            if (parameter == NULL || parameter->kind != AST_VAR_DECL) {
                parameter_types[parameter_index] = TYPE_ID_ERROR;
            } else {
                parameter_types[parameter_index] = resolve_type_ref(
                    analyzer, &parameter->as.var_decl.type, 0, "parametro");
                annotate(parameter, parameter_types[parameter_index], SYMBOL_ID_NONE,
                         parameter_types[parameter_index] == TYPE_ID_ERROR
                             ? AST_SEMANTIC_ERROR : 0U);
            }
        }
        if (declare_symbol(analyzer, node->as.function_decl.name,
                           node->as.function_decl.name_span,
                           SYMBOL_NAMESPACE_FUNCTION,
                           node->as.function_decl.is_principal
                               ? SYMBOL_PRINCIPAL : SYMBOL_FUNCTION,
                           return_type, analyzer->result->global_scope_id,
                           &symbol_id)) {
            Symbol *symbol = symbol_table_get_mut(&analyzer->result->symbols, symbol_id);
            symbol->is_principal = node->as.function_decl.is_principal;
            if (!symbol_set_parameters(&analyzer->result->symbols, symbol_id,
                                       parameter_types, parameter_count)) {
                allocation_error(analyzer, node->span);
            }
            annotate(node, return_type, symbol_id,
                     return_type == TYPE_ID_ERROR ? AST_SEMANTIC_ERROR : 0U);
            if (node->as.function_decl.is_principal &&
                analyzer->result->principal_symbol_id == SYMBOL_ID_NONE) {
                analyzer->result->principal_symbol_id = symbol_id;
            }
        } else {
            annotate(node, return_type, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        }
        free(parameter_types);
    }
    /* So a falta de principal e relatada aqui. Uma segunda principal ja gera
     * E-SEM-003 (redefinicao) no ponto exato; relatar E-SEM-004 tambem seria
     * cascata: dois diagnosticos para o mesmo erro. */
    if (principal_count == 0U) {
        semantic_error(analyzer, program_span, "E-SEM-004",
                       "programa deve declarar exatamente uma principal; nenhuma encontrada");
    }
    return !analyzer->allocation_failed;
}

static size_t analyze_expression(SemanticAnalyzer *analyzer, AstNode *node,
                                 int allow_void_call);
static void analyze_statement(SemanticAnalyzer *analyzer, AstNode *node);

static void declare_value_names(SemanticAnalyzer *analyzer,
                                AstNode *node,
                                SymbolCategory category)
{
    AstTypeRef *type_ref;
    AstDeclaredNameList *names;
    size_t value_type;
    size_t declared_type;
    long long array_size = 0;
    size_t index;
    int is_vector = node->kind == AST_VECTOR_DECL;
    if (is_vector) {
        type_ref = &node->as.vector_decl.type;
        names = &node->as.vector_decl.names;
        array_size = node->as.vector_decl.size;
    } else {
        type_ref = &node->as.var_decl.type;
        names = &node->as.var_decl.names;
    }
    value_type = resolve_type_ref(analyzer, type_ref, 0,
                                  category == SYMBOL_FIELD ? "campo" : "variavel");
    declared_type = value_type;
    if (is_vector) {
        if (array_size <= 0 || array_size > INT32_MAX) {
            semantic_error(analyzer, node->as.vector_decl.size_span, "E-SEM-005",
                           "tamanho de vetor deve estar entre 1 e 2147483647; recebeu %lld",
                           array_size);
            declared_type = TYPE_ID_ERROR;
        } else if (value_type != TYPE_ID_ERROR &&
                   !type_table_add_array(&analyzer->result->types, value_type,
                                         array_size, &declared_type)) {
            allocation_error(analyzer, node->span);
            declared_type = TYPE_ID_ERROR;
        }
    }
    annotate(node, declared_type, SYMBOL_ID_NONE,
             declared_type == TYPE_ID_ERROR ? AST_SEMANTIC_ERROR :
             (is_vector ? AST_SEMANTIC_ARRAY : 0U));
    for (index = 0U; index < names->count; ++index) {
        AstDeclaredName *name = &names->items[index];
        size_t symbol_id;
        SymbolCategory declared_category = is_vector ? SYMBOL_VECTOR : category;
        if (declare_symbol(analyzer, name->name, name->span,
                           SYMBOL_NAMESPACE_VALUE, declared_category, declared_type,
                           analyzer->current_scope_id, &symbol_id)) {
            name->symbol_id = symbol_id;
            if (is_vector) {
                symbol_set_vector(&analyzer->result->symbols, symbol_id,
                                  value_type, array_size);
            }
            if (node->semantic.symbol_id == SYMBOL_ID_NONE)
                node->semantic.symbol_id = symbol_id;
        } else {
            name->symbol_id = SYMBOL_ID_NONE;
            node->semantic.flags |= AST_SEMANTIC_ERROR;
        }
    }
}

static size_t analyze_identifier(SemanticAnalyzer *analyzer, AstNode *node)
{
    const char *name = node->as.identifier_expr.name;
    size_t symbol_id = symbol_table_lookup(&analyzer->result->symbols,
                                           analyzer->current_scope_id,
                                           SYMBOL_NAMESPACE_VALUE, name);
    const Symbol *symbol;
    if (symbol_id == SYMBOL_ID_NONE) {
        size_t function_id = symbol_table_lookup(
                                                 &analyzer->result->symbols,
                                                 analyzer->result->global_scope_id,
                                                 SYMBOL_NAMESPACE_FUNCTION, name);
        if (function_id != SYMBOL_ID_NONE) {
            semantic_error(analyzer, node->span, "E-SEM-006",
                           "funcao '%s' nao pode ser usada como valor sem chamada", name);
        } else {
            semantic_error(analyzer, node->span, "E-SEM-007",
                           "identificador '%s' usado antes da declaracao ou nao declarado",
                           name);
        }
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    symbol = symbol_table_get(&analyzer->result->symbols, symbol_id);
    if (symbol != NULL) {
        const Type *type = type_table_get(&analyzer->result->types,
                                          symbol->type_id);
        if (type != NULL && type->kind == TYPE_ARRAY) {
            semantic_error(analyzer, node->span, "E-SEM-026",
                           "vetor '%s' nao pode ser usado como valor inteiro",
                           name);
            annotate(node, TYPE_ID_ERROR, symbol_id,
                     AST_SEMANTIC_ARRAY | AST_SEMANTIC_ERROR);
            return TYPE_ID_ERROR;
        }
    }
    annotate(node, symbol->type_id, symbol_id,
             AST_SEMANTIC_LVALUE | AST_SEMANTIC_RVALUE);
    return symbol->type_id;
}

static size_t analyze_call(SemanticAnalyzer *analyzer, AstNode *node,
                           int allow_void_call)
{
    size_t symbol_id = symbol_table_lookup(&analyzer->result->symbols,
                                           analyzer->result->global_scope_id,
                                           SYMBOL_NAMESPACE_FUNCTION,
                                           node->as.call_expr.callee);
    const Symbol *function = symbol_table_get(&analyzer->result->symbols, symbol_id);
    size_t index;
    size_t argument_count = node->as.call_expr.arguments.count;
    int call_has_error = 0;
    if (function == NULL) {
        size_t value_id = symbol_table_lookup(&analyzer->result->symbols,
                                              analyzer->current_scope_id,
                                              SYMBOL_NAMESPACE_VALUE,
                                              node->as.call_expr.callee);
        if (value_id != SYMBOL_ID_NONE) {
            semantic_error(analyzer, node->as.call_expr.callee_span, "E-SEM-008",
                           "'%s' refere-se a valor, nao a funcao",
                           node->as.call_expr.callee);
        } else {
            semantic_error(analyzer, node->as.call_expr.callee_span, "E-SEM-009",
                           "funcao '%s' nao foi declarada", node->as.call_expr.callee);
        }
        call_has_error = 1;
    } else if (function->is_principal) {
        semantic_error(analyzer, node->as.call_expr.callee_span, "E-SEM-010",
                       "principal nao pode ser chamada como funcao comum");
        call_has_error = 1;
    }
    for (index = 0U; index < argument_count; ++index) {
        size_t actual = analyze_expression(analyzer,
                                           node->as.call_expr.arguments.items[index], 0);
        if (function != NULL && index < function->parameter_count &&
            actual != TYPE_ID_ERROR &&
            !type_is_assignable(function->parameter_type_ids[index], actual)) {
            semantic_error(analyzer,
                           node->as.call_expr.arguments.items[index]->span,
                           "E-SEM-011",
                           "argumento %zu de '%s' esperava %s, recebeu %s",
                           index + 1U, function->name,
                           type_name(&analyzer->result->types,
                                     function->parameter_type_ids[index]),
                           type_name(&analyzer->result->types, actual));
            call_has_error = 1;
        } else if (actual == TYPE_ID_ERROR) {
            call_has_error = 1;
        }
    }
    if (function == NULL || function->is_principal) {
        annotate(node, TYPE_ID_ERROR, symbol_id, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    if (argument_count != function->parameter_count) {
        semantic_error(analyzer, node->span, "E-SEM-012",
                       "chamada de '%s' esperava %zu argumento(s), recebeu %zu",
                       function->name, function->parameter_count,
                       argument_count);
        call_has_error = 1;
    }
    if (function->type_id == TYPE_ID_VOID && !allow_void_call) {
        semantic_error(analyzer, node->span, "E-SEM-013",
                       "retorno vazio de '%s' nao pode compor uma expressao",
                       function->name);
        annotate(node, TYPE_ID_ERROR, symbol_id, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    if (allow_void_call && function->type_id != TYPE_ID_VOID) {
        semantic_error(analyzer, node->span, "E-SEM-027",
                       "chamada usada como comando deve retornar vazio; '%s' retorna %s",
                       function->name,
                       type_name(&analyzer->result->types, function->type_id));
        call_has_error = 1;
    }
    if (call_has_error) {
        annotate(node, TYPE_ID_ERROR, symbol_id, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    annotate(node, function->type_id, symbol_id,
             function->type_id == TYPE_ID_VOID ? 0U : AST_SEMANTIC_RVALUE);
    return function->type_id;
}

/* Mesma aritmetica modular i64 da IR (inteira = i64), sem overflow assinado
 * em C: as contas sao feitas em uint64_t e convertidas de volta em
 * complemento de dois sem depender de comportamento definido pela
 * implementacao. */
static long long integer_from_bits(uint64_t bits)
{
    if (bits <= (uint64_t)LLONG_MAX) return (long long)bits;
    return -(long long)(~bits) - 1LL;
}

static int evaluate_integer_constant(const AstNode *node, long long *result)
{
    long long left;
    long long right;

    if (node == NULL || result == NULL ||
        node->semantic.type_id != TYPE_ID_INTEGER ||
        (node->semantic.flags & AST_SEMANTIC_CONSTANT) == 0U) {
        return 0;
    }
    if (node->kind == AST_LITERAL_EXPR &&
        node->as.literal_expr.literal_kind == AST_LITERAL_INTEGER) {
        *result = node->as.literal_expr.value.integer;
        return 1;
    }
    if (node->kind == AST_UNARY_EXPR &&
        evaluate_integer_constant(node->as.unary_expr.operand, &left)) {
        if (node->as.unary_expr.operator_kind == AST_OP_UNARY_PLUS) {
            *result = left;
            return 1;
        }
        if (node->as.unary_expr.operator_kind == AST_OP_UNARY_MINUS) {
            *result = integer_from_bits((uint64_t)0U - (uint64_t)left);
            return 1;
        }
        return 0;
    }
    if (node->kind != AST_BINARY_EXPR ||
        !evaluate_integer_constant(node->as.binary_expr.left, &left) ||
        !evaluate_integer_constant(node->as.binary_expr.right, &right)) {
        return 0;
    }
    switch (node->as.binary_expr.operator_kind) {
    case AST_OP_ADD:
        *result = integer_from_bits((uint64_t)left + (uint64_t)right);
        return 1;
    case AST_OP_SUBTRACT:
        *result = integer_from_bits((uint64_t)left - (uint64_t)right);
        return 1;
    case AST_OP_MULTIPLY:
        *result = integer_from_bits((uint64_t)left * (uint64_t)right);
        return 1;
    case AST_OP_INTEGER_DIVIDE:
        if (right == 0 || (left == LLONG_MIN && right == -1)) return 0;
        *result = left / right;
        return 1;
    case AST_OP_POWER:
        if (right < 0) return 0;
        {
            unsigned long long exponent = (unsigned long long)right;
            uint64_t power = (uint64_t)left;
            uint64_t value = 1U;
            while (exponent != 0U) {
                if ((exponent & 1U) != 0U) value *= power;
                exponent >>= 1U;
                if (exponent != 0U) power *= power;
            }
            *result = integer_from_bits(value);
            return 1;
        }
    default:
        return 0;
    }
}

static size_t analyze_index(SemanticAnalyzer *analyzer, AstNode *node)
{
    size_t base_type = TYPE_ID_ERROR;
    size_t index_type = analyze_expression(analyzer, node->as.index_expr.index, 0);
    const Type *array_type;
    size_t symbol_id = SYMBOL_ID_NONE;
    int invalid_index = 0;
    if (node->as.index_expr.base != NULL &&
        node->as.index_expr.base->kind == AST_IDENTIFIER_EXPR) {
        const char *name = node->as.index_expr.base->as.identifier_expr.name;
        const Symbol *symbol;
        symbol_id = symbol_table_lookup(&analyzer->result->symbols,
                                        analyzer->current_scope_id,
                                        SYMBOL_NAMESPACE_VALUE, name);
        symbol = symbol_table_get(&analyzer->result->symbols, symbol_id);
        if (symbol == NULL) {
            semantic_error(analyzer, node->as.index_expr.base->span, "E-SEM-007",
                           "identificador '%s' usado antes da declaracao ou nao declarado",
                           name);
            annotate(node->as.index_expr.base, TYPE_ID_ERROR, SYMBOL_ID_NONE,
                     AST_SEMANTIC_ERROR);
        } else {
            const Type *base_value_type;
            base_type = symbol->type_id;
            base_value_type = type_table_get(&analyzer->result->types,
                                             base_type);
            annotate(node->as.index_expr.base, base_type, symbol_id,
                     base_value_type != NULL &&
                             base_value_type->kind == TYPE_ARRAY
                         ? AST_SEMANTIC_ARRAY
                         : AST_SEMANTIC_LVALUE | AST_SEMANTIC_RVALUE);
        }
    } else {
        base_type = analyze_expression(analyzer, node->as.index_expr.base, 0);
        symbol_id = node->as.index_expr.base != NULL
                        ? node->as.index_expr.base->semantic.symbol_id
                        : SYMBOL_ID_NONE;
    }
    array_type = type_table_get(&analyzer->result->types, base_type);
    if (index_type != TYPE_ID_ERROR && index_type != TYPE_ID_INTEGER) {
        semantic_error(analyzer, node->as.index_expr.index->span, "E-SEM-014",
                       "indice de vetor deve ser inteira, recebeu %s",
                       type_name(&analyzer->result->types, index_type));
        invalid_index = 1;
    } else if (index_type == TYPE_ID_ERROR) {
        invalid_index = 1;
    }
    if (base_type == TYPE_ID_ERROR) {
        annotate(node, TYPE_ID_ERROR, symbol_id, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    if (array_type == NULL || array_type->kind != TYPE_ARRAY) {
        semantic_error(analyzer, node->as.index_expr.base->span, "E-SEM-015",
                       "acesso com indice exige vetor, recebeu %s",
                       type_name(&analyzer->result->types, base_type));
        annotate(node, TYPE_ID_ERROR, symbol_id, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    if (index_type == TYPE_ID_INTEGER) {
        long long value;
        if (evaluate_integer_constant(node->as.index_expr.index, &value)) {
        if (value < 0 || value >= array_type->array_size) {
            semantic_error(analyzer, node->as.index_expr.index->span, "E-SEM-016",
                           "indice %lld fora dos limites do vetor [0, %lld)",
                           value, array_type->array_size);
            invalid_index = 1;
        }
        }
    }
    if (invalid_index) {
        annotate(node, TYPE_ID_ERROR, symbol_id, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    annotate(node, array_type->element_type_id, symbol_id,
             AST_SEMANTIC_LVALUE | AST_SEMANTIC_RVALUE);
    return array_type->element_type_id;
}

static size_t invalid_operator(SemanticAnalyzer *analyzer, AstNode *node,
                               size_t left, size_t right)
{
    if (left != TYPE_ID_ERROR && right != TYPE_ID_ERROR) {
        semantic_error(analyzer, node->as.binary_expr.operator_span, "E-SEM-017",
                       "operador %s nao aceita operandos %s e %s",
                       ast_operator_name(node->as.binary_expr.operator_kind),
                       type_name(&analyzer->result->types, left),
                       type_name(&analyzer->result->types, right));
    }
    annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
    return TYPE_ID_ERROR;
}

static size_t record_scope_id(const SemanticResult *result, size_t type_id)
{
    const Type *type = type_table_get(&result->types, type_id);
    size_t index;
    if (type == NULL || type->kind != TYPE_RECORD) return SCOPE_ID_NONE;
    for (index = 0U; index < result->symbols.scope_count; ++index) {
        const Scope *scope = &result->symbols.scopes[index];
        if (scope->kind == SCOPE_RECORD &&
            scope->owner_symbol_id == type->declaration_symbol_id)
            return scope->id;
    }
    return SCOPE_ID_NONE;
}

static size_t analyze_field(SemanticAnalyzer *analyzer, AstNode *node)
{
    AstNode *base = node->as.field_expr.base;
    size_t base_type = analyze_expression(analyzer, base, 0);
    const Type *type = type_table_get(&analyzer->result->types, base_type);
    size_t scope_id;
    size_t field_id;
    const Symbol *field;
    unsigned int flags;
    if (base_type == TYPE_ID_ERROR) {
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    if (type == NULL || type->kind != TYPE_RECORD) {
        semantic_error(analyzer, node->as.field_expr.field_span, "E-SEM-035",
                       "acesso a campo exige registro, recebeu %s",
                       type_name(&analyzer->result->types, base_type));
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    scope_id = record_scope_id(analyzer->result, base_type);
    field_id = symbol_table_lookup_current(&analyzer->result->symbols, scope_id,
                                           SYMBOL_NAMESPACE_VALUE,
                                           node->as.field_expr.field_name);
    field = symbol_table_get(&analyzer->result->symbols, field_id);
    if (field == NULL || field->category != SYMBOL_FIELD) {
        semantic_error(analyzer, node->as.field_expr.field_span, "E-SEM-036",
                       "registro '%s' nao possui campo '%s'", type->name,
                       node->as.field_expr.field_name);
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    flags = base->semantic.flags & (AST_SEMANTIC_LVALUE | AST_SEMANTIC_RVALUE);
    if (field->type_id == TYPE_ID_ERROR) flags |= AST_SEMANTIC_ERROR;
    annotate(node, field->type_id, field_id, flags);
    return field->type_id;
}

static size_t analyze_binary(SemanticAnalyzer *analyzer, AstNode *node)
{
    size_t left = analyze_expression(analyzer, node->as.binary_expr.left, 0);
    size_t right = analyze_expression(analyzer, node->as.binary_expr.right, 0);
    /* Expressao mista (D-012): a conta e feita no tipo numerico mais largo. */
    size_t common = type_numeric_common(left, right);
    size_t operand_type = common;
    size_t result_type = TYPE_ID_ERROR;
    int valid = 0;
    if (left == TYPE_ID_ERROR || right == TYPE_ID_ERROR)
        return invalid_operator(analyzer, node, left, right);
    switch (node->as.binary_expr.operator_kind) {
    case AST_OP_ADD:
    case AST_OP_SUBTRACT:
    case AST_OP_MULTIPLY:
    case AST_OP_POWER:
        valid = common != TYPE_ID_ERROR;
        result_type = common;
        break;
    case AST_OP_DIVIDE:
        /* "/" e sempre divisao real: inteira / inteira e feita em duplocarpado. */
        valid = common != TYPE_ID_ERROR;
        if (common == TYPE_ID_INTEGER) operand_type = TYPE_ID_DOUBLE;
        result_type = operand_type;
        break;
    case AST_OP_INTEGER_DIVIDE:
        /* "//" so existe para inteira; com real o programa usa "/". */
        valid = left == TYPE_ID_INTEGER && right == TYPE_ID_INTEGER;
        result_type = TYPE_ID_INTEGER;
        break;
    case AST_OP_LESS:
    case AST_OP_GREATER:
        valid = common != TYPE_ID_ERROR;
        result_type = TYPE_ID_BOOL;
        break;
    case AST_OP_EQUAL:
    case AST_OP_DIFFERENT:
        if (common == TYPE_ID_ERROR) {
            valid = left == right && type_is_scalar(&analyzer->result->types, left);
            operand_type = left;
        } else {
            valid = 1;
        }
        result_type = TYPE_ID_BOOL;
        break;
    case AST_OP_LOGICAL_AND:
    case AST_OP_LOGICAL_XOR:
    case AST_OP_LOGICAL_OR:
        valid = left == TYPE_ID_BOOL && right == TYPE_ID_BOOL;
        operand_type = TYPE_ID_BOOL;
        result_type = TYPE_ID_BOOL;
        break;
    default:
        valid = 0;
        break;
    }
    if (!valid) return invalid_operator(analyzer, node, left, right);
    annotate(node, result_type, SYMBOL_ID_NONE,
             AST_SEMANTIC_RVALUE |
             (((node->as.binary_expr.left->semantic.flags & AST_SEMANTIC_CONSTANT) != 0U &&
               (node->as.binary_expr.right->semantic.flags & AST_SEMANTIC_CONSTANT) != 0U)
                  ? AST_SEMANTIC_CONSTANT
                  : 0U));
    if (left != operand_type || right != operand_type)
        node->semantic.conversion_type_id = operand_type;
    return result_type;
}

static size_t analyze_unary(SemanticAnalyzer *analyzer, AstNode *node)
{
    AstNode *operand_node = node->as.unary_expr.operand;
    size_t operand;
    size_t result_type;
    int valid = 0;
    /* O lexer ja limita a magnitude a INT64_MAX, que cabe em inteira (i64). */
    operand = analyze_expression(analyzer, operand_node, 0);
    result_type = operand;
    if (operand == TYPE_ID_ERROR) {
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    switch (node->as.unary_expr.operator_kind) {
    case AST_OP_UNARY_PLUS:
    case AST_OP_UNARY_MINUS:
        valid = type_is_numeric(&analyzer->result->types, operand);
        break;
    case AST_OP_ROOT:
        /* raiz de inteira e calculada em duplocarpado, como inteira / inteira. */
        valid = type_is_numeric(&analyzer->result->types, operand);
        if (operand == TYPE_ID_INTEGER) result_type = TYPE_ID_DOUBLE;
        break;
    case AST_OP_LOGICAL_NOT:
        valid = operand == TYPE_ID_BOOL;
        result_type = TYPE_ID_BOOL;
        break;
    default:
        break;
    }
    if (!valid) {
        semantic_error(analyzer, node->as.unary_expr.operator_span, "E-SEM-017",
                       "operador %s nao aceita operando %s",
                       ast_operator_name(node->as.unary_expr.operator_kind),
                       type_name(&analyzer->result->types, operand));
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
    annotate(node, result_type, SYMBOL_ID_NONE,
             AST_SEMANTIC_RVALUE |
             ((node->as.unary_expr.operand->semantic.flags & AST_SEMANTIC_CONSTANT) != 0U
                  ? AST_SEMANTIC_CONSTANT
                  : 0U));
    if (node->as.unary_expr.operator_kind == AST_OP_ROOT && result_type != operand)
        node->semantic.conversion_type_id = result_type;
    return result_type;
}

/* Frente dsl-mlir: a regra da DSL mora em dsl.c; aqui so o contexto. */
static size_t dsl_analyze_scalar(void *analyzer, AstNode *node)
{
    return analyze_expression((SemanticAnalyzer *)analyzer, node, 0);
}

static DslContext dsl_context(SemanticAnalyzer *analyzer)
{
    DslContext context;
    context.source = analyzer->source;
    context.diagnostics = analyzer->diagnostics;
    context.result = analyzer->result;
    context.scope_id = analyzer->current_scope_id;
    context.allocation_failed = &analyzer->allocation_failed;
    context.analyzer = analyzer;
    context.analyze_expression = dsl_analyze_scalar;
    return context;
}

static size_t analyze_expression(SemanticAnalyzer *analyzer, AstNode *node,
                                 int allow_void_call)
{
    size_t type_id;
    if (node == NULL) return TYPE_ID_ERROR;
    switch (node->kind) {
    case AST_IDENTIFIER_EXPR:
        return analyze_identifier(analyzer, node);
    case AST_LITERAL_EXPR:
        switch (node->as.literal_expr.literal_kind) {
        case AST_LITERAL_INTEGER:
            /* inteira = i64: o lexer rejeita magnitudes acima de INT64_MAX
             * (E-LEX-006), entao todo literal aceito cabe no tipo. */
            type_id = TYPE_ID_INTEGER;
            break;
        case AST_LITERAL_REAL:
            type_id = TYPE_ID_FLOAT;
            if (!isfinite(node->as.literal_expr.value.real) ||
                fabs(node->as.literal_expr.value.real) > FLT_MAX) {
                semantic_error(analyzer, node->span, "E-SEM-038",
                               "literal flut fora da faixa binary32");
                type_id = TYPE_ID_ERROR;
            } else {
                node->as.literal_expr.value.real =
                    (double)(float)node->as.literal_expr.value.real;
            }
            break;
        case AST_LITERAL_STRING: type_id = TYPE_ID_WORD; break;
        case AST_LITERAL_BOOLEAN: type_id = TYPE_ID_BOOL; break;
        default: type_id = TYPE_ID_ERROR; break;
        }
        annotate(node, type_id, SYMBOL_ID_NONE,
                 AST_SEMANTIC_RVALUE | AST_SEMANTIC_CONSTANT |
                 (type_id == TYPE_ID_ERROR ? AST_SEMANTIC_ERROR : 0U));
        return type_id;
    case AST_CALL_EXPR:
        if (dsl_is_reduction_call(analyzer->result, node)) { /* Frente dsl-mlir */
            DslContext context = dsl_context(analyzer);
            return dsl_analyze_reduction(&context, node, allow_void_call);
        }
        return analyze_call(analyzer, node, allow_void_call);
    case AST_INDEX_EXPR:
        return analyze_index(analyzer, node);
    case AST_FIELD_EXPR:
        return analyze_field(analyzer, node);
    case AST_BINARY_EXPR:
        return analyze_binary(analyzer, node);
    case AST_UNARY_EXPR:
        return analyze_unary(analyzer, node);
    default:
        semantic_error(analyzer, node->span, "E-SEM-018",
                       "no %s nao e uma expressao valida", ast_kind_name(node->kind));
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return TYPE_ID_ERROR;
    }
}

/* Simbolo do vetor nomeado por um identificador simples (sem indice), ou
 * SYMBOL_ID_NONE se o no nao for isso. */
static size_t whole_vector_symbol(SemanticAnalyzer *analyzer, const AstNode *node)
{
    size_t symbol_id;
    const Symbol *symbol;
    const Type *type;
    if (node == NULL || node->kind != AST_IDENTIFIER_EXPR) return SYMBOL_ID_NONE;
    symbol_id = symbol_table_lookup(&analyzer->result->symbols, analyzer->current_scope_id,
                                    SYMBOL_NAMESPACE_VALUE, node->as.identifier_expr.name);
    if (symbol_id == SYMBOL_ID_NONE) return SYMBOL_ID_NONE;
    symbol = symbol_table_get(&analyzer->result->symbols, symbol_id);
    if (symbol == NULL) return SYMBOL_ID_NONE;
    type = type_table_get(&analyzer->result->types, symbol->type_id);
    return type != NULL && type->kind == TYPE_ARRAY ? symbol_id : SYMBOL_ID_NONE;
}

static void analyze_assignment(SemanticAnalyzer *analyzer, AstNode *node)
{
    DslContext context = dsl_context(analyzer); /* Frente dsl-mlir */
    AstNode *target_node = node->as.assignment.target;
    AstNode *value_node = node->as.assignment.value;
    size_t target_vector;
    size_t target;
    size_t value;
    /* A DSL vetorial trata `c receba a + b` (e `-`, `*`) com vetores inteiros;
     * qualquer outra atribuicao de vetor inteiro continua recusada (E-SEM-026). */
    if (dsl_claims_assignment(&context, node)) {
        dsl_analyze_assignment(&context, node);
        return;
    }
    target_vector = whole_vector_symbol(analyzer, target_node);
    if (target_vector != SYMBOL_ID_NONE) {
        /* Vetor inteiro como alvo (`destino receba origem`): a linguagem nao
         * copia vetores (D-006), entao ha um erro so, no alvo. Se o valor
         * tambem e um vetor inteiro, e o mesmo erro e nao e relatado de novo;
         * qualquer outro valor e analisado normalmente. */
        semantic_error(analyzer, target_node->span, "E-SEM-026",
                       "vetor '%s' nao pode ser atribuido de uma vez; atribua elemento por elemento",
                       target_node->as.identifier_expr.name);
        annotate(target_node, TYPE_ID_ERROR, target_vector,
                 AST_SEMANTIC_ARRAY | AST_SEMANTIC_ERROR);
        value = whole_vector_symbol(analyzer, value_node);
        if (value != SYMBOL_ID_NONE) {
            annotate(value_node, TYPE_ID_ERROR, value, AST_SEMANTIC_ARRAY | AST_SEMANTIC_ERROR);
        } else {
            (void)analyze_expression(analyzer, value_node, 0);
        }
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return;
    }
    target = analyze_expression(analyzer, target_node, 0);
    value = analyze_expression(analyzer, value_node, 0);
    if ((node->as.assignment.target->semantic.flags & AST_SEMANTIC_LVALUE) == 0U &&
        target != TYPE_ID_ERROR) {
        semantic_error(analyzer, node->as.assignment.target->span, "E-SEM-019",
                       "alvo da atribuicao nao e gravavel");
    }
    if (target != TYPE_ID_ERROR && value != TYPE_ID_ERROR &&
        !type_is_assignable(target, value)) {
        semantic_error(analyzer, node->as.assignment.value->span, "E-SEM-020",
                       "atribuicao esperava %s, recebeu %s",
                       type_name(&analyzer->result->types, target),
                       type_name(&analyzer->result->types, value));
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
    } else {
        annotate(node, target, node->as.assignment.target->semantic.symbol_id,
                 target == TYPE_ID_ERROR || value == TYPE_ID_ERROR
                     ? AST_SEMANTIC_ERROR : 0U);
    }
}

static void analyze_condition(SemanticAnalyzer *analyzer, AstNode *condition)
{
    size_t type_id = analyze_expression(analyzer, condition, 0);
    if (type_id != TYPE_ID_ERROR && type_id != TYPE_ID_BOOL) {
        semantic_error(analyzer, condition->span, "E-SEM-021",
                       "condicao deve resultar em bool, recebeu %s",
                       type_name(&analyzer->result->types, type_id));
        condition->semantic.flags |= AST_SEMANTIC_ERROR;
    }
}

static void analyze_block(SemanticAnalyzer *analyzer, AstNode *block)
{
    size_t index;
    if (block == NULL || block->kind != AST_BLOCK) return;
    annotate(block, TYPE_ID_VOID, SYMBOL_ID_NONE, 0U);
    for (index = 0U; index < block->as.block.statements.count; ++index)
        analyze_statement(analyzer, block->as.block.statements.items[index]);
}

static void analyze_return(SemanticAnalyzer *analyzer, AstNode *node)
{
    AstNode *value = node->as.return_stmt.value;
    if (!analyzer->inside_function) {
        semantic_error(analyzer, node->span, "E-SEM-022",
                       "respost somente pode aparecer em funcao");
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        return;
    }
    if (analyzer->current_return_type_id == TYPE_ID_VOID) {
        if (value != NULL) {
            (void)analyze_expression(analyzer, value, 0);
            semantic_error(analyzer, value->span, "E-SEM-023",
                           "funcao vazio nao pode retornar valor");
            annotate(node, TYPE_ID_ERROR, analyzer->current_function_id,
                     AST_SEMANTIC_ERROR);
        } else {
            annotate(node, TYPE_ID_VOID, analyzer->current_function_id, 0U);
        }
        return;
    }
    if (value == NULL) {
        semantic_error(analyzer, node->span, "E-SEM-024",
                       "funcao com retorno %s exige valor em respost",
                       type_name(&analyzer->result->types,
                                 analyzer->current_return_type_id));
        annotate(node, TYPE_ID_ERROR, analyzer->current_function_id,
                 AST_SEMANTIC_ERROR);
    } else {
        size_t actual = analyze_expression(analyzer, value, 0);
        if (actual == TYPE_ID_ERROR) {
            annotate(node, TYPE_ID_ERROR, analyzer->current_function_id,
                     AST_SEMANTIC_ERROR);
        } else if (
            !type_is_assignable(analyzer->current_return_type_id, actual)) {
            semantic_error(analyzer, value->span, "E-SEM-025",
                           "retorno esperava %s, recebeu %s",
                           type_name(&analyzer->result->types,
                                     analyzer->current_return_type_id),
                           type_name(&analyzer->result->types, actual));
            annotate(node, TYPE_ID_ERROR, analyzer->current_function_id,
                     AST_SEMANTIC_ERROR);
        } else {
            annotate(node, analyzer->current_return_type_id,
                     analyzer->current_function_id, 0U);
        }
    }
}

static void analyze_if(SemanticAnalyzer *analyzer, AstNode *node)
{
    size_t index;
    for (index = 0U; index < node->as.if_stmt.branches.count; ++index) {
        AstIfBranch *branch = &node->as.if_stmt.branches.items[index];
        analyze_condition(analyzer, branch->condition);
        analyze_block(analyzer, branch->body);
    }
    analyze_block(analyzer, node->as.if_stmt.else_body);
    annotate(node, TYPE_ID_VOID, SYMBOL_ID_NONE, 0U);
}

static void analyze_statement(SemanticAnalyzer *analyzer, AstNode *node)
{
    size_t expression_type;
    if (node == NULL || analyzer->allocation_failed) return;
    switch (node->kind) {
    case AST_VAR_DECL:
        declare_value_names(analyzer, node, SYMBOL_LOCAL_VARIABLE);
        break;
    case AST_VECTOR_DECL:
        declare_value_names(analyzer, node, SYMBOL_LOCAL_VARIABLE);
        break;
    case AST_ASSIGNMENT:
        analyze_assignment(analyzer, node);
        break;
    case AST_IF_STMT:
        analyze_if(analyzer, node);
        break;
    case AST_WHILE_STMT:
        analyze_condition(analyzer, node->as.while_stmt.condition);
        analyze_block(analyzer, node->as.while_stmt.body);
        annotate(node, TYPE_ID_VOID, SYMBOL_ID_NONE, 0U);
        break;
    case AST_FOR_STMT:
        analyze_statement(analyzer, node->as.for_stmt.initializer);
        analyze_condition(analyzer, node->as.for_stmt.condition);
        analyze_statement(analyzer, node->as.for_stmt.iteration);
        analyze_block(analyzer, node->as.for_stmt.body);
        annotate(node, TYPE_ID_VOID, SYMBOL_ID_NONE, 0U);
        break;
    case AST_RETURN_STMT:
        analyze_return(analyzer, node);
        break;
    case AST_EXPR_STMT:
        if (node->as.expr_stmt.expression == NULL ||
            node->as.expr_stmt.expression->kind != AST_CALL_EXPR) {
            semantic_error(analyzer, node->span, "E-SEM-028",
                           "somente chamada de funcao vazio pode ser comando");
            if (node->as.expr_stmt.expression != NULL)
                (void)analyze_expression(analyzer,
                                         node->as.expr_stmt.expression, 0);
            annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE,
                     AST_SEMANTIC_ERROR);
            break;
        }
        expression_type = analyze_expression(
            analyzer, node->as.expr_stmt.expression, 1);
        annotate(node,
                 expression_type == TYPE_ID_VOID ? TYPE_ID_VOID : TYPE_ID_ERROR,
                 node->as.expr_stmt.expression->semantic.symbol_id,
                 expression_type == TYPE_ID_VOID ? 0U : AST_SEMANTIC_ERROR);
        break;
    case AST_BLOCK:
        analyze_block(analyzer, node);
        break;
    default:
        semantic_error(analyzer, node->span, "E-SEM-029",
                       "no %s nao e um comando valido",
                       ast_kind_name(node->kind));
        annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE, AST_SEMANTIC_ERROR);
        break;
    }
}

static void analyze_record(SemanticAnalyzer *analyzer, AstNode *node)
{
    size_t old_scope = analyzer->current_scope_id;
    size_t record_scope;
    size_t index;
    size_t field_index = 0U;
    if (node->semantic.symbol_id == SYMBOL_ID_NONE) return;
    if (!symbol_table_add_scope(&analyzer->result->symbols,
                                analyzer->result->global_scope_id,
                                SCOPE_RECORD, node->as.struct_decl.name,
                                node->semantic.symbol_id, &record_scope)) {
        allocation_error(analyzer, node->span);
        return;
    }
    analyzer->current_scope_id = record_scope;
    for (index = 0U; index < node->as.struct_decl.fields.count; ++index) {
        AstNode *field = node->as.struct_decl.fields.items[index];
        if (field != NULL && field->kind == AST_VAR_DECL) {
            size_t name_index;
            declare_value_names(analyzer, field, SYMBOL_FIELD);
            for (name_index = 0U; name_index < field->as.var_decl.names.count;
                 ++name_index) {
                Symbol *symbol = symbol_table_get_mut(
                    &analyzer->result->symbols,
                    field->as.var_decl.names.items[name_index].symbol_id);
                if (symbol != NULL) symbol->field_index = field_index;
                ++field_index;
            }
        } else if (field != NULL) {
            semantic_error(analyzer, field->span, "E-SEM-030",
                           "declaracao de campo invalida");
            annotate(field, TYPE_ID_ERROR, SYMBOL_ID_NONE,
                     AST_SEMANTIC_ERROR);
        }
    }
    analyzer->current_scope_id = old_scope;
}

/* Busca em profundidade com pilha explicita: registros por valor nao podem
 * conter um caminho de campos que volte ao proprio tipo. */
static void validate_record_cycles(SemanticAnalyzer *analyzer, SourceSpan span)
{
    typedef struct {
        size_t type_id;
        size_t scope_id;
        size_t next_symbol;
    } RecordFrame;
    size_t type_count = analyzer->result->types.count;
    unsigned char *color;
    RecordFrame *stack;
    size_t start;
    if (type_count > ((size_t)-1) / sizeof(*stack)) {
        allocation_error(analyzer, span);
        return;
    }
    color = (unsigned char *)calloc(type_count, sizeof(*color));
    stack = (RecordFrame *)malloc(type_count * sizeof(*stack));
    if (color == NULL || stack == NULL) {
        free(color);
        free(stack);
        allocation_error(analyzer, span);
        return;
    }
    for (start = 0U; start < type_count; ++start) {
        size_t depth = 0U;
        if (analyzer->result->types.items[start].kind != TYPE_RECORD ||
            color[start] != 0U) continue;
        stack[depth].type_id = start;
        stack[depth].scope_id = record_scope_id(analyzer->result, start);
        stack[depth++].next_symbol = 0U;
        color[start] = 1U;
        while (depth != 0U) {
            RecordFrame *frame = &stack[depth - 1U];
            const Symbol *field = NULL;
            size_t target;
            while (frame->next_symbol < analyzer->result->symbols.count) {
                const Symbol *candidate = &analyzer->result->symbols.items[
                    frame->next_symbol++];
                const Type *field_type;
                if (candidate->category != SYMBOL_FIELD ||
                    candidate->scope_id != frame->scope_id) continue;
                field_type = type_table_get(&analyzer->result->types,
                                             candidate->type_id);
                if (field_type != NULL && field_type->kind == TYPE_RECORD) {
                    field = candidate;
                    break;
                }
            }
            if (field == NULL) {
                color[frame->type_id] = 2U;
                --depth;
                continue;
            }
            target = field->type_id;
            if (color[target] == 1U) {
                semantic_error(analyzer, field->declaration_span, "E-SEM-039",
                               "campo '%s' cria ciclo de registros por valor com '%s'",
                               field->name,
                               type_name(&analyzer->result->types, target));
            } else if (color[target] == 0U) {
                color[target] = 1U;
                stack[depth].type_id = target;
                stack[depth].scope_id = record_scope_id(analyzer->result, target);
                stack[depth++].next_symbol = 0U;
            }
        }
    }
    free(stack);
    free(color);
}

/*
 * `durante` e `repete` nunca contam como retorno garantido: a condicao pode ser
 * falsa na primeira avaliacao e o corpo executar zero vezes.
 */
static int block_always_returns(const AstNode *block);

static int statement_always_returns(const AstNode *node)
{
    size_t index;
    if (node == NULL) return 0;
    switch (node->kind) {
    case AST_RETURN_STMT:
        return 1;
    case AST_BLOCK:
        return block_always_returns(node);
    case AST_IF_STMT:
        if (node->as.if_stmt.else_body == NULL ||
            !block_always_returns(node->as.if_stmt.else_body)) {
            return 0;
        }
        for (index = 0U; index < node->as.if_stmt.branches.count; ++index) {
            if (!block_always_returns(node->as.if_stmt.branches.items[index].body))
                return 0;
        }
        return 1;
    default:
        return 0;
    }
}

static int block_always_returns(const AstNode *block)
{
    size_t index;
    if (block == NULL || block->kind != AST_BLOCK) return 0;
    for (index = 0U; index < block->as.block.statements.count; ++index) {
        if (statement_always_returns(block->as.block.statements.items[index]))
            return 1;
    }
    return 0;
}

static void analyze_function(SemanticAnalyzer *analyzer, AstNode *node)
{
    size_t old_scope = analyzer->current_scope_id;
    size_t old_function = analyzer->current_function_id;
    size_t old_return_type = analyzer->current_return_type_id;
    int old_inside_function = analyzer->inside_function;
    size_t function_scope;
    size_t index;
    int valid_signature = node->semantic.symbol_id != SYMBOL_ID_NONE;

    if (!symbol_table_add_scope(&analyzer->result->symbols,
                                analyzer->result->global_scope_id,
                                SCOPE_FUNCTION,
                                node->as.function_decl.name,
                                node->semantic.symbol_id,
                                &function_scope)) {
        allocation_error(analyzer, node->span);
        return;
    }
    analyzer->current_scope_id = function_scope;
    analyzer->current_function_id = node->semantic.symbol_id;
    analyzer->current_return_type_id =
        node->as.function_decl.return_type.resolved_type_id;
    analyzer->inside_function = 1;

    for (index = 0U; index < node->as.function_decl.parameters.count; ++index) {
        AstNode *parameter = node->as.function_decl.parameters.items[index];
        if (parameter != NULL && parameter->kind == AST_VAR_DECL) {
            declare_value_names(analyzer, parameter, SYMBOL_PARAMETER);
        } else if (parameter != NULL) {
            semantic_error(analyzer, parameter->span, "E-SEM-031",
                           "parametro deve ser declaracao simples");
            annotate(parameter, TYPE_ID_ERROR, SYMBOL_ID_NONE,
                     AST_SEMANTIC_ERROR);
        }
    }
    analyze_block(analyzer, node->as.function_decl.body);

    if (valid_signature &&
        analyzer->current_return_type_id != TYPE_ID_VOID &&
        analyzer->current_return_type_id != TYPE_ID_ERROR &&
        !block_always_returns(node->as.function_decl.body)) {
        semantic_error(analyzer, node->as.function_decl.name_span,
                       "E-SEM-032",
                       "funcao '%s' com retorno %s nao garante respost de valor "
                       "em todos os caminhos de execucao",
                       node->as.function_decl.name,
                       type_name(&analyzer->result->types,
                                 analyzer->current_return_type_id));
        node->semantic.flags |= AST_SEMANTIC_ERROR;
    }

    analyzer->current_scope_id = old_scope;
    analyzer->current_function_id = old_function;
    analyzer->current_return_type_id = old_return_type;
    analyzer->inside_function = old_inside_function;
}

int semantic_result_init(SemanticResult *result)
{
    if (result == NULL) return 0;
    memset(result, 0, sizeof(*result));
    result->global_scope_id = SCOPE_ID_NONE;
    result->principal_symbol_id = SYMBOL_ID_NONE;
    if (!type_table_init(&result->types)) return 0;
    if (!symbol_table_init(&result->symbols)) {
        type_table_destroy(&result->types);
        return 0;
    }
    if (result->symbols.scope_count == 0U) {
        symbol_table_destroy(&result->symbols);
        type_table_destroy(&result->types);
        return 0;
    }
    result->global_scope_id = result->symbols.scopes[0].id;
    return 1;
}

void semantic_result_destroy(SemanticResult *result)
{
    if (result == NULL) return;
    symbol_table_destroy(&result->symbols);
    type_table_destroy(&result->types);
    memset(result, 0, sizeof(*result));
    result->global_scope_id = SCOPE_ID_NONE;
    result->principal_symbol_id = SYMBOL_ID_NONE;
}

FrontendStatus semantic_analyze(const SourceFile *source,
                                AstNode *root,
                                DiagnosticBag *diagnostics,
                                SemanticResult *result)
{
    SemanticAnalyzer analyzer;
    size_t index;
    size_t errors_before;
    if (source == NULL || root == NULL || diagnostics == NULL || result == NULL ||
        root->kind != AST_PROGRAM || result->analyzed ||
        result->global_scope_id == SCOPE_ID_NONE) {
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }
    memset(&analyzer, 0, sizeof(analyzer));
    analyzer.source = source;
    analyzer.diagnostics = diagnostics;
    analyzer.result = result;
    analyzer.current_scope_id = result->global_scope_id;
    analyzer.current_function_id = SYMBOL_ID_NONE;
    analyzer.current_return_type_id = TYPE_ID_ERROR;
    errors_before = diagnostics->error_count;

    if (!declare_builtins(&analyzer) ||
        !dsl_declare_builtins(result) || /* Frente dsl-mlir: somavet, maxvet, prodesc */
        !collect_record_names(&analyzer, root) ||
        !collect_function_signatures(&analyzer, root)) {
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }

    /* Campos devem estar resolvidos mesmo quando o registro aparece depois
     * da funcao que o utiliza. Globais continuam seguindo a ordem textual. */
    for (index = 0U; index < root->as.program.declarations.count &&
                     !analyzer.allocation_failed; ++index) {
        AstNode *node = root->as.program.declarations.items[index];
        if (node != NULL && node->kind == AST_STRUCT_DECL)
            analyze_record(&analyzer, node);
    }
    validate_record_cycles(&analyzer, root->span);

    for (index = 0U;
         index < root->as.program.declarations.count &&
         !analyzer.allocation_failed;
         ++index) {
        AstNode *node = root->as.program.declarations.items[index];
        if (node == NULL) continue;
        analyzer.current_scope_id = result->global_scope_id;
        switch (node->kind) {
        case AST_STRUCT_DECL:
            break;
        case AST_FUNCTION_DECL:
            analyze_function(&analyzer, node);
            break;
        case AST_VAR_DECL:
            declare_value_names(&analyzer, node, SYMBOL_GLOBAL_VARIABLE);
            break;
        case AST_VECTOR_DECL:
            declare_value_names(&analyzer, node, SYMBOL_GLOBAL_VARIABLE);
            break;
        default:
            semantic_error(&analyzer, node->span, "E-SEM-033",
                           "no %s nao e declaracao global valida",
                           ast_kind_name(node->kind));
            annotate(node, TYPE_ID_ERROR, SYMBOL_ID_NONE,
                     AST_SEMANTIC_ERROR);
            break;
        }
    }

    annotate(root, TYPE_ID_VOID, SYMBOL_ID_NONE,
             diagnostics->error_count == errors_before
                 ? 0U : AST_SEMANTIC_ERROR);
    result->analyzed = !analyzer.allocation_failed;
    if (analyzer.allocation_failed) {
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }
    return diagnostics->error_count == errors_before
               ? FRONTEND_STATUS_OK
               : FRONTEND_STATUS_SOURCE_ERROR;
}

static void dump_type(const TypeTable *types, size_t type_id, FILE *stream)
{
    const Type *type = type_table_get(types, type_id);
    if (type == NULL) {
        (void)fprintf(stream, "<tipo #%zu invalido>", type_id);
    } else if (type->kind == TYPE_ARRAY) {
        (void)fprintf(stream, "vetor[%lld] de ", type->array_size);
        dump_type(types, type->element_type_id, stream);
    } else {
        (void)fprintf(stream, "%s", type_name(types, type_id));
    }
}

void semantic_dump_symbols(const SemanticResult *result, FILE *stream)
{
    size_t scope_index;
    if (result == NULL) return;
    if (stream == NULL) stream = stdout;
    (void)fprintf(stream, "tabela de simbolos: %zu escopo(s), %zu simbolo(s)\n",
                  result->symbols.scope_count,
                  result->symbols.count);
    for (scope_index = 0U;
         scope_index < result->symbols.scope_count;
         ++scope_index) {
        const Scope *scope = &result->symbols.scopes[scope_index];
        size_t symbol_index;
        (void)fprintf(stream, "escopo #%zu %s '%s' pai=",
                      scope->id,
                      scope_kind_name(scope->kind), scope->name);
        if (scope->parent_id == SCOPE_ID_NONE)
            (void)fprintf(stream, "nenhum");
        else
            (void)fprintf(stream, "#%zu", scope->parent_id);
        (void)fprintf(stream, " dono=");
        if (scope->owner_symbol_id == SYMBOL_ID_NONE)
            (void)fprintf(stream, "nenhum\n");
        else
            (void)fprintf(stream, "#%zu\n", scope->owner_symbol_id);

        for (symbol_index = 0U;
             symbol_index < result->symbols.count;
             ++symbol_index) {
            const Symbol *symbol = &result->symbols.items[symbol_index];
            size_t parameter_index;
            if (symbol->scope_id != scope->id) continue;
            (void)fprintf(stream,
                          "  simbolo #%zu %s/%s '%s' tipo=",
                          symbol->id,
                          symbol_namespace_name(symbol->name_space),
                          symbol_category_name(symbol->category),
                          symbol->name);
            dump_type(&result->types, symbol->type_id, stream);
            (void)fprintf(stream, " declaracao=%zu:%zu",
                          symbol->declaration_span.start.line,
                          symbol->declaration_span.start.column);
            if (symbol->category == SYMBOL_FUNCTION ||
                symbol->category == SYMBOL_PRINCIPAL) {
                (void)fprintf(stream, " parametros=(");
                for (parameter_index = 0U;
                     parameter_index < symbol->parameter_count;
                     ++parameter_index) {
                    if (parameter_index != 0U)
                        (void)fprintf(stream, ", ");
                    dump_type(&result->types,
                              symbol->parameter_type_ids[parameter_index],
                              stream);
                }
                (void)fprintf(stream, ")");
            }
            if (symbol->category == SYMBOL_VECTOR) {
                (void)fprintf(stream, " elemento=");
                dump_type(&result->types, symbol->element_type_id, stream);
                (void)fprintf(stream, " tamanho=%lld", symbol->array_size);
            }
            (void)fprintf(stream, "\n");
        }
    }
}
