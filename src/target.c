#include "target.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct {
    const SourceFile *source;
    const SemanticResult *semantic;
    DiagnosticBag *diagnostics;
    int esp32;
    int failed;
} TargetCheck;

static void target_error(TargetCheck *check, SourceSpan span,
                         const char *code, const char *message)
{
    if (!diagnostic_bag_add(check->diagnostics, check->source, span,
                            DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SEMANTIC,
                            code, "%s", message)) check->failed = 1;
}

static int is_esp32_builtin(BuiltinKind kind)
{
    return kind == BUILTIN_PIN_MODE || kind == BUILTIN_PIN_WRITE ||
           kind == BUILTIN_PIN_READ || kind == BUILTIN_DELAY_MS;
}

static void check_node(TargetCheck *check, const AstNode *node)
{
    size_t i;
    const Symbol *symbol;
    if (node == NULL || check->failed) return;
    switch (node->kind) {
    case AST_PROGRAM:
        for (i = 0U; i < node->as.program.declarations.count; ++i)
            check_node(check, node->as.program.declarations.items[i]);
        break;
    case AST_FUNCTION_DECL:
        check_node(check, node->as.function_decl.body);
        break;
    case AST_BLOCK:
        for (i = 0U; i < node->as.block.statements.count; ++i)
            check_node(check, node->as.block.statements.items[i]);
        break;
    case AST_ASSIGNMENT:
        check_node(check, node->as.assignment.target);
        check_node(check, node->as.assignment.value);
        break;
    case AST_IF_STMT:
        for (i = 0U; i < node->as.if_stmt.branches.count; ++i) {
            check_node(check, node->as.if_stmt.branches.items[i].condition);
            check_node(check, node->as.if_stmt.branches.items[i].body);
        }
        check_node(check, node->as.if_stmt.else_body);
        break;
    case AST_WHILE_STMT:
        check_node(check, node->as.while_stmt.condition);
        check_node(check, node->as.while_stmt.body);
        break;
    case AST_FOR_STMT:
        check_node(check, node->as.for_stmt.initializer);
        check_node(check, node->as.for_stmt.condition);
        check_node(check, node->as.for_stmt.iteration);
        check_node(check, node->as.for_stmt.body);
        break;
    case AST_RETURN_STMT:
        check_node(check, node->as.return_stmt.value);
        break;
    case AST_EXPR_STMT:
        check_node(check, node->as.expr_stmt.expression);
        break;
    case AST_CALL_EXPR:
        symbol = symbol_table_get(&check->semantic->symbols,
                                  node->semantic.symbol_id);
        if (!check->esp32 && symbol != NULL &&
            is_esp32_builtin(symbol->builtin_kind)) {
            target_error(check, node->as.call_expr.callee_span, "E-TGT-001",
                         "intrinseca de GPIO/espera exige --alvo=esp32");
        }
        for (i = 0U; i < node->as.call_expr.arguments.count; ++i)
            check_node(check, node->as.call_expr.arguments.items[i]);
        break;
    case AST_INDEX_EXPR:
        check_node(check, node->as.index_expr.base);
        check_node(check, node->as.index_expr.index);
        break;
    case AST_FIELD_EXPR:
        check_node(check, node->as.field_expr.base);
        break;
    case AST_BINARY_EXPR:
        check_node(check, node->as.binary_expr.left);
        check_node(check, node->as.binary_expr.right);
        break;
    case AST_UNARY_EXPR:
        check_node(check, node->as.unary_expr.operand);
        break;
    default:
        break;
    }
}

/* ------------------------------------------------ memoria da placa */

/* Verificacao de memoria do --alvo=esp32 (E-TGT-002), com a mesma conta que
 * o runtime da placa faz (runtime/zl_runtime_esp32.c, docs/placa.md):
 * - zl_principal roda numa tarefa FreeRTOS cuja pilha tem ram_limit bytes
 *   (--ram-limite; o mesmo numero vai para ZL_PILHA_BYTES). Nela ficam os
 *   parametros, as variaveis locais (inclusive vetores) e os temporarios de
 *   cada funcao ativa;
 * - as globais ficam na DRAM estatica (.bss), fora da pilha.
 * Os tamanhos sao os do C gerado (codegen_c.c) no Xtensa LX6 de 32 bits:
 * inteira int64_t (8), duplocarpado 8, flut 4, bool int32_t (4), palavra
 * ponteiro (4), registro como struct C com alinhamento natural. */
