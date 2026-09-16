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
 * Parses only the supplied tokens; it never scans SourceFile.buffer.
 * On success or recoverable syntax failure, out_program receives an owned AST.
 * Distinguishes success, source rejection and internal/allocation failure.
 */
FrontendStatus parser_parse(const SourceFile *source,
                            const TokenBuffer *tokens,
                            DiagnosticBag *diagnostics,
                            AstNode **out_program);

#endif
