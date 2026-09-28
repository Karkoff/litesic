/*
 * util.cpp — Implementações de utilitários (M3)
 *
 * M3-T1: FormatTime()
 * M3-T2: CalcScrollOffset()
 * M3-T3: CalcApproxBitrateKbps()
 * M3-T4/M3-T5: PixelToValue(), ValueToPixel()
 */

#include "util.h"
#include <stdlib.h>
#include <math.h>

/* ------------------------------------------------------------------ */
/* Tempo LCD — M3-T1                                                   */
/* ------------------------------------------------------------------ */

void FormatTime(double seconds, BOOL showRemaining, double duration,
                LPWSTR sz, int cchMax)
{
    if (cchMax < 6) /* MM:SS\0 = mínimo 6 chars */
        return;

    /* Calcular tempo a exibir. */
    double t = seconds;
    if (showRemaining && duration > 0.0)
    {
        t = duration - seconds;
        if (t < 0.0)
            t = 0.0;
    }

    /* Limitar a 99:59 máximo. */
    if (t < 0.0)
        t = 0.0;
    if (t > 99 * 60.0 + 59.0)
        t = 99 * 60.0 + 59.0;

    int totalSec = (int)floor(t + 0.5); /* arredondar */
    int mins = totalSec / 60;
    int secs = totalSec % 60;

    if (showRemaining && duration > 0.0 && seconds > 0.01)
    {
        /* Tempo restante: "-MM:SS" */
        if (mins > 99)
            mins = 99;
        wsprintfW(sz, L"-%02d:%02d", mins, secs);
    }
    else
    {
        /* Tempo decorrido: "MM:SS" */
        wsprintfW(sz, L"%02d:%02d", mins, secs);
    }
}

/* ------------------------------------------------------------------ */
/* Título rolando — M3-T2                                              */
/* ------------------------------------------------------------------ */

BOOL CalcScrollOffset(int *pOffset, int textWidth, int areaWidth,
                      DWORD elapsedMs, DWORD /*pauseMs*/, float speedPxPerSec)
{
    /* Se o texto cabe na área, não precisa rolar. */
    if (textWidth <= areaWidth)
    {
        *pOffset = 0;
        return FALSE;
    }

    int scrollRange = textWidth - areaWidth;

    /* Ciclo completo: scroll + pausa final + scroll reverso + pausa inicial
     * Comportamento clássico Winamp: rola da esquerda para direita,
     * pausa, volta ao início (sem reverso — simples reset). */
    DWORD cycleMs = (DWORD)((double)scrollRange / (double)speedPxPerSec * 1000.0);
    if (cycleMs == 0)
    {
        *pOffset = 0;
        return FALSE;
    }

    /* Tempo dentro do ciclo (wrap). */
    DWORD t = elapsedMs % cycleMs;

    /* Posição linear: começa em 0, vai até scrollRange. */
    float progress = (float)t / (float)cycleMs;
    if (progress > 1.0f)
        progress = 1.0f;

    *pOffset = (int)(progress * (float)scrollRange);

    return TRUE;
}

/* ------------------------------------------------------------------ */
/* Bitrate aproximado — M3-T3                                          */
/* ------------------------------------------------------------------ */

int CalcApproxBitrateKbps(__int64 fileSizeBytes, double durationSec)
{
    if (fileSizeBytes <= 0 || durationSec <= 0.0)
        return 0;

    /* bitrate = (bytes * 8) / duration / 1000 */
    double bits = (double)fileSizeBytes * 8.0;
    double kbps = bits / durationSec / 1000.0;

    return (int)round(kbps);
}

/* ------------------------------------------------------------------ */
/* Slider helpers — M3-T4 / M3-T5                                      */
/* ------------------------------------------------------------------ */

float PixelToValue(int pixel, int controlSize, float minVal, float maxVal)
{
    if (controlSize <= 1)
        return minVal;

    /* Clamp pixel dentro dos limites. */
    if (pixel < 0) pixel = 0;
    if (pixel > controlSize - 1) pixel = controlSize - 1;

    float t = (float)pixel / (float)(controlSize - 1);
    return minVal + t * (maxVal - minVal);
}

int ValueToPixel(float value, int controlSize, float minVal, float maxVal)
{
    if (controlSize <= 1)
        return 0;

    /* Clamp value dentro dos limites. */
    if (value <= minVal) return 0;
    if (value >= maxVal) return controlSize - 1;

    float t = (value - minVal) / (maxVal - minVal);
    return (int)round(t * (float)(controlSize - 1));
}
