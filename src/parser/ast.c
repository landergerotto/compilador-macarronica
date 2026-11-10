#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int reserve(void **items, size_t *capacity, size_t count, size_t item_size)
{
    size_t next_capacity;
    void *next_items;

    if (count <= *capacity) {
        return 1;
    }
    next_capacity = *capacity == 0U ? 4U : *capacity;
    while (next_capacity < count) {
        if (next_capacity > ((size_t)-1) / 2U) {
            return 0;
        }
        next_capacity *= 2U;
    }
    if (item_size != 0U && next_capacity > ((size_t)-1) / item_size) {
        return 0;
    }
    next_items = realloc(*items, next_capacity * item_size);
    if (next_items == NULL) {
        return 0;
    }
    *items = next_items;
    *capacity = next_capacity;
    return 1;
}

AstNode *ast_node_create(AstKind kind, SourceSpan span, size_t node_id)
{
    AstNode *node = (AstNode *)calloc(1U, sizeof(*node));
    if (node == NULL) {
        return NULL;
    }
    node->kind = kind;
    node->span = span;
    node->node_id = node_id;
    node->semantic.symbol_id = AST_ID_NONE;
    node->semantic.type_id = AST_ID_NONE;
    return node;
}

int ast_node_list_append(AstNodeList *list, AstNode *node)
{
    if (list == NULL ||
        !reserve((void **)&list->items, &list->capacity,
                 list->count + 1U, sizeof(*list->items))) {
        return 0;
    }
    list->items[list->count++] = node;
    return 1;
}

int ast_declared_name_list_append(AstDeclaredNameList *list,
                                  char *owned_name,
                                  SourceSpan span)
{
    AstDeclaredName *name;

    if (list == NULL ||
        !reserve((void **)&list->items, &list->capacity,
                 list->count + 1U, sizeof(*list->items))) {
        return 0;
    }
    name = &list->items[list->count++];
    name->name = owned_name;
    name->span = span;
    name->symbol_id = AST_ID_NONE;
    return 1;
}

int ast_if_branch_list_append(AstIfBranchList *list,
                              AstNode *condition,
                              AstNode *body,
                              SourceSpan span)
{
    AstIfBranch *branch;

    if (list == NULL ||
        !reserve((void **)&list->items, &list->capacity,
                 list->count + 1U, sizeof(*list->items))) {
        return 0;
    }
    branch = &list->items[list->count++];
    branch->condition = condition;
    branch->body = body;
    branch->span = span;
    return 1;
}

void ast_type_ref_destroy(AstTypeRef *type)
{
    if (type == NULL) {
        return;
    }
    free(type->name);
    type->name = NULL;
    type->resolved_type_id = AST_ID_NONE;
}

static void destroy_node_list(AstNodeList *list)
{
    size_t index;
    for (index = 0U; index < list->count; ++index) {
        ast_node_destroy(list->items[index]);
    }
    free(list->items);
}

static void destroy_names(AstDeclaredNameList *list)
{
    size_t index;
    for (index = 0U; index < list->count; ++index) {
        free(list->items[index].name);
    }
    free(list->items);
}