#define ESP32_PONTEIRO 4U
/* Por quadro, alem das variaveis: salvamento da janela de registradores do
 * Xtensa (16 bytes), area extra de call8/call12 (ate 32 bytes) e o
 * arredondamento do quadro para 16 bytes. */
#define ESP32_QUADRO_BASE 64U
/* Pilha que o runtime usa abaixo da funcao mais funda: o printf de double
 * do newlib (_svfprintf_r e _dtoa_r), os drivers de GPIO e UART e a troca de
 * contexto do FreeRTOS. Folga sobre o medido no .elf (docs/placa.md). */
#define ESP32_RESERVA_RUNTIME 8192U
/* DRAM estatica para as globais do programa (docs/placa.md): o link do
 * nucleo Arduino 3.3.11 aceita ate ~100 KiB de .bss alem do sistema; o resto
 * da DRAM vira heap, de onde sai a pilha da tarefa. Fica um limite redondo
 * com folga. */
#define ESP32_GLOBAIS_LIMITE 65536U

static size_t add_size(size_t left, size_t right, int *overflow)
{
    if (right > SIZE_MAX - left) {
        *overflow = 1;
        return SIZE_MAX;
    }
    return left + right;
}

static size_t align_up(size_t value, size_t alignment, int *overflow)
{
    size_t rest = alignment == 0U ? 0U : value % alignment;
    return rest == 0U ? value : add_size(value, alignment - rest, overflow);
}

typedef struct {
    size_t size;
    size_t alignment;
} CLayout;

/* Tamanho e alinhamento do tipo no C gerado, no ESP32. */
static CLayout c_layout(const SemanticResult *semantic, size_t type_id,
                        size_t depth, int *overflow)
{
    const Type *type = type_table_get(&semantic->types, type_id);
    CLayout layout, element;
    size_t i;
    int fields = 0;
    layout.size = 0U;
    layout.alignment = 1U;
    if (type == NULL || depth > semantic->types.count) {
        *overflow = 1;
        return layout;
    }
    switch (type->kind) {
    case TYPE_BOOL: case TYPE_FLOAT:
        layout.size = 4U;
        layout.alignment = 4U;
        return layout;
    case TYPE_WORD:
        layout.size = ESP32_PONTEIRO;
        layout.alignment = ESP32_PONTEIRO;
        return layout;
    case TYPE_INTEGER: case TYPE_DOUBLE:
        layout.size = 8U;
        layout.alignment = 8U;
        return layout;
    case TYPE_VOID:
        return layout;
    case TYPE_ARRAY:
        if (type->array_size < 0) {
            *overflow = 1;
            return layout;
        }
        element = c_layout(semantic, type->element_type_id, depth + 1U, overflow);
        if (element.size != 0U && (size_t)type->array_size > SIZE_MAX / element.size) {
            *overflow = 1;
            return layout;
        }
        layout.size = element.size * (size_t)type->array_size;
        layout.alignment = element.alignment;
        return layout;
    case TYPE_RECORD:
        /* Mesma ordem de campos que codegen_c.c usa ao escrever a struct. */
        for (i = 0U; i < semantic->symbols.count; ++i) {
            const Symbol *field = &semantic->symbols.items[i];
            const Scope *scope = symbol_table_get_scope(&semantic->symbols,
                                                         field->scope_id);
            if (field->category != SYMBOL_FIELD || scope == NULL ||
                scope->owner_symbol_id != type->declaration_symbol_id) continue;
            element = c_layout(semantic, field->type_id, depth + 1U, overflow);
            layout.size = align_up(layout.size, element.alignment, overflow);
            layout.size = add_size(layout.size, element.size, overflow);
            if (element.alignment > layout.alignment) layout.alignment = element.alignment;
            fields = 1;
        }
        if (!fields) layout.size = 1U; /* unsigned char mac_empty */
        layout.size = align_up(layout.size, layout.alignment, overflow);
        return layout;
    default:
        *overflow = 1;
        return layout;
    }
}

