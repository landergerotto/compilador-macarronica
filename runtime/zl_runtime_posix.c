#include "zl_runtime.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Texto impresso, mensagem de erro e validacao da entrada sao os mesmos do
 * runtime da placa: ficam em zl_formato.h, compilado dentro deste arquivo. */
#define ZL_FORMATO_IMPLEMENTACAO
#include "zl_formato.h"

#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || FLT_MAX_EXP != 128 || \
    DBL_MANT_DIG != 53 || DBL_MAX_EXP != 1024
#error "O runtime requer float binary32 e double binary64"
#endif

void zl_erro_execucao(int32_t codigo, int32_t linha)
{
    char texto[ZL_FORMATO_ERRO_TAMANHO];
    (void)fflush(stdout);
    (void)fprintf(stderr, "%s\n", zl_formato_erro(codigo, linha, texto));
    exit(EXIT_FAILURE);
}

void zl_imprime_int(int64_t valor)
{
    char texto[ZL_FORMATO_TAMANHO];
    (void)fputs(zl_formato_int(valor, texto), stdout);
}

void zl_imprime_flut(float valor)
{
    char texto[ZL_FORMATO_TAMANHO];
    (void)fputs(zl_formato_flut(valor, texto), stdout);
}

void zl_imprime_duplocarpado(double valor)
{
    char texto[ZL_FORMATO_TAMANHO];
    (void)fputs(zl_formato_duplocarpado(valor, texto), stdout);
}

void zl_imprime_texto(const char *valor)
{
    (void)fputs(valor != NULL ? valor : "", stdout);
}

void zl_imprime_bool(int32_t valor)
{
    (void)fputs(zl_formato_bool(valor), stdout);
}

void zl_imprime_nl(void)
{
    (void)fputc('\n', stdout);
}

/* Consome um token inteiro, mesmo se exceder o limite do buffer. Assim uma
 * entrada longa nunca e aceita silenciosamente pelo seu prefixo. */
static void le_token(char *buffer, size_t capacidade, int32_t linha)
{
    size_t tamanho = 0U;
    int longo = 0;
    int invalido = 0;
    int caractere;
    do {
        caractere = fgetc(stdin);
    } while (caractere != EOF && isspace((unsigned char)caractere));
    if (caractere == EOF) {
        zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    }
    do {
        /* NUL nao termina um token de stdin. Rejeita o token completo para
         * strto* nao aceitar somente o prefixo anterior a esse byte. */
        if (caractere == '\0') invalido = 1;
        if (tamanho + 1U < capacidade) {
            buffer[tamanho++] = (char)caractere;
        } else {
            longo = 1;
        }
        caractere = fgetc(stdin);
    } while (caractere != EOF && !isspace((unsigned char)caractere));
    buffer[tamanho] = '\0';
    if (longo || invalido || ferror(stdin)) {
        zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    }
}

int64_t zl_le_int(int32_t linha)
{
    char token[256];
    int64_t valor = 0;
    le_token(token, sizeof(token), linha);
    if (!zl_formato_le_int(token, &valor)) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    return valor;
}

float zl_le_flut(int32_t linha)
{
    char token[256];
    float valor = 0.0f;
    le_token(token, sizeof(token), linha);
    if (!zl_formato_le_flut(token, &valor)) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    return valor;
}

double zl_le_duplocarpado(int32_t linha)
{
    char token[256];
    double valor = 0.0;
    le_token(token, sizeof(token), linha);
    if (!zl_formato_le_duplocarpado(token, &valor)) zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    return valor;
}

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
    if (expoente < 0) {
        zl_erro_execucao(ZL_ERRO_EXPOENTE, linha);
    }
    restante = (uint64_t)expoente;
    /* Aritmetica sem sinal: reduz modulo 2^64 sem overflow assinado em C. */
    while (restante != 0U) {
        if ((restante & UINT64_C(1)) != 0U) resultado *= fator;
        restante >>= 1U;
        if (restante != 0U) fator *= fator;
    }
    /* Evita conversao unsigned -> signed fora da faixa, dependente de C. */
    if (resultado <= (uint64_t)INT64_MAX) return (int64_t)resultado;
    return -INT64_C(1) - (int64_t)(UINT64_MAX - resultado);
}

float zl_potencia_flut(float base, float expoente, int32_t linha)
{
    float resultado;
    errno = 0;
    resultado = powf(base, expoente);
    if (errno == EDOM || (isnan(resultado) && !isnan(base) && !isnan(expoente))) {
        zl_erro_execucao(ZL_ERRO_POTENCIA, linha);
    }
    return resultado;
}

double zl_potencia_duplocarpado(double base, double expoente, int32_t linha)
{
    double resultado;
    errno = 0;
    resultado = pow(base, expoente);
    if (errno == EDOM || (isnan(resultado) && !isnan(base) && !isnan(expoente))) {
        zl_erro_execucao(ZL_ERRO_POTENCIA, linha);
    }
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

#ifndef ZL_RUNTIME_NO_MAIN
int main(void)
{
    return (int)zl_principal();
}
#endif
