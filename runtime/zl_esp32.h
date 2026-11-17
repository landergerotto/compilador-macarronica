#ifndef MACARONICA_ZL_ESP32_H
#define MACARONICA_ZL_ESP32_H

/* Ligacao entre o runtime da placa (zl_runtime_esp32.c, em C) e a rota que o
 * empacota. Cada rota fornece as quatro funcoes de serial e chama
 * zl_esp32_inicia uma vez:
 *   - rota Arduino (make placa): zl_plataforma_arduino.cpp usa a Serial e o
 *     sketch chama zl_esp32_inicia em setup();
 *   - rota ESP-IDF (ROTA=idf, tools/esp32.ps1): zl_plataforma_idf.c usa o
 *     driver de UART e o app_main chama zl_esp32_inicia. */
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Pilha, em bytes, da tarefa FreeRTOS que executa zl_principal. Deve ser o
 * mesmo numero do --ram-limite do compilador: e nela que ficam as variaveis
 * locais (e os vetores locais) de cada funcao Macaronica. A rota Arduino
 * (tools/placa.sh) escreve o numero em zl_config.h, no sketch; sem esse
 * arquivo (rota ESP-IDF) vale o padrao. */
#if defined(__has_include)
#if __has_include("zl_config.h")
#include "zl_config.h"
#endif
#endif
#ifndef ZL_PILHA_BYTES
#define ZL_PILHA_BYTES 65536
#endif

/* LED azul da DOIT DevKit V1 (ativo em nivel alto): pisca sem parar depois de
 * um erro de execucao. */
#define ZL_LED_ERRO 2

/* Velocidade da serial (UART0, a mesma do USB da placa). */
#define ZL_SERIAL_BAUD 115200

/* Abre a serial. Chamada uma vez, antes de qualquer outra. */
void zl_plataforma_inicia(void);
/* Envia bytes pela serial. */
void zl_plataforma_escreve(const char *dados, size_t tamanho);
/* Espera a transmissao terminar (antes de parar no erro de execucao). */
void zl_plataforma_descarrega(void);
/* Bloqueia ate chegar um byte; devolve 0..255 (ou -1 se a serial falhar). */
int zl_plataforma_le_byte(void);

/* Abre a serial e cria a tarefa que executa zl_principal, com pilha de
 * ZL_PILHA_BYTES. Volta logo; o programa roda na tarefa nova. */
void zl_esp32_inicia(void);

#ifdef __cplusplus
}
#endif

#endif
