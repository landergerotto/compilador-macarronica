#ifndef MACARONICA_ZL_FORMATO_H
#define MACARONICA_ZL_FORMATO_H

/* Formato textual comum aos runtimes do PC (zl_runtime_posix.c) e da placa
 * (zl_runtime_esp32.c). O mesmo codigo produz o texto impresso, a mensagem de
 * erro de execucao e a validacao do texto lido nos dois alvos: e isso que
 * garante a mesma saida no PC e na placa. Nao faz E/S; so converte.
 *
 * Uso "cabecalho com implementacao": exatamente um arquivo de cada runtime
 * define ZL_FORMATO_IMPLEMENTACAO antes de incluir este cabecalho. Assim toda
 * build que ja liga zl_runtime_posix.c (testes do PjBL 3, bench, qa) continua
 * igual, sem um arquivo novo na linha de ligacao. */
#include <stddef.h>
#include <stdint.h>

/* Maior numero formatado, com o '\0': "-9223372036854775808" (21 bytes) e
 * "%.17g" de um double, como "-2.2250738585072014e-308" (25 bytes). */
#define ZL_FORMATO_TAMANHO 32

/* "linha N: erro de execucao C: " + a maior mensagem + '\0'. */
#define ZL_FORMATO_ERRO_TAMANHO 128

/* Cada funcao escreve em texto e devolve texto. */
const char *zl_formato_int(int64_t valor, char texto[ZL_FORMATO_TAMANHO]);
/* flut: 9 algarismos significativos (%.9g), o suficiente para voltar ao
 * mesmo binary32; duplocarpado: 17 (%.17g), o mesmo para binary64. */
const char *zl_formato_flut(float valor, char texto[ZL_FORMATO_TAMANHO]);
const char *zl_formato_duplocarpado(double valor, char texto[ZL_FORMATO_TAMANHO]);
const char *zl_formato_bool(int32_t valor);

/* Mensagem curta de cada codigo ZL_ERRO_* e a linha completa do erro, sem a
 * quebra de linha: "linha 4: erro de execucao 1: divisao por zero". */
const char *zl_formato_mensagem_erro(int32_t codigo);
const char *zl_formato_erro(int32_t codigo, int32_t linha,
                            char texto[ZL_FORMATO_ERRO_TAMANHO]);
/* O mesmo, com uma mensagem propria (erros dos servicos da placa, que dizem
 * qual regra do pino foi violada). */
const char *zl_formato_erro_mensagem(int32_t codigo, int32_t linha,
                                     const char *mensagem,
                                     char texto[ZL_FORMATO_ERRO_TAMANHO]);

/* Entrada: o token inteiro precisa ser um numero valido. Devolvem 1 e o
 * valor; 0 se o token for invalido ou estiver fora da faixa do tipo. */
int zl_formato_le_int(const char *token, int64_t *valor);
int zl_formato_le_flut(const char *token, float *valor);
int zl_formato_le_duplocarpado(const char *token, double *valor);

#ifdef ZL_FORMATO_IMPLEMENTACAO

#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zl_runtime.h"

/* Conversao decimal manual: nao depende de %lld/PRId64 na libc do alvo
 * (msvcrt no MinGW, newlib na placa). */
const char *zl_formato_int(int64_t valor, char texto[ZL_FORMATO_TAMANHO])
{
    char digitos[20];
    size_t total = 0U, saida = 0U;
    uint64_t magnitude = valor < 0 ? UINT64_C(0) - (uint64_t)valor : (uint64_t)valor;
    do {
        digitos[total++] = (char)('0' + (int)(magnitude % 10U));
        magnitude /= 10U;
    } while (magnitude != 0U);
    if (valor < 0) texto[saida++] = '-';
    while (total > 0U) texto[saida++] = digitos[--total];
    texto[saida] = '\0';
    return texto;
}

/* %.*g com tres ajustes, para o texto nao depender da libc:
 * - NaN sai sempre "nan": o glibc escreve "-nan" quando o bit de sinal esta
 *   ligado (o NaN padrao do x86 e negativo; o da placa, em software, nao);
 * - infinito sai "inf" e "-inf";
 * - o expoente fica com o minimo de 2 algarismos que o C99 pede (o msvcrt,
 *   sem o printf ANSI do MinGW, escreveria "e+005").
 * Os algarismos vem da libc: glibc, o printf ANSI do MinGW e o newlib da placa
 * arredondam corretamente, entao concordam em todo valor finito. */
static const char *zl_formato_real(double valor, int algarismos,
                                   char texto[ZL_FORMATO_TAMANHO])
{
    char *expoente;
    if (isnan(valor)) {
        (void)strcpy(texto, "nan");
        return texto;
    }
    if (isinf(valor)) {
        (void)strcpy(texto, valor < 0.0 ? "-inf" : "inf");
        return texto;
    }
    (void)snprintf(texto, ZL_FORMATO_TAMANHO, "%.*g", algarismos, valor);
    expoente = strchr(texto, 'e');
    if (expoente != NULL && (expoente[1] == '+' || expoente[1] == '-')) {
        char *numero = expoente + 2;
        size_t inicio = 0U, tamanho = strlen(numero);
        while (tamanho - inicio > 2U && numero[inicio] == '0') ++inicio;
        if (inicio != 0U) (void)memmove(numero, numero + inicio, tamanho - inicio + 1U);
    }
    return texto;
}

