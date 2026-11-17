/*
 * referencia_c.c - as reduções de flut escritas em C (pergunta obrigatória).
 *
 * Servem de contraprova para os núcleos MLIR: em C, -ffast-math faz o clang
 * marcar cada fadd com "fast" (que inclui reassoc) já na geração do IR, e aí
 * o vetorizador do LLVM pode reassociar a soma. Sobre um .ll vindo do MLIR,
 * o mesmo -ffast-math não muda as instruções (configuração C1-ffast-math).
 *
 * Mesma assinatura dos núcleos MLIR (ponteiros simples), então o mesmo
 * driver.c serve. Compile com -DN=<elementos> e -DNUCLEO_SOMAVET ou
 * -DNUCLEO_PRODESC; na configuração sem -ffast-math use -ffp-contract=off,
 * para que a*b + s seja uma multiplicação e uma soma, como no núcleo MLIR.
 */
#include <stddef.h>

#ifndef N
#error "defina -DN=<elementos>"
#endif

#if defined(NUCLEO_SOMAVET)
float somavet_f32(float *a);

float somavet_f32(float *a)
{
    float s = 0.0f;
    for (size_t i = 0; i < (size_t)(N); i++)
        s += a[i];
    return s;
}
#elif defined(NUCLEO_PRODESC)
float prodesc_f32(float *a, float *b);

float prodesc_f32(float *a, float *b)
{
    float s = 0.0f;
    for (size_t i = 0; i < (size_t)(N); i++)
        s += a[i] * b[i];
    return s;
}
#else
#error "defina -DNUCLEO_SOMAVET ou -DNUCLEO_PRODESC"
#endif
