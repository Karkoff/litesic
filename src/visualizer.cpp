/*
 * visualizer.cpp — Visualizador de áudio (M4)
 *
 * M4-T2: osciloscópio (waveform).
 * M4-T3: espectro via FFT com janela Hann e barras log-scale.
 */

#include "visualizer.h"
#include "audio.h"
#include <math.h>
#include <string.h>

/* M_PI não é definido por padrão no MinGW/C++17. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ------------------------------------------------------------------ */
/* Constantes auxiliares                                              */
/* ------------------------------------------------------------------ */

static const COLORREF OSCILLOSCOPE_COLOR = RGB(0x00, 0xcc, 0xff);
static const COLORREF SPECTRUM_BG_COLOR  = RGB(0x0a, 0x0a, 0x1a);
static const COLORREF SPECTRUM_BAR_COLOR = RGB(0x00, 0xff, 0x40);
static const COLORREF SPECTRUM_BORDER_COLOR = RGB(0x22, 0x44, 0x66);

/* ------------------------------------------------------------------ */
/* FFT simples (radix-2, tamanho fixo) — M4-T3                        */
/* ------------------------------------------------------------------ */

/* Estrutura para número complexo. */
typedef struct
{
    float real;
    float imag;
} Complex;

/* Aplica janela de Hann em um buffer de amostras. */
static void ApplyHannWindow(float* samples, int size)
{
    for (int i = 0; i < size; i++)
    {
        float window = 0.5f - 0.5f * cosf(2.0f * M_PI * (float)i / (float)(size - 1));
        samples[i] *= window;
    }
}

/* Calcula a FFT radix-2 in-place (algoritmo Cooley-Tukey). */
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
        Complex wStep = { cosf(angle), sinf(angle) };

        for (int start = 0; start < N; start += size)
        {
            Complex w = { 1.0f, 0.0f };
            for (int k = 0; k < size / 2; k++)
            {
                int evenIdx = start + k;
                int oddIdx = start + k + size / 2;

                Complex t = {
                    w.real * input[oddIdx].real - w.imag * input[oddIdx].imag,
                    w.real * input[oddIdx].imag + w.imag * input[oddIdx].real
                };

                Complex u = input[evenIdx];
                input[evenIdx] = { u.real + t.real, u.imag + t.imag };
                input[oddIdx] = { u.real - t.real, u.imag - t.imag };

                Complex newW = {
                    w.real * wStep.real - w.imag * wStep.imag,
                    w.real * wStep.imag + w.imag * wStep.real
                };
                w = newW;
            }
        }
    }
}

/* Calcula a magnitude de um array complexo. */
static void FFTMagnitude(const Complex* input, float* output, int size)
{
    for (int i = 0; i < size / 2; i++)
    {
        output[i] = sqrtf(input[i].real * input[i].real +
                          input[i].imag * input[i].imag);
    }
}

/* ------------------------------------------------------------------ */
/* Espectro log-scale — M4-T3                                         */
/* Agrupa bins de frequência em faixas logarítmicas.                  */
/* ------------------------------------------------------------------ */

/* Calcula os índices de início/fim para cada faixa log-scale. */
static void CalculateLogBins(int* startBins, int* endBins, int numBars, int fftSize)
{
    /* Faixa de frequência visível: 20Hz a Nyquist (sampleRate/2).
     * Usamos escala logarítmica para melhor representação visual. */
    float minLog = logf(20.0f);   /* 20 Hz — limite inferior audível. */
    float maxLog = logf((float)fftSize / 2.0f); /* Nyquist em bins. */
    float range = maxLog - minLog;

    for (int i = 0; i < numBars; i++)
    {
        float startLog = minLog + (float)i * range / (float)numBars;
        float endLog = minLog + (float)(i + 1) * range / (float)numBars;

        startBins[i] = (int)expf(startLog);
        endBins[i] = (int)expf(endLog);

        /* Clamp aos limites válidos. */
        if (startBins[i] < 0) startBins[i] = 0;
        if (endBins[i] >= fftSize / 2) endBins[i] = fftSize / 2 - 1;
        if (startBins[i] > endBins[i]) startBins[i] = endBins[i];
    }
}

/* ------------------------------------------------------------------ */
/* Osciloscópio — M4-T2                                               */
/* ------------------------------------------------------------------ */

int MapSampleToY(float sample, int height)
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