void ast_node_destroy(AstNode *node)
{
    size_t index;

    if (node == NULL) {
        return;
    }
    switch (node->kind) {
    case AST_PROGRAM:
        destroy_node_list(&node->as.program.declarations);
        break;
    case AST_FUNCTION_DECL:
        ast_type_ref_destroy(&node->as.function_decl.return_type);
        free(node->as.function_decl.name);
        destroy_node_list(&node->as.function_decl.parameters);
        ast_node_destroy(node->as.function_decl.body);
        break;
    case AST_STRUCT_DECL:
        free(node->as.struct_decl.name);
        destroy_node_list(&node->as.struct_decl.fields);
        break;
    case AST_VAR_DECL:
        ast_type_ref_destroy(&node->as.var_decl.type);
        destroy_names(&node->as.var_decl.names);
        break;
    case AST_VECTOR_DECL:
        ast_type_ref_destroy(&node->as.vector_decl.type);
        destroy_names(&node->as.vector_decl.names);
        break;
    case AST_BLOCK:
        destroy_node_list(&node->as.block.statements);
        break;
    case AST_ASSIGNMENT:
        ast_node_destroy(node->as.assignment.target);
        ast_node_destroy(node->as.assignment.value);
        break;
    case AST_IF_STMT:
        for (index = 0U; index < node->as.if_stmt.branches.count; ++index) {
            ast_node_destroy(node->as.if_stmt.branches.items[index].condition);
            ast_node_destroy(node->as.if_stmt.branches.items[index].body);
        }
        free(node->as.if_stmt.branches.items);
        ast_node_destroy(node->as.if_stmt.else_body);
        break;
    case AST_WHILE_STMT:
        ast_node_destroy(node->as.while_stmt.condition);
        ast_node_destroy(node->as.while_stmt.body);
        break;
    case AST_FOR_STMT:
        ast_node_destroy(node->as.for_stmt.initializer);
        ast_node_destroy(node->as.for_stmt.condition);
        ast_node_destroy(node->as.for_stmt.iteration);
        ast_node_destroy(node->as.for_stmt.body);
        break;
    case AST_RETURN_STMT:
        ast_node_destroy(node->as.return_stmt.value);
        break;
    case AST_EXPR_STMT:
        ast_node_destroy(node->as.expr_stmt.expression);
        break;
    case AST_CALL_EXPR:
        free(node->as.call_expr.callee);
        destroy_node_list(&node->as.call_expr.arguments);
        break;
    case AST_INDEX_EXPR:
        ast_node_destroy(node->as.index_expr.base);
        ast_node_destroy(node->as.index_expr.index);
        break;
    case AST_FIELD_EXPR:
        ast_node_destroy(node->as.field_expr.base);
        free(node->as.field_expr.field_name);
        break;
    case AST_BINARY_EXPR:
        ast_node_destroy(node->as.binary_expr.left);
        ast_node_destroy(node->as.binary_expr.right);
        break;
    case AST_UNARY_EXPR:
        ast_node_destroy(node->as.unary_expr.operand);
        break;
    case AST_LITERAL_EXPR:
        if (node->as.literal_expr.literal_kind == AST_LITERAL_STRING) {
            free(node->as.literal_expr.value.string);
        }
        break;
    case AST_IDENTIFIER_EXPR:
        free(node->as.identifier_expr.name);
        break;
    }
    free(node);
}

const char *ast_kind_name(AstKind kind)
{
    static const char *const names[] = {
        "Program", "FunctionDecl", "StructDecl", "VarDecl", "VectorDecl",
        "Block", "Assignment", "IfStmt", "WhileStmt", "ForStmt",
        "ReturnStmt", "ExprStmt", "CallExpr", "IndexExpr", "BinaryExpr",
        "UnaryExpr", "LiteralExpr", "IdentifierExpr", "FieldExpr"
    };
    if ((size_t)kind >= sizeof(names) / sizeof(names[0])) {
        return "Unknown";
    }
    return names[(size_t)kind];
}

const char *ast_operator_name(AstOperator operator_kind)
{
    static const char *const names[] = {
        "none", "+", "-", "*", "/", "//", "^", "<", ">", "==", "!",
        "NAO", "E", "XOR", "OU", "raiz", "+ (unario)", "- (unario)"
    };
    if ((size_t)operator_kind >= sizeof(names) / sizeof(names[0])) {
        return "unknown";
    }
    return names[(size_t)operator_kind];
}

#define AST_DUMP_MAX_DEPTH 512U

static void dump_indent(FILE *stream, size_t depth)
{
    size_t index;

    for (index = 0U; index < depth; ++index) {
        fputs("  ", stream);
    }
}

