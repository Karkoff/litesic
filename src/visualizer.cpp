/*
 * visualizer.cpp — Visualizador de áudio (M4)
 *
 * M4-T2: osciloscópio (waveform).
 * M4-T3: espectro via FFT com janela Hann e barras log-scale.
 */

#include "visualizer.h"
#include "audio.h"
#include "ui_draw.h"
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
/* ------------------------------------------------------------------ */
/* Espectro log-scale — M4-T3                                         */
/* Agrupa bins de frequência em faixas logarítmicas.                  */
/* ------------------------------------------------------------------ */

/* Calcula os bins de início/fim (inclusivos) de cada barra.
 * As faixas vão de 40 Hz a min(16 kHz, Nyquist), espaçadas em log,
 * convertendo Hz -> índice de bin (bin = freq * fftSize / sampleRate). */
static void CalculateLogBins(int* startBins, int* endBins, int numBars,
                             int fftSize, int sampleRate)
{
    const float nyquist = (float)sampleRate / 2.0f;
    float fMin = 40.0f;
    float fMax = nyquist < 16000.0f ? nyquist : 16000.0f;
    const float logMin = logf(fMin);
    const float logRange = logf(fMax) - logMin;
    const int maxBin = fftSize / 2 - 1;

    int prevEnd = 0; /* bin 0 (DC) nunca é usado. */
    for (int i = 0; i < numBars; i++)
    {
        float fHi = expf(logMin + (float)(i + 1) * logRange / (float)numBars);
        int endBin = (int)(fHi * (float)fftSize / (float)sampleRate + 0.5f);
        if (endBin > maxBin) endBin = maxBin;

        int startBin = prevEnd + 1;
        if (startBin > maxBin) startBin = maxBin;
        if (endBin < startBin) endBin = startBin;

        startBins[i] = startBin;
        endBins[i] = endBin;
        prevEnd = endBin;
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
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, vizRect.left, vizRect.top, vizRect.right, vizRect.bottom);
    SelectObject(hdc, oldBrush);

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
    /* Altura das barras suavizada entre quadros (queda gradual). */
    static float smoothed[SPECTRUM_BARS];

    /* Fundo escuro. */
    RECT vizRect = { x, y, x + w, y + h };
    HBRUSH bgBrush = CreateSolidBrush(SPECTRUM_BG_COLOR);
    FillRect(hdc, &vizRect, bgBrush);
    DeleteObject(bgBrush);

    /* Borda sutil. */
    HPEN borderPen = CreatePen(PS_SOLID, 1, SPECTRUM_BORDER_COLOR);
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, vizRect.left, vizRect.top, vizRect.right, vizRect.bottom);
    SelectObject(hdc, oldBrush);

    /* Pega as FFT_SIZE amostras mais recentes (ordem cronológica). */
    float samples[FFT_SIZE];
    int count = Audio_GetVisualizationSamples(samples, FFT_SIZE);

    int sampleRate = Audio_GetSampleRate();
    if (sampleRate <= 0) sampleRate = 44100;

    float levels[SPECTRUM_BARS];
    if (count >= FFT_SIZE)
    {
        ApplyHannWindow(samples, FFT_SIZE);

        Complex data[FFT_SIZE];
        for (int i = 0; i < FFT_SIZE; i++)
        {
            data[i].real = samples[i];
            data[i].imag = 0.0f;
        }
        FFT(data, FFT_SIZE);

        float magnitudes[FFT_SIZE / 2];
        FFTMagnitude(data, magnitudes, FFT_SIZE);

        int startBins[SPECTRUM_BARS];
        int endBins[SPECTRUM_BARS];
        CalculateLogBins(startBins, endBins, SPECTRUM_BARS, FFT_SIZE, sampleRate);

        /* Referência fixa: uma senoide de escala cheia com janela Hann
         * gera magnitude FFT_SIZE/4. Mapeia -60 dB..0 dB para 0..1. */
        const float ref = (float)FFT_SIZE / 4.0f;
        for (int i = 0; i < SPECTRUM_BARS; i++)
        {
            float peak = 0.0f;
            for (int j = startBins[i]; j <= endBins[i]; j++)
                if (magnitudes[j] > peak)
                    peak = magnitudes[j];

            float db = 20.0f * log10f(peak / ref + 1e-9f);
            float level = (db + 60.0f) / 60.0f;
            if (level < 0.0f) level = 0.0f;
            if (level > 1.0f) level = 1.0f;
            levels[i] = level;
        }
    }
    else
    {
        for (int i = 0; i < SPECTRUM_BARS; i++)
            levels[i] = 0.0f;
    }

    HBRUSH barBrush = CreateSolidBrush(SPECTRUM_BAR_COLOR);
    int barWidth = w / SPECTRUM_BARS;
    if (barWidth < 1) barWidth = 1;

    for (int i = 0; i < SPECTRUM_BARS; i++)
    {
        float fall = smoothed[i] * 0.85f;
        smoothed[i] = levels[i] > fall ? levels[i] : fall;

        int barH = (int)(smoothed[i] * (float)(h - 2));
        if (barH < 1) barH = 1;

        RECT barRect = {
            x + i * barWidth, y + h - 1 - barH,
            x + (i + 1) * barWidth - 1, y + h - 1
        };
        FillRect(hdc, &barRect, barBrush);
    }

    DeleteObject(barBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}

