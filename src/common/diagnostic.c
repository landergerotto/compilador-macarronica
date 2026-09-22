#include "diagnostic.h"

#include <stdarg.h>
#include <stdlib.h>

static int reserve_diagnostics(DiagnosticBag *bag, size_t needed)
{
    Diagnostic *items;
    size_t capacity;

    if (needed <= bag->capacity) {
        return 1;
    }
    capacity = bag->capacity == 0U ? 8U : bag->capacity;
    while (capacity < needed) {
        if (capacity > ((size_t)-1) / 2U) {
            return 0;
        }
        capacity *= 2U;
    }
    if (capacity > ((size_t)-1) / sizeof(*items)) {
        return 0;
    }
    items = (Diagnostic *)realloc(bag->items, capacity * sizeof(*items));
    if (items == NULL) {
        return 0;
    }
    bag->items = items;
    bag->capacity = capacity;
    return 1;
}

void diagnostic_bag_init(DiagnosticBag *bag)
{
    if (bag == NULL) {
        return;
    }
    bag->items = NULL;
    bag->count = 0U;
    bag->capacity = 0U;
    bag->error_count = 0U;
}

void diagnostic_bag_destroy(DiagnosticBag *bag)
{
    size_t index;

    if (bag == NULL) {
        return;
    }
    for (index = 0U; index < bag->count; ++index) {
        free(bag->items[index].message);
    }
    free(bag->items);
    diagnostic_bag_init(bag);
}

int diagnostic_bag_add(DiagnosticBag *bag,
                       const SourceFile *source,
                       SourceSpan span,
                       DiagnosticSeverity severity,
                       DiagnosticPhase phase,
                       const char *code,
                       const char *format,
                       ...)
{
    va_list arguments;
    va_list copy;
    int required;
    int written;
    char *message;
    Diagnostic *diagnostic;

    if (bag == NULL || format == NULL || bag->count == (size_t)-1 ||
        !reserve_diagnostics(bag, bag->count + 1U)) {
        return 0;
    }
    va_start(arguments, format);
    va_copy(copy, arguments);
    required = vsnprintf(NULL, 0U, format, copy);
    va_end(copy);
    if (required < 0) {
        va_end(arguments);
        return 0;
    }
    message = (char *)malloc((size_t)required + 1U);
    if (message == NULL) {
        va_end(arguments);
        return 0;
    }
    written = vsnprintf(message, (size_t)required + 1U, format, arguments);
    va_end(arguments);
    if (written != required) {
        free(message);
        return 0;
    }

    diagnostic = &bag->items[bag->count++];
    diagnostic->severity = severity;
    diagnostic->phase = phase;
    diagnostic->code = code != NULL ? code : "E-DIAG-000";
    diagnostic->message = message;
    diagnostic->source = source;
    diagnostic->span = span;
    if (severity == DIAGNOSTIC_ERROR) {
        ++bag->error_count;
    }
    return 1;
}

int diagnostic_bag_has_errors(const DiagnosticBag *bag)
{
    return bag != NULL && bag->error_count != 0U;
}

const char *diagnostic_phase_name(DiagnosticPhase phase)
{
    switch (phase) {
    case DIAGNOSTIC_PHASE_LEXICAL: return "lexico";
    case DIAGNOSTIC_PHASE_SYNTACTIC: return "sintatico";
    case DIAGNOSTIC_PHASE_SEMANTIC: return "semantico";
    default: return "desconhecido";
    }
}

void diagnostic_print(const Diagnostic *diagnostic, FILE *stream)
{
    const char *path;

    if (diagnostic == NULL) {
        return;
    }
    if (stream == NULL) {
        stream = stderr;
    }
    path = diagnostic->source != NULL && diagnostic->source->path != NULL
               ? diagnostic->source->path
               : "<fonte>";
    (void)fprintf(stream, "%s:%zu:%zu: %s %s: %s\n",
                  path,
                  diagnostic->span.start.line,
                  diagnostic->span.start.column,
                  diagnostic_phase_name(diagnostic->phase),
                  diagnostic->code,
                  diagnostic->message);
}

void diagnostic_bag_print(const DiagnosticBag *bag, FILE *stream)
{
    size_t index;

    if (bag == NULL) {
        return;
    }
    for (index = 0U; index < bag->count; ++index) {
        diagnostic_print(&bag->items[index], stream);
    }
}
