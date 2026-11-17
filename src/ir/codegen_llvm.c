#include "codegen_llvm.h"
#include "codegen_mlir.h" /* Frente dsl-mlir: nucleos da DSL vetorial. */

#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Um valor LLVM e seu tipo resolvido: nao e uma nova IR intermediaria. */
typedef struct {
    size_t type_id;
    char operand[64];
} LlvmValue;

typedef struct {
    const SourceFile *source;
    const SemanticResult *semantic;
    DiagnosticBag *diagnostics;
    FILE *output;
    FrontendStatus status;
    const AstNode *function;
    size_t temporary;
    size_t label;
    int terminated;
    size_t last_line;
    char current_block[64];
} Codegen;

static void fail(Codegen *gen, const AstNode *node, const char *code,
                 const char *message)
{
    if (gen->status != FRONTEND_STATUS_OK) return;
    gen->status = FRONTEND_STATUS_INTERNAL_ERROR;
    (void)diagnostic_bag_add(gen->diagnostics, gen->source, node->span,
                            DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_CODEGEN,
                            code, "%s", message);
}

static void write_ir(Codegen *gen, const char *format, ...)
{
    va_list args;
    if (gen->status != FRONTEND_STATUS_OK) return;
    va_start(args, format);
    if (vfprintf(gen->output, format, args) < 0) {
        fail(gen, gen->function, "E-IR-004", "falha ao escrever LLVM IR");
    }
    va_end(args);
}

/* Os nomes de tipos/identificadores da Macaronica sao ASCII alfanumericos. */
static void emit_type(Codegen *gen, size_t type_id)
{
    const Type *type = type_table_get(&gen->semantic->types, type_id);
    if (type == NULL) {
        fail(gen, gen->function, "E-IR-002", "tipo ausente na AST anotada");
        return;
    }
    switch (type->kind) {
    case TYPE_BOOL: write_ir(gen, "i1"); break;
    case TYPE_INTEGER: write_ir(gen, "i64"); break;
    case TYPE_FLOAT: write_ir(gen, "float"); break;
    case TYPE_DOUBLE: write_ir(gen, "double"); break;
    case TYPE_WORD: write_ir(gen, "ptr"); break;
    case TYPE_VOID: write_ir(gen, "void"); break;
    case TYPE_RECORD: write_ir(gen, "%%struct.%s", type->name); break;
    case TYPE_ARRAY:
        write_ir(gen, "[%lld x ", type->array_size);
        emit_type(gen, type->element_type_id);
        write_ir(gen, "]");
        break;
    default:
        fail(gen, gen->function, "E-IR-002", "tipo invalido para LLVM IR");
        break;
    }
}

static LlvmValue value_empty(size_t type_id)
{
    LlvmValue value;
    value.type_id = type_id;
    value.operand[0] = '\0';
    return value;
}

static LlvmValue new_temporary(Codegen *gen, size_t type_id)
{
    LlvmValue value = value_empty(type_id);
    (void)snprintf(value.operand, sizeof(value.operand), "%%t.%zu", gen->temporary++);
    return value;
}

/*
 * Esquema de nomes locais (IDENT da Macaronica e [A-Za-z][A-Za-z0-9]*, sem
 * ponto), escolhido para nunca colidir:
 *   %nome.N        temporario derivado da fonte (load, resultado de chamada);
 *                  N vem do mesmo contador de %t.N, logo e unico;
 *   %nome.addr     slot do alloca (%nome.ID.addr se o nome se repete na funcao);
 *   %nome          valor recebido pelo parametro;
 *   a.b.N / entry  rotulos de bloco: sempre duas palavras antes do numero.
 */
static LlvmValue new_named_temporary(Codegen *gen, size_t type_id, const char *base)
{
    LlvmValue value = value_empty(type_id);
    int length = snprintf(value.operand, sizeof(value.operand), "%%%s.%zu",
                          base, gen->temporary);
    if (length < 0 || (size_t)length >= sizeof(value.operand))
        (void)snprintf(value.operand, sizeof(value.operand), "%%t.%zu", gen->temporary);
    gen->temporary++;
    return value;
}

static void emit_typed_value(Codegen *gen, LlvmValue value)
{
    emit_type(gen, value.type_id);
    write_ir(gen, " %s", value.operand);
}

/* Dois locais da mesma funcao com o mesmo nome (escopos irmaos) dividem o
 * entry; so o primeiro fica com %nome.addr. */
static int local_name_is_shared(const Codegen *gen, const Symbol *symbol)
{
    const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, symbol->scope_id);
    size_t index;
    if (scope == NULL) return 1;
    for (index = 0U; index < gen->semantic->symbols.count; ++index) {
        const Symbol *other = &gen->semantic->symbols.items[index];
        const Scope *other_scope;
        if (other->id >= symbol->id || strcmp(other->name, symbol->name) != 0) continue;
        other_scope = symbol_table_get_scope(&gen->semantic->symbols, other->scope_id);
        if (other_scope != NULL && other_scope->kind == SCOPE_FUNCTION &&
            other_scope->owner_symbol_id == scope->owner_symbol_id) return 1;
    }
    return 0;
}

/* Globais sao internal: @nome nao conflita com simbolos do runtime ou da
 * placa (setup, loop...) na ligacao. */
static void symbol_address(Codegen *gen, const Symbol *symbol, char *buffer,
                           size_t capacity)
{
    int global = symbol->scope_id == gen->semantic->global_scope_id;
    int length;
    if (global) length = snprintf(buffer, capacity, "@%s", symbol->name);
    else if (local_name_is_shared(gen, symbol))
        length = snprintf(buffer, capacity, "%%%s.%zu.addr", symbol->name, symbol->id);
    else length = snprintf(buffer, capacity, "%%%s.addr", symbol->name);
    /* Nome longo demais para o operando: volta ao nome numerico, sempre unico. */
    if (length < 0 || (size_t)length >= capacity)
        (void)snprintf(buffer, capacity, global ? "@mac.global.%zu" : "%%v.%zu.addr",
                       symbol->id);
}