static size_t c_size(const SemanticResult *semantic, size_t type_id, int *overflow)
{
    return c_layout(semantic, type_id, 0U, overflow).size;
}

/* Uma funcao do programa: o quadro que ela ocupa na pilha e quem ela chama. */
typedef struct {
    const AstNode *declaration;
    size_t symbol_id;
    size_t frame;
    size_t *callees;
    size_t callee_count;
    size_t callee_capacity;
    size_t need;   /* quadro + a cadeia de chamadas mais funda abaixo dela */
    size_t deepest; /* chamada que da essa cadeia; SIZE_MAX se nenhuma */
    int state;     /* 0 nao visitada, 1 na pilha da busca, 2 pronta */
} StackFunction;

typedef struct {
    TargetCheck *check;
    StackFunction *functions;
    size_t count;
    int overflow;
} StackCheck;

static size_t function_index(const StackCheck *stack, size_t symbol_id)
{
    size_t i;
    for (i = 0U; i < stack->count; ++i)
        if (stack->functions[i].symbol_id == symbol_id) return i;
    return SIZE_MAX;
}

static void add_callee(StackCheck *stack, StackFunction *function, size_t callee)
{
    size_t i, *items;
    for (i = 0U; i < function->callee_count; ++i)
        if (function->callees[i] == callee) return;
    if (function->callee_count == function->callee_capacity) {
        size_t capacity = function->callee_capacity == 0U ? 4U : function->callee_capacity * 2U;
        items = (size_t *)realloc(function->callees, capacity * sizeof(*items));
        if (items == NULL) {
            stack->check->failed = 1;
            return;
        }
        function->callees = items;
        function->callee_capacity = capacity;
    }
    function->callees[function->callee_count++] = callee;
}

/* Soma os temporarios das expressoes (o C gerado cria um por subexpressao;
 * o gcc reaproveita muitos em registradores, entao isto e um teto) e anota
 * as chamadas a outras funcoes do programa. */
static void scan_body(StackCheck *stack, StackFunction *function, const AstNode *node)
{
    size_t i, callee;
    const Symbol *symbol;
    const Type *type;
    if (node == NULL || stack->check->failed) return;
    switch (node->kind) {
    case AST_BLOCK:
        for (i = 0U; i < node->as.block.statements.count; ++i)
            scan_body(stack, function, node->as.block.statements.items[i]);
        return;
    case AST_ASSIGNMENT:
        scan_body(stack, function, node->as.assignment.target);
        scan_body(stack, function, node->as.assignment.value);
        return;
    case AST_IF_STMT:
        for (i = 0U; i < node->as.if_stmt.branches.count; ++i) {
            scan_body(stack, function, node->as.if_stmt.branches.items[i].condition);
            scan_body(stack, function, node->as.if_stmt.branches.items[i].body);
        }
        scan_body(stack, function, node->as.if_stmt.else_body);
        return;
    case AST_WHILE_STMT:
        scan_body(stack, function, node->as.while_stmt.condition);
        scan_body(stack, function, node->as.while_stmt.body);
        return;
    case AST_FOR_STMT:
        scan_body(stack, function, node->as.for_stmt.initializer);
        scan_body(stack, function, node->as.for_stmt.condition);
        scan_body(stack, function, node->as.for_stmt.iteration);
        scan_body(stack, function, node->as.for_stmt.body);
        return;
    case AST_RETURN_STMT:
        scan_body(stack, function, node->as.return_stmt.value);
        return;
    case AST_EXPR_STMT:
        scan_body(stack, function, node->as.expr_stmt.expression);
        return;
    case AST_CALL_EXPR:
        symbol = symbol_table_get(&stack->check->semantic->symbols,
                                  node->semantic.symbol_id);
        if (symbol != NULL && symbol->category == SYMBOL_FUNCTION) {
            callee = function_index(stack, node->semantic.symbol_id);
            if (callee != SIZE_MAX) add_callee(stack, function, callee);
        }
        for (i = 0U; i < node->as.call_expr.arguments.count; ++i)
            scan_body(stack, function, node->as.call_expr.arguments.items[i]);
        break;
    case AST_INDEX_EXPR:
        scan_body(stack, function, node->as.index_expr.base);
        scan_body(stack, function, node->as.index_expr.index);
        break;
    case AST_FIELD_EXPR:
        scan_body(stack, function, node->as.field_expr.base);
        break;
    case AST_BINARY_EXPR:
        scan_body(stack, function, node->as.binary_expr.left);
        scan_body(stack, function, node->as.binary_expr.right);
        break;
    case AST_UNARY_EXPR:
        scan_body(stack, function, node->as.unary_expr.operand);
        break;
    case AST_LITERAL_EXPR:
    case AST_IDENTIFIER_EXPR:
        break;
    default:
        return;
    }
    /* So as expressoes chegam aqui: cada uma vale um temporario do seu tipo
     * (um registro lido vira copia; um vetor e acessado por ponteiro, sem
     * copia, e o ponteiro cabe na conta das outras expressoes). */
    type = type_table_get(&stack->check->semantic->types, node->semantic.type_id);
    if (type != NULL && type->kind != TYPE_ARRAY) {
        function->frame = add_size(function->frame,
                                   c_size(stack->check->semantic, node->semantic.type_id,
                                          &stack->overflow),
                                   &stack->overflow);
    }
}

