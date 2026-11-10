#ifndef MACARONICA_SEMANTIC_SEMANTIC_H
#define MACARONICA_SEMANTIC_SEMANTIC_H

/*
 * Depois que a AST esta pronta, a semantica confere nomes, tipos, chamadas,
 * escopos e retornos. Ela tambem anota a arvore com o que descobriu.
 */

#include "../common/diagnostic.h"
#include "../common/source.h"
#include "../common/status.h"
#include "../parser/ast.h"
#include "symbol.h"
#include "type.h"

#include <stdio.h>

#ifndef MACARONICA_SEMANTIC_DEBUG
#define MACARONICA_SEMANTIC_DEBUG 1
#endif

/*
 * Flags que a semantica pode juntar no mesmo no: se ele recebe valor, se pode
 * ser lido, se e constante, se ja deu erro ou se representa um vetor inteiro.
 */
#define AST_SEMANTIC_LVALUE 0x01U
#define AST_SEMANTIC_RVALUE 0x02U
#define AST_SEMANTIC_CONSTANT 0x04U
#define AST_SEMANTIC_ERROR 0x08U
#define AST_SEMANTIC_ARRAY 0x10U

/*
 * Tudo que a semantica montou fica aqui. Como a AST guarda IDs dessas tabelas,
 * nao da pra destruir SemanticResult e continuar usando a AST anotada.
 */
typedef struct {
    TypeTable types;
    SymbolTable symbols;
    size_t global_scope_id;
    size_t principal_symbol_id;
    size_t semantic_error_count;
    int analyzed;
} SemanticResult;

/* Prepara as tabelas e cria o escopo global. */
int semantic_result_init(SemanticResult *result);

/* Limpa tudo que foi criado pela semantica. */
void semantic_result_destroy(SemanticResult *result);

/*
 * Percorre a AST, coloca os erros em diagnostics e preenche os IDs semanticos.
 * result precisa estar inicializado e so pode ser analisado uma vez.
 *
 * Se o codigo tiver erro, as tabelas podem ficar parciais, mas ainda servem
 * para o dump e precisam ser destruidas normalmente.
 */
FrontendStatus semantic_analyze(const SourceFile *source,
                                AstNode *root,
                                DiagnosticBag *diagnostics,
                                SemanticResult *result);

/* Mostra as tabelas no formato do --dump-symbols. */
void semantic_dump_symbols(const SemanticResult *result, FILE *stream);

#endif
