#ifndef MACARONICA_PARSER_PARSER_H
#define MACARONICA_PARSER_PARSER_H

#include "../common/diagnostic.h"
#include "../common/status.h"
#include "../lexer/token.h"
#include "ast.h"

#ifndef MACARONICA_PARSER_DEBUG
#define MACARONICA_PARSER_DEBUG 1
#endif

/*
 * Consome somente os tokens, sem reler SourceFile.buffer.
 * Em sucesso ou erro sintático recuperável, out_program recebe a AST e sua posse.
 * Distingue sucesso, rejeição da fonte e falha interna ou de alocação.
 */
FrontendStatus parser_parse(const SourceFile *source,
                            const TokenBuffer *tokens,
                            DiagnosticBag *diagnostics,
                            AstNode **out_program);

#endif
