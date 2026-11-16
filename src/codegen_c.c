#include "codegen_c.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    size_t type_id;
    char name[64];
} CValue;

typedef struct {
    const SourceFile *source;
    const AstNode *function;
    const SemanticResult *semantic;
    DiagnosticBag *diagnostics;
    FILE *output;
    FrontendStatus status;
    size_t temporary;
} CCodegen;

static void c_fail(CCodegen *gen, const AstNode *node, const char *message)
{
    if (gen->status != FRONTEND_STATUS_OK) return;
    gen->status = FRONTEND_STATUS_INTERNAL_ERROR;
    (void)diagnostic_bag_add(gen->diagnostics, gen->source,
                            node != NULL ? node->span : gen->function->span,
                            DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_CODEGEN,
                            "E-C-001", "%s", message);
}

static void c_write(CCodegen *gen, const char *format, ...)
{
    va_list args;
    if (gen->status != FRONTEND_STATUS_OK) return;
    va_start(args, format);
    if (vfprintf(gen->output, format, args) < 0)
        c_fail(gen, gen->function, "falha ao escrever C gerado");
    va_end(args);
}

static const Type *c_type_info(CCodegen *gen, size_t id)
{
    return type_table_get(&gen->semantic->types, id);
}

static const Symbol *c_symbol(CCodegen *gen, size_t id)
{
    return symbol_table_get(&gen->semantic->symbols, id);
}

static void c_type(CCodegen *gen, size_t id)
{
    const Type *type = c_type_info(gen, id);
    if (type == NULL) {
        c_fail(gen, gen->function, "tipo ausente na AST anotada");
        return;
    }
    switch (type->kind) {
    /* bool cruza a ABI do runtime como int32_t 0/1, igual ao LLVM (i1 -> i32). */
    case TYPE_BOOL: c_write(gen, "int32_t"); break;
    /* Contrato do PjBL 3/4: inteira e i64 na IR e int64_t no C e no runtime. */
    case TYPE_INTEGER: c_write(gen, "int64_t"); break;
    case TYPE_FLOAT: c_write(gen, "float"); break;
    case TYPE_DOUBLE: c_write(gen, "double"); break;
    case TYPE_WORD: c_write(gen, "const char *"); break;
    case TYPE_VOID: c_write(gen, "void"); break;
    case TYPE_RECORD: c_write(gen, "mac_r_%zu", type->id); break;
    default: c_fail(gen, gen->function, "tipo C invalido para valor"); break;
    }
}

static void c_variable_name(CCodegen *gen, const Symbol *symbol)
{
    if (symbol->scope_id == gen->semantic->global_scope_id)
        c_write(gen, "mac_g_%zu", symbol->id);
    else c_write(gen, "mac_v_%zu", symbol->id);
}

static void c_function_name(CCodegen *gen, const Symbol *symbol)
{
    if (symbol->is_principal) c_write(gen, "zl_principal");
    else c_write(gen, "mac_fn_%zu", symbol->id);
}

static CValue c_new_value(CCodegen *gen, size_t type_id)
{
    CValue value;
    value.type_id = type_id;
    (void)snprintf(value.name, sizeof(value.name), "mac_t_%zu", gen->temporary++);
    return value;
}

static CValue c_new_address(CCodegen *gen, size_t type_id)
{
    CValue value;
    value.type_id = type_id;
    (void)snprintf(value.name, sizeof(value.name), "mac_p_%zu", gen->temporary++);
    return value;
}

static void c_declare_value(CCodegen *gen, CValue value)
{
    c_write(gen, "  ");
    c_type(gen, value.type_id);
    c_write(gen, " %s", value.name);
}

static void c_declare_pointer(CCodegen *gen, CValue value)
{
    c_write(gen, "  ");
    c_type(gen, value.type_id);
    c_write(gen, " *%s", value.name);
}

