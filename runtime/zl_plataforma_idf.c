/* Serial e ponto de entrada da rota ESP-IDF (ROTA=idf, tools/esp32.ps1).
 *
 * Usa o driver de UART do ESP-IDF na UART0, a mesma porta do USB da placa e
 * do console. app_main so cria a tarefa do programa (zl_esp32_inicia) e
 * retorna; no ESP-IDF isso encerra apenas a tarefa main. */
#include "zl_esp32.h"

#include <stdint.h>

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ZL_UART UART_NUM_0
#define ZL_UART_RECEPCAO 1024

void zl_plataforma_inicia(void)
{
    /* O console do ESP-IDF ja deixa a UART0 a 115200, mas escreve direto na
     * FIFO, sem driver. O driver e o que permite ler com bloqueio. Sem buffer
     * de transmissao: cada escrita espera os bytes entrarem na FIFO. */
    if (!uart_is_driver_installed(ZL_UART)) {
        (void)uart_driver_install(ZL_UART, ZL_UART_RECEPCAO, 0, 0, NULL, 0);
    }
}

void zl_plataforma_escreve(const char *dados, size_t tamanho)
{
    (void)uart_write_bytes(ZL_UART, dados, tamanho);
}

void zl_plataforma_descarrega(void)
{
    (void)uart_wait_tx_done(ZL_UART, portMAX_DELAY);
}

int zl_plataforma_le_byte(void)
{
    uint8_t byte = 0U;
    for (;;) {
        int lidos = uart_read_bytes(ZL_UART, &byte, 1U, portMAX_DELAY);
        if (lidos == 1) return (int)byte;
        if (lidos < 0) return -1;
    }
}

void app_main(void)
{
    zl_esp32_inicia();
}
