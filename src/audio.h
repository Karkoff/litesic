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

/* Retorna a posição atual em segundos. */
double Audio_GetPositionSeconds(void);

/* ------------------------------------------------------------------ */
/* Informações do arquivo — M3-T3                                       */
/* ------------------------------------------------------------------ */

/* Retorna a taxa de amostragem em Hz (ex.: 44100, 48000).
 * Retorna 0 se nenhum arquivo estiver carregado. */
int Audio_GetSampleRate(void);

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
/* Navegação — M3-T6 (interface mínima provisória)                      */
/* ------------------------------------------------------------------ */

/* Avança para a próxima faixa.
 * shuffle: se TRUE, pula para uma faixa aleatória.
 * repeat:  se TRUE, volta à primeira faixa ao chegar no fim. */
void Audio_Next(BOOL shuffle, BOOL repeat);

/* Volta para a faixa anterior (ou reinicia a atual se posição > 3s). */
void Audio_Previous(void);

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
