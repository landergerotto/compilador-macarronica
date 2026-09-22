#ifndef MACARONICA_LEXER_LEXER_H
#define MACARONICA_LEXER_LEXER_H

/*
 * O lexer pega o texto puro e separa em tokens. Aqui ainda nao importa se a
 * ordem deles faz sentido ou se os tipos combinam.
 */

#include "../common/diagnostic.h"
#include "../common/status.h"
#include "token.h"

#ifndef MACARONICA_LEXER_DEBUG
#define MACARONICA_LEXER_DEBUG 1
#endif

/*
 * Le a fonte inteira e coloca os tokens em out_tokens. Mesmo quando encontra
 * algo errado, guarda TOKEN_INVALID, e no fim sempre coloca TOKEN_EOF.
 *
 * A fonte precisa continuar viva porque os tokens apontam direto pra ela.
 * Os erros encontrados vao para diagnostics.
 */
FrontendStatus lexer_scan_all(const SourceFile *source,
                              DiagnosticBag *diagnostics,
                              TokenBuffer *out_tokens);

#endif
