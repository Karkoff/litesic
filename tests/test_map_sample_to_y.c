/*
 * test_map_sample_to_y.c — Testes unitários da função MapSampleToY (M4-T2)
 *
 * Testa a conversão de amplitude de amostra (-1.0..1.0) para posição Y em
 * pixels, isolada de qualquer chamada GDI/janelas.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ------------------------------------------------------------------ */
/* Função a testar (espelho da implementação em mainwnd.cpp)           */
/* ------------------------------------------------------------------ */

static int MapSampleToY(float sample, int height)
{
    /* Clamp entre -1.0 e 1.0. */
    if (sample < -1.0f) sample = -1.0f;
    if (sample > 1.0f)  sample =  1.0f;

    /* Mapear: -1.0 → 0, 0.0 → height/2, +1.0 → height. */
    int y = (int)((1.0f + sample) * (float)height / 2.0f);
    if (y < 0) y = 0;
    if (y > height) y = height;
    return y;
}

/* ------------------------------------------------------------------ */
/* Testes                                                             */
/* ------------------------------------------------------------------ */

static int test_zero_amplitude(void)
{
    printf("  test_zero_amplitude... ");

    /* Amplitude 0.0 deve mapear para o centro (height/2). */
    assert(MapSampleToY(0.0f, 100) == 50);
    assert(MapSampleToY(0.0f, 32) == 16);
    assert(MapSampleToY(0.0f, 1) == 0); /* Caso extremo: height=1. */

    printf("PASS\n");
    return TRUE;
}

static int test_positive_amplitude(void)
{
    printf("  test_positive_amplitude... ");

    /* Amplitude +1.0 deve mapear para o fundo (height). */
    assert(MapSampleToY(1.0f, 100) == 100);
    assert(MapSampleToY(0.5f, 100) == 75); /* Meio caminho entre centro e fundo. */

    printf("PASS\n");
    return TRUE;
}

static int test_negative_amplitude(void)
{
    printf("  test_negative_amplitude... ");

    /* Amplitude -1.0 deve mapear para o topo (0). */
    assert(MapSampleToY(-1.0f, 100) == 0);
    assert(MapSampleToY(-0.5f, 100) == 25); /* Meio caminho entre centro e topo. */

    printf("PASS\n");
    return TRUE;
}

static int test_clamping(void)
{
    printf("  test_clamping... ");

    /* Amplitudes fora do intervalo [-1.0, 1.0] devem ser clampadas. */
    assert(MapSampleToY(-2.0f, 100) == 0);   /* Clamp para -1.0. */
    assert(MapSampleToY(2.0f, 100) == 100);  /* Clamp para +1.0. */
    assert(MapSampleToY(-10.0f, 50) == 0);   /* Clamp extremo. */
    assert(MapSampleToY(10.0f, 50) == 50);   /* Clamp extremo. */

    printf("PASS\n");
    return TRUE;
}

static int test_small_height(void)
{
    printf("  test_small_height... ");

    /* Alturas pequenas devem funcionar corretamente. */
    assert(MapSampleToY(-1.0f, 1) == 0);
    assert(MapSampleToY(0.0f, 1) == 0);      /* height/2 = 0. */
    assert(MapSampleToY(1.0f, 1) == 1);

    assert(MapSampleToY(-1.0f, 2) == 0);
    assert(MapSampleToY(0.0f, 2) == 1);      /* height/2 = 1. */
    assert(MapSampleToY(1.0f, 2) == 2);

    printf("PASS\n");
    return TRUE;
}

static int test_linear_interpolation(void)
{
    printf("  test_linear_interpolation... ");

    /* Verificar que a interpolação é linear. */
    int h = 100;
    for (int i = 0; i <= 100; i++)
    {
        float sample = (float)i / 50.0f - 1.0f; /* -1.0 a +1.0 em passos de 0.02. */
        int expected = (int)((1.0f + sample) * (float)h / 2.0f);
        if (expected < 0) expected = 0;
        if (expected > h) expected = h;

        int result = MapSampleToY(sample, h);
        assert(result == expected);
    }

    printf("PASS\n");
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* Main                                                               */
/* ------------------------------------------------------------------ */

int main(void)
{
    int passed = 0;
    int failed = 0;
    int total;

    printf("\n=== MapSampleToY Tests (M4-T2) ===\n\n");

    if (test_zero_amplitude())      passed++; else failed++;
    if (test_positive_amplitude())  passed++; else failed++;
    if (test_negative_amplitude())  passed++; else failed++;
    if (test_clamping())            passed++; else failed++;
    if (test_small_height())        passed++; else failed++;
    if (test_linear_interpolation())passed++; else failed++;

    total = passed + failed;

    printf("\nResults: %d/%d tests passed", passed, total);
    if (failed > 0)
        printf(" (%d FAILED)", failed);
    printf("\n\n");

    return failed > 0 ? 1 : 0;
}
