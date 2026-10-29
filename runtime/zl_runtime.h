#ifndef MACARONICA_ZL_RUNTIME_H
#define MACARONICA_ZL_RUNTIME_H

/* Fronteira C99 do programa LLVM com o hospedeiro POSIX. */
#include <stdint.h>

/* Codigos estaveis usados pelas guardas emitidas pelo compilador. */
enum {
    ZL_ERRO_DIVISAO_ZERO = 1,
    ZL_ERRO_DIVISAO_OVERFLOW = 2,
    ZL_ERRO_INDICE = 3,
    ZL_ERRO_RAIZ = 4,
    ZL_ERRO_EXPOENTE = 5,
    ZL_ERRO_ENTRADA = 6,
    ZL_ERRO_POTENCIA = 7
};

/* A implementacao termina o processo. LLVM declara esta funcao noreturn. */
void zl_erro_execucao(int32_t codigo, int32_t linha);

void zl_imprime_int(int32_t valor);
void zl_imprime_flut(float valor);
void zl_imprime_duplocarpado(double valor);
void zl_imprime_texto(const char *valor);
/* i1 e ampliado para i32 pelo gerador antes da chamada. */
void zl_imprime_bool(int32_t valor);
void zl_imprime_nl(void);

/* A linha e um argumento oculto acrescentado pelo gerador. */
int32_t zl_le_int(int32_t linha);
float zl_le_flut(int32_t linha);
double zl_le_duplocarpado(int32_t linha);

/* Ponteiro nulo representa a palavra vazia na inicializacao padrao. */
int32_t zl_texto_igual(const char *esquerda, const char *direita);

int32_t zl_potencia_int(int32_t base, int32_t expoente, int32_t linha);
float zl_potencia_flut(float base, float expoente, int32_t linha);
double zl_potencia_duplocarpado(double base, double expoente, int32_t linha);
float zl_raiz_flut(float valor, int32_t linha);
double zl_raiz_duplocarpado(double valor, int32_t linha);

/* Definida pelo programa gerado; o ponto de entrada main fica no runtime. */
int32_t zl_principal(void);

#endif