static void parameter_value_name(const Symbol *symbol, size_t index, char *buffer,
                                 size_t capacity)
{
    int length = -1;
    /* "entry" e o unico nome simples que colide com um rotulo. */
    if (symbol != NULL && strcmp(symbol->name, "entry") == 0)
        length = snprintf(buffer, capacity, "%%%s.arg", symbol->name);
    else if (symbol != NULL)
        length = snprintf(buffer, capacity, "%%%s", symbol->name);
    if (length < 0 || (size_t)length >= capacity)
        (void)snprintf(buffer, capacity, "%%param.arg.%zu", index);
}

static void emit_function_name(Codegen *gen, const Symbol *function)
{
    if (function->is_principal) write_ir(gen, "@zl_principal");
    else write_ir(gen, "@mac.%s", function->name);
}

static void emit_label(Codegen *gen, const char *prefix, size_t number)
{
    (void)snprintf(gen->current_block, sizeof(gen->current_block), "%s.%zu", prefix, number);
    write_ir(gen, "%s:\n", gen->current_block);
    gen->terminated = 0;
}

/* Guardas abrem dois blocos: guarda.erro termina; so guarda.ok chega a operacao. */
static void emit_guard(Codegen *gen, LlvmValue bad, int code, size_t line)
{
    size_t label = gen->label++;
    write_ir(gen, "  br i1 %s, label %%guarda.erro.%zu, label %%guarda.ok.%zu\n",
             bad.operand, label, label);
    emit_label(gen, "guarda.erro", label);
    write_ir(gen, "  call void @zl_erro_execucao(i32 %d, i32 %zu)\n  unreachable\n", code, line);
    emit_label(gen, "guarda.ok", label);
}

static LlvmValue emit_expression(Codegen *gen, const AstNode *node);
static void emit_statement(Codegen *gen, const AstNode *node);

/* O destino e calculado antes do RHS. Este helper nunca carrega o valor. */
static LlvmValue emit_address(Codegen *gen, const AstNode *node)
{
    LlvmValue address = value_empty(node->semantic.type_id);
    const Symbol *symbol;
    const Type *type;
    if (gen->status != FRONTEND_STATUS_OK) return address;
    switch (node->kind) {
    case AST_IDENTIFIER_EXPR:
        symbol = symbol_table_get(&gen->semantic->symbols, node->semantic.symbol_id);
        if (symbol == NULL) {
            fail(gen, node, "E-IR-002", "identificador sem simbolo resolvido");
            return address;
        }
        symbol_address(gen, symbol, address.operand, sizeof(address.operand));
        return address;
    case AST_INDEX_EXPR: {
        LlvmValue base = emit_address(gen, node->as.index_expr.base);
        LlvmValue index = emit_expression(gen, node->as.index_expr.index);
        LlvmValue low = new_temporary(gen, TYPE_ID_BOOL);
        LlvmValue high = new_temporary(gen, TYPE_ID_BOOL);
        LlvmValue bad = new_temporary(gen, TYPE_ID_BOOL);
        type = type_table_get(&gen->semantic->types,
                              node->as.index_expr.base->semantic.type_id);
        if (type == NULL || type->kind != TYPE_ARRAY || index.type_id != TYPE_ID_INTEGER) {
            fail(gen, node, "E-IR-002", "indice sem tipo de vetor valido");
            return address;
        }
        write_ir(gen, "  %s = icmp slt i64 %s, 0\n", low.operand, index.operand);
        write_ir(gen, "  %s = icmp sge i64 %s, %lld\n", high.operand,
                 index.operand, type->array_size);
        write_ir(gen, "  %s = or i1 %s, %s\n", bad.operand, low.operand, high.operand);
        emit_guard(gen, bad, 3, node->span.start.line);
        {
            /* Depois da guarda, 0 <= indice < tamanho <= INT32_MAX: o trunc e
             * exato e a GEP continua com indices i32 em qualquer alvo. */
            LlvmValue narrow = new_temporary(gen, TYPE_ID_INTEGER);
            write_ir(gen, "  %s = trunc i64 %s to i32\n", narrow.operand, index.operand);
            address = new_temporary(gen, node->semantic.type_id);
            write_ir(gen, "  %s = getelementptr ", address.operand);
            emit_type(gen, type->id);
            write_ir(gen, ", ptr %s, i32 0, i32 %s\n", base.operand, narrow.operand);
        }
        return address;
    }
    case AST_FIELD_EXPR:
        type = type_table_get(&gen->semantic->types,
                              node->as.field_expr.base->semantic.type_id);
        symbol = symbol_table_get(&gen->semantic->symbols, node->semantic.symbol_id);
        if (type == NULL || type->kind != TYPE_RECORD || symbol == NULL ||
            (node->as.field_expr.base->semantic.flags & AST_SEMANTIC_LVALUE) == 0U) {
            fail(gen, node, "E-IR-002", "campo sem base gravavel ou simbolo valido");
            return address;
        }
        {
            LlvmValue base = emit_address(gen, node->as.field_expr.base);
            address = new_temporary(gen, node->semantic.type_id);
            write_ir(gen, "  %s = getelementptr ", address.operand);
            emit_type(gen, type->id);
            write_ir(gen, ", ptr %s, i32 0, i32 %zu\n", base.operand,
                     symbol->field_index);
        }
        return address;
    default:
        fail(gen, node, "E-IR-001", "destino de atribuicao nao suportado");
        return address;
    }
}

