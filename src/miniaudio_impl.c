/*
 * miniaudio_impl.c — Implementação do miniaudio para LiteSic
 *
 * Este é o único arquivo .c/.cpp do projeto que define
 * MINIAUDIO_IMPLEMENTATION antes de incluir miniaudio.h, evitando
 * duplicação de símbolos em outras translation units.
 *
 * O header stb_vorbis.h é incluído antes do miniaudio.h para habilitar
 * o suporte nativo a decodificação OGG Vorbis do miniaudio.
 */

#define MINIAUDIO_IMPLEMENTATION

/* Inclui stb_vorbis primeiro — o miniaudio detecta o header e ativa
   o decoder Vorbis automaticamente. */
#include "../third_party/stb_vorbis.c"
#include "../third_party/miniaudio.h"
