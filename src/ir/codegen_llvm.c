#include "codegen_llvm.h"

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
    case TYPE_INTEGER: write_ir(gen, "i32"); break;
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

static void emit_typed_value(Codegen *gen, LlvmValue value)
{
    emit_type(gen, value.type_id);
    write_ir(gen, " %s", value.operand);
}

static void symbol_address(Codegen *gen, const Symbol *symbol, char *buffer,
                           size_t capacity)
{
    (void)snprintf(buffer, capacity,
                   symbol->scope_id == gen->semantic->global_scope_id
                       ? "@mac.global.%zu" : "%%v.%zu.addr", symbol->id);
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

/* Guardas abrem dois blocos: erro termina; somente continua chega a operacao. */
static void emit_guard(Codegen *gen, LlvmValue bad, int code, size_t line)
{
    size_t label = gen->label++;
    write_ir(gen, "  br i1 %s, label %%erro.%zu, label %%continua.%zu\n",
             bad.operand, label, label);
    emit_label(gen, "erro", label);
    write_ir(gen, "  call void @zl_erro_execucao(i32 %d, i32 %zu)\n  unreachable\n", code, line);
    emit_label(gen, "continua", label);
}

static LlvmValue emit_expression(Codegen *gen, const AstNode *node);

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
        LlvmValue widened = new_temporary(gen, TYPE_ID_INTEGER);
        write_ir(gen, "  %s = zext i1 %s to i32\n", widened.operand, values[0].operand);
        values[0] = widened;
    }
    if (result.type_id == TYPE_ID_VOID) write_ir(gen, "  call void ");
    else {
        result = new_temporary(gen, result.type_id);
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
    LlvmValue right = emit_expression(gen, node->as.binary_expr.right);
    LlvmValue result = new_temporary(gen, node->semantic.type_id);
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
            write_ir(gen, "  %s = icmp eq i32 %s, -2147483648\n", minimum.operand, left.operand);
            write_ir(gen, "  %s = icmp eq i32 %s, -1\n", negative_one.operand, right.operand);
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
    if (op == AST_OP_UNARY_MINUS && operand->kind == AST_LITERAL_EXPR &&
        operand->as.literal_expr.literal_kind == AST_LITERAL_INTEGER &&
        operand->as.literal_expr.value.integer == 2147483648LL) {
        result = value_empty(TYPE_ID_INTEGER);
        (void)strcpy(result.operand, "-2147483648");
        return result;
    }
    value = emit_expression(gen, operand);
    if (op == AST_OP_UNARY_PLUS) return value;
    result = new_temporary(gen, node->semantic.type_id);
    if (op == AST_OP_LOGICAL_NOT) {
        write_ir(gen, "  %s = xor i1 %s, true\n", result.operand, value.operand);
    } else if (op == AST_OP_UNARY_MINUS && value.type_id == TYPE_ID_INTEGER) {
        write_ir(gen, "  %s = sub i32 0, %s\n", result.operand, value.operand);
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

static LlvmValue emit_expression(Codegen *gen, const AstNode *node)
{
    LlvmValue value = value_empty(node->semantic.type_id);
    if (gen->status != FRONTEND_STATUS_OK) return value;
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
        symbol_address(gen, symbol, address, sizeof(address));
        value = new_temporary(gen, node->semantic.type_id);
        write_ir(gen, "  %s = load ", value.operand);
        emit_type(gen, value.type_id);
        write_ir(gen, ", ptr %s\n", address);
        break;
    }
    case AST_CALL_EXPR: return emit_call(gen, node);
    case AST_BINARY_EXPR: return emit_binary(gen, node);
    case AST_UNARY_EXPR: return emit_unary(gen, node);
    default: fail(gen, node, "E-IR-001", "expressao ainda nao implementada no gerador"); break;
    }
    return value;
}