static LlvmValue convert_value(Codegen *gen, LlvmValue value, size_t target,
                               const AstNode *node)
{
    LlvmValue converted;
    const char *operation;
    if (value.type_id == target || gen->status != FRONTEND_STATUS_OK) return value;
    if (value.type_id == TYPE_ID_INTEGER &&
        (target == TYPE_ID_FLOAT || target == TYPE_ID_DOUBLE)) operation = "sitofp";
    else if (value.type_id == TYPE_ID_FLOAT && target == TYPE_ID_DOUBLE) operation = "fpext";
    else {
        fail(gen, node, "E-IR-002", "conversao nao autorizada pela semantica");
        return value_empty(target);
    }
    converted = new_temporary(gen, target);
    write_ir(gen, "  %s = %s ", converted.operand, operation);
    emit_typed_value(gen, value);
    write_ir(gen, " to ");
    emit_type(gen, target);
    write_ir(gen, "\n");
    return converted;
}

static const char *builtin_name(BuiltinKind kind)
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
    default: return NULL;
    }
}

static LlvmValue emit_call(Codegen *gen, const AstNode *node)
{
    const Symbol *function = symbol_table_get(&gen->semantic->symbols, node->semantic.symbol_id);
    const AstNodeList *arguments = &node->as.call_expr.arguments;
    LlvmValue *values = NULL;
    LlvmValue result = value_empty(node->semantic.type_id);
    size_t index;
    const char *builtin;
    if (function == NULL || function->parameter_count != arguments->count) {
        fail(gen, node, "E-IR-002", "assinatura ausente ou incoerente na chamada");
        return result;
    }
    if (arguments->count != 0U) {
        if (arguments->count > (size_t)-1 / sizeof(*values) ||
            (values = (LlvmValue *)calloc(arguments->count, sizeof(*values))) == NULL) {
            fail(gen, node, "E-IR-003", "memoria insuficiente para argumentos");
            return result;
        }
    }
    /* Avaliar antes de imprimir a instrucao call garante esquerda -> direita. */
    for (index = 0U; index < arguments->count; ++index) {
        values[index] = emit_expression(gen, arguments->items[index]);
        values[index] = convert_value(gen, values[index], function->parameter_type_ids[index], node);
    }
    if (function->builtin_kind == BUILTIN_PRINT_BOOL) {
        /* bool vai como i32 0/1: a ABI do runtime nao depende de zeroext e
         * nao acompanha a largura de inteira (i64). */
        LlvmValue widened = new_temporary(gen, TYPE_ID_INTEGER);
        write_ir(gen, "  %s = zext i1 %s to i32\n", widened.operand, values[0].operand);
        write_ir(gen, "  call void @zl_imprime_bool(i32 %s)\n", widened.operand);
        free(values);
        return result;
    }
    if (result.type_id == TYPE_ID_VOID) write_ir(gen, "  call void ");
    else {
        result = new_named_temporary(gen, result.type_id, function->name);
        write_ir(gen, "  %s = call ", result.operand);
        emit_type(gen, result.type_id);
        write_ir(gen, " ");
    }
    builtin = builtin_name(function->builtin_kind);
    if (builtin != NULL) write_ir(gen, "@%s", builtin);
    else emit_function_name(gen, function);
    write_ir(gen, "(");
    for (index = 0U; index < arguments->count; ++index) {
        if (index != 0U) write_ir(gen, ", ");
        emit_typed_value(gen, values[index]);
    }
    if (function->builtin_kind == BUILTIN_READ_INTEGER ||
        function->builtin_kind == BUILTIN_READ_FLOAT ||
        function->builtin_kind == BUILTIN_READ_DOUBLE) {
        write_ir(gen, "i32 %zu", node->span.start.line);
    }
    write_ir(gen, ")\n");
    free(values);
    return result;
}

