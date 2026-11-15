/* Runtime da placa DOIT ESP32 DevKit V1 (Xtensa LX6, ESP-IDF e FreeRTOS).
 *
 * Implementa toda a interface runtime/zl_runtime.h para o C gerado pelo
 * compilador com --alvo=esp32. Escrita a mao e so a plataforma: o programa
 * vem inteiro do compilador. Serve as duas rotas de build (Arduino e ESP-IDF);
 * o que muda entre elas (a serial e o ponto de entrada) fica em
 * zl_plataforma_arduino.cpp e zl_plataforma_idf.c, atras de zl_esp32.h.
 *
 * Mesma saida que o PC: o texto de cada valor, a mensagem de erro e a leitura
 * vem de zl_formato.h, o mesmo codigo do runtime POSIX. A diferenca e o
 * destino: no PC, stdout e stderr; aqui, tudo pela serial, na mesma ordem. */
#include "zl_runtime.h"
#include "zl_esp32.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define ZL_FORMATO_IMPLEMENTACAO
#include "zl_formato.h"
#define ZL_PLACA_IMPLEMENTACAO
#include "zl_placa.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || DBL_MANT_DIG != 53
#error "O runtime requer float binary32 e double binary64"
#endif

/* Marcas na serial em volta da saida do programa. Nao sao saida do programa:
 * o script de captura (tools/placa_serial.ps1) compara com o .esperado so o
 * que fica entre elas. A marca final comeca com '\n' para nao grudar numa
 * ultima linha sem quebra; o script tira exatamente essa quebra. */
#define ZL_MARCA_INICIO "--- Macaronica: inicio do programa ---\n"
#define ZL_MARCA_FIM "\n--- Macaronica: fim do programa, status "
#define ZL_MARCA_ERRO "\n--- Macaronica: erro de execucao, LED GPIO2 piscando ---\n"

/* ---------------------------------------------------------------- saida */

static void zl_escreve(const char *texto)
{
    zl_plataforma_escreve(texto, strlen(texto));
}

void zl_imprime_int(int64_t valor)
{
    char texto[ZL_FORMATO_TAMANHO];
    zl_escreve(zl_formato_int(valor, texto));
}

void zl_imprime_flut(float valor)
{
    char texto[ZL_FORMATO_TAMANHO];
    zl_escreve(zl_formato_flut(valor, texto));
}

/* duplocarpado e binary64: o FPU do ESP32 so tem precisao simples, entao as
 * contas em double (inclusive esta conversao para texto) sao feitas em
 * software pela libgcc/newlib, com o mesmo resultado IEEE 754 do PC. */
void zl_imprime_duplocarpado(double valor)
{
    char texto[ZL_FORMATO_TAMANHO];
    zl_escreve(zl_formato_duplocarpado(valor, texto));
}

void zl_imprime_texto(const char *valor)
{
    zl_escreve(valor != NULL ? valor : "");
}

void zl_imprime_bool(int32_t valor)
{
    zl_escreve(zl_formato_bool(valor));
}

void zl_imprime_nl(void)
{
    zl_escreve("\n");
}

/* ------------------------------------------------- erro de execucao */

/* Depois de um erro a placa nao reinicia (isso repetiria o programa): avisa
 * pela serial e pisca o LED azul (GPIO2) para sempre, um sinal visivel mesmo
 * sem o computador. So esta tarefa para; o resto do sistema segue vivo. */
