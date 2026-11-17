#include "codegen_mlir.h"
#include "dsl.h"

#include <stdarg.h>
#include <string.h>

typedef struct {
    const SemanticResult *semantic;
    FILE *output;
    int failed;
} MlirGen;

static void out(MlirGen *g, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    if (vfprintf(g->output, format, args) < 0) g->failed = 1;
    va_end(args);
}

static const char *scalar_type(size_t id)
{
    if (id == TYPE_ID_INTEGER) return "i32";
    if (id == TYPE_ID_FLOAT) return "f32";
    return "f64";
}

static const Type *type_of(MlirGen *g, const AstNode *n)
{
    return type_table_get(&g->semantic->types, n->semantic.type_id);
}

static void argument_type(MlirGen *g, const Type *type)
{
    if (type->kind == TYPE_ARRAY)
        out(g, "memref<%lldx%s>", type->array_size, scalar_type(type->element_type_id));
    else out(g, "%s", scalar_type(type->id));
}

/* Loads and widening are explicit. No reassociation/fastmath is promised. */
static void operand(MlirGen *g, const Type *type, const char *arg,
                    const char *name, size_t target, int strict_loop)
{
    size_t source = type->kind == TYPE_ARRAY ? type->element_type_id : type->id;
    char original[64];
    if (type->kind == TYPE_ARRAY) {
        (void)snprintf(original, sizeof(original), "%%%sraw", name);
        out(g, "      %s = %s.load %%%s[%%i] : ", original,
            strict_loop ? "memref" : "affine", arg);
        argument_type(g, type);
        out(g, "\n");
    } else (void)snprintf(original, sizeof(original), "%%%s", arg);
    if (source != target)
        out(g, "      %%%s = arith.%s %s : %s to %s\n", name,
            source == TYPE_ID_INTEGER ? "sitofp" : "extf", original,
            scalar_type(source), scalar_type(target));
    else {
        /* An identity add would change signed zero for floats. Names of the
         * actual SSA operands are selected by value_name instead. */
    }
}

static void value_name(char *buffer, size_t capacity, const Type *type,
                       const char *arg, const char *name, size_t target)
{
    size_t source = type->kind == TYPE_ARRAY ? type->element_type_id : type->id;
    (void)snprintf(buffer, capacity, "%%%s%s",
                   source != target ? name : type->kind == TYPE_ARRAY ? name : arg,
                   source == target && type->kind == TYPE_ARRAY ? "raw" : "");
}