/* Parametros e variaveis locais da funcao, com o alinhamento do C. */
static size_t locals_size(StackCheck *stack, size_t function_symbol)
{
    const SymbolTable *symbols = &stack->check->semantic->symbols;
    size_t i, j, total = 0U;
    for (i = 0U; i < symbols->scope_count; ++i) {
        const Scope *scope = &symbols->scopes[i];
        if (scope->kind != SCOPE_FUNCTION || scope->owner_symbol_id != function_symbol)
            continue;
        for (j = 0U; j < symbols->count; ++j) {
            const Symbol *symbol = &symbols->items[j];
            CLayout layout;
            if (symbol->scope_id != scope->id ||
                (symbol->category != SYMBOL_PARAMETER &&
                 symbol->category != SYMBOL_LOCAL_VARIABLE &&
                 symbol->category != SYMBOL_VECTOR)) continue;
            layout = c_layout(stack->check->semantic, symbol->type_id, 0U, &stack->overflow);
            total = align_up(total, layout.alignment, &stack->overflow);
            total = add_size(total, layout.size, &stack->overflow);
        }
    }
    return total;
}

/* Pilha da cadeia de chamadas mais funda a partir de index. Uma chamada
 * recursiva (de volta a uma funcao que ja esta na cadeia) conta uma vez so:
 * a profundidade da recursao depende dos dados e nao e verificada aqui. */
static size_t stack_need(StackCheck *stack, size_t index)
{
    StackFunction *function = &stack->functions[index];
    size_t i, best = 0U;
    if (function->state == 2) return function->need;
    if (function->state == 1) return 0U;
    function->state = 1;
    function->deepest = SIZE_MAX;
    for (i = 0U; i < function->callee_count; ++i) {
        size_t callee = function->callees[i];
        size_t need;
        if (stack->functions[callee].state == 1) continue;
        need = stack_need(stack, callee);
        if (need > best) {
            best = need;
            function->deepest = callee;
        }
    }
    function->need = add_size(function->frame, best, &stack->overflow);
    function->state = 2;
    return function->need;
}

static size_t globals_size(TargetCheck *check, int *overflow)
{
    const SymbolTable *symbols = &check->semantic->symbols;
    size_t i, total = 0U;
    for (i = 0U; i < symbols->count; ++i) {
        const Symbol *symbol = &symbols->items[i];
        CLayout layout;
        if (symbol->scope_id != check->semantic->global_scope_id ||
            (symbol->category != SYMBOL_GLOBAL_VARIABLE &&
             symbol->category != SYMBOL_VECTOR)) continue;
        layout = c_layout(check->semantic, symbol->type_id, 0U, overflow);
        total = align_up(total, layout.alignment, overflow);
        total = add_size(total, layout.size, overflow);
    }
    return total;
}