static void c_declare_symbol(CCodegen *gen, const Symbol *symbol, int initial)
{
    const Type *type = c_type_info(gen, symbol->type_id);
    if (type == NULL) {
        c_fail(gen, gen->function, "simbolo sem tipo C");
        return;
    }
    if (type->kind == TYPE_ARRAY) {
        c_type(gen, type->element_type_id);
        c_write(gen, " ");
        c_variable_name(gen, symbol);
        c_write(gen, "[%lld]", type->array_size);
    } else {
        c_type(gen, symbol->type_id);
        c_write(gen, " ");
        c_variable_name(gen, symbol);
    }
    if (initial) c_write(gen, " = {0}");
    c_write(gen, ";\n");
}

/* Uma struct so e definida depois de todas as structs que ela contem por valor. */
static void c_records(CCodegen *gen)
{
    size_t i, j, remaining = 0U;
    unsigned char *done = (unsigned char *)calloc(gen->semantic->types.count, 1U);
    if (done == NULL) {
        c_fail(gen, gen->function, "memoria insuficiente para ordenar registros");
        return;
    }
    for (i = 0U; i < gen->semantic->types.count; ++i) {
        const Type *type = &gen->semantic->types.items[i];
        if (type->kind == TYPE_RECORD) {
            c_write(gen, "typedef struct mac_r_%zu mac_r_%zu;\n", type->id, type->id);
            ++remaining;
        }
    }
    c_write(gen, "\n");
    while (remaining != 0U && gen->status == FRONTEND_STATUS_OK) {
        int progressed = 0;
        for (i = 0U; i < gen->semantic->types.count; ++i) {
            const Type *type = &gen->semantic->types.items[i];
            int ready = 1;
            size_t fields = 0U;
            if (type->kind != TYPE_RECORD || done[i]) continue;
            for (j = 0U; j < gen->semantic->symbols.count; ++j) {
                const Symbol *field = &gen->semantic->symbols.items[j];
                const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, field->scope_id);
                const Type *field_type = c_type_info(gen, field->type_id);
                if (field->category != SYMBOL_FIELD || scope == NULL ||
                    scope->owner_symbol_id != type->declaration_symbol_id) continue;
                ++fields;
                if (field_type != NULL && field_type->kind == TYPE_RECORD &&
                    !done[field_type->id]) ready = 0;
            }
            if (!ready) continue;
            c_write(gen, "struct mac_r_%zu {\n", type->id);
            for (j = 0U; j < gen->semantic->symbols.count; ++j) {
                const Symbol *field = &gen->semantic->symbols.items[j];
                const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, field->scope_id);
                if (field->category != SYMBOL_FIELD || scope == NULL ||
                    scope->owner_symbol_id != type->declaration_symbol_id) continue;
                c_write(gen, "  ");
                c_type(gen, field->type_id);
                c_write(gen, " f_%zu;\n", field->field_index);
            }
            if (fields == 0U) c_write(gen, "  unsigned char mac_empty;\n");
            c_write(gen, "};\n\n");
            done[i] = 1U;
            --remaining;
            progressed = 1;
        }
        if (!progressed) c_fail(gen, gen->function, "ciclo de registros por valor");
    }
    free(done);
}

static CValue c_expression(CCodegen *gen, const AstNode *node);
static CValue c_address(CCodegen *gen, const AstNode *node);
static void c_statement(CCodegen *gen, const AstNode *node);

static CValue c_convert(CCodegen *gen, CValue value, size_t target,
                        const AstNode *node)
{
    CValue result;
    if (value.type_id == target || gen->status != FRONTEND_STATUS_OK) return value;
    if (!((value.type_id == TYPE_ID_INTEGER &&
           (target == TYPE_ID_FLOAT || target == TYPE_ID_DOUBLE)) ||
          (value.type_id == TYPE_ID_FLOAT && target == TYPE_ID_DOUBLE))) {
        c_fail(gen, node, "conversao nao permitida pela semantica");
        return value;
    }
    result = c_new_value(gen, target);
    c_declare_value(gen, result);
    c_write(gen, " = (");
    c_type(gen, target);
    c_write(gen, ")%s;\n", value.name);
    return result;
}

