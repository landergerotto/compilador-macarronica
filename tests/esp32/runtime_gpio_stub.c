#include "zl_runtime.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

/* Este runtime captura chamadas no host. Nao dirige GPIO fisico. */
static int configurado;
static int esperando;
static int escritas;
static int esperas;

static void falha(const char *mensagem)
{
    (void)fprintf(stderr, "stub GPIO: %s\n", mensagem);
    exit(2);
}

void zl_erro_execucao(int32_t codigo, int32_t linha)
{
    (void)fprintf(stderr, "erro inesperado %" PRId32 " linha %" PRId32 "\n", codigo, linha);
    exit(3);
}

void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha)
{
    if (configurado || pino != 2 || modo != 1 || linha <= 0)
        falha("configuracao deve ocorrer uma vez: GPIO2 saida");
    configurado = 1;
    (void)printf("modo 2 1 %" PRId32 "\n", linha);
}

void zl_pino_escreve(int32_t pino, int32_t nivel, int32_t linha)
{
    int esperado = (escritas % 2) == 0 ? 1 : 0;
    if (!configurado || esperando || pino != 2 || nivel != esperado || linha <= 0)
        falha("escritas devem alternar 1/0 e ser separadas por espera");
    ++escritas;
    esperando = 1;
    (void)printf("escreve 2 %" PRId32 " %" PRId32 "\n", nivel, linha);
}

void zl_espera_ms(int32_t milissegundos, int32_t linha)
{
    if (!esperando || milissegundos != 1000 || linha <= 0)
        falha("cada escrita deve ser seguida de espera(1000)");
    esperando = 0;
    ++esperas;
    (void)printf("espera 1000 %" PRId32 "\n", linha);
    if (esperas == 6) exit(0); /* Limita o teste a tres ciclos completos. */
}

/* Mensagens UART nao fazem parte do contrato GPIO capturado no host. */
void zl_imprime_texto(const char *valor) { (void)valor; }
void zl_imprime_nl(void) { }

int main(void)
{
    (void)zl_principal();
    falha("principal retornou: faltou laco infinito");
    return 2;
}