static void dump_quoted(FILE *stream, const char *text)
{
    const unsigned char *cursor;

    fputc('"', stream);
    if (text != NULL) {
        cursor = (const unsigned char *)text;
        while (*cursor != '\0') {
            switch (*cursor) {
            case '\\':
                fputs("\\\\", stream);
                break;
            case '"':
                fputs("\\\"", stream);
                break;
            case '\n':
                fputs("\\n", stream);
                break;
            case '\r':
                fputs("\\r", stream);
                break;
            case '\t':
                fputs("\\t", stream);
                break;
            case '\b':
                fputs("\\b", stream);
                break;
            case '\f':
                fputs("\\f", stream);
                break;
            default:
                if (*cursor < 0x20U || *cursor == 0x7fU) {
                    fprintf(stream, "\\x%02X", (unsigned int)*cursor);
                } else {
                    fputc((int)*cursor, stream);
                }
                break;
            }
            ++cursor;
        }
    }
    fputc('"', stream);
}

static void dump_span(FILE *stream, SourceSpan span)
{
    fprintf(stream,
            "%zu:%zu@%zu..%zu:%zu@%zu",
            span.start.line,
            span.start.column,
            span.start.offset,
            span.end.line,
            span.end.column,
            span.end.offset);
}

static int semantic_info_present(const AstSemanticInfo *semantic)
{
    return semantic->symbol_id != AST_ID_NONE ||
           semantic->type_id != AST_ID_NONE ||
           semantic->flags != 0U;
}

static void dump_optional_id(FILE *stream, const char *name, size_t value)
{
    if (value != AST_ID_NONE) {
        fprintf(stream, " %s=%zu", name, value);
    }
}

static void dump_semantic(FILE *stream,
                          const AstSemanticInfo *semantic,
                          size_t depth)
{
    if (!semantic_info_present(semantic)) {
        return;
    }
    dump_indent(stream, depth);
    fputs("semantic", stream);
    dump_optional_id(stream, "symbol", semantic->symbol_id);
    dump_optional_id(stream, "type", semantic->type_id);
    if (semantic->flags != 0U) {
        fprintf(stream, " flags=%u", semantic->flags);
    }
    fputc('\n', stream);
}

static void dump_type_ref(FILE *stream,
                          const char *label,
                          const AstTypeRef *type,
                          size_t depth)
{
    dump_indent(stream, depth);
    fprintf(stream, "%s name=", label);
    dump_quoted(stream, type->name);
    fputs(" span=", stream);
    dump_span(stream, type->span);
    dump_optional_id(stream, "resolved_type", type->resolved_type_id);
    fputc('\n', stream);
}

static void dump_declared_names(FILE *stream,
                                const AstDeclaredNameList *names,
                                size_t depth)
{
    size_t index;

    dump_indent(stream, depth);
    fprintf(stream, "names count=%zu\n", names->count);
    for (index = 0U; index < names->count; ++index) {
        const AstDeclaredName *name = &names->items[index];
        dump_indent(stream, depth + 1U);
        fprintf(stream, "[%zu] name=", index);
        dump_quoted(stream, name->name);
        fputs(" span=", stream);
        dump_span(stream, name->span);
        dump_optional_id(stream, "symbol", name->symbol_id);
        fputc('\n', stream);
    }
}

static void dump_node(const AstNode *node, FILE *stream, size_t depth);

static void dump_child(FILE *stream,
                       const char *label,
                       const AstNode *child,
                       size_t depth)
{
    dump_indent(stream, depth);
    fprintf(stream, "%s\n", label);
    dump_node(child, stream, depth + 1U);
}

static void dump_node_list(FILE *stream,
                           const char *label,
                           const AstNodeList *list,
                           size_t depth)
{
    size_t index;

    dump_indent(stream, depth);
    fprintf(stream, "%s count=%zu\n", label, list->count);
    for (index = 0U; index < list->count; ++index) {
        dump_indent(stream, depth + 1U);
        fprintf(stream, "[%zu]\n", index);
        dump_node(list->items[index], stream, depth + 2U);
    }
}

static const char *literal_kind_name(AstLiteralKind kind)
{
    static const char *const names[] = {
        "integer", "real", "string", "boolean"
    };

    if ((size_t)kind >= sizeof(names) / sizeof(names[0])) {
        return "unknown";
    }
    return names[(size_t)kind];
}

