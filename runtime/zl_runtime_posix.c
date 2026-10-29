#include "zl_runtime.h"

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if FLT_RADIX != 2 || FLT_MANT_DIG != 24 || DBL_MANT_DIG != 53
#error "O runtime requer float binary32 e double binary64"
#endif

void zl_erro_execucao(int32_t codigo, int32_t linha)
{
    const char *mensagem;
    switch (codigo) {
        case ZL_ERRO_DIVISAO_ZERO: mensagem = "divisao por zero"; break;
        case ZL_ERRO_DIVISAO_OVERFLOW: mensagem = "overflow da divisao inteira"; break;
        case ZL_ERRO_INDICE: mensagem = "indice de vetor fora dos limites"; break;
        case ZL_ERRO_RAIZ: mensagem = "raiz de valor negativo"; break;
        case ZL_ERRO_EXPOENTE: mensagem = "expoente inteiro negativo"; break;
        case ZL_ERRO_ENTRADA: mensagem = "entrada numerica invalida ou ausente"; break;
        case ZL_ERRO_POTENCIA: mensagem = "dominio invalido de potencia real"; break;
        default: mensagem = "falha de execucao"; break;
    }
    (void)fflush(stdout);
    (void)fprintf(stderr, "linha %" PRId32 ": erro de execucao %" PRId32 ": %s\n",
                  linha, codigo, mensagem);
    exit(EXIT_FAILURE);
}

void zl_imprime_int(int32_t valor)
{
    (void)printf("%" PRId32, valor);
}

void zl_imprime_flut(float valor)
{
    (void)printf("%.9g", (double)valor);
}

void zl_imprime_duplocarpado(double valor)
{
    (void)printf("%.17g", valor);
}

void zl_imprime_texto(const char *valor)
{
    (void)fputs(valor != NULL ? valor : "", stdout);
}

void zl_imprime_bool(int32_t valor)
{
    (void)fputs(valor != 0 ? "VER" : "FAL", stdout);
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
    int caractere;
    do {
        caractere = fgetc(stdin);
    } while (caractere != EOF && isspace((unsigned char)caractere));
    if (caractere == EOF) {
        zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    }
    do {
        if (tamanho + 1U < capacidade) {
            buffer[tamanho++] = (char)caractere;
        } else {
            longo = 1;
        }
        caractere = fgetc(stdin);
    } while (caractere != EOF && !isspace((unsigned char)caractere));
    buffer[tamanho] = '\0';
    if (longo || ferror(stdin)) {
        zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    }
}

int32_t zl_le_int(int32_t linha)
{
    char token[256];
    char *fim;
    intmax_t valor;
    le_token(token, sizeof(token), linha);
    errno = 0;
    valor = strtoimax(token, &fim, 10);
    if (fim == token || *fim != '\0' || errno == ERANGE ||
        valor < INT32_MIN || valor > INT32_MAX) {
        zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    }
    return (int32_t)valor;
}

/* Aceita notacao decimal de entrada, inclusive expoente; rejeita hexadecimal,
 * NaN e infinito. Esta sintaxe de stdin e distinta dos literais da linguagem. */
static int numero_decimal(const char *texto)
{
    const char *p = texto;
    int digitos = 0;
    if (*p == '+' || *p == '-') ++p;
    while (*p >= '0' && *p <= '9') { ++p; ++digitos; }
    if (*p == '.') {
        ++p;
        while (*p >= '0' && *p <= '9') { ++p; ++digitos; }
    }
    if (digitos == 0) return 0;
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') ++p;
        if (*p < '0' || *p > '9') return 0;
        while (*p >= '0' && *p <= '9') ++p;
    }
    return *p == '\0';
}

float zl_le_flut(int32_t linha)
{
    char token[256];
    char *fim;
    float valor;
    le_token(token, sizeof(token), linha);
    errno = 0;
    valor = strtof(token, &fim);
    if (!numero_decimal(token) || fim == token || *fim != '\0' ||
        !isfinite(valor) || (errno == ERANGE && fabsf(valor) >= FLT_MAX)) {
        zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    }
    return valor;
}

double zl_le_duplocarpado(int32_t linha)
{
    char token[256];
    char *fim;
    double valor;
    le_token(token, sizeof(token), linha);
    errno = 0;
    valor = strtod(token, &fim);
    if (!numero_decimal(token) || fim == token || *fim != '\0' ||
        !isfinite(valor) || (errno == ERANGE && fabs(valor) >= DBL_MAX)) {
        zl_erro_execucao(ZL_ERRO_ENTRADA, linha);
    }
    return valor;
}

int32_t zl_texto_igual(const char *esquerda, const char *direita)
{
    return strcmp(esquerda != NULL ? esquerda : "",
                  direita != NULL ? direita : "") == 0 ? 1 : 0;
}

int32_t zl_potencia_int(int32_t base, int32_t expoente, int32_t linha)
{
    uint32_t resultado = UINT32_C(1);
    uint32_t fator = (uint32_t)base;
    uint32_t restante;
    if (expoente < 0) {
        zl_erro_execucao(ZL_ERRO_EXPOENTE, linha);
    }
    restante = (uint32_t)expoente;
    /* O produto cabe em uint64_t; a conversao reduz modulo 2^32 em C99. */
    while (restante != 0U) {
        if ((restante & UINT32_C(1)) != 0U) {
            resultado = (uint32_t)((uint64_t)resultado * fator);
        }
        restante >>= 1U;
        if (restante != 0U) fator = (uint32_t)((uint64_t)fator * fator);
    }
    /* Evita conversao unsigned -> signed fora da faixa, dependente de C. */
    if (resultado <= INT32_MAX) return (int32_t)resultado;
    return -INT32_C(1) - (int32_t)(UINT32_MAX - resultado);
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

int main(void)
{
    return (int)zl_principal();
}
