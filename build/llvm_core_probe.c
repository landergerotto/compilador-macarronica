#include "common/source.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantic/semantic.h"
#include "ir/codegen_llvm.h"
#include <string.h>
int main(int argc, char **argv)
{
    SourceFile source;
    DiagnosticBag bag;
    TokenBuffer tokens;
    AstNode *ast = NULL;
    SemanticResult semantic;
    char error[512];
    FILE *out;
    int status = 1;
    if (argc != 3) return 2;
    source_file_from_buffer(&source, NULL, NULL, 0);
    diagnostic_bag_init(&bag);
    token_buffer_init(&tokens);
    if (!semantic_result_init(&semantic)) return 4;
    if (!source_file_load(&source, argv[1], error, sizeof(error))) return 3;
    if (lexer_scan_all(&source, &bag, &tokens) != FRONTEND_STATUS_OK) goto done;
    if (parser_parse(&source, &tokens, &bag, &ast) != FRONTEND_STATUS_OK) goto done;
    if (semantic_analyze(&source, ast, &bag, &semantic) != FRONTEND_STATUS_OK) goto done;
    out = fopen(argv[2], "wb");
    if (!out) goto done;
    status = codegen_llvm_emit(&source, ast, &semantic, &bag, out) == FRONTEND_STATUS_OK ? 0 : 4;
    if (fclose(out)) status = 3;
done:
    diagnostic_bag_print(&bag, stderr);
    semantic_result_destroy(&semantic);
    ast_node_destroy(ast);
    token_buffer_destroy(&tokens);
    diagnostic_bag_destroy(&bag);
    source_file_destroy(&source);
    return status;
}
