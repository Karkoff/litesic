/*
 * audio.h — Interface pública do motor de áudio (LiteSic)
 *
 * M2-T1: inicialização e desligamento.
 * M2-T2: abrir/arquivo, duração.
 * M2-T3: play/pause/stop, posição.
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

#endif /* AUDIO_H */
