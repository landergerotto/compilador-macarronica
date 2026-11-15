/*
 * driver.c - driver de medição da Parte C do PjBL 4 (estudo de otimização).
 *
 * Inicializa os vetores com valores pseudoaleatórios fixos, chama o núcleo
 * MLIR (compilado à parte, em outro objeto, para que nada seja embutido aqui)
 * e mede só a execução da chamada com clock_gettime(CLOCK_MONOTONIC). O tempo
 * de compilação é medido fora daqui, por medir.py.
 *
 * Compilado uma vez por núcleo, tipo e tamanho:
 *   -DNUCLEO_SOMA | -DNUCLEO_SOMAVET | -DNUCLEO_PRODESC
 *   -DTIPO_I64 | -DTIPO_F32
 *   -DN=<elementos>  -DFUNCAO=<símbolo do núcleo, por exemplo soma_f32>
 *   -DBENCH_OPENMP   (C3: registra quantas threads o OpenMP usou)
 *
 * Uso: ./bench [repetições]
 *   Padrão: 5 repetições cronometradas, depois de 1 chamada de aquecimento
 *   descartada (que traz os vetores para a cache quando cabem e cria as
 *   threads do OpenMP em C3).
 *
 * Saída: uma linha "chave=valor ...", lida por medir.py.
 *
 * Compile este arquivo SEM -ffast-math e com -ffp-contract=off: a soma
 * sequencial e a referência de Kahan abaixo precisam seguir a ordem e os
 * arredondamentos do programa (sem FMA), senão a comparação perde o sentido.
 */
#define _POSIX_C_SOURCE 200112L

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef BENCH_OPENMP
#include <omp.h>
#endif

#ifndef N
#error "defina -DN=<elementos>"
#endif
#ifndef FUNCAO
#error "defina -DFUNCAO=<simbolo do nucleo>"
#endif

#if defined(TIPO_I64)
typedef int64_t elem_t; /* inteira da Macaronica */
#define TIPO_NOME "i64"
#elif defined(TIPO_F32)
typedef float elem_t; /* flut da Macaronica */
#define TIPO_NOME "f32"
#else
#error "defina -DTIPO_I64 ou -DTIPO_F32"
#endif

/*
 * Convenção de chamada: o pipeline usa
 * --convert-func-to-llvm=use-bare-ptr-memref-call-conv=1, então cada
 * memref<N x T> estático chega ao núcleo como um ponteiro para o elemento 0
 * (sem o descritor de 5 campos da convenção padrão).
 */
#if defined(NUCLEO_SOMA)
#define NUCLEO_NOME "soma"
void FUNCAO(elem_t *a, elem_t *b, elem_t *c);
#define CHAMA() (FUNCAO(a, b, c), (elem_t)0)
#elif defined(NUCLEO_SOMAVET)
#define NUCLEO_NOME "somavet"
elem_t FUNCAO(elem_t *a);
#define CHAMA() FUNCAO(a)
#elif defined(NUCLEO_PRODESC)
#define NUCLEO_NOME "prodesc"
elem_t FUNCAO(elem_t *a, elem_t *b);
#define CHAMA() FUNCAO(a, b)
#else
#error "defina -DNUCLEO_SOMA, -DNUCLEO_SOMAVET ou -DNUCLEO_PRODESC"
#endif

/* splitmix64 com semente fixa: os mesmos dados em toda configuração. */
static uint64_t estado = 0x2545F4914F6CDD1Dull;