const char *zl_formato_flut(float valor, char texto[ZL_FORMATO_TAMANHO])
{
    /* float -> double e exato; so a escrita arredonda (9 algarismos). */
    return zl_formato_real((double)valor, 9, texto);
}

const char *zl_formato_duplocarpado(double valor, char texto[ZL_FORMATO_TAMANHO])
{
    return zl_formato_real(valor, 17, texto);
}

const char *zl_formato_bool(int32_t valor)
{
    return valor != 0 ? "VER" : "FAL";
}

const char *zl_formato_mensagem_erro(int32_t codigo)
{
    switch (codigo) {
        case ZL_ERRO_DIVISAO_ZERO: return "divisao por zero";
        case ZL_ERRO_DIVISAO_OVERFLOW: return "overflow da divisao inteira";
        case ZL_ERRO_INDICE: return "indice de vetor fora dos limites";
        case ZL_ERRO_RAIZ: return "raiz de valor negativo";
        case ZL_ERRO_EXPOENTE: return "expoente inteiro negativo";
        case ZL_ERRO_ENTRADA: return "entrada numerica invalida ou ausente";
        case ZL_ERRO_POTENCIA: return "dominio invalido de potencia real";
        case 8: return "uso invalido de pino ou espera"; /* ZL_ERRO_PLACA */
        default: return "falha de execucao";
    }
}

static size_t zl_formato_anexa(char *texto, size_t usado, size_t capacidade,
                               const char *parte)
{
    while (*parte != '\0' && usado + 1U < capacidade) texto[usado++] = *parte++;
    texto[usado] = '\0';
    return usado;
}

const char *zl_formato_erro_mensagem(int32_t codigo, int32_t linha,
                                     const char *mensagem,
                                     char texto[ZL_FORMATO_ERRO_TAMANHO])
{
    char numero[ZL_FORMATO_TAMANHO];
    size_t usado = 0U;
    texto[0] = '\0';
    usado = zl_formato_anexa(texto, usado, ZL_FORMATO_ERRO_TAMANHO, "linha ");
    usado = zl_formato_anexa(texto, usado, ZL_FORMATO_ERRO_TAMANHO,
                             zl_formato_int(linha, numero));
    usado = zl_formato_anexa(texto, usado, ZL_FORMATO_ERRO_TAMANHO,
                             ": erro de execucao ");
    usado = zl_formato_anexa(texto, usado, ZL_FORMATO_ERRO_TAMANHO,
                             zl_formato_int(codigo, numero));
    usado = zl_formato_anexa(texto, usado, ZL_FORMATO_ERRO_TAMANHO, ": ");
    (void)zl_formato_anexa(texto, usado, ZL_FORMATO_ERRO_TAMANHO, mensagem);
    return texto;
}

const char *zl_formato_erro(int32_t codigo, int32_t linha,
                            char texto[ZL_FORMATO_ERRO_TAMANHO])
{
    return zl_formato_erro_mensagem(codigo, linha,
                                    zl_formato_mensagem_erro(codigo), texto);
}

int zl_formato_le_int(const char *token, int64_t *valor)
{
    char *fim;
    intmax_t lido;
    int fora_da_faixa;
    errno = 0;
    lido = strtoimax(token, &fim, 10);
    fora_da_faixa = errno == ERANGE;
#if INTMAX_MAX > INT64_MAX
    /* Onde intmax_t passa de 64 bits, ERANGE nao cobre a faixa de inteira. */
    if (lido < INT64_MIN || lido > INT64_MAX) fora_da_faixa = 1;
#endif
    if (fim == token || *fim != '\0' || fora_da_faixa) return 0;
    *valor = (int64_t)lido;
    return 1;
}

/* Aceita notacao decimal de entrada, inclusive expoente; rejeita hexadecimal,
 * NaN e infinito. Esta sintaxe de entrada e distinta dos literais da linguagem. */
static int zl_formato_numero_decimal(const char *texto)
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

int zl_formato_le_flut(const char *token, float *valor)
{
    char *fim;
    float lido;
    errno = 0;
    lido = strtof(token, &fim);
    if (!zl_formato_numero_decimal(token) || fim == token || *fim != '\0' ||
        !isfinite(lido) || (errno == ERANGE && fabsf(lido) >= FLT_MAX)) return 0;
    *valor = lido;
    return 1;
}

int zl_formato_le_duplocarpado(const char *token, double *valor)
{
    char *fim;
    double lido;
    errno = 0;
    lido = strtod(token, &fim);
    if (!zl_formato_numero_decimal(token) || fim == token || *fim != '\0' ||
        !isfinite(lido) || (errno == ERANGE && fabs(lido) >= DBL_MAX)) return 0;
    *valor = lido;
    return 1;
}

#endif /* ZL_FORMATO_IMPLEMENTACAO */
#endif /* MACARONICA_ZL_FORMATO_H */
