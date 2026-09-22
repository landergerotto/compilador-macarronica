#ifndef MACARONICA_COMMON_DIAGNOSTIC_H
#define MACARONICA_COMMON_DIAGNOSTIC_H

#include "source.h"

#include <stddef.h>
#include <stdio.h>

typedef enum {
    DIAGNOSTIC_ERROR,
    DIAGNOSTIC_WARNING
} DiagnosticSeverity;

typedef enum {
    DIAGNOSTIC_PHASE_LEXICAL,
    DIAGNOSTIC_PHASE_SYNTACTIC,
    DIAGNOSTIC_PHASE_SEMANTIC
} DiagnosticPhase;

typedef struct {
    DiagnosticSeverity severity;
    DiagnosticPhase phase;
    const char *code;            /* Texto estavel sem posse, como E-LEX-001. */
    char *message;               /* Pertence ao conjunto de diagnosticos. */
    const SourceFile *source;    /* Sem posse; deve sobreviver ao conjunto. */
    SourceSpan span;
} Diagnostic;

typedef struct {
    Diagnostic *items;
    size_t count;
    size_t capacity;
    size_t error_count;
} DiagnosticBag;

void diagnostic_bag_init(DiagnosticBag *bag);
void diagnostic_bag_destroy(DiagnosticBag *bag);
int diagnostic_bag_add(DiagnosticBag *bag,
                       const SourceFile *source,
                       SourceSpan span,
                       DiagnosticSeverity severity,
                       DiagnosticPhase phase,
                       const char *code,
                       const char *format,
                       ...);
int diagnostic_bag_has_errors(const DiagnosticBag *bag);
void diagnostic_print(const Diagnostic *diagnostic, FILE *stream);
void diagnostic_bag_print(const DiagnosticBag *bag, FILE *stream);
const char *diagnostic_phase_name(DiagnosticPhase phase);

#endif