static CValue c_address(CCodegen *gen, const AstNode *node)
{
    CValue result = c_new_address(gen, node->semantic.type_id);
    const Symbol *symbol;
    const Type *type;
    if (gen->status != FRONTEND_STATUS_OK) return result;
    switch (node->kind) {
    case AST_IDENTIFIER_EXPR:
        symbol = c_symbol(gen, node->semantic.symbol_id);
        if (symbol == NULL) {
            c_fail(gen, node, "identificador sem simbolo para endereco");
            break;
        }
        c_declare_pointer(gen, result);
        c_write(gen, " = &");
        c_variable_name(gen, symbol);
        c_write(gen, ";\n");
        break;
    case AST_INDEX_EXPR: {
        const AstNode *base = node->as.index_expr.base;
        CValue index;
        type = c_type_info(gen, base->semantic.type_id);
        if (base->kind != AST_IDENTIFIER_EXPR || type == NULL || type->kind != TYPE_ARRAY) {
            c_fail(gen, node, "base de vetor nao suportada na geracao C");
            break;
        }
        symbol = c_symbol(gen, base->semantic.symbol_id);
        if (symbol == NULL) {
            c_fail(gen, node, "vetor sem simbolo resolvido");
            break;
        }
        index = c_expression(gen, node->as.index_expr.index);
        c_write(gen, "  if (%s < 0 || %s >= %lld) zl_erro_execucao(3, %zu);\n",
                index.name, index.name, type->array_size, node->span.start.line);
        c_declare_pointer(gen, result);
        c_write(gen, " = &");
        c_variable_name(gen, symbol);
        c_write(gen, "[%s];\n", index.name);
        break;
    }
    case AST_FIELD_EXPR: {
        CValue base = c_address(gen, node->as.field_expr.base);
        symbol = c_symbol(gen, node->semantic.symbol_id);
        if (symbol == NULL) {
            c_fail(gen, node, "campo sem simbolo resolvido");
            break;
        }
        c_declare_pointer(gen, result);
        c_write(gen, " = &%s->f_%zu;\n", base.name, symbol->field_index);
        break;
    }
    default: c_fail(gen, node, "destino de atribuicao C nao suportado"); break;
    }
    return result;
}

static void c_literal(CCodegen *gen, const AstNode *node, CValue result)
{
    const unsigned char *text;
    size_t i;
    c_declare_value(gen, result);
    switch (node->as.literal_expr.literal_kind) {
    case AST_LITERAL_INTEGER:
        /* O lexer limita a magnitude a INT64_MAX e o sinal vem do menos
         * unario; um literal negativo aqui seria erro interno. */
        if (node->as.literal_expr.value.integer < 0) {
            c_fail(gen, node, "literal inteira negativa na AST");
            return;
        }
        /* INT64_C da o tipo de 64 bits mesmo onde long tem 32 (Xtensa). */
        c_write(gen, " = INT64_C(%lld);\n", node->as.literal_expr.value.integer);
        break;
    case AST_LITERAL_REAL:
        c_write(gen, " = (");
        c_type(gen, result.type_id);
        c_write(gen, ")%a;\n", node->as.literal_expr.value.real);
        break;
    case AST_LITERAL_BOOLEAN:
        c_write(gen, " = %d;\n", node->as.literal_expr.value.boolean ? 1 : 0);
        break;
    case AST_LITERAL_STRING:
        c_write(gen, " = \"");
        text = (const unsigned char *)node->as.literal_expr.value.string;
        for (i = 0U; text[i] != 0U; ++i) c_write(gen, "\\%03o", (unsigned int)text[i]);
        c_write(gen, "\";\n");
        break;
    }
}

