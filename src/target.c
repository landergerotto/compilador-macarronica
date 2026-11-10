#include "target.h"

#include <stdint.h>

typedef struct {
    const SourceFile *source;
    const SemanticResult *semantic;
    DiagnosticBag *diagnostics;
    int esp32;
    int failed;
} TargetCheck;

static void target_error(TargetCheck *check, SourceSpan span,
                         const char *code, const char *message)
{
    if (!diagnostic_bag_add(check->diagnostics, check->source, span,
                            DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SEMANTIC,
                            code, "%s", message)) check->failed = 1;
}

static int is_esp32_builtin(BuiltinKind kind)
{
    return kind == BUILTIN_PIN_MODE || kind == BUILTIN_PIN_WRITE ||
           kind == BUILTIN_PIN_READ || kind == BUILTIN_DELAY_MS;
}

static void check_node(TargetCheck *check, const AstNode *node)
{
    size_t i;
    const Symbol *symbol;
    if (node == NULL || check->failed) return;
    switch (node->kind) {
    case AST_PROGRAM:
        for (i = 0U; i < node->as.program.declarations.count; ++i)
            check_node(check, node->as.program.declarations.items[i]);
        break;
    case AST_FUNCTION_DECL:
        check_node(check, node->as.function_decl.body);
        break;
    case AST_BLOCK:
        for (i = 0U; i < node->as.block.statements.count; ++i)
            check_node(check, node->as.block.statements.items[i]);
        break;
    case AST_ASSIGNMENT:
        check_node(check, node->as.assignment.target);
        check_node(check, node->as.assignment.value);
        break;
    case AST_IF_STMT:
        for (i = 0U; i < node->as.if_stmt.branches.count; ++i) {
            check_node(check, node->as.if_stmt.branches.items[i].condition);
            check_node(check, node->as.if_stmt.branches.items[i].body);
        }
        check_node(check, node->as.if_stmt.else_body);
        break;
    case AST_WHILE_STMT:
        check_node(check, node->as.while_stmt.condition);
        check_node(check, node->as.while_stmt.body);
        break;
    case AST_FOR_STMT:
        check_node(check, node->as.for_stmt.initializer);
        check_node(check, node->as.for_stmt.condition);
        check_node(check, node->as.for_stmt.iteration);
        check_node(check, node->as.for_stmt.body);
        break;
    case AST_RETURN_STMT:
        check_node(check, node->as.return_stmt.value);
        break;
    case AST_EXPR_STMT:
        check_node(check, node->as.expr_stmt.expression);
        break;
    case AST_CALL_EXPR:
        symbol = symbol_table_get(&check->semantic->symbols,
                                  node->semantic.symbol_id);
        if (!check->esp32 && symbol != NULL &&
            is_esp32_builtin(symbol->builtin_kind)) {
            target_error(check, node->as.call_expr.callee_span, "E-TGT-001",
                         "intrinseca de GPIO/espera exige --alvo=esp32");
        }
        for (i = 0U; i < node->as.call_expr.arguments.count; ++i)
            check_node(check, node->as.call_expr.arguments.items[i]);
        break;
    case AST_INDEX_EXPR:
        check_node(check, node->as.index_expr.base);
        check_node(check, node->as.index_expr.index);
        break;
    case AST_FIELD_EXPR:
        check_node(check, node->as.field_expr.base);
        break;
    case AST_BINARY_EXPR:
        check_node(check, node->as.binary_expr.left);
        check_node(check, node->as.binary_expr.right);
        break;
    case AST_UNARY_EXPR:
        check_node(check, node->as.unary_expr.operand);
        break;
    default:
        break;
    }
}

static size_t add_size(size_t left, size_t right, int *overflow)
{
    if (right > SIZE_MAX - left) {
        *overflow = 1;
        return SIZE_MAX;
    }
    return left + right;
}

