#ifndef MACARONICA_LEXER_LEXER_H
#define MACARONICA_LEXER_LEXER_H

#include "../common/diagnostic.h"
#include "../common/status.h"
#include "token.h"

#ifndef MACARONICA_LEXER_DEBUG
#define MACARONICA_LEXER_DEBUG 1
#endif

/*
 * Appends every token, including INVALIDO and a final EOF, to out_tokens.
 * The SourceFile must outlive out_tokens because token lexemes are slices.
 * Distinguishes success, source rejection and internal/allocation failure.
 */
FrontendStatus lexer_scan_all(const SourceFile *source,
                              DiagnosticBag *diagnostics,
                              TokenBuffer *out_tokens);

#endif