static const char *c_builtin(BuiltinKind kind)
{
    switch (kind) {
    case BUILTIN_PRINT_INTEGER: return "zl_imprime_int";
    case BUILTIN_PRINT_FLOAT: return "zl_imprime_flut";
    case BUILTIN_PRINT_DOUBLE: return "zl_imprime_duplocarpado";
    case BUILTIN_PRINT_WORD: return "zl_imprime_texto";
    case BUILTIN_PRINT_BOOL: return "zl_imprime_bool";
    case BUILTIN_PRINT_NEWLINE: return "zl_imprime_nl";
    case BUILTIN_READ_INTEGER: return "zl_le_int";
    case BUILTIN_READ_FLOAT: return "zl_le_flut";
    case BUILTIN_READ_DOUBLE: return "zl_le_duplocarpado";
    case BUILTIN_PIN_MODE: return "zl_pino_modo";
    case BUILTIN_PIN_WRITE: return "zl_pino_escreve";
    case BUILTIN_PIN_READ: return "zl_pino_le";
    case BUILTIN_DELAY_MS: return "zl_espera_ms";
    default: return NULL;
    }
}

static int c_builtin_line(BuiltinKind kind)
{
    return kind == BUILTIN_READ_INTEGER || kind == BUILTIN_READ_FLOAT ||
           kind == BUILTIN_READ_DOUBLE || kind == BUILTIN_PIN_MODE ||
           kind == BUILTIN_PIN_WRITE || kind == BUILTIN_PIN_READ ||
           kind == BUILTIN_DELAY_MS;
}

/* Servicos da placa recebem int32_t (zl_runtime.h); a inteira e int64_t. */
static int c_builtin_board(BuiltinKind kind)
{
    return kind == BUILTIN_PIN_MODE || kind == BUILTIN_PIN_WRITE ||
           kind == BUILTIN_PIN_READ || kind == BUILTIN_DELAY_MS;
}

static CValue c_call(CCodegen *gen, const AstNode *node)
{
    const Symbol *function = c_symbol(gen, node->semantic.symbol_id);
    CValue result = c_new_value(gen, node->semantic.type_id);
    CValue *arguments = NULL;
    const char *builtin;
    size_t i, count = node->as.call_expr.arguments.count;
    if (function == NULL || function->parameter_count != count) {
        c_fail(gen, node, "assinatura da chamada ausente");
        return result;
    }
    if (count != 0U) {
        arguments = (CValue *)calloc(count, sizeof(*arguments));
        if (arguments == NULL) {
            c_fail(gen, node, "memoria insuficiente para argumentos");
            return result;
        }
    }
    for (i = 0U; i < count; ++i) {
        arguments[i] = c_expression(gen, node->as.call_expr.arguments.items[i]);
        arguments[i] = c_convert(gen, arguments[i], function->parameter_type_ids[i], node);
    }
    if (result.type_id != TYPE_ID_VOID) {
        c_declare_value(gen, result);
        c_write(gen, " = ");
    } else c_write(gen, "  ");
    builtin = c_builtin(function->builtin_kind);
    if (builtin != NULL) c_write(gen, "%s", builtin);
    else c_function_name(gen, function);
    c_write(gen, "(");
    for (i = 0U; i < count; ++i) {
        if (i != 0U) c_write(gen, ", ");
        /* Pino, modo e milissegundos: satura em vez de truncar, para que um
         * valor fora de int32_t chegue ao runtime como invalido, nao como
         * outro pino valido (2^32 + 2 nao pode virar o GPIO2). */
        if (c_builtin_board(function->builtin_kind) &&
            arguments[i].type_id == TYPE_ID_INTEGER)
            c_write(gen, "mac_arg_i32(%s)", arguments[i].name);
        else c_write(gen, "%s", arguments[i].name);
    }
    if (c_builtin_line(function->builtin_kind))
        c_write(gen, "%s%zu", count != 0U ? ", " : "", node->span.start.line);
    c_write(gen, ");\n");
    free(arguments);
    return result;
}

