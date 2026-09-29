/*
 * test_fft.c — Testes unitários da FFT (M4-T3)
 *
 * Testa a implementação da FFT radix-2:
 *   - Gera um sinal senoidal puro de frequência conhecida.
 *   - Roda a FFT e confirma que o pico de magnitude cai no bin esperado.
 *   - Testa a janela de Hann.
 *   - Testa faixas log-scale do espectro.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ------------------------------------------------------------------ */
/* Configurações                                                      */
/* ------------------------------------------------------------------ */

#define FFT_SIZE      512
#define SAMPLE_RATE   44100
#define NUM_BARS      32

/* ------------------------------------------------------------------ */
/* Estruturas e funções da FFT (espelho de visualizer.cpp)            */
/* ------------------------------------------------------------------ */

typedef struct
{
    float real;
    float imag;
} Complex;

static void ApplyHannWindow(float* samples, int size)
{
    for (int i = 0; i < size; i++)
    {
        float window = 0.5f - 0.5f * cosf(2.0f * M_PI * (float)i / (float)(size - 1));
        samples[i] *= window;
    }
}

static void FFT(Complex* input, int forwardSize)
{
    int N = forwardSize;

    /* Bit-reversal permutation. */
    int j = 0;
    for (int i = 1; i < N; i++)
    {
        int bit = N >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;

        if (i < j)
        {
            Complex temp = input[i];
            input[i] = input[j];
            input[j] = temp;
        }
    }

    /* Cooley-Tukey butterfly. */
    for (int size = 2; size <= N; size *= 2)
    {
        float angle = -2.0f * M_PI / (float)size;
        Complex wStep;
        wStep.real = cosf(angle);
        wStep.imag = sinf(angle);

        for (int start = 0; start < N; start += size)
        {
            Complex w;
            w.real = 1.0f;
            w.imag = 0.0f;

            for (int k = 0; k < size / 2; k++)
            {
                int evenIdx = start + k;
                int oddIdx = start + k + size / 2;

                Complex t;
                t.real = w.real * input[oddIdx].real - w.imag * input[oddIdx].imag;
                t.imag = w.real * input[oddIdx].imag + w.imag * input[oddIdx].real;

                Complex u = input[evenIdx];
                input[evenIdx].real = u.real + t.real;
                input[evenIdx].imag = u.imag + t.imag;
                input[oddIdx].real = u.real - t.real;
                input[oddIdx].imag = u.imag - t.imag;

                Complex newW;
                newW.real = w.real * wStep.real - w.imag * wStep.imag;
                newW.imag = w.real * wStep.imag + w.imag * wStep.real;
                w = newW;
            }
        }
    }
}

static void FFTMagnitude(const Complex* input, float* output, int size)
{
    for (int i = 0; i < size / 2; i++)
    {
        output[i] = sqrtf(input[i].real * input[i].real +
                          input[i].imag * input[i].imag);
    }
}

/* ------------------------------------------------------------------ */
/* Testes                                                             */
/* ------------------------------------------------------------------ */

static int test_fft_sine_peak(void)
{
    printf("  test_fft_sine_peak... ");

    /* Gerar um sinal senoidal de 1 kHz a 44100 Hz.
     * Período = 44100/1000 = 44.1 amostras.
     * Bin esperado = freq * FFT_SIZE / sampleRate = 1000 * 512 / 44100 ≈ 11.6. */
    float sineSamples[FFT_SIZE];
    float frequency = 1000.0f; /* 1 kHz */

    for (int i = 0; i < FFT_SIZE; i++)
    {
        sineSamples[i] = sinf(2.0f * M_PI * frequency * i / SAMPLE_RATE);
    }

    /* Converter para complexo e aplicar janela de Hann corretamente. */
    Complex complexInput[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++)
    {
        complexInput[i].real = sineSamples[i];
        complexInput[i].imag = 0.0f;
    }

    /* Aplicar janela de Hann ao canal real. */
    for (int i = 0; i < FFT_SIZE; i++)
    {
        float window = 0.5f - 0.5f * cosf(2.0f * M_PI * (float)i / (float)(FFT_SIZE - 1));
        complexInput[i].real *= window;
    }

    /* Calcular FFT. */
    FFT(complexInput, FFT_SIZE);

    /* Calcular magnitudes. */
    float magnitudes[FFT_SIZE / 2];
    FFTMagnitude(complexInput, magnitudes, FFT_SIZE);

    /* Encontrar o bin com magnitude máxima (ignorar DC bin 0). */
    int peakBin = 1;
    float maxMag = magnitudes[1];
    for (int i = 2; i < FFT_SIZE / 2; i++)
    {
        if (magnitudes[i] > maxMag)
        {
            maxMag = magnitudes[i];
            peakBin = i;
        }
    }

    /* O bin esperado é ~11.6 — verificar que o pico está dentro de 2 bins. */
    int expectedBin = (int)(frequency * FFT_SIZE / SAMPLE_RATE + 0.5f);
    int diff = abs(peakBin - expectedBin);

    printf("(freq=%.0fHz peakBin=%d expectedBin=%d) ", frequency, peakBin, expectedBin);

    if (diff > 2)
    {
        printf("FAIL (peak at bin %d, expected ~%d)\n", peakBin, expectedBin);
        return FALSE;
    }

    printf("PASS\n");
    return TRUE;
}

