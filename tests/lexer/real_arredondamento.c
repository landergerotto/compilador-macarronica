#include "common/diagnostic.h"
#include "common/source.h"
#include "lexer/lexer.h"
#include "lexer/token.h"

#include <stdio.h>
#include <string.h>

static int verifica(const char *texto, float esperado)
{
    SourceFile fonte;
    DiagnosticBag diagnosticos;
    TokenBuffer tokens;
    FrontendStatus status;
    int ok;

    source_file_from_buffer(&fonte, "<teste>", texto, strlen(texto));
    diagnostic_bag_init(&diagnosticos);
    token_buffer_init(&tokens);
    status = lexer_scan_all(&fonte, &diagnosticos, &tokens);
    ok = status == FRONTEND_STATUS_OK && tokens.count == 2U &&
         tokens.items[0].kind == TOKEN_LITERAL_REAL &&
         tokens.items[0].literal.real == (double)esperado;
    if (!ok) {
        (void)fprintf(stderr, "arredondamento incorreto para %s\n", texto);
    }
    token_buffer_destroy(&tokens);
    diagnostic_bag_destroy(&diagnosticos);
    return ok;
}

int main(void)
{
    /* Empate: arredonda para o valor com ultimo bit par. */
    int empate = verifica("1.000000536441802978515625", 0x1.000008p0f);
    /* Um valor apenas acima do empate deve arredondar para cima, mesmo quando
     * arredondar primeiro para double apagaria a diferenca. */
    int acima = verifica("1.000000536441802978515625000000000001", 0x1.00000ap0f);
    return empate && acima ? 0 : 1;
}