static CValue c_binary(CCodegen *gen, const AstNode *node)
{
    AstOperator op = node->as.binary_expr.operator_kind;
    CValue left = c_expression(gen, node->as.binary_expr.left);
    CValue right;
    CValue result = c_new_value(gen, node->semantic.type_id);
    const char *operation = NULL;
    int integer = left.type_id == TYPE_ID_INTEGER;
    if (op == AST_OP_LOGICAL_AND || op == AST_OP_LOGICAL_OR) {
        c_declare_value(gen, result);
        c_write(gen, " = %s;\n  if (%s%s) {\n", left.name,
                op == AST_OP_LOGICAL_OR ? "!" : "", left.name);
        right = c_expression(gen, node->as.binary_expr.right);
        c_write(gen, "  %s = %s;\n  }\n", result.name, right.name);
        return result;
    }
    right = c_expression(gen, node->as.binary_expr.right);
    if (op == AST_OP_DIVIDE || op == AST_OP_INTEGER_DIVIDE) {
        c_write(gen, "  if (%s == 0) zl_erro_execucao(ZL_ERRO_DIVISAO_ZERO, %zu);\n",
                right.name, node->span.start.line);
        /* INT64_MIN // -1 nao cabe em int64_t: em C seria comportamento
         * indefinido, na IR seria sdiv indefinido. Os dois geram o erro 2. */
        if (integer)
            c_write(gen, "  if (%s == INT64_MIN && %s == -1) "
                    "zl_erro_execucao(ZL_ERRO_DIVISAO_OVERFLOW, %zu);\n",
                    left.name, right.name, node->span.start.line);
    }
    c_declare_value(gen, result);
    c_write(gen, " = ");
    if (left.type_id == TYPE_ID_WORD) {
        c_write(gen, "%szl_texto_igual(%s, %s);\n",
                op == AST_OP_DIFFERENT ? "!" : "", left.name, right.name);
        return result;
    }
    if (op == AST_OP_POWER) {
        c_write(gen, "%s(%s, %s, %zu);\n",
                integer ? "zl_potencia_int" : left.type_id == TYPE_ID_FLOAT ?
                "zl_potencia_flut" : "zl_potencia_duplocarpado",
                left.name, right.name, node->span.start.line);
        return result;
    }
    switch (op) {
    case AST_OP_ADD: operation = "+"; break;
    case AST_OP_SUBTRACT: operation = "-"; break;
    case AST_OP_MULTIPLY: operation = "*"; break;
    case AST_OP_DIVIDE:
    case AST_OP_INTEGER_DIVIDE: operation = "/"; break;
    case AST_OP_LESS: operation = "<"; break;
    case AST_OP_GREATER: operation = ">"; break;
    case AST_OP_EQUAL: operation = "=="; break;
    case AST_OP_DIFFERENT: operation = "!="; break;
    case AST_OP_LOGICAL_XOR: operation = "!="; break;
    default: c_fail(gen, node, "operador binario C nao suportado"); return result;
    }
    /* Overflow assinado e indefinido em C: a conta e feita em uint64_t
     * (modulo 2^64) e volta a int64_t, como add/sub/mul i64 da IR. */
    if (integer && (op == AST_OP_ADD || op == AST_OP_SUBTRACT || op == AST_OP_MULTIPLY))
        c_write(gen, "mac_i64((uint64_t)%s %s (uint64_t)%s);\n",
                left.name, operation, right.name);
    else c_write(gen, "%s %s %s;\n", left.name, operation, right.name);
    return result;
}

static CValue c_unary(CCodegen *gen, const AstNode *node)
{
    AstOperator op = node->as.unary_expr.operator_kind;
    const AstNode *operand = node->as.unary_expr.operand;
    CValue result = c_new_value(gen, node->semantic.type_id);
    CValue value;
    value = c_expression(gen, operand);
    if (op == AST_OP_UNARY_PLUS) return value;
    c_declare_value(gen, result);
    c_write(gen, " = ");
    switch (op) {
    case AST_OP_LOGICAL_NOT: c_write(gen, "!%s;\n", value.name); break;
    case AST_OP_UNARY_MINUS:
        /* -INT64_MIN volta a INT64_MIN, como "sub i64 0, x" na IR. */
        if (value.type_id == TYPE_ID_INTEGER)
            c_write(gen, "mac_i64(UINT64_C(0) - (uint64_t)%s);\n", value.name);
        else c_write(gen, "-%s;\n", value.name);
        break;
    case AST_OP_ROOT:
        c_write(gen, "%s(%s, %zu);\n", value.type_id == TYPE_ID_FLOAT ?
                "zl_raiz_flut" : "zl_raiz_duplocarpado", value.name,
                node->span.start.line);
        break;
    default: c_fail(gen, node, "operador unario C nao suportado"); break;
    }
    return result;
}

