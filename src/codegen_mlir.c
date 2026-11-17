#include "codegen_mlir.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static int is_real(size_t type_id)
{
    return type_id == TYPE_ID_FLOAT || type_id == TYPE_ID_DOUBLE;
}

static int is_reduction(const DslOperation *kernel)
{
    return kernel->form == DSL_FORM_SUM || kernel->form == DSL_FORM_MAX ||
           kernel->form == DSL_FORM_DOT;
}

static const char *mlir_type(size_t type_id)
{
    return type_id == TYPE_ID_FLOAT ? "f32" : type_id == TYPE_ID_DOUBLE ? "f64" : "i64";
}

static const char *llvm_type(size_t type_id)
{
    return type_id == TYPE_ID_FLOAT ? "float" : type_id == TYPE_ID_DOUBLE ? "double" : "i64";
}

static const char *c_type(size_t type_id)
{
    return type_id == TYPE_ID_FLOAT ? "float" : type_id == TYPE_ID_DOUBLE ? "double" : "int64_t";
}

static const char *mlir_operation(AstOperator op, int real)
{
    switch (op) {
    case AST_OP_SUBTRACT: return real ? "arith.subf" : "arith.subi";
    case AST_OP_MULTIPLY: return real ? "arith.mulf" : "arith.muli";
    default: return real ? "arith.addf" : "arith.addi";
    }
}

static const char *c_operator(AstOperator op)
{
    return op == AST_OP_SUBTRACT ? "-" : op == AST_OP_MULTIPLY ? "*" : "+";
}

/*
 * Alarga `value` de `from` para `to` (a semantica so deixa alargamentos:
 * inteira->flut/duplocarpado e flut->duplocarpado) e devolve em `result` o
 * nome SSA que vale depois da conversao.
 */
static void mlir_convert(FILE *out, const char *value, size_t from, size_t to,
                         const char *name, char *result, size_t capacity)
{
    if (from == to) {
        (void)snprintf(result, capacity, "%s", value);
        return;
    }
    (void)fprintf(out, "      %%%s = %s %%%s : %s to %s\n", name,
                  from == TYPE_ID_INTEGER ? "arith.sitofp" : "arith.extf",
                  value, mlir_type(from), mlir_type(to));
    (void)snprintf(result, capacity, "%s", name);
}

/* c{i} = a{i} op b{i} (ou com o escalar %k): affine.for, que o
 * --affine-super-vectorize transforma em operacoes do dialeto vector. */
static void write_mlir_elementwise(FILE *out, const DslOperation *kernel)
{
    size_t a = kernel->element_type_ids[0];
    size_t r = kernel->result_type_id;
    size_t c = kernel->target_type_id;
    const char *op = mlir_operation(kernel->op, is_real(r));
    char x[8], y[8], z[8];
    (void)fprintf(out, "  func.func @%s(", kernel->kernel);
    if (kernel->form == DSL_FORM_VECTOR_VECTOR)
        (void)fprintf(out, "%%a: memref<?x%s>, %%b: memref<?x%s>, %%c: memref<?x%s>) {\n",
                      mlir_type(a), mlir_type(kernel->element_type_ids[1]), mlir_type(c));
    else if (kernel->form == DSL_FORM_SCALAR_VECTOR)
        (void)fprintf(out, "%%k: %s, %%a: memref<?x%s>, %%c: memref<?x%s>) {\n",
                      mlir_type(r), mlir_type(a), mlir_type(c));
    else
        (void)fprintf(out, "%%a: memref<?x%s>, %%k: %s, %%c: memref<?x%s>) {\n",
                      mlir_type(a), mlir_type(r), mlir_type(c));
    (void)fprintf(out,
                  "    %%c0 = arith.constant 0 : index\n"
                  "    %%n = memref.dim %%c, %%c0 : memref<?x%s>\n"
                  "    affine.for %%i = 0 to %%n {\n"
                  "      %%x = affine.load %%a[%%i] : memref<?x%s>\n",
                  mlir_type(c), mlir_type(a));
    mlir_convert(out, "x", a, r, "xr", x, sizeof(x));
    if (kernel->form == DSL_FORM_VECTOR_VECTOR) {
        size_t b = kernel->element_type_ids[1];
        (void)fprintf(out, "      %%y = affine.load %%b[%%i] : memref<?x%s>\n", mlir_type(b));
        mlir_convert(out, "y", b, r, "yr", y, sizeof(y));
        (void)fprintf(out, "      %%r = %s %%%s, %%%s : %s\n", op, x, y, mlir_type(r));
    } else if (kernel->form == DSL_FORM_SCALAR_VECTOR) {
        (void)fprintf(out, "      %%r = %s %%k, %%%s : %s\n", op, x, mlir_type(r));
    } else {
        (void)fprintf(out, "      %%r = %s %%%s, %%k : %s\n", op, x, mlir_type(r));
    }
    mlir_convert(out, "r", r, c, "rc", z, sizeof(z));
    (void)fprintf(out,
                  "      affine.store %%%s, %%c[%%i] : memref<?x%s>\n"
                  "    }\n"
                  "    return\n"
                  "  }\n\n",
                  z, mlir_type(c));
}

