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

/* Inclui Windows headers primeiro (via miniaudio) para que possamos
   undefinir os macros de canal que stb_vorbis redefine depois. */
#include "../third_party/miniaudio.h"

/* stb_vorbis redefine PLAYBACK_LEFT/PLAYBACK_RIGHT que conflitam com
   winuser.h (canal masks). Undefinimos antes de incluir o .c. */
#undef PLAYBACK_LEFT
#undef PLAYBACK_RIGHT
#undef PLAYBACK_RIGHT_SURROUND
#undef PLAYBACK_LEFT_SURROUND
#undef PLAYBACK_CENTER
#undef PLAYBACK_LOW_FREQUENCY
#undef PLAYBACK_BACK_LEFT
#undef PLAYBACK_BACK_RIGHT
#undef PLAYBACK_FRONT_LEFT_OF_CENTER
#undef PLAYBACK_FRONT_RIGHT_OF_CENTER
#undef PLAYBACK_BACK_CENTER
#undef PLAYBACK_SIDE_LEFT
#undef PLAYBACK_SIDE_RIGHT

/* stb_vorbis como .c — contém a implementação do decoder. */
#include "../third_party/stb_vorbis.c"