static CValue c_expression(CCodegen *gen, const AstNode *node)
{
    CValue result = c_new_value(gen, node->semantic.type_id);
    if (gen->status != FRONTEND_STATUS_OK) return result;
    switch (node->kind) {
    case AST_LITERAL_EXPR: c_literal(gen, node, result); break;
    case AST_IDENTIFIER_EXPR:
    case AST_INDEX_EXPR: {
        CValue address = c_address(gen, node);
        c_declare_value(gen, result);
        c_write(gen, " = *%s;\n", address.name);
        break;
    }
    case AST_FIELD_EXPR:
        if ((node->semantic.flags & AST_SEMANTIC_LVALUE) != 0U) {
            CValue address = c_address(gen, node);
            c_declare_value(gen, result);
            c_write(gen, " = *%s;\n", address.name);
        } else {
            CValue base = c_expression(gen, node->as.field_expr.base);
            const Symbol *field = c_symbol(gen, node->semantic.symbol_id);
            if (field == NULL) c_fail(gen, node, "campo sem simbolo C");
            else {
                c_declare_value(gen, result);
                c_write(gen, " = %s.f_%zu;\n", base.name, field->field_index);
            }
        }
        break;
    case AST_CALL_EXPR: return c_call(gen, node);
    case AST_BINARY_EXPR: return c_binary(gen, node);
    case AST_UNARY_EXPR: return c_unary(gen, node);
    default: c_fail(gen, node, "expressao C nao suportada"); break;
    }
    return result;
}

/* Os temporarios de cada condicao precisam executar somente no seu ramo. */
static void c_if(CCodegen *gen, const AstNode *node, size_t index)
{
    if (index < node->as.if_stmt.branches.count) {
        const AstIfBranch *branch = &node->as.if_stmt.branches.items[index];
        CValue condition = c_expression(gen, branch->condition);
        c_write(gen, "  if (%s) {\n", condition.name);
        c_statement(gen, branch->body);
        c_write(gen, "  } else {\n");
        c_if(gen, node, index + 1U);
        c_write(gen, "  }\n");
    } else if (node->as.if_stmt.else_body != NULL)
        c_statement(gen, node->as.if_stmt.else_body);
}

static void c_statement(CCodegen *gen, const AstNode *node)
{
    size_t i;
    if (node == NULL || gen->status != FRONTEND_STATUS_OK) return;
    c_write(gen, "  /* linha %zu */\n", node->span.start.line);
    switch (node->kind) {
    case AST_BLOCK:
        for (i = 0U; i < node->as.block.statements.count; ++i)
            c_statement(gen, node->as.block.statements.items[i]);
        break;
    case AST_VAR_DECL:
    case AST_VECTOR_DECL: break; /* Inicializadas uma vez na entrada da funcao. */
    case AST_ASSIGNMENT: {
        CValue address = c_address(gen, node->as.assignment.target);
        CValue value = c_expression(gen, node->as.assignment.value);
        value = c_convert(gen, value, address.type_id, node);
        c_write(gen, "  *%s = %s;\n", address.name, value.name);
        break;
    }
    case AST_IF_STMT: c_if(gen, node, 0U); break;
    case AST_WHILE_STMT: {
        CValue condition;
        c_write(gen, "  while (1) {\n");
        condition = c_expression(gen, node->as.while_stmt.condition);
        c_write(gen, "  if (!%s) break;\n", condition.name);
        c_statement(gen, node->as.while_stmt.body);
        c_write(gen, "  }\n");
        break;
    }
    case AST_FOR_STMT: {
        CValue condition;
        c_statement(gen, node->as.for_stmt.initializer);
        c_write(gen, "  while (1) {\n");
        condition = c_expression(gen, node->as.for_stmt.condition);
        c_write(gen, "  if (!%s) break;\n", condition.name);
        c_statement(gen, node->as.for_stmt.body);
        c_statement(gen, node->as.for_stmt.iteration);
        c_write(gen, "  }\n");
        break;
    }
    case AST_EXPR_STMT: {
        CValue value = c_expression(gen, node->as.expr_stmt.expression);
        if (value.type_id != TYPE_ID_VOID) c_write(gen, "  (void)%s;\n", value.name);
        break;
    }
    case AST_RETURN_STMT:
        if (node->as.return_stmt.value != NULL) {
            CValue value = c_expression(gen, node->as.return_stmt.value);
            value = c_convert(gen, value,
                gen->function->as.function_decl.return_type.resolved_type_id, node);
            /* zl_principal mantem a ABI int32_t com o runtime: o status
             * recebe os 32 bits baixos da inteira, como o trunc da IR. */
            if (gen->function->as.function_decl.is_principal)
                c_write(gen, "  return mac_status(%s);\n", value.name);
            else c_write(gen, "  return %s;\n", value.name);
        } else c_write(gen, gen->function->as.function_decl.is_principal ?
                       "  return 0;\n" : "  return;\n");
        break;
    default: c_fail(gen, node, "comando C nao suportado"); break;
    }
}

