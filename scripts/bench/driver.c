#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#if defined(_WIN32)
#include <windows.h>
#endif

#ifndef BENCH_N
#error BENCH_N must be specified
#endif
#ifndef BENCH_KERNEL
#error BENCH_KERNEL must name the kernel extracted from generated MLIR
#endif

/* Bare-pointer ABI produced by convert-func-to-llvm. The kernel itself is
 * generated from Macaronica; this file only supplies inputs and measures it. */
#ifdef BENCH_ADD
extern void BENCH_KERNEL(float *dest, float *left, float *right);
typedef float BenchElement;
#else
extern int32_t BENCH_KERNEL(int32_t *left, int32_t *right);
typedef int32_t BenchElement;
#endif

static double now_seconds(void)
{
#if defined(_WIN32)
    LARGE_INTEGER counter, frequency;
    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&counter)) {
        fputs("performance counter unavailable\n", stderr);
        exit(2);
    }
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    struct timespec value;
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        perror("clock_gettime");
        exit(2);
    }
    return (double)value.tv_sec + (double)value.tv_nsec / 1e9;
#endif
}

int main(int argc, char **argv)
{
    const size_t n = (size_t)BENCH_N;
    BenchElement *a = NULL, *b = NULL;
#ifdef BENCH_ADD
    float *c = NULL;
#endif
    long repeats = 5;
    char *end = NULL;
    long sample;
    size_t i;
    if (argc == 2) {
        errno = 0;
        repeats = strtol(argv[1], &end, 10);
        if (errno || *end || repeats < 5 || repeats > 10000) return 2;
    } else if (argc != 1) return 2;
    a = (BenchElement *)malloc(n * sizeof(*a));
    b = (BenchElement *)malloc(n * sizeof(*b));
#ifdef BENCH_ADD
    c = (float *)calloc(n, sizeof(*c));
    if (!c) { free(a); free(b); return 2; }
#endif
    if (!a || !b) { free(a); free(b);
#ifdef BENCH_ADD
        free(c);
#endif
        return 2;
    }
    for (i = 0; i < n; ++i) { a[i] = 1; b[i] = 1; }
    puts("sample,seconds,checksum");
    /* One warm-up; validation is outside the measured region on every run. */
    for (sample = -1; sample < repeats; ++sample) {
        double start, elapsed, checksum = 0.0;
#ifdef BENCH_ADD
        for (i = 0; i < n; ++i) c[i] = 0.0f;
#endif
        start = now_seconds();
#ifdef BENCH_ADD
        BENCH_KERNEL(c, a, b);
#else
        checksum = (double)BENCH_KERNEL(a, b);
#endif
        elapsed = now_seconds() - start;
#ifdef BENCH_ADD
        for (i = 0; i < n; ++i) {
            if (c[i] != 2.0f) { fprintf(stderr, "incorrect add at %zu\n", i); return 1; }
            checksum += (double)c[i];
        }
        if (checksum != 2.0 * (double)n) return 1;
#else
        /* All products and partial sums fit i32 for these inputs and N <= 10^7.
         * Modular i32 reduction can be reordered without floating-point drift. */
        if (checksum != (double)n) {
            fprintf(stderr, "incorrect dot: %.17g, expected %zu\n", checksum, n);
            return 1;
        }
#endif
        if (sample >= 0) printf("%ld,%.12f,%.17g\n", sample + 1, elapsed, checksum);
    }
    free(a); free(b);
#ifdef BENCH_ADD
    free(c);
#endif
    return 0;
}