/*
 * Reducoes. As inteiras usam affine.for com iter_args: a soma modulo 2^64 e o
 * maximo sao associativos, entao reordenar e permitido. As de reais usam
 * scf.for, que o vetorizador afim nao toca: a ordem de arredondamento fica a
 * do laco escalar (D-021, sem reassociacao).
 */
static void write_mlir_reduction(FILE *out, const DslOperation *kernel)
{
    size_t a = kernel->element_type_ids[0];
    size_t r = kernel->result_type_id;
    int real = is_real(r);
    int is_max = kernel->form == DSL_FORM_MAX;
    const char *load = real ? "memref.load" : "affine.load";
    const char *type = mlir_type(r);
    char x[8], y[8];
    (void)fprintf(out, "  func.func @%s(%%a: memref<?x%s>", kernel->kernel, mlir_type(a));
    if (kernel->form == DSL_FORM_DOT)
        (void)fprintf(out, ", %%b: memref<?x%s>", mlir_type(kernel->element_type_ids[1]));
    (void)fprintf(out, ") -> %s {\n    %%c0 = arith.constant 0 : index\n", type);
    if (real) (void)fprintf(out, "    %%c1 = arith.constant 1 : index\n");
    (void)fprintf(out, "    %%n = memref.dim %%a, %%c0 : memref<?x%s>\n", mlir_type(a));
    if (is_max)
        (void)fprintf(out, "    %%init = memref.load %%a[%%c0] : memref<?x%s>\n", type);
    else
        (void)fprintf(out, "    %%init = arith.constant %s : %s\n", real ? "0.0" : "0", type);
    if (real)
        (void)fprintf(out, "    %%s = scf.for %%i = %%%s to %%n step %%c1 "
                           "iter_args(%%acc = %%init) -> (%s) {\n",
                      is_max ? "c1" : "c0", type);
    else
        (void)fprintf(out, "    %%s = affine.for %%i = %d to %%n "
                           "iter_args(%%acc = %%init) -> (%s) {\n",
                      is_max ? 1 : 0, type);
    (void)fprintf(out, "      %%x = %s %%a[%%i] : memref<?x%s>\n", load, mlir_type(a));
    mlir_convert(out, "x", a, r, "xr", x, sizeof(x));
    switch (kernel->form) {
    case DSL_FORM_SUM:
        (void)fprintf(out, "      %%t = %s %%acc, %%%s : %s\n",
                      real ? "arith.addf" : "arith.addi", x, type);
        break;
    case DSL_FORM_MAX:
        /* `se (x > m)` do laco escalar: NaN no meio e ignorado. */
        if (real)
            (void)fprintf(out,
                          "      %%gt = arith.cmpf ogt, %%%s, %%acc : %s\n"
                          "      %%t = arith.select %%gt, %%%s, %%acc : %s\n",
                          x, type, x, type);
        else
            (void)fprintf(out, "      %%t = arith.maxsi %%acc, %%%s : %s\n", x, type);
        break;
    default: {
        size_t b = kernel->element_type_ids[1];
        (void)fprintf(out, "      %%y = %s %%b[%%i] : memref<?x%s>\n", load, mlir_type(b));
        mlir_convert(out, "y", b, r, "yr", y, sizeof(y));
        (void)fprintf(out,
                      "      %%p = %s %%%s, %%%s : %s\n"
                      "      %%t = %s %%acc, %%p : %s\n",
                      real ? "arith.mulf" : "arith.muli", x, y, type,
                      real ? "arith.addf" : "arith.addi", type);
        break;
    }
    }
    (void)fprintf(out,
                  "      %s %%t : %s\n"
                  "    }\n"
                  "    return %%s : %s\n"
                  "  }\n\n",
                  real ? "scf.yield" : "affine.yield", type, type);
}

