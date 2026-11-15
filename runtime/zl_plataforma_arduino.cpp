/* Serial da rota Arduino (make placa, nucleo arduino-esp32 3.x).
 *
 * A Serial do Arduino e um objeto C++; o runtime da placa e C. Esta ponte
 * expoe as quatro funcoes de zl_esp32.h com ligacao C. O sketch que o make
 * monta chama zl_esp32_inicia() em setup(); o programa roda numa tarefa
 * FreeRTOS propria (zl_runtime_esp32.c), nao na tarefa do loop(), que tem so
 * 8 KB de pilha. */
#include <Arduino.h>

#include "zl_esp32.h"

extern "C" void zl_plataforma_inicia(void)
{
    /* O buffer de recepcao padrao e de 256 bytes; com 1024 cabe uma entrada
     * colada de uma vez. Precisa vir antes de begin(). */
    (void)Serial.setRxBufferSize(1024);
    Serial.begin(ZL_SERIAL_BAUD);
}

extern "C" void zl_plataforma_escreve(const char *dados, size_t tamanho)
{
    (void)Serial.write(reinterpret_cast<const uint8_t *>(dados), tamanho);
}

extern "C" void zl_plataforma_descarrega(void)
{
    Serial.flush();
}

/* Serial.read() nao bloqueia: espera 1 tick (1 ms) entre tentativas, sem
 * ocupar a CPU. A 115200 baud chegam ~12 bytes por ms; o buffer guarda. */
extern "C" int zl_plataforma_le_byte(void)
{
    for (;;) {
        int caractere = Serial.read();
        if (caractere >= 0) return caractere;
        vTaskDelay(1);
    }
}
