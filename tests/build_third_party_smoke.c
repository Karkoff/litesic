/*
 * build_third_party_smoke.c — Teste de smoke para third_party/
 *
 * Este arquivo só inclui os headers das bibliotecas de terceiros.
 * Se compila, a integração não quebra por conflito de macros.
 * miniaudio.h integra stb_vorbis automaticamente quando detecta o header.
 * Não contém lógica alguma.
 */

#include "miniaudio.h"

int main(void)
{
    /* Apenas verifica se os simbolos basicos existem e compilam. */
    return 0;
}