static void emit_kernel(const AstNode *node, const AstNode *destination, void *context)
{
    MlirGen *g = (MlirGen *)context;
    const Type *left, *right = NULL, *dest = NULL;
    const AstNode *left_node, *right_node = NULL;
    const Symbol *function = NULL;
    size_t element;
    long long count;
    char lv[64], rv[64];
    int reduction = destination == NULL;
    int maximum = 0;
    int strict_loop;
    if (reduction) {
        function = symbol_table_get(&g->semantic->symbols, node->semantic.symbol_id);
        maximum = function->builtin_kind == BUILTIN_VECTOR_MAX;
        left_node = node->as.call_expr.arguments.items[0];
        if (node->as.call_expr.arguments.count == 2U)
            right_node = node->as.call_expr.arguments.items[1];
        element = node->semantic.type_id;
    } else {
        left_node = node->as.binary_expr.left;
        right_node = node->as.binary_expr.right;
        dest = type_of(g, destination);
        element = type_of(g, node)->element_type_id;
    }
    left = type_of(g, left_node);
    if (right_node != NULL) right = type_of(g, right_node);
    count = left->kind == TYPE_ARRAY ? left->array_size : right->array_size;
    /* Keep real reductions in SCF so affine vectorization/parallelization
     * cannot reassociate them. Integer modular sums remain associative. */
    strict_loop = reduction && element != TYPE_ID_INTEGER;
    out(g, "  // linha %zu; %s; ordem real estrita; alias exato permitido\n",
        node->span.start.line, reduction ? "reducao" : "elemento a elemento");
    out(g, "  func.func @mac_vec_%zu(", node->node_id);
    if (!reduction) {
        out(g, "%%dst: "); argument_type(g, dest); out(g, ", ");
    }
    out(g, "%%a: "); argument_type(g, left);
    if (right != NULL) { out(g, ", %%b: "); argument_type(g, right); }
    out(g, ")");
    if (reduction) out(g, " -> %s", scalar_type(element));
    out(g, " attributes {passthrough = [\"nounwind\"]} {\n");
    if (reduction) {
        if (maximum) {
            out(g, "    %%init = affine.load %%a[0] : "); argument_type(g, left); out(g, "\n");
        } else out(g, "    %%init = arith.constant %s : %s\n",
                    element == TYPE_ID_INTEGER ? "0" : "0.0", scalar_type(element));
        if (strict_loop) {
            out(g, "    %%lo = arith.constant %d : index\n", maximum ? 1 : 0);
            out(g, "    %%hi = arith.constant %lld : index\n", count);
            out(g, "    %%step = arith.constant 1 : index\n");
            out(g, "    %%result = scf.for %%i = %%lo to %%hi step %%step iter_args(%%acc = %%init) -> (%s) {\n",
                scalar_type(element));
        } else out(g, "    %%result = affine.for %%i = %d to %lld iter_args(%%acc = %%init) -> (%s) {\n",
                   maximum ? 1 : 0, count, scalar_type(element));
    } else out(g, "    affine.for %%i = 0 to %lld {\n", count);
    operand(g, left, "a", "av", element, strict_loop);
    value_name(lv, sizeof(lv), left, "a", "av", element);
    if (right != NULL) {
        operand(g, right, "b", "bv", element, strict_loop);
        value_name(rv, sizeof(rv), right, "b", "bv", element);
    }
    if (!reduction || right != NULL) {
        const char *op = "mul";
        if (!reduction && node->as.binary_expr.operator_kind == AST_OP_ADD) op = "add";
        if (!reduction && node->as.binary_expr.operator_kind == AST_OP_SUBTRACT) op = "sub";
        out(g, "      %%value = arith.%s%s %s, %s : %s\n", op,
            element == TYPE_ID_INTEGER ? "i" : "f", lv, rv, scalar_type(element));
    }
    if (reduction) {
        if (maximum) {
            out(g, "      %%greater = arith.%s %s, %s, %%acc : %s\n",
                element == TYPE_ID_INTEGER ? "cmpi" : "cmpf",
                element == TYPE_ID_INTEGER ? "sgt" : "ogt", lv, scalar_type(element));
            out(g, "      %%next = arith.select %%greater, %s, %%acc : %s\n", lv, scalar_type(element));
        } else out(g, "      %%next = arith.add%s %%acc, %s : %s\n",
                    element == TYPE_ID_INTEGER ? "i" : "f",
                    right != NULL ? "%value" : lv, scalar_type(element));
        out(g, "      %s.yield %%next : %s\n    }\n    return %%result : %s\n",
            strict_loop ? "scf" : "affine", scalar_type(element), scalar_type(element));
    } else {
        if (dest->element_type_id != element)
            out(g, "      %%converted = arith.%s %%value : %s to %s\n",
                element == TYPE_ID_INTEGER ? "sitofp" : "extf", scalar_type(element),
                scalar_type(dest->element_type_id));
        out(g, "      affine.store %s, %%dst[%%i] : ",
            dest->element_type_id == element ? "%value" : "%converted");
        argument_type(g, dest);
        out(g, "\n    }\n    return\n");
    }
    out(g, "  }\n\n");
}

FrontendStatus codegen_mlir_emit(const SourceFile *source, const AstNode *program,
                                 const SemanticResult *semantic,
                                 DiagnosticBag *diagnostics, FILE *output)
{
    MlirGen g;
    if (source == NULL || program == NULL || semantic == NULL || diagnostics == NULL ||
        output == NULL || !semantic->analyzed || semantic->semantic_error_count != 0U)
        return FRONTEND_STATUS_INTERNAL_ERROR;
    g.semantic = semantic; g.output = output; g.failed = 0;
    out(&g, "// Macaronica: nucleos MLIR; ligar com o modulo host LLVM.\nmodule {\n");
    dsl_visit(program, emit_kernel, &g);
    out(&g, "}\n");
    if (g.failed || ferror(output)) {
        (void)diagnostic_bag_add(diagnostics, source, program->span, DIAGNOSTIC_ERROR,
                                DIAGNOSTIC_PHASE_CODEGEN, "E-MLIR-001",
                                "falha ao escrever nucleos MLIR");
        return FRONTEND_STATUS_INTERNAL_ERROR;
    }
    return FRONTEND_STATUS_OK;
}
