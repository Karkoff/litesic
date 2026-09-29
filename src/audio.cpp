/*
 * audio.cpp — Motor de áudio (miniaudio engine)
 *
 * M2-T1: inicialização e desligamento do ma_engine com WASAPI como backend
 * preferido (fallback automático para DirectSound/WinMM se indisponível).
 * M2-T2: carregar arquivo + duração.
 * M2-T3: play/pause/stop + posição.
 * M3-T3: sample rate, canais, bitrate.
 * M3-T4: seek.
 * M3-T5: volume, pan (balanço).
 * M3-T6: navegação prev/next — interface mínima provisória (lista de 1 item).
 */

#include "ma_config.h"
#include "audio.h"
#include "util.h"

/* Incluir miniaudio SOMENTE aqui (implementação em miniaudio_impl.c). */
#include "../third_party/miniaudio.h"

#include <cstdint>

/* ------------------------------------------------------------------ */
/* Estado interno (opaco)                                               */
/* ------------------------------------------------------------------ */

static struct AudioEngine
{
    ma_engine  engine;     /* Engine (device + resampler + mixing) */
    ma_sound   sound;      /* Sound carregado                      */
    BOOL       active;     /* TRUE se engine inicializada          */
    AudioState state;      /* Estado atual da reprodução           */
    wchar_t    currentPath[MAX_PATH]; /* Caminho do arquivo atual    */
    __int64    fileSizeBytes;   /* Tamanho em bytes (para bitrate) */

    /* M4-T1: Ring buffer de visualização. */
    float        vizBuffer[VISUALIZATION_BUFFER_SIZE]; /* Buffer circular */
    int          vizWritePos;      /* Próxima posição de escrita     */
    int          vizCount;         /* Amostras disponíveis para leitura */
    CRITICAL_SECTION vizLock;      /* Protege acesso concorrente     */
    BOOL         vizActive;        /* TRUE se viz buffer inicializado */
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

    g_audio.active   = TRUE;
    g_audio.state    = AUDIO_STATE_STOPPED;
    g_audio.fileSizeBytes = 0;
    g_audio.currentPath[0] = L'\0';
    /* sound será inicializado em Audio_Load(). */

    return TRUE;
}

/* ------------------------------------------------------------------ */
/* Visualização — M4-T1                                                 */
/* Ring buffer protegido por CRITICAL_SECTION.                          */
/* Decoder auxiliar para leitura de amostras sem interferir no playback.*/
/* ------------------------------------------------------------------ */

/* Decoder separado usado apenas para visualização.
 * Lê à frente do playback para alimentar o ring buffer sem travar. */
static ma_decoder g_vizDecoder{};
static BOOL       g_vizDecoderActive = FALSE;

BOOL Audio_VizInit(void)
{
    if (!g_audio.active || g_audio.vizActive)
        return TRUE; /* Já inicializado ou engine não ativa. */

    /* Zerar buffer e estado. */
    memset(g_audio.vizBuffer, 0, sizeof(g_audio.vizBuffer));
    g_audio.vizWritePos = 0;
    g_audio.vizCount    = 0;
    g_audio.vizActive   = TRUE;

    /* Inicializar critical section para proteção concorrente. */
    InitializeCriticalSection(&g_audio.vizLock);

    /* Inicializar decoder auxiliar para visualização (se houver arquivo carregado). */
    if (g_audio.currentPath[0] != L'\0')
    {
        ma_result result = ma_decoder_init_file_w(g_audio.currentPath, NULL, &g_vizDecoder);
        if (result == MA_SUCCESS)
        {
            g_vizDecoderActive = TRUE;
        }
    }

    return TRUE;
}

void Audio_VizShutdown(void)
{
    /* Parar decoder auxiliar. */
    if (g_vizDecoderActive)
    {
        ma_decoder_uninit(&g_vizDecoder);
        g_vizDecoderActive = FALSE;
    }

    if (g_audio.vizActive)
    {
        DeleteCriticalSection(&g_audio.vizLock);
        g_audio.vizActive = FALSE;
        g_audio.vizCount  = 0;
    }
}

/* Escreve amostras no ring buffer de visualização. */
static void Audio_VizWrite(const float* samples, int frameCount)
{
    if (!g_audio.vizActive || samples == NULL || frameCount <= 0)
        return;

    EnterCriticalSection(&g_audio.vizLock);

    for (int i = 0; i < frameCount; i++)
    {
        g_audio.vizBuffer[g_audio.vizWritePos] = samples[i];
        g_audio.vizWritePos = (g_audio.vizWritePos + 1) % VISUALIZATION_BUFFER_SIZE;

        if (g_audio.vizCount < VISUALIZATION_BUFFER_SIZE)
            g_audio.vizCount++;
    }

    LeaveCriticalSection(&g_audio.vizLock);
}

