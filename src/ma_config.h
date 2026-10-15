/*
 * ma_config.h — Configuração do miniaudio antes da implementação
 *
 * Define macros para habilitar apenas o necessário (WAV, MP3, FLAC, OGG)
 * e desligar subsystems não usados, reduzindo o tamanho do binário.
 */

#ifndef MA_CONFIG_H
#define MA_CONFIG_H

/* ------------------------------------------------------------------ */
/* Backends — usar WASAPI como preferido (fallback automático para      */
/* DirectSound/WinMM se indisponível). Windows 7 tem WASAPI nativo.     */
/* ------------------------------------------------------------------ */

/* Backends Windows: WASAPI preferido, com fallback DirectSound/WinMM. */
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_WASAPI
#define MA_ENABLE_DSOUND
#define MA_ENABLE_WINMM
/* Test builds only: a scripted backend runs miniaudio's real device worker and
 * injects output failures (tests/test_audio_recovery.cpp). */
#ifdef LITESIC_TEST
#define MA_ENABLE_CUSTOM
#endif

/* ------------------------------------------------------------------ */
/* Decoders — habilitar apenas os formatos que o LiteSic suporta.       */
/* O miniaudio v0.11+ usa dr_* internos (dr_wav, dr_mp3, dr_flac).     */
/* OGG/Vorbis é habilitado via stb_vorbis incluído antes da            */
/* MINIAUDIO_IMPLEMENTATION em miniaudio_impl.c.                        */
/* ------------------------------------------------------------------ */

/* MA_NO_WAV/MA_NO_MP3/MA_NO_FLAC devem permanecer indefinidas:
 * essas macros desabilitam os decoders, inclusive com valor 0. */

/* Desligar o que NÃO vamos usar. */
#define MA_NO_ENCODING              /* Não gravamos áudio */
#define MA_NO_GENERATION            /* Não geramos tons/silêncio */

/* NÃO desligar node_graph nem resource_manager — ma_engine depende deles. */

/* ------------------------------------------------------------------ */
/* Outros                                                             */
/* ------------------------------------------------------------------ */

/* MA_DEBUG_OUTPUT é testada com #ifdef; deixá-la indefinida evita logs. */

#endif /* MA_CONFIG_H */
