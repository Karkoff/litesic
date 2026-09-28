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

/* Habilitar apenas WASAPI — remove Alsa/PulseAudio/JACK/Core Audio/etc. */
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_WASAPI

/* ------------------------------------------------------------------ */
/* Decoders — habilitar apenas os formatos que o LiteSic suporta.       */
/* O miniaudio v0.11+ usa dr_* internos (dr_wav, dr_mp3, dr_flac).     */
/* OGG/Vorbis é habilitado via stb_vorbis incluído antes da            */
/* MINIAUDIO_IMPLEMENTATION em miniaudio_impl.c.                        */
/* ------------------------------------------------------------------ */

#define MA_NO_WAV       /* NÃO desligar — necessário para WAV */
#define MA_NO_MP3       /* NÃO desligar — necessário para MP3 */
#define MA_NO_FLAC      /* NÃO desligar — necessário para FLAC */

/* Desligar o que NÃO vamos usar. */
#define MA_NO_ENCODING              /* Não gravamos áudio */
#define MA_NO_GENERATION            /* Não geramos tons/silêncio */

/* NÃO desligar node_graph nem resource_manager — ma_engine depende deles. */

/* ------------------------------------------------------------------ */
/* Outros                                                             */
/* ------------------------------------------------------------------ */

#define MA_DEBUG_OUTPUT   0   /* Desligar logs verbosos em release */

#endif /* MA_CONFIG_H */