/* Atualiza o buffer de visualização lendo do decoder auxiliar.
 * Chamado a partir da UI thread (timer), nunca do callback de áudio.
 * O decoder auxiliar é sincronizado com a posição atual do playback. */
void Audio_UpdateVisualization(void)
{
    if (!g_audio.active || !g_audio.vizActive)
        return;

    /* Só ler amostras se há um sound carregado e está tocando. */
    if (g_audio.state != AUDIO_STATE_PLAYING)
        return;

    /* Sincronizar posição do decoder auxiliar com o playback atual.
     * Isso é necessário porque o decoder lê à frente e precisa acompanhar. */
    if (g_vizDecoderActive)
    {
        double posSec = Audio_GetPositionSeconds();
        int sampleRate = Audio_GetSampleRate();
        if (sampleRate > 0)
        {
            ma_uint64 targetFrame = (ma_uint64)(posSec * (double)sampleRate);
            /* Apenas seek se a diferença for maior que 512 frames (~11ms). */
            ma_uint64 currentFrame = 0;
            ma_decoder_get_cursor_in_pcm_frames(&g_vizDecoder, &currentFrame);
            if (llabs((long long)targetFrame - (long long)currentFrame) > 512)
            {
                ma_decoder_seek_to_pcm_frame(&g_vizDecoder, targetFrame);
            }
        }
    }

    /* Se o decoder auxiliar não está ativo, tentar inicializar. */
    if (!g_vizDecoderActive && g_audio.currentPath[0] != L'\0')
    {
        ma_result result = ma_decoder_init_file_w(g_audio.currentPath, NULL, &g_vizDecoder);
        if (result == MA_SUCCESS)
        {
            g_vizDecoderActive = TRUE;
            /* Sincronizar posição com o sound principal. */
            double posSec = Audio_GetPositionSeconds();
            int sampleRate = Audio_GetSampleRate();
            if (sampleRate > 0)
            {
                ma_uint64 targetFrame = (ma_uint64)(posSec * (double)sampleRate);
                ma_decoder_seek_to_pcm_frame(&g_vizDecoder, targetFrame);
            }
        }
    }

    if (!g_vizDecoderActive)
        return;

    /* Verificar formato do decoder auxiliar. */
    ma_format format = ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;
    if (ma_decoder_get_data_format(&g_vizDecoder, &format, &channels, &sampleRate, NULL, 0) != MA_SUCCESS)
        return;

    if (format != ma_format_f32)
        return;

    /* Ler blocos de 64 frames e converter para mono. */
    float tmp[64 * 2]; /* Máx 2 canais. */
    ma_uint32 channelsU32 = (ma_uint32)channels;

    /* Ler apenas UM bloco por timer tick para não ler à frente demais. */
    ma_uint64 framesRead64 = 0;
    ma_result result = ma_decoder_read_pcm_frames(&g_vizDecoder, tmp, 64, &framesRead64);
    if (result != MA_SUCCESS || framesRead64 == 0)
        return;

    ma_uint32 framesRead = (ma_uint32)framesRead64;

    /* Converter para mono e escrever no buffer. */
    for (ma_uint32 i = 0; i < framesRead; i++)
    {
        float sample = 0.0f;
        for (ma_uint32 ch = 0; ch < channelsU32; ch++)
            sample += tmp[i * channelsU32 + ch];
        sample /= (float)channelsU32;

        Audio_VizWrite(&sample, 1);
    }
}

int Audio_GetVisualizationSamples(float* out, int count)
{
    if (!g_audio.vizActive || out == NULL || count <= 0)
        return 0;

    /* Limitar ao tamanho do buffer. */
    if (count > VISUALIZATION_BUFFER_SIZE)
        count = VISUALIZATION_BUFFER_SIZE;

    EnterCriticalSection(&g_audio.vizLock);

    int available = g_audio.vizCount;
    if (available == 0)
    {
        LeaveCriticalSection(&g_audio.vizLock);
        return 0;
    }

    /* Copiar amostras mais recentes.
     * As amostras mais recentes estão em:
     *   writePos - 1, writePos - 2, ... (com wraparound) */
    int toCopy = count < available ? count : available;
    for (int i = 0; i < toCopy; i++)
    {
        int idx = (g_audio.vizWritePos - 1 - i + VISUALIZATION_BUFFER_SIZE) % VISUALIZATION_BUFFER_SIZE;
        out[i] = g_audio.vizBuffer[idx];
    }

    LeaveCriticalSection(&g_audio.vizLock);
    return toCopy;
}