int codegen_mlir_write_module(FILE *output, const DslKernelList *kernels)
{
    size_t index;
    if (output == NULL || kernels == NULL) return 0;
    (void)fputs("// Macaronica: nucleos da DSL vetorial (docs/dsl.md, Secao 6).\n"
                "// Desca com scripts/pipeline.sh e ligue ao .ll com llvm-link.\n"
                "module {\n", output);
    for (index = 0U; index < kernels->count; ++index) {
        if (is_reduction(&kernels->items[index]))
            write_mlir_reduction(output, &kernels->items[index]);
        else
            write_mlir_elementwise(output, &kernels->items[index]);
    }
    (void)fputs("}\n", output);
    return !ferror(output);
}

int codegen_mlir_write_llvm_declarations(FILE *output, const AstNode *program,
                                         const SemanticResult *semantic)
{
    DslKernelList kernels;
    size_t index;
    if (output == NULL || !dsl_collect_kernels(program, semantic, &kernels)) return 0;
    if (kernels.count != 0U)
        (void)fputs("; Nucleos da DSL (MLIR, docs/dsl.md): cada memref<?xT> descido\n"
                    "; vira ptr alocado, ptr alinhado, deslocamento, tamanho e passo.\n",
                    output);
    for (index = 0U; index < kernels.count; ++index) {
        const DslOperation *kernel = &kernels.items[index];
        DslArgument arguments[3];
        size_t count = dsl_kernel_arguments(kernel, arguments);
        size_t argument;
        (void)fprintf(output, "declare %s @%s(",
                      is_reduction(kernel) ? llvm_type(kernel->result_type_id) : "void",
                      kernel->kernel);
        for (argument = 0U; argument < count; ++argument) {
            if (argument != 0U) (void)fputs(", ", output);
            (void)fputs(arguments[argument].is_scalar ? llvm_type(kernel->result_type_id)
                                                      : "ptr, ptr, i64, i64, i64",
                        output);
        }
        (void)fputs(") nounwind\n", output);
    }
    if (kernels.count != 0U) (void)fputs("\n", output);
    dsl_kernel_list_destroy(&kernels);
    return !ferror(output);
}

/* Mesmo calculo do MLIR, em C99. A inteira faz a conta em uint64_t e volta
 * com mac_i64, do preludio do back-end C (modulo 2^64, D-021). */
static void write_c_kernel(FILE *out, const DslOperation *kernel)
{
    size_t a = kernel->element_type_ids[0];
    size_t b = kernel->element_type_ids[1];
    size_t r = kernel->result_type_id;
    size_t c = kernel->target_type_id;
    const char *op = c_operator(kernel->op);
    const char *zero = r == TYPE_ID_FLOAT ? "0.0f" : "0.0";
    int integer = r == TYPE_ID_INTEGER;
    switch (kernel->form) {
    case DSL_FORM_VECTOR_VECTOR:
        (void)fprintf(out, "static void %s(const %s *a, const %s *b, %s *c, int64_t n)\n"
                           "{\n  int64_t i;\n  for (i = 0; i < n; ++i)\n",
                      kernel->kernel, c_type(a), c_type(b), c_type(c));
        if (integer)
            (void)fprintf(out, "    c[i] = mac_i64((uint64_t)a[i] %s (uint64_t)b[i]);\n}\n\n", op);
        else
            (void)fprintf(out, "    c[i] = (%s)((%s)a[i] %s (%s)b[i]);\n}\n\n",
                          c_type(c), c_type(r), op, c_type(r));
        break;
    case DSL_FORM_SCALAR_VECTOR:
    case DSL_FORM_VECTOR_SCALAR: {
        int left = kernel->form == DSL_FORM_SCALAR_VECTOR;
        if (left)
            (void)fprintf(out, "static void %s(%s k, const %s *a, %s *c, int64_t n)\n",
                          kernel->kernel, c_type(r), c_type(a), c_type(c));
        else
            (void)fprintf(out, "static void %s(const %s *a, %s k, %s *c, int64_t n)\n",
                          kernel->kernel, c_type(a), c_type(r), c_type(c));
        (void)fputs("{\n  int64_t i;\n  for (i = 0; i < n; ++i)\n", out);
        if (integer)
            (void)fprintf(out, left ? "    c[i] = mac_i64((uint64_t)k %s (uint64_t)a[i]);\n}\n\n"
                                    : "    c[i] = mac_i64((uint64_t)a[i] %s (uint64_t)k);\n}\n\n",
                          op);
        else if (left)
            (void)fprintf(out, "    c[i] = (%s)(k %s (%s)a[i]);\n}\n\n",
                          c_type(c), op, c_type(r));
        else
            (void)fprintf(out, "    c[i] = (%s)((%s)a[i] %s k);\n}\n\n",
                          c_type(c), c_type(r), op);
        break;
    }
    case DSL_FORM_SUM:
        (void)fprintf(out, "static %s %s(const %s *a, int64_t n)\n{\n  int64_t i;\n",
                      c_type(r), kernel->kernel, c_type(a));
        if (integer)
            (void)fputs("  uint64_t s = 0;\n"
                        "  for (i = 0; i < n; ++i) s += (uint64_t)a[i];\n"
                        "  return mac_i64(s);\n}\n\n", out);
        else
            (void)fprintf(out, "  %s s = %s;\n"
                               "  for (i = 0; i < n; ++i) s = s + a[i];\n"
                               "  return s;\n}\n\n", c_type(r), zero);
        break;
    case DSL_FORM_MAX:
        (void)fprintf(out, "static %s %s(const %s *a, int64_t n)\n"
                           "{\n  %s m = a[0];\n  int64_t i;\n"
                           "  for (i = 1; i < n; ++i)\n"
                           "    if (a[i] > m) m = a[i];\n"
                           "  return m;\n}\n\n",
                      c_type(r), kernel->kernel, c_type(a), c_type(r));
        break;
    default:
        (void)fprintf(out, "static %s %s(const %s *a, const %s *b, int64_t n)\n{\n  int64_t i;\n",
                      c_type(r), kernel->kernel, c_type(a), c_type(b));
        if (integer)
            (void)fputs("  uint64_t s = 0;\n"
                        "  for (i = 0; i < n; ++i) s += (uint64_t)a[i] * (uint64_t)b[i];\n"
                        "  return mac_i64(s);\n}\n\n", out);
        else
            (void)fprintf(out, "  %s s = %s;\n"
                               "  for (i = 0; i < n; ++i) s = s + (%s)a[i] * (%s)b[i];\n"
                               "  return s;\n}\n\n",
                          c_type(r), zero, c_type(r), c_type(r));
        break;
    }
}