static void c_function_signature(CCodegen *gen, const AstNode *node)
{
    const Symbol *symbol = c_symbol(gen, node->semantic.symbol_id);
    size_t i;
    if (symbol == NULL) {
        c_fail(gen, node, "funcao C sem simbolo");
        return;
    }
    /* principal vira int32_t zl_principal(void), a assinatura de zl_runtime.h. */
    if (symbol->is_principal) c_write(gen, "int32_t");
    else c_type(gen, symbol->type_id);
    c_write(gen, " ");
    c_function_name(gen, symbol);
    c_write(gen, "(");
    if (node->as.function_decl.parameters.count == 0U) c_write(gen, "void");
    for (i = 0U; i < node->as.function_decl.parameters.count; ++i) {
        const AstNode *parameter = node->as.function_decl.parameters.items[i];
        const Symbol *argument = c_symbol(gen, parameter->as.var_decl.names.items[0].symbol_id);
        if (i != 0U) c_write(gen, ", ");
        if (argument == NULL) {
            c_fail(gen, node, "parametro C sem simbolo");
            return;
        }
        c_type(gen, argument->type_id);
        c_write(gen, " ");
        c_variable_name(gen, argument);
    }
    c_write(gen, ")");
}

static void c_function(CCodegen *gen, const AstNode *node)
{
    const Symbol *function = c_symbol(gen, node->semantic.symbol_id);
    size_t i;
    gen->function = node;
    gen->temporary = 0U;
    if (function == NULL) {
        c_fail(gen, node, "funcao C sem simbolo");
        return;
    }
    c_function_signature(gen, node);
    c_write(gen, "\n{\n");
    for (i = 0U; i < gen->semantic->symbols.count; ++i) {
        const Symbol *local = &gen->semantic->symbols.items[i];
        const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, local->scope_id);
        if (scope == NULL || scope->kind != SCOPE_FUNCTION ||
            scope->owner_symbol_id != function->id) continue;
        if (local->category == SYMBOL_LOCAL_VARIABLE || local->category == SYMBOL_VECTOR) {
            c_write(gen, "  ");
            c_declare_symbol(gen, local, 1);
        }
        if (local->category == SYMBOL_LOCAL_VARIABLE || local->category == SYMBOL_VECTOR ||
            local->category == SYMBOL_PARAMETER) {
            c_write(gen, "  (void)");
            c_variable_name(gen, local);
            c_write(gen, ";\n");
        }
    }
    c_statement(gen, node->as.function_decl.body);
    /* Semantica ja verifica os retornos. O fallback tambem cobre principal vazio. */
    if (function->is_principal) c_write(gen, "  return 0;\n");
    else if (function->type_id == TYPE_ID_VOID) c_write(gen, "  return;\n");
    else {
        c_write(gen, "  return (");
        c_type(gen, function->type_id);
        c_write(gen, "){0};\n");
    }
    c_write(gen, "}\n\n");
}

