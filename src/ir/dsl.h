#ifndef MACARONICA_IR_DSL_H
#define MACARONICA_IR_DSL_H

#include "../parser/ast.h"

/* Visita somente nucleos DSL; destination existe apenas para operacao vetorial.
 * A AST continua sendo a unica representacao interna do compilador. */
typedef void (*DslVisitor)(const AstNode *operation, const AstNode *destination,
                           void *context);
void dsl_visit(const AstNode *node, DslVisitor visitor, void *context);

#endif