static LlvmValue emit_binary(Codegen *gen, const AstNode *node)
{
    AstOperator op = node->as.binary_expr.operator_kind;
    LlvmValue left = emit_expression(gen, node->as.binary_expr.left);
    LlvmValue right;
    LlvmValue result;
    if (op == AST_OP_LOGICAL_AND || op == AST_OP_LOGICAL_OR) {
        size_t label = gen->label++;
        char left_block[64], right_block[64];
        (void)snprintf(left_block, sizeof(left_block), "%s", gen->current_block);
        if (op == AST_OP_LOGICAL_AND)
            write_ir(gen, "  br i1 %s, label %%logic.rhs.%zu, label %%logic.end.%zu\n",
                     left.operand, label, label);
        else
            write_ir(gen, "  br i1 %s, label %%logic.end.%zu, label %%logic.rhs.%zu\n",
                     left.operand, label, label);
        emit_label(gen, "logic.rhs", label);
        right = emit_expression(gen, node->as.binary_expr.right);
        (void)snprintf(right_block, sizeof(right_block), "%s", gen->current_block);
        write_ir(gen, "  br label %%logic.end.%zu\n", label);
        emit_label(gen, "logic.end", label);
        result = new_temporary(gen, TYPE_ID_BOOL);
        write_ir(gen, "  %s = phi i1 [ %s, %%%s ], [ %s, %%%s ]\n",
                 result.operand, op == AST_OP_LOGICAL_AND ? "false" : "true",
                 left_block, right.operand, right_block);
        return result;
    }
    right = emit_expression(gen, node->as.binary_expr.right);
    if (node->semantic.conversion_type_id != AST_ID_NONE) {
        /* Expressao mista: so as conversoes anotadas pela semantica (D-012). */
        left = convert_value(gen, left, node->semantic.conversion_type_id, node);
        right = convert_value(gen, right, node->semantic.conversion_type_id, node);
    }
    result = new_temporary(gen, node->semantic.type_id);
    int real = left.type_id == TYPE_ID_FLOAT || left.type_id == TYPE_ID_DOUBLE;
    const char *instruction = NULL;
    if (left.type_id == TYPE_ID_WORD) {
        LlvmValue equal = new_temporary(gen, TYPE_ID_INTEGER);
        write_ir(gen, "  %s = call i32 @zl_texto_igual(ptr %s, ptr %s)\n",
                 equal.operand, left.operand, right.operand);
        write_ir(gen, "  %s = icmp %s i32 %s, 0\n", result.operand,
                 op == AST_OP_EQUAL ? "ne" : "eq", equal.operand);
        return result;
    }
    if (op == AST_OP_POWER) {
        const char *name = left.type_id == TYPE_ID_INTEGER ? "zl_potencia_int" :
                           left.type_id == TYPE_ID_FLOAT ? "zl_potencia_flut" : "zl_potencia_duplocarpado";
        write_ir(gen, "  %s = call ", result.operand);
        emit_type(gen, result.type_id);
        write_ir(gen, " @%s(", name);
        emit_typed_value(gen, left);
        write_ir(gen, ", ");
        emit_typed_value(gen, right);
        write_ir(gen, ", i32 %zu)\n", node->span.start.line);
        return result;
    }
    if (op == AST_OP_DIVIDE || op == AST_OP_INTEGER_DIVIDE) {
        LlvmValue zero = new_temporary(gen, TYPE_ID_BOOL);
        write_ir(gen, "  %s = %s ", zero.operand, real ? "fcmp oeq" : "icmp eq");
        emit_typed_value(gen, right);
        write_ir(gen, real ? ", 0.000000e+00\n" : ", 0\n");
        emit_guard(gen, zero, 1, node->span.start.line);
        if (!real) {
            LlvmValue minimum = new_temporary(gen, TYPE_ID_BOOL);
            LlvmValue negative_one = new_temporary(gen, TYPE_ID_BOOL);
            LlvmValue overflow = new_temporary(gen, TYPE_ID_BOOL);
            write_ir(gen, "  %s = icmp eq i64 %s, -9223372036854775808\n", minimum.operand, left.operand);
            write_ir(gen, "  %s = icmp eq i64 %s, -1\n", negative_one.operand, right.operand);
            write_ir(gen, "  %s = and i1 %s, %s\n", overflow.operand, minimum.operand, negative_one.operand);
            emit_guard(gen, overflow, 2, node->span.start.line);
        }
    }
    switch (op) {
    case AST_OP_ADD: instruction = real ? "fadd" : "add"; break;
    case AST_OP_SUBTRACT: instruction = real ? "fsub" : "sub"; break;
    case AST_OP_MULTIPLY: instruction = real ? "fmul" : "mul"; break;
    case AST_OP_DIVIDE: instruction = "fdiv"; break;
    case AST_OP_INTEGER_DIVIDE: instruction = "sdiv"; break;
    case AST_OP_LESS: instruction = real ? "fcmp olt" : "icmp slt"; break;
    case AST_OP_GREATER: instruction = real ? "fcmp ogt" : "icmp sgt"; break;
    case AST_OP_EQUAL: instruction = real ? "fcmp oeq" : "icmp eq"; break;
    case AST_OP_DIFFERENT: instruction = real ? "fcmp une" : "icmp ne"; break;
    case AST_OP_LOGICAL_XOR: instruction = "xor"; break;
    default: fail(gen, node, "E-IR-001", "operador ainda nao implementado no gerador"); return result;
    }
    write_ir(gen, "  %s = %s ", result.operand, instruction);
    emit_typed_value(gen, left);
    write_ir(gen, ", %s\n", right.operand);
    return result;
}

static LlvmValue emit_unary(Codegen *gen, const AstNode *node)
{
    AstOperator op = node->as.unary_expr.operator_kind;
    const AstNode *operand = node->as.unary_expr.operand;
    LlvmValue value;
    LlvmValue result;
    value = emit_expression(gen, operand);
    if (node->semantic.conversion_type_id != AST_ID_NONE)
        value = convert_value(gen, value, node->semantic.conversion_type_id, node);
    if (op == AST_OP_UNARY_PLUS) return value;
    result = new_temporary(gen, node->semantic.type_id);
    if (op == AST_OP_LOGICAL_NOT) {
        write_ir(gen, "  %s = xor i1 %s, true\n", result.operand, value.operand);
    } else if (op == AST_OP_UNARY_MINUS && value.type_id == TYPE_ID_INTEGER) {
        write_ir(gen, "  %s = sub i64 0, %s\n", result.operand, value.operand);
    } else if (op == AST_OP_UNARY_MINUS) {
        write_ir(gen, "  %s = fneg ", result.operand);
        emit_typed_value(gen, value);
        write_ir(gen, "\n");
    } else if (op == AST_OP_ROOT) {
        write_ir(gen, "  %s = call ", result.operand);
        emit_type(gen, result.type_id);
        write_ir(gen, " @%s(", value.type_id == TYPE_ID_FLOAT ? "zl_raiz_flut" : "zl_raiz_duplocarpado");
        emit_typed_value(gen, value);
        write_ir(gen, ", i32 %zu)\n", node->span.start.line);
    } else fail(gen, node, "E-IR-001", "operador unario ainda nao implementado");
    return result;
}

/*
 * Frente dsl-mlir: a operacao vetorial vira uma chamada ao nucleo MLIR
 * (docs/dsl.md, 6.2). Cada vetor vai como memref descido: ptr alocado, ptr
 * alinhado, deslocamento 0, tamanho N e passo 1.
 */
static LlvmValue emit_dsl_operation(Codegen *gen, const AstNode *node)
{
    DslOperation operation;
    DslArgument arguments[3];
    LlvmValue result = value_empty(node->semantic.type_id);
    LlvmValue scalar = result;
    size_t count, index;
    if (!dsl_describe(gen->semantic, node, &operation)) {
        fail(gen, node, "E-IR-002", "operacao vetorial sem descricao da DSL");
        return result;
    }
    if (operation.scalar != NULL)
        scalar = convert_value(gen, emit_expression(gen, operation.scalar),
                               operation.result_type_id, node);
    count = dsl_kernel_arguments(&operation, arguments);
    if (operation.target_id == SYMBOL_ID_NONE) {
        result = new_temporary(gen, operation.result_type_id);
        write_ir(gen, "  %s = call ", result.operand);
        emit_type(gen, operation.result_type_id);
    } else write_ir(gen, "  call void");
    write_ir(gen, " @%s(", operation.kernel);
    for (index = 0U; index < count; ++index) {
        const Symbol *vector;
        char address[64];
        if (index != 0U) write_ir(gen, ", ");
        if (arguments[index].is_scalar) {
            emit_typed_value(gen, scalar);
            continue;
        }
        vector = symbol_table_get(&gen->semantic->symbols, arguments[index].symbol_id);
        if (vector == NULL) {
            fail(gen, node, "E-IR-002", "vetor da DSL sem simbolo resolvido");
            return result;
        }
        symbol_address(gen, vector, address, sizeof(address));
        write_ir(gen, "ptr %s, ptr %s, i64 0, i64 %lld, i64 1", address, address,
                 operation.length);
    }
    write_ir(gen, ")\n");
    return result;
}

