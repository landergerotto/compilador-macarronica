#ifndef MACARONICA_COMMON_DIAGNOSTIC_H
#define MACARONICA_COMMON_DIAGNOSTIC_H

#include "source.h"

#include <stddef.h>
#include <stdio.h>

/* Um diagnostico pode dar erro fatal ou apenas informativo. */
typedef enum {
    DIAGNOSTIC_ERROR,
    DIAGNOSTIC_WARNING
} DiagnosticSeverity;

/* Em qual fase que identificou o problema. */
typedef enum {
    DIAGNOSTIC_PHASE_LEXICAL,
    DIAGNOSTIC_PHASE_SYNTACTIC,
    DIAGNOSTIC_PHASE_SEMANTIC
} DiagnosticPhase;

/*
 * Um erro ou aviso associado a um trecho da fonte. A mensagem pertence ao
 * DiagnosticBag; code e source sao referencias sem posse.
 */
typedef struct {
    DiagnosticSeverity severity;
    DiagnosticPhase phase;
    const char *code;            /* Texto estavel sem posse, como E-LEX-001. */
    char *message;               /* Pertence ao conjunto de diagnosticos. */
    const SourceFile *source;    /* Sem posse; deve sobreviver ao conjunto. */
    SourceSpan span;
} Diagnostic;

/*
 * Vetor dinamico que acumula diagnosticos durante o pipeline. count informa
 * quantos itens existem e error_count conta somente os de severidade ERROR.
 */
typedef struct {
    Diagnostic *items;
    size_t count;
    size_t capacity;
    size_t error_count;
} DiagnosticBag;

/* Inicializa uma lista vazia. Deve ser pareada com diagnostic_bag_destroy. */
void diagnostic_bag_init(DiagnosticBag *bag);

/* Libera as mensagens e o vetor, mas nao libera o arquivo lido (Source) referenciados. */
void diagnostic_bag_destroy(DiagnosticBag *bag);

/*
 * Formata e adiciona um diagnostico. Retorna 1 no sucesso e 0 se nao houver
 * memoria.
 */
int diagnostic_bag_add(DiagnosticBag *bag,
                       const SourceFile *source,
                       SourceSpan span,
                       DiagnosticSeverity severity,
                       DiagnosticPhase phase,
                       const char *code,
                       const char *format,
                       ...);

/* Retorna verdadeiro quando pelo menos um erro foi encontrado. */
int diagnostic_bag_has_errors(const DiagnosticBag *bag);

/* PRINTF KK */
void diagnostic_print(const Diagnostic *diagnostic, FILE *stream);
void diagnostic_bag_print(const DiagnosticBag *bag, FILE *stream);

/* Tradutor de nome de fase pra pt-br */
const char *diagnostic_phase_name(DiagnosticPhase phase);

#endif