void DrawOscilloscope(HDC hdc, int x, int y, int w, int h)
{
    /* Fundo escuro da área de visualização. */
    RECT vizRect = { x, y, x + w, y + h };
    HBRUSH bgBrush = CreateSolidBrush(SPECTRUM_BG_COLOR);
    FillRect(hdc, &vizRect, bgBrush);
    DeleteObject(bgBrush);

    /* Borda sutil. */
    HPEN borderPen = CreatePen(PS_SOLID, 1, SPECTRUM_BORDER_COLOR);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    Rectangle(hdc, vizRect.left, vizRect.top, vizRect.right, vizRect.bottom);

    /* Puxar amostras do buffer de visualização. */
    float samples[FFT_SIZE];
    int count = Audio_GetVisualizationSamples(samples, FFT_SIZE);

    if (count < 2)
    {
        /* Sem amostras — desenhar linha plana no centro. */
        int centerY = y + h / 2;
        HPEN linePen = CreatePen(PS_SOLID, 1, OSCILLOSCOPE_COLOR);
        HGDIOBJ oldPen2 = SelectObject(hdc, linePen);
        MoveToEx(hdc, x + 1, centerY, NULL);
        LineTo(hdc, x + w - 1, centerY);
        SelectObject(hdc, oldPen2);
        DeleteObject(linePen);
    }
    else
    {
        /* Desenhar waveform conectando as amostras. */
        HPEN linePen = CreatePen(PS_SOLID, 1, OSCILLOSCOPE_COLOR);
        HGDIOBJ oldPen2 = SelectObject(hdc, linePen);

        int step = count > w ? count / w : 1; /* Reduzir amostras se necessário. */

        for (int i = 0; i < w && (i * step) < count; i++)
        {
            float sample = samples[i * step];
            int currY = y + MapSampleToY(sample, h);

            if (i == 0)
                MoveToEx(hdc, x + i, currY, NULL);
            else
                LineTo(hdc, x + i, currY);
        }

        SelectObject(hdc, oldPen2);
        DeleteObject(linePen);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

/* ------------------------------------------------------------------ */
/* Espectro via FFT — M4-T3                                           */
/* ------------------------------------------------------------------ */

void DrawSpectrum(HDC hdc, int x, int y, int w, int h)
{
    /* Fundo escuro. */
    RECT vizRect = { x, y, x + w, y + h };
    HBRUSH bgBrush = CreateSolidBrush(SPECTRUM_BG_COLOR);
    FillRect(hdc, &vizRect, bgBrush);
    DeleteObject(bgBrush);

    /* Borda sutil. */
    HPEN borderPen = CreatePen(PS_SOLID, 1, SPECTRUM_BORDER_COLOR);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    Rectangle(hdc, vizRect.left, vizRect.top, vizRect.right, vizRect.bottom);

    /* Puxar amostras do buffer de visualização. */
    float samples[FFT_SIZE];
    int count = Audio_GetVisualizationSamples(samples, FFT_SIZE);

    if (count >= FFT_SIZE)
    {
        /* Copiar amostras para um buffer temporário (FFT pode modificar). */
        float tempBuffer[FFT_SIZE];
        memcpy(tempBuffer, samples, sizeof(tempBuffer));

        /* Aplicar janela de Hann. */
        ApplyHannWindow(tempBuffer, FFT_SIZE);

        /* Converter para complexo. */
        Complex complexInput[FFT_SIZE];
        for (int i = 0; i < FFT_SIZE; i++)
        {
            complexInput[i].real = tempBuffer[i];
            complexInput[i].imag = 0.0f;
        }

        /* Calcular FFT. */
        FFT(complexInput, FFT_SIZE);

        /* Calcular magnitudes. */
        float magnitudes[FFT_SIZE / 2];
        FFTMagnitude(complexInput, magnitudes, FFT_SIZE);

        /* Normalizar (encontrar magnitude máxima). */
        float maxMag = 0.0f;
        for (int i = 0; i < FFT_SIZE / 2; i++)
        {
            if (magnitudes[i] > maxMag)
                maxMag = magnitudes[i];
        }
        if (maxMag > 0.0f)
        {
            for (int i = 0; i < FFT_SIZE / 2; i++)
                magnitudes[i] /= maxMag;
        }

        /* Calcular faixas log-scale. */
        int startBins[SPECTRUM_BARS];
        int endBins[SPECTRUM_BARS];
        CalculateLogBins(startBins, endBins, SPECTRUM_BARS, FFT_SIZE);

        /* Desenhar barras. */
        int barWidth = w / SPECTRUM_BARS;
        if (barWidth < 1) barWidth = 1;

        HPEN barPen = CreatePen(PS_SOLID, 0, SPECTRUM_BAR_COLOR);
        HGDIOBJ oldPen2 = SelectObject(hdc, barPen);

        for (int i = 0; i < SPECTRUM_BARS; i++)
        {
            /* Calcular magnitude média da faixa. */
            float sum = 0.0f;
            int count = 0;
            for (int j = startBins[i]; j <= endBins[i]; j++)
            {
                sum += magnitudes[j];
                count++;
            }
            float avgMag = count > 0 ? sum / (float)count : 0.0f;

            /* Altura da barra. */
            int barH = (int)(avgMag * (h - 2));
            if (barH < 1) barH = 1;
            if (barH > h - 2) barH = h - 2;

            RECT barRect = {
                x + i * barWidth, y + h - barH,
                x + (i + 1) * barWidth, y + h
            };
            FillRect(hdc, &barRect, CreateSolidBrush(SPECTRUM_BAR_COLOR));
        }

        SelectObject(hdc, oldPen2);
        DeleteObject(barPen);
    }
    else
    {
        /* Sem amostras suficientes — desenhar linha plana. */
        HPEN linePen = CreatePen(PS_SOLID, 1, SPECTRUM_BAR_COLOR);
        HGDIOBJ oldPen2 = SelectObject(hdc, linePen);
        MoveToEx(hdc, x + 1, y + h - 1, NULL);
        LineTo(hdc, x + w - 1, y + h - 1);
        SelectObject(hdc, oldPen2);
        DeleteObject(linePen);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}