static LlvmValue emit_expression(Codegen *gen, const AstNode *node)
{
    LlvmValue value = value_empty(node->semantic.type_id);
    if (gen->status != FRONTEND_STATUS_OK) return value;
    if (node->kind == AST_CALL_EXPR && dsl_is_operation(node)) /* Frente dsl-mlir */
        return emit_dsl_operation(gen, node);
    switch (node->kind) {
    case AST_LITERAL_EXPR:
        switch (node->as.literal_expr.literal_kind) {
        case AST_LITERAL_INTEGER:
            (void)snprintf(value.operand, sizeof(value.operand), "%lld", node->as.literal_expr.value.integer);
            break;
        case AST_LITERAL_BOOLEAN:
            (void)strcpy(value.operand, node->as.literal_expr.value.boolean ? "true" : "false");
            break;
        case AST_LITERAL_REAL: {
            uint64_t bits;
            double real = node->as.literal_expr.value.real;
            /* LLVM escreve constantes float exatas usando a codificacao hex de double. */
            (void)memcpy(&bits, &real, sizeof(bits));
            (void)snprintf(value.operand, sizeof(value.operand), "0x%016" PRIX64, bits);
            break;
        }
        case AST_LITERAL_STRING:
            value = new_temporary(gen, TYPE_ID_WORD);
            write_ir(gen, "  %s = getelementptr [%zu x i8], ptr @.str.%zu, i32 0, i32 0\n",
                     value.operand, strlen(node->as.literal_expr.value.string) + 1U, node->node_id);
            break;
        }
        break;
    case AST_IDENTIFIER_EXPR: {
        const Symbol *symbol = symbol_table_get(&gen->semantic->symbols, node->semantic.symbol_id);
        char address[64];
        if (symbol == NULL) {
            fail(gen, node, "E-IR-002", "identificador sem simbolo resolvido");
            break;
        }
        if (type_table_get(&gen->semantic->types, node->semantic.type_id)->kind == TYPE_ARRAY) {
            fail(gen, node, "E-IR-001", "vetor inteiro nao e valor da linguagem");
            break;
        }
        symbol_address(gen, symbol, address, sizeof(address));
        value = new_named_temporary(gen, node->semantic.type_id, symbol->name);
        write_ir(gen, "  %s = load ", value.operand);
        emit_type(gen, value.type_id);
        write_ir(gen, ", ptr %s\n", address);
        break;
    }
    case AST_INDEX_EXPR: {
        LlvmValue address = emit_address(gen, node);
        value = new_temporary(gen, node->semantic.type_id);
        write_ir(gen, "  %s = load ", value.operand);
        emit_type(gen, value.type_id);
        write_ir(gen, ", ptr %s\n", address.operand);
        break;
    }
    case AST_FIELD_EXPR: {
        const Symbol *field = symbol_table_get(&gen->semantic->symbols,
                                                node->semantic.symbol_id);
        if (field == NULL) {
            fail(gen, node, "E-IR-002", "campo sem simbolo resolvido");
            break;
        }
        if ((node->semantic.flags & AST_SEMANTIC_LVALUE) != 0U) {
            LlvmValue address = emit_address(gen, node);
            value = new_temporary(gen, node->semantic.type_id);
            write_ir(gen, "  %s = load ", value.operand);
            emit_type(gen, value.type_id);
            write_ir(gen, ", ptr %s\n", address.operand);
        } else {
            LlvmValue base = emit_expression(gen, node->as.field_expr.base);
            value = new_temporary(gen, node->semantic.type_id);
            write_ir(gen, "  %s = extractvalue ", value.operand);
            emit_typed_value(gen, base);
            write_ir(gen, ", %zu\n", field->field_index);
        }
        break;
    }
    case AST_CALL_EXPR: return emit_call(gen, node);
    case AST_BINARY_EXPR: return emit_binary(gen, node);
    case AST_UNARY_EXPR: return emit_unary(gen, node);
    default: fail(gen, node, "E-IR-001", "expressao ainda nao implementada no gerador"); break;
    }
    return value;
}

static void emit_if(Codegen *gen, const AstNode *node)
{
    size_t index;
    size_t end_label = gen->label++;
    int reaches_end = node->as.if_stmt.else_body == NULL;
    for (index = 0U; index < node->as.if_stmt.branches.count; ++index) {
        const AstIfBranch *branch = &node->as.if_stmt.branches.items[index];
        size_t then_label = gen->label++;
        size_t next_label = gen->label++;
        LlvmValue condition = emit_expression(gen, branch->condition);
        write_ir(gen, "  br i1 %s, label %%if.then.%zu, label %%if.next.%zu\n",
                 condition.operand, then_label, next_label);
        emit_label(gen, "if.then", then_label);
        emit_statement(gen, branch->body);
        if (!gen->terminated) {
            write_ir(gen, "  br label %%if.end.%zu\n", end_label);
            reaches_end = 1;
        }
        emit_label(gen, "if.next", next_label);
    }
    if (node->as.if_stmt.else_body != NULL) {
        emit_statement(gen, node->as.if_stmt.else_body);
        if (!gen->terminated) {
            write_ir(gen, "  br label %%if.end.%zu\n", end_label);
            reaches_end = 1;
        }
    } else write_ir(gen, "  br label %%if.end.%zu\n", end_label);
    if (reaches_end) emit_label(gen, "if.end", end_label);
    else gen->terminated = 1;
}

