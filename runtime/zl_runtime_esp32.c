/* Runtime ESP-IDF: apenas a plataforma e manual; o programa vem da AST. */
#include "zl_runtime.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* 0 = nao configurado, 1 = entrada, 2 = saida. */
static unsigned char zl_gpio_modos[GPIO_NUM_MAX];

void zl_erro_execucao(int32_t codigo, int32_t linha)
{
    (void)fflush(stdout);
    (void)fprintf(stderr, "linha %" PRId32 ": erro de execucao %" PRId32 "\n", linha, codigo);
    (void)fflush(stderr);
    /* Encerra somente a tarefa do programa; nao reinicia em ciclo de erro. */
    vTaskDelete(NULL);
    for (;;) { }
}

static void zl_gpio_erro(int32_t linha, const char *mensagem)
{
    (void)fprintf(stderr, "GPIO: %s\n", mensagem);
    zl_erro_execucao(8, linha);
}

static void zl_gpio_valida(int32_t pino, int32_t linha)
{
    if (pino < 0 || pino >= GPIO_NUM_MAX || !GPIO_IS_VALID_GPIO(pino) ||
        (pino >= 6 && pino <= 11)) {
        zl_gpio_erro(linha, "pino inexistente ou reservado para flash");
    }
}

void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha)
{
    gpio_config_t config = {0};
    zl_gpio_valida(pino, linha);
    if (modo < 0 || modo > 3) zl_gpio_erro(linha, "modo invalido");
    if (modo == 1 && !GPIO_IS_VALID_OUTPUT_GPIO(pino))
        zl_gpio_erro(linha, "pino nao permite saida");
    if ((modo == 2 || modo == 3) && pino >= 34)
        zl_gpio_erro(linha, "pino nao permite pull interno");
    config.pin_bit_mask = UINT64_C(1) << (unsigned int)pino;
    config.mode = modo == 1 ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    config.pull_up_en = modo == 2 ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    config.pull_down_en = modo == 3 ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&config) != ESP_OK) zl_gpio_erro(linha, "falha ao configurar pino");
    zl_gpio_modos[pino] = modo == 1 ? 2U : 1U;
}

void zl_pino_escreve(int32_t pino, int32_t nivel, int32_t linha)
{
    zl_gpio_valida(pino, linha);
    if (zl_gpio_modos[pino] != 2U) zl_gpio_erro(linha, "saida nao configurada");
    if (nivel != 0 && nivel != 1) zl_gpio_erro(linha, "nivel deve ser 0 ou 1");
    if (gpio_set_level((gpio_num_t)pino, (uint32_t)nivel) != ESP_OK)
        zl_gpio_erro(linha, "falha ao escrever pino");
}

int32_t zl_pino_le(int32_t pino, int32_t linha)
{
    zl_gpio_valida(pino, linha);
    if (zl_gpio_modos[pino] != 1U) zl_gpio_erro(linha, "entrada nao configurada");
    return gpio_get_level((gpio_num_t)pino) != 0 ? 1 : 0;
}

void zl_espera_ms(int32_t milissegundos, int32_t linha)
{
    uint64_t ticks;
    if (milissegundos < 0) zl_gpio_erro(linha, "duracao negativa");
    if (milissegundos == 0) return;
    ticks = ((uint64_t)(uint32_t)milissegundos * configTICK_RATE_HZ + 999U) / 1000U;
    if (ticks >= (uint64_t)portMAX_DELAY) zl_gpio_erro(linha, "duracao excede ticks");
    vTaskDelay((TickType_t)ticks);
}

void zl_imprime_texto(const char *valor)
{
    (void)fputs(valor != NULL ? valor : "", stdout);
}

void zl_imprime_nl(void)
{
    (void)fputc('\n', stdout);
    (void)fflush(stdout);
}

void app_main(void)
{
    int32_t resultado;
    (void)puts("Macaronica: iniciando zl_principal");
    resultado = zl_principal();
    (void)printf("Macaronica: zl_principal retornou %" PRId32 "\n", resultado);
}