#ifndef MACARONICA_IR_CODEGEN_LLVM_H
#define MACARONICA_IR_CODEGEN_LLVM_H

#include "../semantic/semantic.h"

/*
 * Emite LLVM IR diretamente da AST aceita pela semantica. A AST e as tabelas
 * sao emprestadas e precisam continuar vivas durante a chamada.
 *
 * O chamador e dono de output: deve conferir fclose e publicar o arquivo
 * somente se esta funcao retornar OK. Uma falha de escrita pode deixar uma
 * saida parcial, que deve ser descartada pelo chamador.
 */
FrontendStatus codegen_llvm_emit(const SourceFile *source,
                                const AstNode *program,
                                const SemanticResult *semantic,
                                DiagnosticBag *diagnostics,
                                FILE *output);

#endif
