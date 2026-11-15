#ifndef MACARONICA_ZL_PLACA_H
#define MACARONICA_ZL_PLACA_H

/* Regras dos servicos fisicos da placa DOIT ESP32 DevKit V1 (modulo
 * ESP32-WROOM-32): quais pinos existem, quais podem ser saida, quais modos
 * cada um aceita e o que vale para espera. Usadas pelo runtime da placa
 * (zl_runtime_esp32.c) e pela simulacao no PC (zl_placa_pc.c), para que um
 * uso invalido gere o mesmo erro, com o mesmo texto, nos dois.
 *
 * Numeracao: o numero do pino na linguagem e o numero do GPIO do ESP32 (o que
 * esta impresso na placa como D2, D4...). Nao ha numeracao propria.
 *
 * Mesmo esquema de zl_formato.h: o arquivo que inclui com
 * ZL_PLACA_IMPLEMENTACAO definido recebe as definicoes. */
#include <stdint.h>

/* Erro de execucao dos servicos da placa (uso invalido de pino ou espera).
 * Os codigos 1 a 7 sao os de zl_runtime.h; o 8 e so dos servicos da placa. */
#define ZL_ERRO_PLACA 8

/* Modos de pinoModo(p, modo). */
enum {
    ZL_MODO_ENTRADA = 0,
    ZL_MODO_SAIDA = 1,
    ZL_MODO_ENTRADA_PULLUP = 2,
    ZL_MODO_ENTRADA_PULLDOWN = 3
};

/* Maior GPIO do ESP32 mais um. */
#define ZL_PLACA_PINOS 40

/* Estado que cada runtime guarda por pino (0 = nao configurado). */
enum { ZL_PINO_LIVRE = 0, ZL_PINO_ENTRADA = 1, ZL_PINO_SAIDA = 2 };

/* Cada regra devolve NULL se o uso e valido ou o texto do erro. */
const char *zl_placa_regra_modo(int32_t pino, int32_t modo);
const char *zl_placa_regra_escrita(int32_t pino, int32_t nivel, int estado);
const char *zl_placa_regra_leitura(int32_t pino, int estado);
const char *zl_placa_regra_espera(int32_t milissegundos);

#ifdef ZL_PLACA_IMPLEMENTACAO

#include <stddef.h>

/* GPIO 20, 24 e 28 a 31 nao existem no ESP32-WROOM-32. */
static int zl_placa_existe(int32_t pino)
{
    return pino >= 0 && pino < ZL_PLACA_PINOS && pino != 20 && pino != 24 &&
           (pino < 28 || pino > 31);
}

static const char *zl_placa_regra_pino(int32_t pino)
{
    if (!zl_placa_existe(pino)) return "pino inexistente na DOIT ESP32 DevKit V1";
    /* 6 a 11 ligam a memoria flash; 1 e 3 sao TX/RX da serial USB (UART0):
     * mexer neles trava a placa ou corta a saida do programa. */
    if (pino >= 6 && pino <= 11) return "pino reservado para a memoria flash (GPIO6 a GPIO11)";
    if (pino == 1 || pino == 3) return "pino reservado para a serial USB (GPIO1 e GPIO3)";
    return NULL;
}

const char *zl_placa_regra_modo(int32_t pino, int32_t modo)
{
    const char *erro = zl_placa_regra_pino(pino);
    if (erro != NULL) return erro;
    if (modo < ZL_MODO_ENTRADA || modo > ZL_MODO_ENTRADA_PULLDOWN)
        return "modo invalido (0 entrada, 1 saida, 2 pull-up, 3 pull-down)";
    /* 34 a 39 sao so de entrada e nao tem resistor interno. */
    if (modo == ZL_MODO_SAIDA && pino >= 34) return "pino so de entrada (GPIO34 a GPIO39)";
    if ((modo == ZL_MODO_ENTRADA_PULLUP || modo == ZL_MODO_ENTRADA_PULLDOWN) && pino >= 34)
        return "pino sem pull-up/pull-down interno (GPIO34 a GPIO39)";
    return NULL;
}

const char *zl_placa_regra_escrita(int32_t pino, int32_t nivel, int estado)
{
    const char *erro = zl_placa_regra_pino(pino);
    if (erro != NULL) return erro;
    if (estado != ZL_PINO_SAIDA) return "pinoEscreve em pino nao configurado como saida";
    if (nivel != 0 && nivel != 1) return "nivel invalido";
    return NULL;
}

const char *zl_placa_regra_leitura(int32_t pino, int estado)
{
    const char *erro = zl_placa_regra_pino(pino);
    if (erro != NULL) return erro;
    if (estado != ZL_PINO_ENTRADA) return "pinoLe em pino nao configurado como entrada";
    return NULL;
}

const char *zl_placa_regra_espera(int32_t milissegundos)
{
    return milissegundos < 0 ? "espera negativa" : NULL;
}

#endif /* ZL_PLACA_IMPLEMENTACAO */
#endif /* MACARONICA_ZL_PLACA_H */