FrontendStatus codegen_c_emit(const SourceFile *source, const AstNode *program,
                              const SemanticResult *semantic,
                              DiagnosticBag *diagnostics, FILE *output)
{
    CCodegen gen;
    size_t i;
    if (source == NULL || program == NULL || program->kind != AST_PROGRAM ||
        semantic == NULL || diagnostics == NULL || output == NULL ||
        !semantic->analyzed || semantic->semantic_error_count != 0U)
        return FRONTEND_STATUS_INTERNAL_ERROR;
    (void)memset(&gen, 0, sizeof(gen));
    gen.source = source;
    gen.function = program;
    gen.semantic = semantic;
    gen.diagnostics = diagnostics;
    gen.output = output;
    gen.status = FRONTEND_STATUS_OK;
    c_write(&gen, "/* Macaronica: C99 gerado automaticamente da AST anotada. */\n"
                  "#include <stdint.h>\n#include <limits.h>\n#include \"zl_runtime.h\"\n\n"
                  "/* zl_erro_execucao nao retorna (como o noreturn da IR). Sem isso,\n"
                  " * com otimizacao o gcc trata o acesso logo depois da checagem de\n"
                  " * indice como alcancavel e pode usar o comportamento indefinido\n"
                  " * dele para tirar a checagem do laco (-Waggressive-loop-optimizations). */\n"
                  "#if defined(__GNUC__)\n"
                  "void zl_erro_execucao(int32_t codigo, int32_t linha) __attribute__((noreturn));\n"
                  "#endif\n\n"
                  "/* inteira = int64_t. A conta inteira e feita em uint64_t (modulo\n"
                  " * 2^64) e volta a int64_t sem conversao fora da faixa, que em C\n"
                  " * dependeria do compilador. */\n"
                  "static inline int64_t mac_i64(uint64_t value)\n{\n"
                  "  return value <= (uint64_t)INT64_MAX ? (int64_t)value :\n"
                  "      -1 - (int64_t)(UINT64_MAX - value);\n}\n\n"
                  "/* Status de zl_principal: 32 bits baixos, como trunc i64 to i32. */\n"
                  "static inline int32_t mac_status(int64_t value)\n{\n"
                  "  uint32_t low = (uint32_t)((uint64_t)value & UINT32_MAX);\n"
                  "  return low <= (uint32_t)INT32_MAX ? (int32_t)low :\n"
                  "      -1 - (int32_t)(UINT32_MAX - low);\n}\n\n"
                  "/* Argumento int32_t dos servicos da placa: satura fora da faixa. */\n"
                  "static inline int32_t mac_arg_i32(int64_t value)\n{\n"
                  "  return value > INT32_MAX ? INT32_MAX :\n"
                  "      value < INT32_MIN ? INT32_MIN : (int32_t)value;\n}\n\n");
    c_records(&gen);
    for (i = 0U; i < semantic->symbols.count; ++i) {
        const Symbol *symbol = &semantic->symbols.items[i];
        if (symbol->scope_id == semantic->global_scope_id &&
            (symbol->category == SYMBOL_GLOBAL_VARIABLE || symbol->category == SYMBOL_VECTOR)) {
            c_declare_symbol(&gen, symbol, 1);
        }
    }
    for (i = 0U; i < program->as.program.declarations.count; ++i) {
        const AstNode *node = program->as.program.declarations.items[i];
        if (node->kind == AST_FUNCTION_DECL) {
            c_function_signature(&gen, node);
            c_write(&gen, ";\n");
        }
    }
    c_write(&gen, "\n");
    for (i = 0U; i < program->as.program.declarations.count; ++i) {
        const AstNode *node = program->as.program.declarations.items[i];
        if (node->kind == AST_FUNCTION_DECL) c_function(&gen, node);
    }
    if (ferror(output) && gen.status == FRONTEND_STATUS_OK)
        c_fail(&gen, program, "falha ao escrever C gerado");
    return gen.status;
}
