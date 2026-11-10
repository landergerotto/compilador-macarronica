#ifndef MACARONICA_PARSER_PARSER_H
#define MACARONICA_PARSER_PARSER_H

/*
 * O parser pega os tokens, confere se eles seguem a gramatica e monta a AST.
 */

#include "../common/diagnostic.h"
#include "../common/status.h"
#include "../lexer/token.h"
#include "ast.h"

#ifndef MACARONICA_PARSER_DEBUG
#define MACARONICA_PARSER_DEBUG 1
#endif

/*
 * Ele so olha os tokens; source entra aqui para localizar os erros. A AST sai
 * por out_program e passa a ser responsabilidade de quem chamou.
 *
 * Mesmo com erro recuperavel pode sair uma arvore parcial. Se out_program nao
 * for NULL no final, ela tambem precisa passar por ast_node_destroy.
 */
FrontendStatus parser_parse(const SourceFile *source,
                            const TokenBuffer *tokens,
                            DiagnosticBag *diagnostics,
                            AstNode **out_program);

#endif