void Audio_Shutdown(void)
{
    if (!g_audio.active)
        return;

    /* Parar e liberar o sound antes de desligar a engine. */
    ma_sound_uninit(&g_audio.sound);

    /* Encerrar visualização. */
    Audio_VizShutdown();

    ma_engine_uninit(&g_audio.engine);
    g_audio.active   = FALSE;
    g_audio.state    = AUDIO_STATE_STOPPED;
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

    /* Salvar caminho e tamanho do arquivo. */
    wcsncpy_s(g_audio.currentPath, ARRAYSIZE(g_audio.currentPath),
              path, _TRUNCATE);

    /* Obter tamanho do arquivo para cálculo de bitrate. */
    g_audio.fileSizeBytes = 0;
    {
        WIN32_FILE_ATTRIBUTE_DATA info{};
        if (GetFileAttributesExW(path, GetFileExInfoStandard, &info))
        {
            ULARGE_INTEGER size;
            size.LowPart  = info.nFileSizeLow;
            size.HighPart = info.nFileSizeHigh;
            g_audio.fileSizeBytes = (int64_t)size.QuadPart;
        }
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

/* ------------------------------------------------------------------ */
/* Informações do arquivo — M3-T3                                       */
/* ------------------------------------------------------------------ */

int Audio_GetSampleRate(void)
{
    if (!g_audio.active)
        return 0;

    return ma_engine_get_sample_rate(&g_audio.engine);
}

int Audio_GetChannels(void)
{
    if (!g_audio.active)
        return 0;

    /* Obter o número de canais do sound via data source format. */
    ma_format format = ma_format_unknown;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;
    ma_result result = ma_sound_get_data_format(&g_audio.sound, &format, &channels, &sampleRate, NULL, 0);
    if (result != MA_SUCCESS)
        return 0;

    return (int)channels;
}

int Audio_GetBitrateKbps(void)
{
    if (g_audio.fileSizeBytes <= 0)
        return 0;

    double duration = Audio_GetDurationSeconds();
    if (duration <= 0.0)
        return 0;

    return CalcApproxBitrateKbps(g_audio.fileSizeBytes, duration);
}

/* ------------------------------------------------------------------ */
/* Seek — M3-T4                                                         */
/* ------------------------------------------------------------------ */

void Audio_SeekSeconds(double seconds)
{
    if (!g_audio.active)
        return;

    /* Limitar aos limites do arquivo. */
    double dur = Audio_GetDurationSeconds();
    if (dur <= 0.0)
        return;

    if (seconds < 0.0) seconds = 0.0;
    if (seconds > dur) seconds = dur;

    /* Converter segundos para frames PCM. */
    int sampleRate = Audio_GetSampleRate();
    if (sampleRate <= 0)
        return;

    ma_uint64 targetFrame = (ma_uint64)(seconds * sampleRate);
    ma_sound_seek_to_pcm_frame(&g_audio.sound, targetFrame);
}

/* ------------------------------------------------------------------ */
/* Volume e balanço — M3-T5                                             */
/* ------------------------------------------------------------------ */

void Audio_SetVolume(float volume)
{
    if (!g_audio.active)
        return;

    /* Clamp entre 0.0 e 1.0. */
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    ma_engine_set_volume(&g_audio.engine, volume);
}

void Audio_SetPan(float pan)
{
    if (!g_audio.active)
        return;

    /* Clamp entre -1.0 e 1.0. */
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f)  pan =  1.0f;

    ma_sound_set_pan(&g_audio.sound, pan);
}

/* ------------------------------------------------------------------ */
/* Navegação — M3-T6 (interface mínima provisória)                      */
/* Lista de 1 item: Next/Previous não saem dos limites.                 */
/* Será substituída pela implementação real no M5.                      */
/* ------------------------------------------------------------------ */

void Audio_Next(BOOL shuffle, BOOL repeat)
{
    (void)shuffle; /* Usado quando playlist real for implementada no M5. */
    (void)repeat;  /* Usado quando playlist real for implementada no M5. */

    /* Provisório: se há um arquivo carregado, reiniciar do início.
     * No M5, isso usará a playlist real. */
    if (g_audio.active && g_audio.currentPath[0] != L'\0')
    {
        Audio_Stop();
        Audio_Play();
    }
}

void Audio_Previous(void)
{
    /* Provisório: se posição > 3s, reiniciar; senão, não fazer nada.
     * No M5, isso voltará para a faixa anterior da playlist. */
    if (g_audio.active && g_audio.state == AUDIO_STATE_PLAYING)
    {
        double pos = Audio_GetPositionSeconds();
        if (pos > 3.0)
        {
            Audio_SeekSeconds(0.0);
        }
    }
}