int codegen_mlir_write_c_kernels(FILE *output, const AstNode *program,
                                 const SemanticResult *semantic)
{
    DslKernelList kernels;
    size_t index;
    if (output == NULL || !dsl_collect_kernels(program, semantic, &kernels)) return 0;
    if (kernels.count != 0U)
        (void)fputs("/* Nucleos da DSL vetorial (docs/dsl.md, Secao 6): os mesmos da\n"
                    " * rota MLIR, como lacos simples para alvos sem MLIR (ESP32). */\n",
                    output);
    for (index = 0U; index < kernels.count; ++index)
        write_c_kernel(output, &kernels.items[index]);
    dsl_kernel_list_destroy(&kernels);
    return !ferror(output);
}

CompilerExitCode codegen_mlir_write_companion(const AstNode *program,
                                              const SemanticResult *semantic,
                                              const char *ll_path)
{
    DslKernelList kernels;
    CompilerExitCode result = COMPILER_EXIT_SUCCESS;
    FILE *output;
    char *path;
    size_t length;
    if (program == NULL || semantic == NULL || ll_path == NULL)
        return COMPILER_EXIT_INTERNAL_ERROR;
    if (!dsl_collect_kernels(program, semantic, &kernels)) {
        (void)fputs("erro interno: memoria insuficiente para os nucleos da DSL\n", stderr);
        return COMPILER_EXIT_INTERNAL_ERROR;
    }
    if (kernels.count == 0U) {
        dsl_kernel_list_destroy(&kernels);
        return COMPILER_EXIT_SUCCESS;
    }
    length = strlen(ll_path);
    path = (char *)malloc(length + sizeof(".mlir"));
    if (path == NULL) {
        dsl_kernel_list_destroy(&kernels);
        (void)fputs("erro interno: memoria insuficiente para o caminho do .mlir\n", stderr);
        return COMPILER_EXIT_INTERNAL_ERROR;
    }
    (void)memcpy(path, ll_path, length + 1U);
    if (length > 3U && strcmp(path + length - 3U, ".ll") == 0) path[length - 3U] = '\0';
    (void)strcat(path, ".mlir");
    output = fopen(path, "wb");
    if (output == NULL) {
        (void)fprintf(stderr, "erro de E/S: nao foi possivel abrir `%s` para escrita: %s\n",
                      path, strerror(errno));
        result = COMPILER_EXIT_IO_ERROR;
    } else {
        int failed = !codegen_mlir_write_module(output, &kernels);
        if (fclose(output) != 0) failed = 1;
        if (failed) {
            (void)fprintf(stderr, "erro de E/S: falha ao gravar `%s`\n", path);
            (void)remove(path);
            result = COMPILER_EXIT_IO_ERROR;
        }
    }
    free(path);
    dsl_kernel_list_destroy(&kernels);
    return result;
}