static void emit_while(Codegen *gen, const AstNode *node)
{
    size_t condition_label = gen->label++;
    size_t body_label = gen->label++;
    size_t end_label = gen->label++;
    write_ir(gen, "  br label %%loop.cond.%zu\n", condition_label);
    emit_label(gen, "loop.cond", condition_label);
    {
        LlvmValue condition = emit_expression(gen, node->as.while_stmt.condition);
        write_ir(gen, "  br i1 %s, label %%loop.body.%zu, label %%loop.end.%zu\n",
                 condition.operand, body_label, end_label);
    }
    emit_label(gen, "loop.body", body_label);
    emit_statement(gen, node->as.while_stmt.body);
    if (!gen->terminated) write_ir(gen, "  br label %%loop.cond.%zu\n", condition_label);
    emit_label(gen, "loop.end", end_label);
}

static void emit_for(Codegen *gen, const AstNode *node)
{
    size_t condition_label = gen->label++;
    size_t body_label = gen->label++;
    size_t iteration_label = gen->label++;
    size_t end_label = gen->label++;
    emit_statement(gen, node->as.for_stmt.initializer);
    if (gen->terminated) return;
    write_ir(gen, "  br label %%for.cond.%zu\n", condition_label);
    emit_label(gen, "for.cond", condition_label);
    {
        LlvmValue condition = emit_expression(gen, node->as.for_stmt.condition);
        write_ir(gen, "  br i1 %s, label %%for.body.%zu, label %%for.end.%zu\n",
                 condition.operand, body_label, end_label);
    }
    emit_label(gen, "for.body", body_label);
    emit_statement(gen, node->as.for_stmt.body);
    if (!gen->terminated) {
        write_ir(gen, "  br label %%for.inc.%zu\n", iteration_label);
        emit_label(gen, "for.inc", iteration_label);
        emit_statement(gen, node->as.for_stmt.iteration);
        if (!gen->terminated) write_ir(gen, "  br label %%for.cond.%zu\n", condition_label);
    }
    emit_label(gen, "for.end", end_label);
}

static void emit_statement(Codegen *gen, const AstNode *node)
{
    size_t index;
    if (gen->status != FRONTEND_STATUS_OK || gen->terminated) return;
    /* Um comentario por linha de origem; blocos e declaracoes nao geram codigo. */
    if (node->kind != AST_BLOCK && node->kind != AST_VAR_DECL &&
        node->kind != AST_VECTOR_DECL && node->span.start.line != gen->last_line) {
        write_ir(gen, "  ; linha %zu\n", node->span.start.line);
        gen->last_line = node->span.start.line;
    }
    if (node->kind == AST_ASSIGNMENT && dsl_is_operation(node)) { /* Frente dsl-mlir */
        (void)emit_dsl_operation(gen, node);
        return;
    }
    switch (node->kind) {
    case AST_BLOCK:
        for (index = 0U; index < node->as.block.statements.count; ++index)
            emit_statement(gen, node->as.block.statements.items[index]);
        break;
    case AST_VAR_DECL:
    case AST_VECTOR_DECL: break; /* Armazenamento e inicializacao ja ocorreram no entry. */
    case AST_ASSIGNMENT: {
        const AstNode *target = node->as.assignment.target;
        LlvmValue address = emit_address(gen, target);
        LlvmValue value;
        value = emit_expression(gen, node->as.assignment.value);
        value = convert_value(gen, value, target->semantic.type_id, node);
        write_ir(gen, "  store ");
        emit_typed_value(gen, value);
        write_ir(gen, ", ptr %s\n", address.operand);
        break;
    }
    case AST_IF_STMT: emit_if(gen, node); break;
    case AST_WHILE_STMT: emit_while(gen, node); break;
    case AST_FOR_STMT: emit_for(gen, node); break;
    case AST_EXPR_STMT: (void)emit_expression(gen, node->as.expr_stmt.expression); break;
    case AST_RETURN_STMT:
        if (node->as.return_stmt.value != NULL) {
            LlvmValue value = emit_expression(gen, node->as.return_stmt.value);
            value = convert_value(gen, value, gen->function->as.function_decl.return_type.resolved_type_id, node);
            if (gen->function->as.function_decl.is_principal) {
                /* zl_principal mantem a ABI i32 com o runtime: o status do
                 * processo recebe os 32 bits baixos do valor inteira (i64). */
                LlvmValue status = new_temporary(gen, TYPE_ID_INTEGER);
                write_ir(gen, "  %s = trunc i64 %s to i32\n  ret i32 %s\n",
                         status.operand, value.operand, status.operand);
            } else {
                write_ir(gen, "  ret ");
                emit_typed_value(gen, value);
                write_ir(gen, "\n");
            }
        } else write_ir(gen, gen->function->as.function_decl.is_principal ? "  ret i32 0\n" : "  ret void\n");
        gen->terminated = 1;
        break;
    default: fail(gen, node, "E-IR-001", "comando ainda nao implementado no gerador"); break;
    }
}