static void zl_para_com_led(void)
{
    zl_escreve(ZL_MARCA_ERRO);
    zl_plataforma_descarrega();
    (void)gpio_reset_pin((gpio_num_t)ZL_LED_ERRO);
    (void)gpio_set_direction((gpio_num_t)ZL_LED_ERRO, GPIO_MODE_OUTPUT);
    for (;;) {
        (void)gpio_set_level((gpio_num_t)ZL_LED_ERRO, 1U);
        vTaskDelay(pdMS_TO_TICKS(250));
        (void)gpio_set_level((gpio_num_t)ZL_LED_ERRO, 0U);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

static void zl_erro_com_mensagem(int32_t codigo, int32_t linha, const char *mensagem)
{
    char texto[ZL_FORMATO_ERRO_TAMANHO];
    zl_escreve(zl_formato_erro_mensagem(codigo, linha, mensagem, texto));
    zl_escreve("\n");
    zl_para_com_led();
}

/* Mesma linha que o runtime POSIX escreve no stderr. Nao retorna. */
void zl_erro_execucao(int32_t codigo, int32_t linha)
{
    zl_erro_com_mensagem(codigo, linha, zl_formato_mensagem_erro(codigo));
}

/* ---------------------------------------------------------- entrada */

/* Le um token separado por espacos, como o runtime POSIX, so que da serial.
 * Na placa nao existe fim de arquivo: a leitura espera o proximo byte. O
 * token inteiro e consumido mesmo se passar do buffer, para uma entrada
 * longa nunca ser aceita pelo seu prefixo. */
static void zl_le_token(char *buffer, size_t capacidade, int32_t linha)
{
    size_t tamanho = 0U;
    int longo = 0;
    int invalido = 0;
    int caractere;
    do {
        caractere = zl_plataforma_le_byte();
    } while (caractere >= 0 && isspace(caractere));
    if (caractere < 0) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    do {
        if (caractere == '\0') invalido = 1;
        if (tamanho + 1U < capacidade) buffer[tamanho++] = (char)caractere;
        else longo = 1;
        caractere = zl_plataforma_le_byte();
    } while (caractere >= 0 && !isspace(caractere));
    buffer[tamanho] = '\0';
    if (longo || invalido) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
}

int64_t zl_le_int(int32_t linha)
{
    char token[256];
    int64_t valor = 0;
    zl_le_token(token, sizeof(token), linha);
    if (!zl_formato_le_int(token, &valor)) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    return valor;
}

float zl_le_flut(int32_t linha)
{
    char token[256];
    float valor = 0.0f;
    zl_le_token(token, sizeof(token), linha);
    if (!zl_formato_le_flut(token, &valor)) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    return valor;
}

double zl_le_duplocarpado(int32_t linha)
{
    char token[256];
    double valor = 0.0;
    zl_le_token(token, sizeof(token), linha);
    if (!zl_formato_le_duplocarpado(token, &valor)) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    return valor;
}

/* ------------------------------------------------ textos e matematica */
/* Mesma semantica do runtime POSIX (zl_runtime_posix.c). */

int32_t zl_texto_igual(const char *esquerda, const char *direita)
{
    return strcmp(esquerda != NULL ? esquerda : "",
                  direita != NULL ? direita : "") == 0 ? 1 : 0;
}

int64_t zl_potencia_int(int64_t base, int64_t expoente, int32_t linha)
{
    uint64_t resultado = UINT64_C(1);
    uint64_t fator = (uint64_t)base;
    uint64_t restante;
    if (expoente < 0) zl_erro_execucao(ZL_ERRO_EXPOENTE, linha);
    restante = (uint64_t)expoente;
    /* Aritmetica sem sinal: reduz modulo 2^64 sem overflow assinado em C. */
    while (restante != 0U) {
        if ((restante & UINT64_C(1)) != 0U) resultado *= fator;
        restante >>= 1U;
        if (restante != 0U) fator *= fator;
    }
    if (resultado <= (uint64_t)INT64_MAX) return (int64_t)resultado;
    return -INT64_C(1) - (int64_t)(UINT64_MAX - resultado);
}

float zl_potencia_flut(float base, float expoente, int32_t linha)
{
    float resultado;
    errno = 0;
    resultado = powf(base, expoente);
    if (errno == EDOM || (isnan(resultado) && !isnan(base) && !isnan(expoente)))
        zl_erro_execucao(ZL_ERRO_POTENCIA, linha);
    return resultado;
}

double zl_potencia_duplocarpado(double base, double expoente, int32_t linha)
{
    double resultado;
    errno = 0;
    resultado = pow(base, expoente);
    if (errno == EDOM || (isnan(resultado) && !isnan(base) && !isnan(expoente)))
        zl_erro_execucao(ZL_ERRO_POTENCIA, linha);
    return resultado;
}

float zl_raiz_flut(float valor, int32_t linha)
{
    if (valor < 0.0f) zl_erro_execucao(ZL_ERRO_RAIZ, linha);
    return sqrtf(valor);
}

double zl_raiz_duplocarpado(double valor, int32_t linha)
{
    if (valor < 0.0) zl_erro_execucao(ZL_ERRO_RAIZ, linha);
    return sqrt(valor);
}

/* ------------------------------------------------- pinos e espera */

/* As regras (que pino existe, que modo aceita) estao em zl_placa.h e sao as
 * mesmas da simulacao no PC; aqui fica so a chamada ao driver do ESP-IDF. */
static unsigned char zl_estado_pinos[ZL_PLACA_PINOS];

static void zl_erro_placa(int32_t linha, const char *regra)
{
    zl_erro_com_mensagem(ZL_ERRO_PLACA, linha, regra);
}

void zl_pino_modo(int32_t pino, int32_t modo, int32_t linha)
{
    gpio_config_t config;
    const char *erro = zl_placa_regra_modo(pino, modo);
    if (erro != NULL) zl_erro_placa(linha, erro);
    (void)memset(&config, 0, sizeof(config));
    config.pin_bit_mask = UINT64_C(1) << (unsigned int)pino;
    config.mode = modo == ZL_MODO_SAIDA ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT;
    config.pull_up_en = modo == ZL_MODO_ENTRADA_PULLUP ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    config.pull_down_en = modo == ZL_MODO_ENTRADA_PULLDOWN ? GPIO_PULLDOWN_ENABLE
                                                           : GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    if (gpio_config(&config) != ESP_OK) zl_erro_placa(linha, "o ESP-IDF recusou a configuracao do pino");
    zl_estado_pinos[pino] = (unsigned char)(modo == ZL_MODO_SAIDA ? ZL_PINO_SAIDA : ZL_PINO_ENTRADA);
}

void zl_pino_escreve(int32_t pino, int32_t nivel, int32_t linha)
{
    int estado = pino >= 0 && pino < ZL_PLACA_PINOS ? zl_estado_pinos[pino] : ZL_PINO_LIVRE;
    const char *erro = zl_placa_regra_escrita(pino, nivel, estado);
    if (erro != NULL) zl_erro_placa(linha, erro);
    if (gpio_set_level((gpio_num_t)pino, (uint32_t)nivel) != ESP_OK)
        zl_erro_placa(linha, "o ESP-IDF recusou a escrita no pino");
}

int32_t zl_pino_le(int32_t pino, int32_t linha)
{
    int estado = pino >= 0 && pino < ZL_PLACA_PINOS ? zl_estado_pinos[pino] : ZL_PINO_LIVRE;
    const char *erro = zl_placa_regra_leitura(pino, estado);
    if (erro != NULL) zl_erro_placa(linha, erro);
    return gpio_get_level((gpio_num_t)pino) != 0 ? 1 : 0;
}

/* Bloqueia so esta tarefa (vTaskDelay), sem ocupar a CPU. Arredonda para cima
 * em ticks do FreeRTOS: no nucleo Arduino o tick e de 1 ms. */
void zl_espera_ms(int32_t milissegundos, int32_t linha)
{
    uint64_t ticks;
    const char *erro = zl_placa_regra_espera(milissegundos);
    if (erro != NULL) zl_erro_placa(linha, erro);
    if (milissegundos == 0) return;
    ticks = ((uint64_t)(uint32_t)milissegundos * (uint64_t)configTICK_RATE_HZ + 999U) / 1000U;
    if (ticks >= (uint64_t)portMAX_DELAY) ticks = (uint64_t)portMAX_DELAY - 1U;
    vTaskDelay((TickType_t)ticks);
}

/* ------------------------------------------- tarefa do programa */

/* zl_principal roda numa tarefa propria, com pilha de ZL_PILHA_BYTES: e nela
 * que ficam as variaveis locais de cada funcao, e e esse o numero que a
 * verificacao de memoria do compilador (--ram-limite, E-TGT-002) usa. */
static void zl_esp32_tarefa(void *parametro)
{
    char numero[ZL_FORMATO_TAMANHO];
    int32_t status;
    UBaseType_t livre;
    (void)parametro;
    zl_escreve(ZL_MARCA_INICIO);
    status = zl_principal();
    /* No ESP-IDF a marca d'agua da pilha vem em bytes. */
    livre = uxTaskGetStackHighWaterMark(NULL);
    zl_escreve(ZL_MARCA_FIM);
    zl_escreve(zl_formato_int(status, numero));
    zl_escreve(", pilha usada ");
    zl_escreve(zl_formato_int((int64_t)ZL_PILHA_BYTES - (int64_t)livre, numero));
    zl_escreve(" de ");
    zl_escreve(zl_formato_int((int64_t)ZL_PILHA_BYTES, numero));
    zl_escreve(" bytes ---\n");
    zl_plataforma_descarrega();
    vTaskDelete(NULL);
}

void zl_esp32_inicia(void)
{
    char numero[ZL_FORMATO_TAMANHO];
    BaseType_t criada;
    zl_plataforma_inicia();
    /* Nucleo 1 (APP_CPU): o watchdog de tarefas vigia o ocioso do nucleo 0,
     * entao um calculo longo sem espera nao reinicia a placa. Prioridade 1,
     * a mesma da tarefa do loop() do Arduino. */
    criada = xTaskCreatePinnedToCore(zl_esp32_tarefa, "macaronica",
                                     (uint32_t)ZL_PILHA_BYTES, NULL, 1, NULL, 1);
    if (criada != pdPASS) {
        zl_escreve("--- Macaronica: sem memoria para a pilha de ");
        zl_escreve(zl_formato_int((int64_t)ZL_PILHA_BYTES, numero));
        zl_escreve(" bytes ---");
        zl_para_com_led();
    }
}
