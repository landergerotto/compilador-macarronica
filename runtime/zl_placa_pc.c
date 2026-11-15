/* Simulacao, no PC, dos servicos fisicos da placa: pinoModo, pinoEscreve,
 * pinoLe e espera. Liga junto com zl_runtime_posix.c para rodar no PC um
 * programa gerado com --alvo=esp32 (make test-c, tests/placa).
 *
 * As regras sao as de zl_placa.h, as mesmas do runtime da placa: um uso
 * invalido de pino gera o mesmo erro, com o mesmo texto, nos dois. Como nao
 * ha pino de verdade:
 * - pinoEscreve so valida;
 * - pinoLe devolve o que um pino sem nada ligado le na placa: 1 com pull-up
 *   (por exemplo o botao BOOT solto, no GPIO0), 0 nos demais modos;
 * - espera dorme de verdade, salvo com ZL_PLACA_ESPERA=0 no ambiente (os
 *   testes usam isso para nao demorar). */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#endif

#include "zl_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zl_formato.h"
#define ZL_PLACA_IMPLEMENTACAO
#include "zl_placa.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <time.h>
#endif

static unsigned char estado_pino[ZL_PLACA_PINOS];
static unsigned char modo_pino[ZL_PLACA_PINOS];

/* Mesma linha do runtime POSIX (stderr, status 1), com o texto da regra. */
static void erro_placa(int32_t linha, const char *regra)
{
    char texto[ZL_FORMATO_ERRO_TAMANHO];
    (void)fflush(stdout);
    (void)fprintf(stderr, "%s\n",
                  zl_formato_erro_mensagem(ZL_ERRO_PLACA, linha, regra, texto));
    exit(EXIT_FAILURE);
}

static int estado(int32_t pino)
{
    return pino >= 0 && pino < ZL_PLACA_PINOS ? estado_pino[pino] : ZL_PINO_LIVRE;
}

void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha)
{
    const char *erro = zl_placa_regra_modo(pino, modo);
    if (erro != NULL) erro_placa(linha, erro);
    estado_pino[pino] = (unsigned char)(modo == ZL_MODO_SAIDA ? ZL_PINO_SAIDA : ZL_PINO_ENTRADA);
    modo_pino[pino] = (unsigned char)modo;
}

void zl_pino_escreve(int32_t pino, int32_t nivel, int32_t linha)
{
    const char *erro = zl_placa_regra_escrita(pino, nivel, estado(pino));
    if (erro != NULL) erro_placa(linha, erro);
}

int32_t zl_pino_le(int32_t pino, int32_t linha)
{
    const char *erro = zl_placa_regra_leitura(pino, estado(pino));
    if (erro != NULL) erro_placa(linha, erro);
    return modo_pino[pino] == ZL_MODO_ENTRADA_PULLUP ? 1 : 0;
}

void zl_espera_ms(int32_t milissegundos, int32_t linha)
{
    const char *erro = zl_placa_regra_espera(milissegundos);
    const char *ambiente = getenv("ZL_PLACA_ESPERA");
    if (erro != NULL) erro_placa(linha, erro);
    if (milissegundos == 0 || (ambiente != NULL && strcmp(ambiente, "0") == 0)) return;
#ifdef _WIN32
    Sleep((DWORD)milissegundos);
#else
    {
        struct timespec resta;
        resta.tv_sec = (time_t)(milissegundos / 1000);
        resta.tv_nsec = (long)(milissegundos % 1000) * 1000000L;
        while (nanosleep(&resta, &resta) != 0) {
        }
    }
#endif
}