/* Esta etapa rejeita construcoes futuras antes de escrever qualquer IR. */
static void visit_tree(Codegen *gen, const AstNode *node, int emit_strings)
{
    size_t index;
    const AstNodeList *list = NULL;
    if (node == NULL || gen->status != FRONTEND_STATUS_OK) return;
    switch (node->kind) {
    case AST_PROGRAM: list = &node->as.program.declarations; break;
    case AST_FUNCTION_DECL:
        visit_tree(gen, node->as.function_decl.body, emit_strings);
        break;
    case AST_STRUCT_DECL: break;
    case AST_VAR_DECL:
    case AST_VECTOR_DECL: break;
    case AST_BLOCK: list = &node->as.block.statements; break;
    case AST_ASSIGNMENT:
        visit_tree(gen, node->as.assignment.target, emit_strings);
        visit_tree(gen, node->as.assignment.value, emit_strings);
        break;
    case AST_RETURN_STMT: visit_tree(gen, node->as.return_stmt.value, emit_strings); break;
    case AST_EXPR_STMT: visit_tree(gen, node->as.expr_stmt.expression, emit_strings); break;
    case AST_CALL_EXPR: list = &node->as.call_expr.arguments; break;
    case AST_IF_STMT:
        for (index = 0U; index < node->as.if_stmt.branches.count; ++index) {
            visit_tree(gen, node->as.if_stmt.branches.items[index].condition, emit_strings);
            visit_tree(gen, node->as.if_stmt.branches.items[index].body, emit_strings);
        }
        visit_tree(gen, node->as.if_stmt.else_body, emit_strings);
        break;
    case AST_WHILE_STMT:
        visit_tree(gen, node->as.while_stmt.condition, emit_strings);
        visit_tree(gen, node->as.while_stmt.body, emit_strings);
        break;
    case AST_FOR_STMT:
        visit_tree(gen, node->as.for_stmt.initializer, emit_strings);
        visit_tree(gen, node->as.for_stmt.condition, emit_strings);
        visit_tree(gen, node->as.for_stmt.iteration, emit_strings);
        visit_tree(gen, node->as.for_stmt.body, emit_strings);
        break;
    case AST_INDEX_EXPR:
        visit_tree(gen, node->as.index_expr.base, emit_strings);
        visit_tree(gen, node->as.index_expr.index, emit_strings);
        break;
    case AST_FIELD_EXPR:
        visit_tree(gen, node->as.field_expr.base, emit_strings);
        break;
    case AST_BINARY_EXPR:
        visit_tree(gen, node->as.binary_expr.left, emit_strings);
        visit_tree(gen, node->as.binary_expr.right, emit_strings);
        break;
    case AST_UNARY_EXPR: visit_tree(gen, node->as.unary_expr.operand, emit_strings); break;
    case AST_LITERAL_EXPR:
        if (emit_strings && node->as.literal_expr.literal_kind == AST_LITERAL_STRING) {
            const unsigned char *text = (const unsigned char *)node->as.literal_expr.value.string;
            size_t length = strlen((const char *)text);
            write_ir(gen, "@.str.%zu = private unnamed_addr constant [%zu x i8] c\"", node->node_id, length + 1U);
            for (index = 0U; index < length; ++index) write_ir(gen, "\\%02X", (unsigned int)text[index]);
            write_ir(gen, "\\00\"\n");
        }
        break;
    case AST_IDENTIFIER_EXPR: break;
    default: fail(gen, node, "E-IR-001", "construcao pendente das etapas de CFG/agregados"); break;
    }
    if (list != NULL)
        for (index = 0U; index < list->count; ++index) visit_tree(gen, list->items[index], emit_strings);
}

static void emit_record_types(Codegen *gen)
{
    size_t index;
    for (index = 0U; index < gen->semantic->types.count; ++index) {
        const Type *type = &gen->semantic->types.items[index];
        size_t field;
        int first = 1;
        if (type->kind != TYPE_RECORD) continue;
        write_ir(gen, "%%struct.%s = type { ", type->name);
        for (field = 0U; field < gen->semantic->symbols.count; ++field) {
            const Symbol *symbol = &gen->semantic->symbols.items[field];
            const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, symbol->scope_id);
            if (symbol->category != SYMBOL_FIELD || scope == NULL ||
                scope->owner_symbol_id != type->declaration_symbol_id) continue;
            if (!first) write_ir(gen, ", ");
            emit_type(gen, symbol->type_id);
            first = 0;
        }
        write_ir(gen, " }\n");
    }
}

static void emit_globals(Codegen *gen)
{
    size_t index;
    for (index = 0U; index < gen->semantic->symbols.count; ++index) {
        const Symbol *symbol = &gen->semantic->symbols.items[index];
        if (symbol->scope_id != gen->semantic->global_scope_id ||
            (symbol->category != SYMBOL_GLOBAL_VARIABLE && symbol->category != SYMBOL_VECTOR)) continue;
        char name[64];
        symbol_address(gen, symbol, name, sizeof(name));
        write_ir(gen, "%s = internal global ", name);
        emit_type(gen, symbol->type_id);
        write_ir(gen, " zeroinitializer\n");
    }
}

static void emit_runtime_declarations(Codegen *gen)
{
    write_ir(gen,
        "declare void @zl_imprime_int(i64) nounwind\n"
        "declare void @zl_imprime_flut(float) nounwind\n"
        "declare void @zl_imprime_duplocarpado(double) nounwind\n"
        "declare void @zl_imprime_texto(ptr) nounwind\n"
        "declare void @zl_imprime_bool(i32) nounwind\n"
        "declare void @zl_imprime_nl() nounwind\n"
        "declare i64 @zl_le_int(i32) nounwind\n"
        "declare float @zl_le_flut(i32) nounwind\n"
        "declare double @zl_le_duplocarpado(i32) nounwind\n"
        "declare i32 @zl_texto_igual(ptr, ptr) nounwind\n"
        "declare void @zl_erro_execucao(i32, i32) noreturn nounwind\n"
        "declare i64 @zl_potencia_int(i64, i64, i32) nounwind\n"
        "declare float @zl_potencia_flut(float, float, i32) nounwind\n"
        "declare double @zl_potencia_duplocarpado(double, double, i32) nounwind\n"
        "declare float @zl_raiz_flut(float, i32) nounwind\n"
        "declare double @zl_raiz_duplocarpado(double, i32) nounwind\n\n");
}

