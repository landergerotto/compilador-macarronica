#ifndef MACARONICA_LEXER_LEXER_H
#define MACARONICA_LEXER_LEXER_H

#include "../common/diagnostic.h"
#include "../common/status.h"
#include "token.h"

#ifndef MACARONICA_LEXER_DEBUG
#define MACARONICA_LEXER_DEBUG 1
#endif

/*
 * Adiciona todos os tokens, inclusive INVALIDO e o EOF final, a out_tokens.
 * SourceFile deve sobreviver aos tokens, pois os lexemas sao fatias da fonte.
 * Distingue sucesso, rejeicao da fonte e falha interna ou de alocacao.
 */
FrontendStatus lexer_scan_all(const SourceFile *source,
                              DiagnosticBag *diagnostics,
                              TokenBuffer *out_tokens);

#endif