static void check_memory(TargetCheck *check, const AstNode *root, size_t stack_limit)
{
    const AstNodeList *declarations = &root->as.program.declarations;
    StackCheck stack;
    size_t i, principal = SIZE_MAX, need, globals, biggest = SIZE_MAX;
    int overflow = 0;

    globals = globals_size(check, &overflow);
    if (!overflow && globals > ESP32_GLOBAIS_LIMITE) {
        if (!diagnostic_bag_add(check->diagnostics, check->source, root->span,
                                DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SEMANTIC, "E-TGT-002",
                                "variaveis globais ocupam %zu bytes e excedem os %u bytes "
                                "de DRAM estatica reservados ao programa na placa",
                                globals, ESP32_GLOBAIS_LIMITE)) check->failed = 1;
    }

    stack.check = check;
    stack.count = 0U;
    stack.overflow = overflow;
    stack.functions = (StackFunction *)calloc(declarations->count == 0U ? 1U : declarations->count,
                                              sizeof(*stack.functions));
    if (stack.functions == NULL) {
        check->failed = 1;
        return;
    }
    for (i = 0U; i < declarations->count; ++i) {
        const AstNode *node = declarations->items[i];
        StackFunction *function;
        if (node == NULL || node->kind != AST_FUNCTION_DECL) continue;
        function = &stack.functions[stack.count++];
        function->declaration = node;
        function->symbol_id = node->semantic.symbol_id;
        if (node->as.function_decl.is_principal) principal = stack.count - 1U;
    }
    for (i = 0U; i < stack.count && !check->failed; ++i) {
        StackFunction *function = &stack.functions[i];
        function->frame = add_size(ESP32_QUADRO_BASE, locals_size(&stack, function->symbol_id),
                                   &stack.overflow);
        scan_body(&stack, function, function->declaration->as.function_decl.body);
        function->frame = align_up(function->frame, 16U, &stack.overflow);
    }
    if (!check->failed && principal != SIZE_MAX) {
        need = add_size(stack_need(&stack, principal), ESP32_RESERVA_RUNTIME, &stack.overflow);
        /* A funcao de maior quadro na cadeia mais funda: e la que o programa
         * precisa diminuir os vetores locais. */
        for (i = principal; i != SIZE_MAX; i = stack.functions[i].deepest)
            if (biggest == SIZE_MAX || stack.functions[i].frame > stack.functions[biggest].frame)
                biggest = i;
        if (stack.overflow) {
            target_error(check, root->span, "E-TGT-002",
                         "estimativa de memoria excede a capacidade de calculo");
        } else if (need > stack_limit) {
            const AstNode *where = stack.functions[biggest].declaration;
            if (!diagnostic_bag_add(check->diagnostics, check->source,
                                    where->as.function_decl.name_span,
                                    DIAGNOSTIC_ERROR, DIAGNOSTIC_PHASE_SEMANTIC, "E-TGT-002",
                                    "pilha estimada de %zu bytes excede os %zu bytes da "
                                    "pilha da tarefa do programa na placa (--ram-limite); "
                                    "maior quadro: '%s', %zu bytes; reserva do runtime: %u bytes",
                                    need, stack_limit, where->as.function_decl.name,
                                    stack.functions[biggest].frame, ESP32_RESERVA_RUNTIME))
                check->failed = 1;
        }
    }
    for (i = 0U; i < stack.count; ++i) free(stack.functions[i].callees);
    free(stack.functions);
}

FrontendStatus target_validate(const SourceFile *source, const AstNode *root,
                               const SemanticResult *semantic,
                               DiagnosticBag *diagnostics, int esp32,
                               size_t ram_limit)
{
    TargetCheck check;
    size_t errors_before;
    if (source == NULL || root == NULL || root->kind != AST_PROGRAM ||
        semantic == NULL || !semantic->analyzed || diagnostics == NULL)
        return FRONTEND_STATUS_INTERNAL_ERROR;
    check.source = source;
    check.semantic = semantic;
    check.diagnostics = diagnostics;
    check.esp32 = esp32;
    check.failed = 0;
    errors_before = diagnostics->error_count;
    check_node(&check, root);
    if (esp32 && !check.failed) check_memory(&check, root, ram_limit);
    if (check.failed) return FRONTEND_STATUS_INTERNAL_ERROR;
    return diagnostics->error_count == errors_before
               ? FRONTEND_STATUS_OK : FRONTEND_STATUS_SOURCE_ERROR;
}