static size_t type_size(const SemanticResult *semantic, size_t type_id,
                        size_t depth, int *overflow)
{
    const Type *type = type_table_get(&semantic->types, type_id);
    size_t i, sum = 0U, element;
    if (type == NULL || depth > semantic->types.count) {
        *overflow = 1;
        return SIZE_MAX;
    }
    switch (type->kind) {
    case TYPE_BOOL: return 1U;
    case TYPE_INTEGER: case TYPE_FLOAT: case TYPE_WORD: return 4U;
    case TYPE_DOUBLE: return 8U;
    case TYPE_ARRAY:
        if (type->array_size < 0) {
            *overflow = 1;
            return SIZE_MAX;
        }
        element = type_size(semantic, type->element_type_id, depth + 1U,
                            overflow);
        if ((size_t)type->array_size > 0U &&
            element > SIZE_MAX / (size_t)type->array_size) {
            *overflow = 1;
            return SIZE_MAX;
        }
        return element * (size_t)type->array_size;
    case TYPE_RECORD:
        for (i = 0U; i < semantic->symbols.count; ++i) {
            const Symbol *field = &semantic->symbols.items[i];
            const Scope *scope = symbol_table_get_scope(&semantic->symbols,
                                                         field->scope_id);
            if (field->category == SYMBOL_FIELD && scope != NULL &&
                scope->owner_symbol_id == type->declaration_symbol_id) {
                sum = add_size(sum, type_size(semantic, field->type_id,
                                              depth + 1U, overflow), overflow);
            }
        }
        return sum;
    default:
        *overflow = 1;
        return SIZE_MAX;
    }
}

static void check_ram(TargetCheck *check, const AstNode *root,
                      size_t ram_limit)
{
    const SymbolTable *symbols = &check->semantic->symbols;
    size_t global = 0U, peak = 0U, total, i, j;
    int overflow = 0;
    for (i = 0U; i < symbols->count; ++i) {
        const Symbol *symbol = &symbols->items[i];
        if (symbol->scope_id == check->semantic->global_scope_id &&
            (symbol->category == SYMBOL_GLOBAL_VARIABLE ||
             symbol->category == SYMBOL_VECTOR)) {
            global = add_size(global, type_size(check->semantic,
                              symbol->type_id, 0U, &overflow), &overflow);
        }
    }
    for (i = 0U; i < symbols->scope_count; ++i) {
        const Scope *scope = &symbols->scopes[i];
        size_t local = 0U;
        if (scope->kind != SCOPE_FUNCTION) continue;
        for (j = 0U; j < symbols->count; ++j) {
            const Symbol *symbol = &symbols->items[j];
            if (symbol->scope_id == scope->id &&
                (symbol->category == SYMBOL_PARAMETER ||
                 symbol->category == SYMBOL_LOCAL_VARIABLE ||
                 symbol->category == SYMBOL_VECTOR)) {
                local = add_size(local, type_size(check->semantic,
                                 symbol->type_id, 0U, &overflow), &overflow);
            }
        }
        if (local > peak) peak = local;
    }
    total = add_size(global, peak, &overflow);
    if (overflow) {
        target_error(check, root->span, "E-TGT-002",
                     "estimativa de RAM excede a capacidade de calculo");
    } else if (total > ram_limit) {
        if (!diagnostic_bag_add(check->diagnostics, check->source, root->span,
                                DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SEMANTIC,
                                "E-TGT-002",
                                "estimativa de RAM %zu bytes excede limite %zu bytes",
                                total, ram_limit)) check->failed = 1;
    }
}

FrontendStatus target_validate(const SourceFile *source, const AstNode *root,
                               const SemanticResult *semantic,
                               DiagnosticBag *diagnostics, int esp32,
                               size_t ram_limit)
{
    TargetCheck check;
    size_t errors_before;
    if (source == NULL || root == NULL || root->kind != AST_PROGRAM ||
        semantic == NULL || !semantic->analyzed || diagnostics == NULL)
        return FRONTEND_STATUS_INTERNAL_ERROR;
    check.source = source;
    check.semantic = semantic;
    check.diagnostics = diagnostics;
    check.esp32 = esp32;
    check.failed = 0;
    errors_before = diagnostics->error_count;
    check_node(&check, root);
    if (esp32 && !check.failed) check_ram(&check, root, ram_limit);
    if (check.failed) return FRONTEND_STATUS_INTERNAL_ERROR;
    return diagnostics->error_count == errors_before
               ? FRONTEND_STATUS_OK : FRONTEND_STATUS_SOURCE_ERROR;
}