static int test_hann_window(void)
{
    printf("  test_hann_window... ");

    /* Gerar sinal constante. */
    float samples[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++)
        samples[i] = 1.0f;

    /* Aplicar janela de Hann — as bordas devem ser zero. */
    ApplyHannWindow(samples, FFT_SIZE);

    /* Primeira e última amostras devem ser ~0. */
    assert(samples[0] < 0.01f);
    assert(samples[FFT_SIZE - 1] < 0.01f);

    /* Amostra do meio deve ser ~1.0 (pico da janela). */
    assert(samples[FFT_SIZE / 2] > 0.99f);

    printf("PASS\n");
    return TRUE;
}

static int test_log_bins(void)
{
    printf("  test_log_bins... ");

    /* Calcular faixas log-scale.
     * Frequências: 20Hz a Nyquist (FFT_SIZE/2 bins a sampleRate). */
    int startBins[NUM_BARS];
    int endBins[NUM_BARS];

    float minFreq = 20.0f;   /* 20 Hz — limite inferior audível. */
    float maxFreq = (float)FFT_SIZE / 2.0f * SAMPLE_RATE / (float)FFT_SIZE; /* Nyquist = sampleRate/2 */
    if (maxFreq > SAMPLE_RATE / 2.0f)
        maxFreq = SAMPLE_RATE / 2.0f;

    float minLog = logf(minFreq);
    float maxLog = logf(maxFreq);
    float range = maxLog - minLog;

    for (int i = 0; i < NUM_BARS; i++)
    {
        float startLog = minLog + (float)i * range / (float)NUM_BARS;
        float endLog = minLog + (float)(i + 1) * range / (float)NUM_BARS;

        /* Converter frequência para bin index: bin = freq * FFT_SIZE / sampleRate. */
        float startBin = expf(startLog) * FFT_SIZE / SAMPLE_RATE;
        float endBin = expf(endLog) * FFT_SIZE / SAMPLE_RATE;

        startBins[i] = (int)startBin;
        endBins[i] = (int)endBin;

        if (startBins[i] < 0) startBins[i] = 0;
        if (endBins[i] >= FFT_SIZE / 2) endBins[i] = FFT_SIZE / 2 - 1;
        if (startBins[i] > endBins[i]) startBins[i] = endBins[i];
    }

    /* Debug: imprimir primeira e última faixa. */
    printf("[0]:%d-%d [%d]:%d-%d ", startBins[0], endBins[0], NUM_BARS-1, startBins[NUM_BARS-1], endBins[NUM_BARS-1]);

    /* Verificar que as faixas cobrem o espectro sem sobreposições. */
    for (int i = 0; i < NUM_BARS - 1; i++)
    {
        assert(endBins[i] >= startBins[i]);
        assert(endBins[i] + 1 >= startBins[i + 1]); /* Sem gaps grandes. */
    }

    /* Primeira faixa deve começar em bin ~0-1 (20Hz é muito baixo). */
    printf("(startBins[0]=%d) ", startBins[0]);
    if (startBins[0] > 2)
    {
        printf("FAIL (first bin too high)\n");
        return FALSE;
    }

    /* Última faixa deve terminar em ~FFT_SIZE/2. */
    assert(endBins[NUM_BARS - 1] >= FFT_SIZE / 2 - 2);

    printf("PASS\n");
    return TRUE;
}

static int test_fft_zero_input(void)
{
    printf("  test_fft_zero_input... ");

    /* FFT de entrada zero deve resultar em magnitude zero. */
    Complex input[FFT_SIZE];
    for (int i = 0; i < FFT_SIZE; i++)
    {
        input[i].real = 0.0f;
        input[i].imag = 0.0f;
    }

    FFT(input, FFT_SIZE);

    float magnitudes[FFT_SIZE / 2];
    FFTMagnitude(input, magnitudes, FFT_SIZE);

    for (int i = 0; i < FFT_SIZE / 2; i++)
    {
        assert(magnitudes[i] < 0.001f); /* Tudo ~zero. */
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

    printf("\n=== FFT Tests (M4-T3) ===\n\n");

    if (test_fft_sine_peak())     passed++; else failed++;
    if (test_hann_window())       passed++; else failed++;
    if (test_log_bins())          passed++; else failed++;
    if (test_fft_zero_input())    passed++; else failed++;

    total = passed + failed;

    printf("\nResults: %d/%d tests passed", passed, total);
    if (failed > 0)
        printf(" (%d FAILED)", failed);
    printf("\n\n");

    return failed > 0 ? 1 : 0;
}
