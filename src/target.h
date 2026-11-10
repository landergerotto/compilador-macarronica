#ifndef MACARONICA_TARGET_H
#define MACARONICA_TARGET_H

#include "common/diagnostic.h"
#include "common/status.h"
#include "parser/ast.h"
#include "semantic/semantic.h"

/* Executar depois de semantic_analyze bem-sucedido. ram_limit e em bytes. */
FrontendStatus target_validate(const SourceFile *source, const AstNode *root,
                               const SemanticResult *semantic,
                               DiagnosticBag *diagnostics, int esp32,
                               size_t ram_limit);

#endif