static void emit_function(Codegen *gen, const AstNode *function)
{
    const Symbol *symbol = symbol_table_get(&gen->semantic->symbols, function->semantic.symbol_id);
    size_t index;
    gen->function = function;
    gen->temporary = 0U;
    gen->label = 0U;
    gen->terminated = 0;
    gen->last_line = 0U;
    (void)strcpy(gen->current_block, "entry");
    if (symbol == NULL) {
        fail(gen, function, "E-IR-002", "funcao sem simbolo resolvido");
        return;
    }
    write_ir(gen, "define ");
    if (symbol->is_principal) write_ir(gen, "i32");
    else emit_type(gen, symbol->type_id);
    write_ir(gen, " ");
    emit_function_name(gen, symbol);
    write_ir(gen, "(");
    for (index = 0U; index < function->as.function_decl.parameters.count; ++index) {
        const AstNode *parameter = function->as.function_decl.parameters.items[index];
        char value_name[64];
        parameter_value_name(symbol_table_get(&gen->semantic->symbols,
                                              parameter->as.var_decl.names.items[0].symbol_id),
                             index, value_name, sizeof(value_name));
        if (index != 0U) write_ir(gen, ", ");
        emit_type(gen, parameter->as.var_decl.type.resolved_type_id);
        write_ir(gen, " %s", value_name);
    }
    write_ir(gen, ") nounwind {\nentry:\n");
    /* Locais de qualquer ramo pertencem a funcao: todos nascem no entry. */
    for (index = 0U; index < gen->semantic->symbols.count; ++index) {
        const Symbol *local = &gen->semantic->symbols.items[index];
        const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, local->scope_id);
        if (scope == NULL || scope->kind != SCOPE_FUNCTION || scope->owner_symbol_id != symbol->id) continue;
        if (local->category != SYMBOL_LOCAL_VARIABLE && local->category != SYMBOL_PARAMETER &&
            local->category != SYMBOL_VECTOR) continue;
        char slot[64];
        symbol_address(gen, local, slot, sizeof(slot));
        write_ir(gen, "  %s = alloca ", slot);
        emit_type(gen, local->type_id);
        write_ir(gen, "\n");
    }
    for (index = 0U; index < gen->semantic->symbols.count; ++index) {
        const Symbol *local = &gen->semantic->symbols.items[index];
        const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, local->scope_id);
        if (scope == NULL || scope->kind != SCOPE_FUNCTION || scope->owner_symbol_id != symbol->id) continue;
        if (local->category != SYMBOL_LOCAL_VARIABLE && local->category != SYMBOL_VECTOR) continue;
        char slot[64];
        symbol_address(gen, local, slot, sizeof(slot));
        write_ir(gen, "  store ");
        emit_type(gen, local->type_id);
        write_ir(gen, " zeroinitializer, ptr %s\n", slot);
    }
    for (index = 0U; index < function->as.function_decl.parameters.count; ++index) {
        const AstNode *parameter = function->as.function_decl.parameters.items[index];
        const Symbol *parameter_symbol = symbol_table_get(
            &gen->semantic->symbols, parameter->as.var_decl.names.items[0].symbol_id);
        char value_name[64], slot[64];
        if (parameter_symbol == NULL) {
            fail(gen, parameter, "E-IR-002", "parametro sem simbolo resolvido");
            return;
        }
        parameter_value_name(parameter_symbol, index, value_name, sizeof(value_name));
        symbol_address(gen, parameter_symbol, slot, sizeof(slot));
        write_ir(gen, "  store ");
        emit_type(gen, parameter->as.var_decl.type.resolved_type_id);
        write_ir(gen, " %s, ptr %s\n", value_name, slot);
    }
    emit_statement(gen, function->as.function_decl.body);
    if (!gen->terminated) {
        if (symbol->type_id == TYPE_ID_VOID) write_ir(gen, symbol->is_principal ? "  ret i32 0\n" : "  ret void\n");
        else fail(gen, function, "E-IR-002", "funcao nao vazia chegou ao fim sem retorno");
    }
    write_ir(gen, "}\n\n");
}

FrontendStatus codegen_llvm_emit(const SourceFile *source,
                                const AstNode *program,
                                const SemanticResult *semantic,
                                DiagnosticBag *diagnostics,
                                FILE *output)
{
    Codegen gen;
    size_t index;
    if (source == NULL || program == NULL || program->kind != AST_PROGRAM ||
        semantic == NULL || diagnostics == NULL || output == NULL ||
        !semantic->analyzed || semantic->semantic_error_count != 0U) return FRONTEND_STATUS_INTERNAL_ERROR;
    (void)memset(&gen, 0, sizeof(gen));
    gen.source = source;
    gen.semantic = semantic;
    gen.diagnostics = diagnostics;
    gen.output = output;
    gen.function = program; /* Posicao util para erros de infraestrutura do modulo. */
    gen.status = FRONTEND_STATUS_OK;
    if (sizeof(double) != sizeof(uint64_t) || FLT_RADIX != 2 ||
        DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024 ||
        FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128) {
        fail(&gen, program, "E-IR-002", "emissor requer float/double IEEE binary32/binary64 no hospedeiro");
        return gen.status;
    }
    visit_tree(&gen, program, 0);
    if (gen.status != FRONTEND_STATUS_OK) return gen.status;
    write_ir(&gen, "; Macaronica: LLVM IR textual portatil\n\n");
    emit_record_types(&gen);
    emit_globals(&gen);
    visit_tree(&gen, program, 1);
    write_ir(&gen, "\n");
    emit_runtime_declarations(&gen);
    /* Frente dsl-mlir: `declare` dos nucleos MLIR que o programa usa. */
    if (gen.status == FRONTEND_STATUS_OK &&
        !codegen_mlir_write_llvm_declarations(output, program, semantic))
        fail(&gen, program, "E-IR-004", "falha ao declarar os nucleos da DSL");
    for (index = 0U; index < program->as.program.declarations.count; ++index) {
        const AstNode *declaration = program->as.program.declarations.items[index];
        if (declaration->kind == AST_FUNCTION_DECL) emit_function(&gen, declaration);
    }
    if (ferror(output) && gen.status == FRONTEND_STATUS_OK)
        fail(&gen, program, "E-IR-004", "falha ao escrever LLVM IR");
    return gen.status;
}