static void dump_real(FILE *stream, double value)
{
    char buffer[64];
    char *comma;

    (void)snprintf(buffer, sizeof(buffer), "%.17g", value);
    comma = strchr(buffer, ',');
    if (comma != NULL) {
        *comma = '.';
    }
    fputs(buffer, stream);
}

static void dump_literal(FILE *stream, const AstNode *node, size_t depth)
{
    dump_indent(stream, depth);
    fprintf(stream, "literal kind=%s value=",
            literal_kind_name(node->as.literal_expr.literal_kind));
    switch (node->as.literal_expr.literal_kind) {
    case AST_LITERAL_INTEGER:
        fprintf(stream, "%lld", node->as.literal_expr.value.integer);
        break;
    case AST_LITERAL_REAL:
        dump_real(stream, node->as.literal_expr.value.real);
        break;
    case AST_LITERAL_STRING:
        dump_quoted(stream, node->as.literal_expr.value.string);
        break;
    case AST_LITERAL_BOOLEAN:
        fputs(node->as.literal_expr.value.boolean ? "true" : "false", stream);
        break;
    default:
        fputs("unknown", stream);
        break;
    }
    fputc('\n', stream);
}

static void dump_node(const AstNode *node, FILE *stream, size_t depth)
{
    size_t index;

    if (node == NULL) {
        dump_indent(stream, depth);
        fputs("null\n", stream);
        return;
    }
    if (depth > AST_DUMP_MAX_DEPTH) {
        dump_indent(stream, depth);
        fputs("<maximum-depth-reached>\n", stream);
        return;
    }

    dump_indent(stream, depth);
    fprintf(stream, "%s id=%zu span=", ast_kind_name(node->kind), node->node_id);
    dump_span(stream, node->span);
    fputc('\n', stream);
    dump_semantic(stream, &node->semantic, depth + 1U);

    switch (node->kind) {
    case AST_PROGRAM:
        dump_node_list(stream, "declarations",
                       &node->as.program.declarations, depth + 1U);
        break;
    case AST_FUNCTION_DECL:
        dump_indent(stream, depth + 1U);
        fputs("name=", stream);
        dump_quoted(stream, node->as.function_decl.name);
        fputs(" span=", stream);
        dump_span(stream, node->as.function_decl.name_span);
        fprintf(stream, " principal=%d\n",
                node->as.function_decl.is_principal ? 1 : 0);
        dump_type_ref(stream, "return_type",
                      &node->as.function_decl.return_type, depth + 1U);
        dump_node_list(stream, "parameters",
                       &node->as.function_decl.parameters, depth + 1U);
        dump_child(stream, "body", node->as.function_decl.body, depth + 1U);
        break;
    case AST_STRUCT_DECL:
        dump_indent(stream, depth + 1U);
        fputs("name=", stream);
        dump_quoted(stream, node->as.struct_decl.name);
        fputs(" span=", stream);
        dump_span(stream, node->as.struct_decl.name_span);
        fputc('\n', stream);
        dump_node_list(stream, "fields", &node->as.struct_decl.fields,
                       depth + 1U);
        break;
    case AST_VAR_DECL:
        dump_type_ref(stream, "type", &node->as.var_decl.type, depth + 1U);
        dump_declared_names(stream, &node->as.var_decl.names, depth + 1U);
        break;
    case AST_VECTOR_DECL:
        dump_type_ref(stream, "type", &node->as.vector_decl.type, depth + 1U);
        dump_indent(stream, depth + 1U);
        fprintf(stream, "size=%lld span=", node->as.vector_decl.size);
        dump_span(stream, node->as.vector_decl.size_span);
        fputc('\n', stream);
        dump_declared_names(stream, &node->as.vector_decl.names, depth + 1U);
        break;
    case AST_BLOCK:
        dump_node_list(stream, "statements", &node->as.block.statements,
                       depth + 1U);
        break;
    case AST_ASSIGNMENT:
        dump_child(stream, "target", node->as.assignment.target, depth + 1U);
        dump_child(stream, "value", node->as.assignment.value, depth + 1U);
        break;
    case AST_IF_STMT:
        dump_indent(stream, depth + 1U);
        fprintf(stream, "branches count=%zu\n",
                node->as.if_stmt.branches.count);
        for (index = 0U; index < node->as.if_stmt.branches.count; ++index) {
            const AstIfBranch *branch = &node->as.if_stmt.branches.items[index];
            dump_indent(stream, depth + 2U);
            fprintf(stream, "[%zu] span=", index);
            dump_span(stream, branch->span);
            fputc('\n', stream);
            dump_child(stream, "condition", branch->condition, depth + 3U);
            dump_child(stream, "body", branch->body, depth + 3U);
        }
        dump_child(stream, "else_body", node->as.if_stmt.else_body,
                   depth + 1U);
        break;
    case AST_WHILE_STMT:
        dump_child(stream, "condition", node->as.while_stmt.condition,
                   depth + 1U);
        dump_child(stream, "body", node->as.while_stmt.body, depth + 1U);
        break;
    case AST_FOR_STMT:
        dump_child(stream, "initializer", node->as.for_stmt.initializer,
                   depth + 1U);
        dump_child(stream, "condition", node->as.for_stmt.condition,
                   depth + 1U);
        dump_child(stream, "iteration", node->as.for_stmt.iteration,
                   depth + 1U);
        dump_child(stream, "body", node->as.for_stmt.body, depth + 1U);
        break;
    case AST_RETURN_STMT:
        dump_child(stream, "value", node->as.return_stmt.value, depth + 1U);
        break;
    case AST_EXPR_STMT:
        dump_child(stream, "expression", node->as.expr_stmt.expression,
                   depth + 1U);
        break;
    case AST_CALL_EXPR:
        dump_indent(stream, depth + 1U);
        fputs("callee=", stream);
        dump_quoted(stream, node->as.call_expr.callee);
        fputs(" span=", stream);
        dump_span(stream, node->as.call_expr.callee_span);
        fputc('\n', stream);
        dump_node_list(stream, "arguments", &node->as.call_expr.arguments,
                       depth + 1U);
        break;
    case AST_INDEX_EXPR:
        dump_child(stream, "base", node->as.index_expr.base, depth + 1U);
        dump_child(stream, "index", node->as.index_expr.index, depth + 1U);
        break;
    case AST_FIELD_EXPR:
        dump_child(stream, "base", node->as.field_expr.base, depth + 1U);
        dump_indent(stream, depth + 1U);
        fputs("field=", stream);
        dump_quoted(stream, node->as.field_expr.field_name);
        fputs(" span=", stream);
        dump_span(stream, node->as.field_expr.field_span);
        fputc('\n', stream);
        break;
    case AST_BINARY_EXPR:
        dump_indent(stream, depth + 1U);
        fprintf(stream, "operator=%s span=",
                ast_operator_name(node->as.binary_expr.operator_kind));
        dump_span(stream, node->as.binary_expr.operator_span);
        fputc('\n', stream);
        dump_child(stream, "left", node->as.binary_expr.left, depth + 1U);
        dump_child(stream, "right", node->as.binary_expr.right, depth + 1U);
        break;
    case AST_UNARY_EXPR:
        dump_indent(stream, depth + 1U);
        fprintf(stream, "operator=%s span=",
                ast_operator_name(node->as.unary_expr.operator_kind));
        dump_span(stream, node->as.unary_expr.operator_span);
        fputc('\n', stream);
        dump_child(stream, "operand", node->as.unary_expr.operand, depth + 1U);
        break;
    case AST_LITERAL_EXPR:
        dump_literal(stream, node, depth + 1U);
        break;
    case AST_IDENTIFIER_EXPR:
        dump_indent(stream, depth + 1U);
        fputs("name=", stream);
        dump_quoted(stream, node->as.identifier_expr.name);
        fputc('\n', stream);
        break;
    default:
        dump_indent(stream, depth + 1U);
        fprintf(stream, "unknown_kind=%d\n", (int)node->kind);
        break;
    }
}

void ast_dump(const AstNode *node, FILE *stream)
{
    if (stream == NULL) {
        return;
    }
    dump_node(node, stream, 0U);
}
