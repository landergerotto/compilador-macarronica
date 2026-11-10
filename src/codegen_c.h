#ifndef MACARONICA_CODEGEN_C_H
#define MACARONICA_CODEGEN_C_H

#include "common/diagnostic.h"
#include "common/status.h"
#include "parser/ast.h"
#include "semantic/semantic.h"

#include <stdio.h>

/* Emite C99 diretamente da AST semanticamente aceita para o alvo ESP-IDF. */
FrontendStatus codegen_c_emit(const SourceFile *source, const AstNode *program,
                              const SemanticResult *semantic,
                              DiagnosticBag *diagnostics, FILE *output);

#endif
