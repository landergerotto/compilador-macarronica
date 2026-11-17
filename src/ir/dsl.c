#include "dsl.h"
#include "../semantic/semantic.h"

void dsl_visit(const AstNode *node, DslVisitor visitor, void *context)
{
    const AstNodeList *list = NULL;
    size_t i;
    if (node == NULL) return;
    switch (node->kind) {
    case AST_PROGRAM: list = &node->as.program.declarations; break;
    case AST_FUNCTION_DECL: dsl_visit(node->as.function_decl.body, visitor, context); break;
    case AST_BLOCK: list = &node->as.block.statements; break;
    case AST_ASSIGNMENT:
        if ((node->as.assignment.value->semantic.flags & AST_SEMANTIC_DSL) != 0U &&
            node->as.assignment.value->kind == AST_BINARY_EXPR)
            visitor(node->as.assignment.value, node->as.assignment.target, context);
        dsl_visit(node->as.assignment.target, visitor, context);
        dsl_visit(node->as.assignment.value, visitor, context);
        break;
    case AST_CALL_EXPR:
        if ((node->semantic.flags & AST_SEMANTIC_DSL) != 0U)
            visitor(node, NULL, context);
        list = &node->as.call_expr.arguments;
        break;
    case AST_BINARY_EXPR:
        dsl_visit(node->as.binary_expr.left, visitor, context);
        dsl_visit(node->as.binary_expr.right, visitor, context);
        break;
    case AST_UNARY_EXPR: dsl_visit(node->as.unary_expr.operand, visitor, context); break;
    case AST_INDEX_EXPR:
        dsl_visit(node->as.index_expr.base, visitor, context);
        dsl_visit(node->as.index_expr.index, visitor, context);
        break;
    case AST_FIELD_EXPR: dsl_visit(node->as.field_expr.base, visitor, context); break;
    case AST_RETURN_STMT: dsl_visit(node->as.return_stmt.value, visitor, context); break;
    case AST_EXPR_STMT: dsl_visit(node->as.expr_stmt.expression, visitor, context); break;
    case AST_IF_STMT:
        for (i = 0U; i < node->as.if_stmt.branches.count; ++i) {
            dsl_visit(node->as.if_stmt.branches.items[i].condition, visitor, context);
            dsl_visit(node->as.if_stmt.branches.items[i].body, visitor, context);
        }
        dsl_visit(node->as.if_stmt.else_body, visitor, context);
        break;
    case AST_WHILE_STMT:
        dsl_visit(node->as.while_stmt.condition, visitor, context);
        dsl_visit(node->as.while_stmt.body, visitor, context);
        break;
    case AST_FOR_STMT:
        dsl_visit(node->as.for_stmt.initializer, visitor, context);
        dsl_visit(node->as.for_stmt.condition, visitor, context);
        dsl_visit(node->as.for_stmt.iteration, visitor, context);
        dsl_visit(node->as.for_stmt.body, visitor, context);
        break;
    default: break;
    }
    if (list != NULL)
        for (i = 0U; i < list->count; ++i) dsl_visit(list->items[i], visitor, context);
}
