/*
 * build_third_party_smoke.c — Teste de smoke para third_party/
 *
 * Este arquivo só inclui os headers das bibliotecas de terceiros.
 * Se compila, a integração não quebra por conflito de macros.
 * Não contém lógica alguma.
 */

#include "../third_party/stb_vorbis.h"
#include "../third_party/miniaudio.h"

int main(void)
{
    /* Apenas verifica se os símbolos básicos existem e compilam. */
    return 0;
}