static uint64_t proximo(void)
{
    uint64_t z = (estado += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static elem_t valor(void)
{
#if defined(TIPO_I64)
    /* [-1000, 1000]: o produto escalar de 10^7 termos cabe folgado em i64. */
    return (int64_t)(proximo() % 2001u) - 1000;
#else
    /* [0, 1) com 24 bits: todo valor é exato em f32. */
    return (float)(proximo() >> 40) * (1.0f / 16777216.0f);
#endif
}

static elem_t *aloca(size_t n)
{
    size_t bytes = (n * sizeof(elem_t) + 63) / 64 * 64;
    void *p = NULL;
    if (posix_memalign(&p, 64, bytes) != 0) {
        fprintf(stderr, "driver: sem memoria para %zu bytes\n", bytes);
        exit(2);
    }
    return (elem_t *)p;
}

static int64_t agora_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (int64_t)t.tv_sec * 1000000000 + (int64_t)t.tv_nsec;
}

int main(int argc, char **argv)
{
    int reps = argc > 1 ? atoi(argv[1]) : 5;
    if (reps < 1)
        reps = 1;
    const size_t n = (size_t)(N);

    elem_t *a = aloca(n);
#if !defined(NUCLEO_SOMAVET)
    elem_t *b = aloca(n);
#endif
#if defined(NUCLEO_SOMA)
    elem_t *c = aloca(n);
    /* Sentinela (-1 em i64, NaN em f32): um núcleo que não escreve falha. */
    memset(c, 0xFF, n * sizeof(elem_t));
#endif
    for (size_t i = 0; i < n; i++) {
        a[i] = valor();
#if !defined(NUCLEO_SOMAVET)
        b[i] = valor();
#endif
    }

    int threads = 1;
#ifdef BENCH_OPENMP
    threads = omp_get_max_threads();
#endif

    int64_t *tempos = malloc(sizeof(int64_t) * (size_t)reps);
    elem_t *resultados = malloc(sizeof(elem_t) * (size_t)reps);
    if (tempos == NULL || resultados == NULL) {
        fprintf(stderr, "driver: sem memoria\n");
        return 2;
    }

    /* Aquecimento, descartado. */
    volatile elem_t sorvedouro = CHAMA();
    for (int k = 0; k < reps; k++) {
        int64_t t0 = agora_ns();
        elem_t r = CHAMA();
        int64_t t1 = agora_ns();
        tempos[k] = t1 - t0;
        resultados[k] = r;
        sorvedouro = r;
    }
    (void)sorvedouro;

    /* Estável: todas as repetições deram exatamente os mesmos bits (uma
     * redução paralela de flut pode combinar as parcelas em outra ordem). */
    int estavel = 1;
    for (int k = 1; k < reps; k++)
        if (memcmp(&resultados[k], &resultados[0], sizeof(elem_t)) != 0)
            estavel = 0;

    printf("nucleo=%s tipo=%s n=%zu threads=%d reps=%d tempos_ns=",
           NUCLEO_NOME, TIPO_NOME, n, threads, reps);
    for (int k = 0; k < reps; k++)
        printf("%s%" PRId64, k ? "," : "", tempos[k]);

    int ok = 1;
#if defined(NUCLEO_SOMA)
    /* Confere cada elemento contra a soma feita aqui, bit a bit. */
    double checksum = 0.0;
    for (size_t i = 0; i < n; i++) {
        elem_t esperado = a[i] + b[i];
        if (memcmp(&c[i], &esperado, sizeof esperado) != 0)
            ok = 0;
        checksum += (double)c[i];
    }
    printf(" resultado=%.17g", checksum);
#else
    /* Soma sequencial, na ordem do programa e no próprio tipo. */
    elem_t seq = 0;
    /* Referência: produtos e somas em double, com soma compensada (Kahan). */
    double ref = 0.0, comp = 0.0;
    for (size_t i = 0; i < n; i++) {
#if defined(NUCLEO_SOMAVET)
        seq += a[i];
        double termo = (double)a[i];
#else
        seq += a[i] * b[i];
        double termo = (double)a[i] * (double)b[i];
#endif
        double y = termo - comp;
        double t = ref + y;
        comp = (t - ref) - y;
        ref = t;
    }
    elem_t r = resultados[reps - 1];
#if defined(TIPO_I64)
    ok = (r == seq);
    printf(" resultado=%" PRId64 " sequencial=%" PRId64 " referencia=%" PRId64,
           r, seq, seq);
#else
    /* Em flut não há resultado "certo" bit a bit, e a própria ordem do
     * programa erra: no produto escalar de 10^7 termos em [0, 1), a soma
     * sequencial fica ~1% abaixo da referência, porque as parcelas menores
     * que meio ulp do acumulador se perdem. Aceita o núcleo se ele não errar
     * mais que o dobro da soma sequencial (ou 0,1% da referência). */
    double erro_seq = fabs((double)seq - ref);
    ok = fabs((double)r - ref) <= fmax(2.0 * erro_seq, 1e-3 * fabs(ref));
    printf(" resultado=%.9g sequencial=%.9g referencia=%.17g resultado_hex=%a",
           (double)r, (double)seq, ref, (double)r);
#endif
#endif
    printf(" ok=%d estavel=%d\n", ok, estavel);

    free(tempos);
    free(resultados);
    return ok ? 0 : 1;
}
