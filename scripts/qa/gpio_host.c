/*
 * Stub de GPIO da frente qa: permite executar no PC o C gerado com
 * --alvo=esp32, ligado ao runtime POSIX (que nao tem GPIO).
 *
 * Nao dirige pino nenhum. Cada chamada vira uma linha no stderr com o prefixo
 * "gpio:" (scripts/qa/diferencial.sh descarta essas linhas antes de comparar),
 * a espera nao espera e a leitura devolve sempre nivel baixo. As regras de pino
 * e modo pertencem ao runtime da placa e sao testadas pela frente da placa.
 */
#include "zl_runtime.h"

#include <inttypes.h>
#include <stdio.h>

void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha)
{
    (void)fprintf(stderr, "gpio: linha %" PRId32 ": modo pino %" PRId32 " = %" PRId32 "\n",
                  linha, pino, modo);
}

void zl_pino_escreve(int32_t pino, int32_t nivel, int32_t linha)
{
    (void)fprintf(stderr, "gpio: linha %" PRId32 ": escreve pino %" PRId32 " = %" PRId32 "\n",
                  linha, pino, nivel);
}

int32_t zl_pino_le(int32_t pino, int32_t linha)
{
    (void)fprintf(stderr, "gpio: linha %" PRId32 ": le pino %" PRId32 "\n", linha, pino);
    return 0;
}

void zl_espera_ms(int32_t milissegundos, int32_t linha)
{
    (void)fprintf(stderr, "gpio: linha %" PRId32 ": espera %" PRId32 " ms\n", linha, milissegundos);
}
