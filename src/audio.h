/*
 * audio.h — Interface pública do motor de áudio (LiteSic)
 *
 * M2-T1: inicialização e desligamento.
 * M2-T2: abrir/arquivo, duração.
 * M2-T3: play/pause/stop, posição.
 * M3-T3: sample rate, canais, bitrate.
 * M3-T4: seek.
 * M3-T5: volume, pan (balanço).
 * M3-T6: navegação (prev/next) — interface mínima provisória.
 * Tipos do miniaudio são opacos — o resto do app nunca os vê diretamente.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <windows.h>
#include "eq_dsp.h"
void Audio_UpdateEq(const Eq::Parameters& parameters);
void Audio_SetEqBand(int band,float gainDb);
void Audio_SetEqEnabled(bool enabled);
void Audio_SetPreamp(float gainDb);

/* ------------------------------------------------------------------ */
/* Inicialização / desligamento                                         */
/* ------------------------------------------------------------------ */

BOOL Audio_Init(void);
void Audio_Shutdown(void);
BOOL Audio_IsActive(void);

/* ------------------------------------------------------------------ */
/* Carregar arquivo (M2-T2)                                             */
/* ------------------------------------------------------------------ */

/* Carrega um arquivo de áudio para reprodução.
 * Retorna TRUE em sucesso, FALSE se falhar (arquivo não encontrado,
 * formato não suportado, etc.). Libera arquivo anterior se houver. */
BOOL Audio_Load(const wchar_t* path);
void Audio_Unload(void);

/* Retorna a duração do arquivo carregado em segundos.
 * Retorna 0.0 se nenhum arquivo estiver carregado. */
double Audio_GetDurationSeconds(void);

/* ------------------------------------------------------------------ */
/* Reprodução (M2-T3)                                                   */
/* ------------------------------------------------------------------ */

/* Estado atual da reprodução. */
typedef enum
{
    AUDIO_STATE_STOPPED = 0,
    AUDIO_STATE_PLAYING,
    AUDIO_STATE_PAUSED
} AudioState;

/* Toca o arquivo carregado a partir da posição atual. */
void Audio_Play(void);

/* Pausa a reprodução (preserva posição). */
void Audio_Pause(void);

/* Para e reseta a posição para 0. */
void Audio_Stop(void);

/* Retorna o estado atual da reprodução. */
AudioState Audio_GetState(void);
BOOL Audio_IsAtEnd(void);

/* ------------------------------------------------------------------ */
/* Saída de áudio: recuperação e diagnóstico                            */
/* ------------------------------------------------------------------ */

/* Result of the periodic check made while playing. */
typedef enum
{
    AUDIO_OUTPUT_OK = 0,    /* Output running, or nothing playing.        */
    AUDIO_OUTPUT_RECOVERED, /* Output had stopped; playback resumed.      */
    AUDIO_OUTPUT_FAILED     /* No working output: transport now PAUSED.   */
} AudioOutputCheck;

/* Call from the UI timer. While PLAYING, detects an output device that
 * stopped on its own (lost endpoint) and recovers it with a bounded number
 * of attempts. Audio_Play performs the same recovery on demand. */
AudioOutputCheck Audio_CheckOutput(void);

/* File for output failure/recovery records (created on the first event). */
void Audio_SetLogPath(const wchar_t* path);

#ifdef LITESIC_TEST
BOOL Audio_TestInit(unsigned rate);
BOOL Audio_TestRead(float* frames, unsigned count);
unsigned long long Audio_TestCursor(void);
void Audio_TestVizLock(BOOL lock);
void Audio_TestSkipEq(BOOL skip);
/* Real device on a caller-owned ma_context (scripted custom backend). */
BOOL Audio_TestInitContext(void* context);
BOOL Audio_TestOutputStarted(void);
void Audio_TestAdvanceClock(unsigned ms);
void Audio_TestStopOutput(void); /* Leaves the real device stopped, as a lost endpoint does. */
#endif

/* Retorna a posição atual em segundos. */
double Audio_GetPositionSeconds(void);

/* ------------------------------------------------------------------ */
/* Informações do arquivo — M3-T3                                       */
/* ------------------------------------------------------------------ */

/* Retorna a taxa de amostragem em Hz (ex.: 44100, 48000).
 * Retorna 0 se nenhum arquivo estiver carregado. */
int Audio_GetSampleRate(void);

/* TRUE se há uma faixa carregada com sucesso. */
BOOL Audio_HasTrack(void);

/* Taxa de amostragem nativa do arquivo carregado (0 se nenhum). */
int Audio_GetFileSampleRate(void);

/* Copia o nome da faixa (nome do arquivo sem extensão) para out.
 * out fica vazio se nenhuma faixa estiver carregada. */
void Audio_GetTrackName(wchar_t* out, int outLen);

/* Retorna o número de canais (1 = mono, 2 = estéreo).
 * Retorna 0 se nenhum arquivo estiver carregado. */
int Audio_GetChannels(void);

/* Retorna o bitrate aproximado em kbps.
 * Para VBR, retorna o bitrate médio calculado a partir do tamanho
 * do arquivo e da duração. Retorna 0 se não disponível. */
int Audio_GetBitrateKbps(void);

/* ------------------------------------------------------------------ */
/* Seek — M3-T4                                                         */
/* ------------------------------------------------------------------ */

/* Pula para a posição especificada em segundos. */
void Audio_SeekSeconds(double seconds);

/* ------------------------------------------------------------------ */
/* Volume e balanço — M3-T5                                             */
/* ------------------------------------------------------------------ */

/* Define o volume master (0.0 = mudo, 1.0 = máximo). */
void Audio_SetVolume(float volume);

/* Define o balanço/pan (-1.0 = totalmente esquerda, 0 = centro,
 * 1.0 = totalmente direita). */
void Audio_SetPan(float pan);

/* ------------------------------------------------------------------ */
/* Navigation is owned by PlaybackController (M5).                     */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Visualização — M4-T1                                                 */
/* Ring buffer de amostras para FFT/osciloscópio.                       */
/* ------------------------------------------------------------------ */

/* Número de amostras no buffer de visualização (potência de 2).
 * 2048 samples → ~46ms a 44100Hz; a UI lê as mais recentes. */
#define VISUALIZATION_BUFFER_SIZE  2048

/* Inicializa o ring buffer de visualização (chamar após Audio_Init).
 * Retorna TRUE em sucesso. */
BOOL Audio_VizInit(void);

/* Encerra o ring buffer de visualização. */
void Audio_VizShutdown(void);

/* Pega uma cópia das N amostras mais recentes do buffer (mono), em ordem
 * cronológica: out[0] é a mais antiga e out[n-1] a mais recente.
 * out: buffer de destino (float[N]).
 * count: número de amostras desejadas (deve ser <= VISUALIZATION_BUFFER_SIZE).
 * Retorna o número real de amostras copiadas. */
int Audio_GetVisualizationSamples(float* out, int count);

#endif /* AUDIO_H */
