/*
 * audio.h — Interface pública do motor de áudio (LiteSic)
 *
 * M2-T1: inicialização e desligamento do miniaudio engine.
 * Tipos do miniaudio são opacos — o resto do app nunca os vê diretamente.
 */

#ifndef AUDIO_H
#define AUDIO_H

#include <windows.h>

/* ------------------------------------------------------------------ */
/* Inicialização / desligamento                                         */
/* ------------------------------------------------------------------ */

/* Inicializa o motor de áudio (engine + device de saída).
 * Retorna TRUE em sucesso, FALSE se nenhum dispositivo de áudio estiver
 * disponível ou se a inicialização falhar. */
BOOL Audio_Init(void);

/* Desliga o motor de áudio e libera todos os recursos. */
void Audio_Shutdown(void);

/* Retorna TRUE se o motor foi inicializado com sucesso. */
BOOL Audio_IsActive(void);

#endif /* AUDIO_H */
