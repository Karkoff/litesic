/*
 * audio.cpp — Motor de áudio (miniaudio engine)
 *
 * M2-T1: inicialização e desligamento do ma_engine com WASAPI como backend
 * preferido (fallback automático para DirectSound/WinMM se indisponível).
 * M2-T2: carregar arquivo + duração.
 * M2-T3: play/pause/stop + posição.
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
    ma_engine  engine;     /* Engine (device + resampler + mixing) */
    ma_sound   sound;      /* Sound carregado                      */
    BOOL       active;     /* TRUE se engine inicializada          */
    AudioState state;      /* Estado atual da reprodução           */
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

    g_audio.active  = TRUE;
    g_audio.state   = AUDIO_STATE_STOPPED;
    /* sound será inicializado em Audio_Load(). */

    return TRUE;
}

void Audio_Shutdown(void)
{
    if (!g_audio.active)
        return;

    /* Parar e liberar o sound antes de desligar a engine. */
    ma_sound_uninit(&g_audio.sound);

    ma_engine_uninit(&g_audio.engine);
    g_audio.active  = FALSE;
    g_audio.state   = AUDIO_STATE_STOPPED;
}

BOOL Audio_IsActive(void)
{
    return g_audio.active;
}

/* ------------------------------------------------------------------ */
/* Carregar arquivo (M2-T2)                                             */
/* ------------------------------------------------------------------ */

BOOL Audio_Load(const wchar_t* path)
{
    ma_result result;

    if (!g_audio.active || path == NULL)
        return FALSE;

    /* Parar e liberar o sound anterior se houver. */
    ma_sound_uninit(&g_audio.sound);

    /* Inicializar sound a partir do arquivo — miniaudio decodifica MP3/WAV/
     * FLAC nativamente. OGG/Vorbis é habilitado pelo stb_vorbis incluído
     * em miniaudio_impl.c antes da MINIAUDIO_IMPLEMENTATION. */
    result = ma_sound_init_from_file_w(&g_audio.engine, path, 0, NULL,
                                       NULL, &g_audio.sound);
    if (result != MA_SUCCESS)
    {
        g_audio.state = AUDIO_STATE_STOPPED;
        return FALSE;
    }

    /* Resetar estado. */
    g_audio.state = AUDIO_STATE_STOPPED;
    ma_sound_seek_to_pcm_frame(&g_audio.sound, 0);

    return TRUE;
}

double Audio_GetDurationSeconds(void)
{
    if (!g_audio.active)
        return 0.0;

    /* Obter duração em frames e converter para segundos. */
    ma_uint64 frameCount = 0;
    ma_sound_get_length_in_pcm_frames(&g_audio.sound, &frameCount);
    if (frameCount == 0)
        return 0.0;

    double duration = (double)frameCount / (double)ma_engine_get_sample_rate(&g_audio.engine);
    return duration;
}

/* ------------------------------------------------------------------ */
/* Reprodução (M2-T3)                                                   */
/* ------------------------------------------------------------------ */

void Audio_Play(void)
{
    if (!g_audio.active || g_audio.state == AUDIO_STATE_PLAYING)
        return;

    ma_sound_start(&g_audio.sound);
    g_audio.state = AUDIO_STATE_PLAYING;
}

void Audio_Pause(void)
{
    if (!g_audio.active || g_audio.state != AUDIO_STATE_PLAYING)
        return;

    ma_sound_stop(&g_audio.sound);
    /* miniaudio não tem pause nativo — paramos o sound mas mantemos a
     * posição. O estado PAUSED indica que devemos retomar de onde parou. */
    g_audio.state = AUDIO_STATE_PAUSED;
}

void Audio_Stop(void)
{
    if (!g_audio.active)
        return;

    ma_sound_stop(&g_audio.sound);
    ma_sound_seek_to_pcm_frame(&g_audio.sound, 0);
    g_audio.state = AUDIO_STATE_STOPPED;
}

AudioState Audio_GetState(void)
{
    return g_audio.state;
}

double Audio_GetPositionSeconds(void)
{
    if (!g_audio.active)
        return 0.0;

    ma_uint64 frameCount = 0;
    ma_sound_get_cursor_in_pcm_frames(&g_audio.sound, &frameCount);
    double position = (double)frameCount / (double)ma_engine_get_sample_rate(&g_audio.engine);
    return position;
}
