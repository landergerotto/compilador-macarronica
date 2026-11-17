#ifndef MACARONICA_IR_CODEGEN_MLIR_H
#define MACARONICA_IR_CODEGEN_MLIR_H
#include "../semantic/semantic.h"

/* Modulo hibrido: somente nucleos vetoriais. O restante fica no LLVM host.
 * Memrefs estaticos usam bare pointers no lowering, sem descritores na ABI. */
FrontendStatus codegen_mlir_emit(const SourceFile *source, const AstNode *program,
                                 const SemanticResult *semantic,
                                 DiagnosticBag *diagnostics, FILE *output);
#endif