// Direct2D presentation shares the production FFT/window/bin algorithms.
// Sample acquisition and FFT run on the UI thread, never in the audio callback.
void DrawVisualizerD2D(Ui::Canvas& c, Ui::Rect r, VizMode mode) {
    c.Fill(r,Ui::Display,2); c.Stroke(r,Ui::Active,2);
    c.Clip(r);
    for(float x=r.x+8;x<r.x+r.w;x+=8) c.Line(x,r.y,x,r.y+r.h,Ui::Active);
    float samples[FFT_SIZE]{};
    int count=Audio_GetVisualizationSamples(samples,FFT_SIZE);
    if(mode==VIZ_MODE_OSCILLOSCOPE) {
        if(count<2) c.Line(r.x,r.y+r.h/2,r.x+r.w,r.y+r.h/2,Ui::Green);
        else for(int i=1;i<count;++i)
            c.Line(r.x+(i-1)*r.w/(count-1),r.y+MapSampleToY(samples[i-1],static_cast<int>(r.h-2))+1,
                r.x+i*r.w/(count-1),r.y+MapSampleToY(samples[i],static_cast<int>(r.h-2))+1,Ui::Green);
    } else {
        static float smooth[SPECTRUM_BARS]{};
        float levels[SPECTRUM_BARS]{};
        if(count>=FFT_SIZE) {
            ApplyHannWindow(samples,FFT_SIZE);
            Complex data[FFT_SIZE]{};
            for(int i=0;i<FFT_SIZE;++i) data[i].real=samples[i];
            FFT(data,FFT_SIZE);
            float magnitudes[FFT_SIZE/2]; FFTMagnitude(data,magnitudes,FFT_SIZE);
            int start[SPECTRUM_BARS],end[SPECTRUM_BARS];
            int rate=Audio_GetSampleRate(); if(rate<=0) rate=44100;
            CalculateLogBins(start,end,SPECTRUM_BARS,FFT_SIZE,rate);
            for(int i=0;i<SPECTRUM_BARS;++i) {
                float peak=0;
                for(int j=start[i];j<=end[i];++j) if(magnitudes[j]>peak) peak=magnitudes[j];
                float value=(20*log10f(peak/(FFT_SIZE/4.f)+1e-9f)+60)/60;
                levels[i]=value<0?0:value>1?1:value;
            }
        }
        for(int i=0;i<SPECTRUM_BARS;++i) {
            smooth[i]=fmaxf(levels[i],smooth[i]*.85f);
            float height=fmaxf(1,smooth[i]*(r.h-2));
            c.Fill({r.x+i*r.w/SPECTRUM_BARS+1,r.y+r.h-height-1,r.w/SPECTRUM_BARS-2,height},Ui::Green);
        }
    }
    c.Unclip();
}
