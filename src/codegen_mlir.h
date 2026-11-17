#ifndef MACARONICA_CODEGEN_MLIR_H
#define MACARONICA_CODEGEN_MLIR_H

/*
 * Nucleos da DSL vetorial (docs/dsl.md, Secao 6). Estrategia hibrida: o
 * programa continua em LLVM IR e so as operacoes vetoriais viram funcoes MLIR
 * (func, arith, memref, affine), descidas por scripts/pipeline.sh e ligadas ao
 * .ll com llvm-link. Para a placa, os mesmos nucleos saem como lacos em C.
 */

#include "compiler.h"
#include "semantic/dsl.h"

#include <stdio.h>

/* Modulo MLIR com um func.func por nucleo da lista. Devolve 1 se escreveu. */
int codegen_mlir_write_module(FILE *output, const DslKernelList *kernels);

/* `declare` LLVM de cada nucleo usado, com o memref descido em 5 campos. */
int codegen_mlir_write_llvm_declarations(FILE *output, const AstNode *program,
                                         const SemanticResult *semantic);

/* Os mesmos nucleos como funcoes C `static`, para o back-end C. */
int codegen_mlir_write_c_kernels(FILE *output, const AstNode *program,
                                 const SemanticResult *semantic);

/* Se o programa usa a DSL, grava os nucleos em <saida sem .ll>.mlir. */
CompilerExitCode codegen_mlir_write_companion(const AstNode *program,
                                              const SemanticResult *semantic,
                                              const char *ll_path);

#endif
