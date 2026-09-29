/*
 * visualizer.h — Visualizador de áudio (M4)
 *
 * M4-T2: osciloscópio (waveform).
 * M4-T3: espectro via FFT.
 * M4-T4: toggle por clique entre os dois modos.
 */

#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <windows.h>

/* ------------------------------------------------------------------ */
/* Configurações                                                      */
/* ------------------------------------------------------------------ */

/* Tamanho da FFT (potência de 2). 512 → ~11ms a 44100Hz. */
#define FFT_SIZE              512

/* Número de barras no espectro (faixas log-scale). */
#define SPECTRUM_BARS         32

/* ------------------------------------------------------------------ */
/* Modos de visualização                                              */
/* ------------------------------------------------------------------ */

typedef enum
{
    VIZ_MODE_OSCILLOSCOPE = 0,
    VIZ_MODE_SPECTRUM
} VizMode;

/* ------------------------------------------------------------------ */
/* Interface pública                                                  */
/* ------------------------------------------------------------------ */

/* Desenha o osciloscópio (waveform) na área especificada.
 * hdc: DC de destino (já escalada por DPI).
 * x, y, w, h: posição e dimensões da área. */
void DrawOscilloscope(HDC hdc, int x, int y, int w, int h);

/* Desenha o espectro (FFT) na área especificada.
 * hdc: DC de destino (já escalada por DPI).
 * x, y, w, h: posição e dimensões da área. */
void DrawSpectrum(HDC hdc, int x, int y, int w, int h);

/* Converte amplitude de amostra (-1.0..1.0) para posição Y em pixels.
 * height: altura da área de desenho.
 * sample: amplitude normalizada (-1.0 a 1.0).
 * Retorna Y em pixels (0 = topo, height/2 = centro). */
int MapSampleToY(float sample, int height);

#endif /* VISUALIZER_H */
