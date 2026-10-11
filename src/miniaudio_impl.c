/*
 * miniaudio_impl.c — Implementação do miniaudio para LiteSic
 *
 * Ordem de inclusão:
 *   1. stb_vorbis.h        → declarações (visíveis ao miniaudio)
 *   2. stb_vorbis.c        → implementação (STB_VORBIS_IMPLEMENTATION)
 *      com STB_VORBIS_NO_INTEGER_CONVERSION para evitar conflito
 *      de macros PLAYBACK_LEFT/RIGHT com headers do Windows.
 *   3. miniaudio.h         → backend Vorbis ativado pelas declarações acima
 */

#include "ma_config.h"

/* 1. Declarações do stb_vorbis — DEVE vir antes de miniaudio.h para ativar
 *    o decoder customizado de OGG Vorbis no miniaudio.               */
#include "../third_party/stb_vorbis.h"

/* 2. Implementação do stb_vorbis (única translation unit).            */
#define STB_VORBIS_IMPLEMENTATION
/* Desabilitar conversão inteira para evitar conflito de macros PLAYBACK_*   */
/* com headers COM/DirectX do Windows. O miniaudio usa apenas float.        */
#define STB_VORBIS_NO_INTEGER_CONVERSION
#include "../third_party/stb_vorbis.c"

/* 3. miniaudio — vê as declarações de stb_vorbis e ativa o backend     */
/*    de decodificação OGG Vorbis automaticamente.                      */
#define MINIAUDIO_IMPLEMENTATION
#include "../third_party/miniaudio.h"
