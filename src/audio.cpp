/*
 * audio.cpp — Motor de áudio (miniaudio engine)
 *
 * M2-T1: inicialização e desligamento do ma_engine com WASAPI como backend
 * preferido (fallback automático para DirectSound/WinMM se indisponível).
 */

#include "ma_config.h"
#include "audio.h"

/* Incluir miniaudio SOMENTE aqui (implementação em miniaudio_impl.c). */
#include "../third_party/miniaudio.h"

/* ------------------------------------------------------------------ */
/* Estado interno (opaco)                                               */
/* ------------------------------------------------------------------ */

static struct AudioEngine
{
    ma_engine engine;   /* Engine (device + resampler + mixing) */
    BOOL      active;   /* TRUE se inicializado com sucesso     */
} g_audio{};

/* ------------------------------------------------------------------ */
/* Interface pública                                                    */
/* ------------------------------------------------------------------ */

BOOL Audio_Init(void)
{
    ma_result result;

    if (g_audio.active)
        return TRUE; /* Já inicializado. */

    /* Inicializar engine com configurações padrão — usa o dispositivo de
     * saída padrão do SO (WASAPI no Windows 7+, fallback automático para
     * DirectSound/WinMM se WASAPI não estiver disponível). */
    result = ma_engine_init(NULL, &g_audio.engine);
    if (result != MA_SUCCESS)
    {
        /* Log de erro — não crashar, apenas retornar FALSE. */
        return FALSE;
    }

    g_audio.active = TRUE;
    return TRUE;
}

void Audio_Shutdown(void)
{
    if (!g_audio.active)
        return;

    ma_engine_uninit(&g_audio.engine);
    g_audio.active = FALSE;
}

BOOL Audio_IsActive(void)
{
    return g_audio.active;
}