static void emit_statement(Codegen *gen, const AstNode *node)
{
    size_t index;
    if (gen->status != FRONTEND_STATUS_OK || gen->terminated) return;
    write_ir(gen, "  ; linha %zu\n", node->span.start.line);
    switch (node->kind) {
    case AST_BLOCK:
        for (index = 0U; index < node->as.block.statements.count; ++index)
            emit_statement(gen, node->as.block.statements.items[index]);
        break;
    case AST_VAR_DECL: break; /* Armazenamento e inicializacao ja ocorreram no entry. */
    case AST_ASSIGNMENT: {
        const AstNode *target = node->as.assignment.target;
        const Symbol *symbol = symbol_table_get(&gen->semantic->symbols, target->semantic.symbol_id);
        char address[64];
        LlvmValue value;
        if (target->kind != AST_IDENTIFIER_EXPR || symbol == NULL) {
            fail(gen, target, "E-IR-001", "destino ainda nao implementado no gerador");
            return;
        }
        symbol_address(gen, symbol, address, sizeof(address));
        value = emit_expression(gen, node->as.assignment.value);
        value = convert_value(gen, value, target->semantic.type_id, node);
        write_ir(gen, "  store ");
        emit_typed_value(gen, value);
        write_ir(gen, ", ptr %s\n", address);
        break;
    }
    case AST_EXPR_STMT: (void)emit_expression(gen, node->as.expr_stmt.expression); break;
    case AST_RETURN_STMT:
        if (node->as.return_stmt.value != NULL) {
            LlvmValue value = emit_expression(gen, node->as.return_stmt.value);
            value = convert_value(gen, value, gen->function->as.function_decl.return_type.resolved_type_id, node);
            write_ir(gen, "  ret ");
            emit_typed_value(gen, value);
            write_ir(gen, "\n");
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
    case AST_VAR_DECL: break;
    case AST_BLOCK: list = &node->as.block.statements; break;
    case AST_ASSIGNMENT:
        visit_tree(gen, node->as.assignment.target, emit_strings);
        visit_tree(gen, node->as.assignment.value, emit_strings);
        break;
    case AST_RETURN_STMT: visit_tree(gen, node->as.return_stmt.value, emit_strings); break;
    case AST_EXPR_STMT: visit_tree(gen, node->as.expr_stmt.expression, emit_strings); break;
    case AST_CALL_EXPR: list = &node->as.call_expr.arguments; break;
    case AST_BINARY_EXPR:
        if (node->as.binary_expr.operator_kind == AST_OP_LOGICAL_AND ||
            node->as.binary_expr.operator_kind == AST_OP_LOGICAL_OR) {
            fail(gen, node, "E-IR-001", "curto-circuito pendente da etapa de CFG");
            break;
        }
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
        write_ir(gen, "; global %s\n@mac.global.%zu = global ", symbol->name, symbol->id);
        emit_type(gen, symbol->type_id);
        write_ir(gen, " zeroinitializer\n");
    }
}

static void emit_runtime_declarations(Codegen *gen)
{
    write_ir(gen,
        "declare void @zl_imprime_int(i32) nounwind\n"
        "declare void @zl_imprime_flut(float) nounwind\n"
        "declare void @zl_imprime_duplocarpado(double) nounwind\n"
        "declare void @zl_imprime_texto(ptr) nounwind\n"
        "declare void @zl_imprime_bool(i32) nounwind\n"
        "declare void @zl_imprime_nl() nounwind\n"
        "declare i32 @zl_le_int(i32) nounwind\n"
        "declare float @zl_le_flut(i32) nounwind\n"
        "declare double @zl_le_duplocarpado(i32) nounwind\n"
        "declare i32 @zl_texto_igual(ptr, ptr) nounwind\n"
        "declare void @zl_erro_execucao(i32, i32) noreturn nounwind\n"
        "declare i32 @zl_potencia_int(i32, i32, i32) nounwind\n"
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
    (void)strcpy(gen->current_block, "entry");
    if (symbol == NULL) {
        fail(gen, function, "E-IR-002", "funcao sem simbolo resolvido");
        return;
    }
    write_ir(gen, "define ");
    emit_type(gen, symbol->is_principal ? TYPE_ID_INTEGER : symbol->type_id);
    write_ir(gen, " ");
    emit_function_name(gen, symbol);
    write_ir(gen, "(");
    for (index = 0U; index < function->as.function_decl.parameters.count; ++index) {
        const AstNode *parameter = function->as.function_decl.parameters.items[index];
        if (index != 0U) write_ir(gen, ", ");
        emit_type(gen, parameter->as.var_decl.type.resolved_type_id);
        write_ir(gen, " %%arg.%zu", index);
    }
    write_ir(gen, ") nounwind {\nentry:\n");
    /* Locais de qualquer ramo pertencem a funcao: todos nascem no entry. */
    for (index = 0U; index < gen->semantic->symbols.count; ++index) {
        const Symbol *local = &gen->semantic->symbols.items[index];
        const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, local->scope_id);
        if (scope == NULL || scope->kind != SCOPE_FUNCTION || scope->owner_symbol_id != symbol->id) continue;
        if (local->category != SYMBOL_LOCAL_VARIABLE && local->category != SYMBOL_PARAMETER &&
            local->category != SYMBOL_VECTOR) continue;
        write_ir(gen, "  ; %s\n  %%v.%zu.addr = alloca ", local->name, local->id);
        emit_type(gen, local->type_id);
        write_ir(gen, "\n");
    }
    for (index = 0U; index < gen->semantic->symbols.count; ++index) {
        const Symbol *local = &gen->semantic->symbols.items[index];
        const Scope *scope = symbol_table_get_scope(&gen->semantic->symbols, local->scope_id);
        if (scope == NULL || scope->kind != SCOPE_FUNCTION || scope->owner_symbol_id != symbol->id) continue;
        if (local->category != SYMBOL_LOCAL_VARIABLE && local->category != SYMBOL_VECTOR) continue;
        write_ir(gen, "  store ");
        emit_type(gen, local->type_id);
        write_ir(gen, " zeroinitializer, ptr %%v.%zu.addr\n", local->id);
    }
    for (index = 0U; index < function->as.function_decl.parameters.count; ++index) {
        const AstNode *parameter = function->as.function_decl.parameters.items[index];
        write_ir(gen, "  store ");
        emit_type(gen, parameter->as.var_decl.type.resolved_type_id);
        write_ir(gen, " %%arg.%zu, ptr %%v.%zu.addr\n", index, parameter->as.var_decl.names.items[0].symbol_id);
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
    for (index = 0U; index < program->as.program.declarations.count; ++index) {
        const AstNode *declaration = program->as.program.declarations.items[index];
        if (declaration->kind == AST_FUNCTION_DECL) emit_function(&gen, declaration);
    }
    if (ferror(output) && gen.status == FRONTEND_STATUS_OK)
        fail(&gen, program, "E-IR-004", "falha ao escrever LLVM IR");
    return gen.status;
}
