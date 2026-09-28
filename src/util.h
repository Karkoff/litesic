/*
 * util.h — Utilitários para LiteSic (M3)
 *
 * Funções puras (sem dependência de GDI/janelas) usadas pelas telas:
 *   - Formatação de tempo LCD (M3-T1)
 *   - Cálculo de offset de scroll do título (M3-T2)
 *   - Cálculo de bitrate aproximado (M3-T3)
 *   - Conversão pixel↔valor para sliders (M3-T4, M3-T5)
 */

#ifndef UTIL_H
#define UTIL_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Tempo LCD — M3-T1                                                   */
/* ------------------------------------------------------------------ */

/* Formata segundos como "MM:SS" ou "-MM:SS" (remaining).
 * max_minutes limita os minutos exibidos (99 = máximo).
 * O buffer sz deve ter tamanho suficiente para MM:SS\0 (5 chars mínimo,
 * mas recomendamos 8 para margem). */
void FormatTime(double seconds, BOOL showRemaining, double duration,
                LPWSTR sz, int cchMax);

/* ------------------------------------------------------------------ */
/* Título rolando — M3-T2                                              */
/* ------------------------------------------------------------------ */

/* Calcula o offset horizontal de scroll para um texto que excede a área.
 * offset: ponteiro onde escrever o deslocamento em pixels (0 = parado).
 * textWidth: largura do texto em pixels (GetTextExtentPoint32W).
 * areaWidth: largura da área disponível.
 * elapsedMs: tempo decorrido desde o início do scroll em ms.
 * pauseMs: duração da pausa nas pontas em ms.
 * speedPxPerSec: velocidade de scroll em pixels por segundo.
 *
 * Retorna TRUE se o texto precisa rolar (textWidth > areaWidth), FALSE
 * caso contrário (offset é zerado). */
BOOL CalcScrollOffset(int *pOffset, int textWidth, int areaWidth,
                      DWORD elapsedMs, DWORD pauseMs, float speedPxPerSec);

/* ------------------------------------------------------------------ */
/* Bitrate aproximado — M3-T3                                          */
/* ------------------------------------------------------------------ */

/* Calcula bitrate médio em kbps a partir do tamanho do arquivo em bytes
 * e da duração em segundos. Retorna 0 se os parâmetros forem inválidos. */
int CalcApproxBitrateKbps(__int64 fileSizeBytes, double durationSec);

/* ------------------------------------------------------------------ */
/* Slider helpers — M3-T4 / M3-T5                                      */
/* ------------------------------------------------------------------ */

/* Converte posição do mouse (0..controlWidth) para valor normalizado
 * (minVal..maxVal). Linear. */
float PixelToValue(int pixel, int controlSize, float minVal, float maxVal);

/* Converte valor normalizado (minVal..maxVal) para posição do mouse
 * (0..controlSize). Linear. */
int ValueToPixel(float value, int controlSize, float minVal, float maxVal);

#ifdef __cplusplus
}
#endif

#endif /* UTIL_H */
