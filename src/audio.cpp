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
 * M5: navigation is implemented by PlaybackController.
 * M4-T1: visualização — nó tap no grafo do engine + ring buffer.
 */

#include "ma_config.h"
#include "audio.h"
#include "util.h"

/* Incluir miniaudio SOMENTE aqui (implementação em miniaudio_impl.c). */
#include "../third_party/miniaudio.h"

#include <cstdint>
#include "eq_state.h"
#include "tags.h"

static Eq::Processor g_eq;
static struct { ma_node_base base; ma_uint32 channels; } g_eqNode{};
static BOOL g_eqActive=FALSE;
static void EqProcess(ma_node*,const float** in,ma_uint32* ni,float** out,ma_uint32* no) {
    ma_uint32 n=*ni<*no?*ni:*no;
    g_eq.Process(out[0],in[0],n);*ni=*no=n;
}
static ma_node_vtable eqVtable={EqProcess,nullptr,1,1,0};
static BOOL InitEq(ma_engine* engine) {
    ma_uint32 channels=ma_engine_get_channels(engine);
    if(!channels || channels>Eq::MaxChannels) return FALSE;
    g_eq.Initialize(ma_engine_get_sample_rate(engine),channels,Eq::State());
    auto config=ma_node_config_init();config.vtable=&eqVtable;
    config.pInputChannels=&channels;config.pOutputChannels=&channels;
    if(ma_node_init(ma_engine_get_node_graph(engine),&config,nullptr,&g_eqNode.base)!=MA_SUCCESS) return FALSE;
    if(ma_node_attach_output_bus(&g_eqNode.base,0,ma_engine_get_endpoint(engine),0)!=MA_SUCCESS) {
        ma_node_uninit(&g_eqNode.base,nullptr);return FALSE;
    }
    g_eqActive=TRUE;return TRUE;
}
void Audio_UpdateEq(const Eq::Parameters& p) { if(g_eqActive) g_eq.Publish(p); }
void Audio_SetEqBand(int b,float v){Eq::SetGain(b,v);}
void Audio_SetEqEnabled(bool v){Eq::SetEnabled(v);}
void Audio_SetPreamp(float v){Eq::SetGain(-1,v);}

/* ------------------------------------------------------------------ */
/* Estado interno (opaco)                                               */
/* ------------------------------------------------------------------ */

static struct AudioEngine
{
    ma_engine  engine;     /* Engine (device + resampler + mixing) */
    ma_sound   sound;      /* Sound carregado                      */
    BOOL       active;     /* TRUE se engine inicializada          */
    BOOL       soundLoaded;
    float      pan;
    AudioState state;      /* Estado atual da reprodução           */
    wchar_t    currentPath[MAX_PATH]; /* Caminho do arquivo atual    */
    __int64    fileSizeBytes;   /* Tamanho em bytes (para bitrate) */
    int        fileSampleRate;  /* Taxa nativa do arquivo carregado */
    int        sourceSampleRate; /* Resource manager may resample to engine rate. */
    double     durationSeconds; /* UI-owned cache; never scan the live decoder. */
    BOOL       streamed;        /* Sound decoded by the resource manager job thread. */

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

static BOOL InitEngine(const ma_engine_config* config)
{
    ma_result result;

    if (g_audio.active)
        return TRUE; /* Já inicializado. */

    /* Inicializar engine com configurações padrão — usa o dispositivo de
     * saída padrão do SO (WASAPI no Windows 7+, fallback automático para
     * DirectSound/WinMM se WASAPI não estiver disponível). */
    result = ma_engine_init(config, &g_audio.engine);
    if (result != MA_SUCCESS)
    {
        /* Log de erro — não crashar, apenas retornar FALSE. */
        return FALSE;
    }

    if(!InitEq(&g_audio.engine)) { ma_engine_uninit(&g_audio.engine);return FALSE; }
    g_audio.active   = TRUE;
    g_audio.state    = AUDIO_STATE_STOPPED;
    g_audio.fileSizeBytes = 0;
    g_audio.currentPath[0] = L'\0';
    /* sound será inicializado em Audio_Load(). */

    return TRUE;
}

BOOL Audio_Init(void) { return InitEngine(nullptr); }
#ifdef LITESIC_TEST
BOOL Audio_TestInit(unsigned rate) {
    ma_engine_config config = ma_engine_config_init();
    config.noDevice = MA_TRUE;
    config.channels = 2;
    config.sampleRate = rate;
    return InitEngine(&config);
}
// A device consumes in real time while the resource manager job thread finishes
// stream seeks and refills pages. This offline device reads instantly, so it waits
// for that thread instead of rendering the silence a real device would never see.
// A pending sound seek is applied by the next read, which for a stream only posts
// the seek job: a device renders that one period silent. Spend it on a scratch frame.
static void WaitForStream() {
    if (!g_audio.soundLoaded || !g_audio.streamed || !g_audio.sound.pResourceManagerDataSource) return;
    auto& s = g_audio.sound.pResourceManagerDataSource->backend.stream;
    auto load = [](const auto& value) { return __atomic_load_n(&value, __ATOMIC_ACQUIRE); };
    if (load(g_audio.sound.seekTarget) != ~ma_uint64(0) && ma_sound_is_playing(&g_audio.sound)) {
        float scratch[2 * Eq::MaxChannels];
        ma_engine_read_pcm_frames(&g_audio.engine, scratch, 1, nullptr);
    }
    for (int i = 0; i < 5000; ++i) {
        if (!load(s.seekCounter) && (load(s.isDecoderAtEnd) || (load(s.isPageValid[0]) && load(s.isPageValid[1])))) return;
        Sleep(1);
    }
}
BOOL Audio_TestRead(float* frames, unsigned count) {
    WaitForStream();
    return ma_engine_read_pcm_frames(&g_audio.engine, frames, count, nullptr) == MA_SUCCESS;
}
unsigned long long Audio_TestCursor(void) {
    ma_uint64 cursor = 0;
    if (g_audio.soundLoaded) ma_sound_get_cursor_in_pcm_frames(&g_audio.sound, &cursor);
    return cursor;
}
void Audio_TestVizLock(BOOL lock) {
    if (lock) EnterCriticalSection(&g_audio.vizLock); else LeaveCriticalSection(&g_audio.vizLock);
}
#endif

/* ------------------------------------------------------------------ */
/* Visualização — M4-T1                                                 */
/* Um nó "tap" é inserido no grafo do ma_engine entre o sound e o       */
/* endpoint, agora depois do EQ. Copia uma mistura mono pós-EQ/pan e    */
/* anterior ao volume master do endpoint; mantém o contrato da M4.      */
/* ------------------------------------------------------------------ */

typedef struct
{
    ma_node_base base;
    ma_uint32    channels;
} VizTapNode;

static VizTapNode g_vizTap{};
static BOOL       g_vizTapActive = FALSE;
#ifdef LITESIC_TEST
void Audio_TestSkipEq(BOOL skip) {
    if(g_audio.soundLoaded) ma_node_attach_output_bus(&g_audio.sound,0,
        skip?(g_vizTapActive?static_cast<ma_node*>(&g_vizTap.base):ma_engine_get_endpoint(&g_audio.engine)):&g_eqNode.base,0);
}
#endif

/* Escreve amostras mono no ring buffer (seção crítica curta: só cópia). */
static void Audio_VizWriteBlock(const float* samples, int frameCount)
{
    /* Visualization is expendable; never wait for a UI reader in the callback. */
    if (!TryEnterCriticalSection(&g_audio.vizLock)) return;

    for (int i = 0; i < frameCount; i++)
    {
        g_audio.vizBuffer[g_audio.vizWritePos] = samples[i];
        g_audio.vizWritePos = (g_audio.vizWritePos + 1) % VISUALIZATION_BUFFER_SIZE;
    }
    g_audio.vizCount += frameCount;
    if (g_audio.vizCount > VISUALIZATION_BUFFER_SIZE)
        g_audio.vizCount = VISUALIZATION_BUFFER_SIZE;

    LeaveCriticalSection(&g_audio.vizLock);
}

/* Roda na thread de áudio: sem alocação, sem janelas. */
static void VizTap_Process(ma_node* pNode, const float** ppFramesIn,
                           ma_uint32* pFrameCountIn, float** ppFramesOut,
                           ma_uint32* pFrameCountOut)
{
    VizTapNode* tap = (VizTapNode*)pNode;
    ma_uint32 frames = *pFrameCountOut;
    if (*pFrameCountIn < frames)
        frames = *pFrameCountIn;

    const float* in  = ppFramesIn[0];
    float*       out = ppFramesOut[0];
    ma_uint32    ch  = tap->channels;

    memcpy(out, in, (size_t)frames * ch * sizeof(float));

    float mono[256];
    ma_uint32 done = 0;
    while (done < frames)
    {
        ma_uint32 n = frames - done;
        if (n > 256) n = 256;

        for (ma_uint32 i = 0; i < n; i++)
        {
            const float* f = in + (size_t)(done + i) * ch;
            float sum = 0.0f;
            for (ma_uint32 c = 0; c < ch; c++)
                sum += f[c];
            mono[i] = sum / (float)ch;
        }
        Audio_VizWriteBlock(mono, (int)n);
        done += n;
    }

    *pFrameCountIn  = frames;
    *pFrameCountOut = frames;
}

static ma_node_vtable g_vizTapVtable =
{
    VizTap_Process,
    NULL,
    1, /* 1 barra de entrada */
    1, /* 1 barra de saída   */
    0
};

BOOL Audio_VizInit(void)
{
    if (!g_audio.active || g_audio.vizActive)
        return TRUE; /* Já inicializado ou engine não ativa. */

    memset(g_audio.vizBuffer, 0, sizeof(g_audio.vizBuffer));
    g_audio.vizWritePos = 0;
    g_audio.vizCount    = 0;
    InitializeCriticalSection(&g_audio.vizLock);
    g_audio.vizActive   = TRUE;

    ma_uint32 channels = ma_engine_get_channels(&g_audio.engine);
    ma_node_config cfg = ma_node_config_init();
    cfg.vtable          = &g_vizTapVtable;
    cfg.pInputChannels  = &channels;
    cfg.pOutputChannels = &channels;
    g_vizTap.channels   = channels;

    if (ma_node_init(ma_engine_get_node_graph(&g_audio.engine), &cfg, NULL,
                     &g_vizTap.base) == MA_SUCCESS)
    {
        ma_node_attach_output_bus(&g_vizTap.base, 0,
                                  ma_engine_get_endpoint(&g_audio.engine), 0);
        g_vizTapActive = TRUE;
        ma_node_attach_output_bus(&g_eqNode.base,0,&g_vizTap.base,0);
    }

    return TRUE;
}

void Audio_VizShutdown(void)
{
    if (g_vizTapActive)
    {
        if(g_eqActive) ma_node_attach_output_bus(&g_eqNode.base,0,ma_engine_get_endpoint(&g_audio.engine),0);
        ma_node_uninit(&g_vizTap.base, NULL);
        g_vizTapActive = FALSE;
    }

    if (g_audio.vizActive)
    {
        g_audio.vizActive = FALSE;
        DeleteCriticalSection(&g_audio.vizLock);
        g_audio.vizCount  = 0;
    }
}

int Audio_GetVisualizationSamples(float* out, int count)
{
    if (!g_audio.vizActive || out == NULL || count <= 0)
        return 0;

    if (count > VISUALIZATION_BUFFER_SIZE)
        count = VISUALIZATION_BUFFER_SIZE;

    EnterCriticalSection(&g_audio.vizLock);

    int toCopy = count < g_audio.vizCount ? count : g_audio.vizCount;

    /* Ordem cronológica: out[0] é a mais antiga, out[toCopy-1] a mais recente. */
    for (int i = 0; i < toCopy; i++)
    {
        int idx = (g_audio.vizWritePos - toCopy + i + VISUALIZATION_BUFFER_SIZE)
                  % VISUALIZATION_BUFFER_SIZE;
        out[i] = g_audio.vizBuffer[idx];
    }

    LeaveCriticalSection(&g_audio.vizLock);
    return toCopy;
}

void Audio_Shutdown(void)
{
    if (!g_audio.active)
        return;

    ma_engine_stop(&g_audio.engine);
    /* Parar e liberar o sound antes de desligar a engine. */
    if (g_audio.soundLoaded) ma_sound_uninit(&g_audio.sound);
    g_audio.soundLoaded = FALSE;
    g_audio.durationSeconds = 0;
    g_audio.currentPath[0] = 0;

    /* Encerrar visualização. */
    Audio_VizShutdown();

    if(g_eqActive) {ma_node_uninit(&g_eqNode.base,nullptr);g_eqActive=FALSE;}

    ma_engine_uninit(&g_audio.engine);
    g_audio.active   = FALSE;
    g_audio.state    = AUDIO_STATE_STOPPED;
}

BOOL Audio_IsActive(void)
{
    return g_audio.active;
}

void Audio_Unload(void) {
    g_eq.RequestReset();
    if (g_audio.soundLoaded) ma_sound_uninit(&g_audio.sound);
    g_audio.soundLoaded = FALSE;
    g_audio.durationSeconds = 0;
    g_audio.currentPath[0] = 0;
    g_audio.fileSampleRate = 0;
    g_audio.fileSizeBytes = 0;
    g_audio.state = AUDIO_STATE_STOPPED;
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
    if (g_audio.soundLoaded) ma_sound_uninit(&g_audio.sound);
    g_audio.soundLoaded = FALSE;
    g_audio.durationSeconds = 0;
    g_audio.sourceSampleRate = 0;
    g_eq.RequestReset();

    /* Inicializar sound a partir do arquivo — miniaudio decodifica MP3/WAV/
     * FLAC nativamente. OGG/Vorbis é habilitado pelo stb_vorbis incluído
     * em miniaudio_impl.c antes da MINIAUDIO_IMPLEMENTATION. */
    __int64 fileSize = 0;
    {
        WIN32_FILE_ATTRIBUTE_DATA info{};
        if (GetFileAttributesExW(path, GetFileExInfoStandard, &info))
        {
            ULARGE_INTEGER size;
            size.LowPart  = info.nFileSizeLow;
            size.HighPart = info.nFileSizeHigh;
            fileSize = (int64_t)size.QuadPart;
        }
    }
    /* Larger files are streamed: the resource manager job thread decodes short
     * pages ahead of the device. Kept whole in memory, a 60-minute FLAC added
     * 502 MiB, a 1.3 GB WAV failed to load on x86, and the UI thread read the
     * entire file. Smaller files (every 4-minute track up to 96 kHz/24-bit FLAC,
     * 56 MiB) stay in memory: seeks and slider scrubbing remain gapless, while
     * each stream seek renders about 10 ms of silence. docs/PERFORMANCE-AUDIT.md */
    const __int64 streamThreshold = 64ll * 1024 * 1024;
    g_audio.streamed = fileSize > streamThreshold;
    result = ma_sound_init_from_file_w(&g_audio.engine, path,
                                       g_audio.streamed ? MA_SOUND_FLAG_STREAM : 0,
                                       NULL, NULL, &g_audio.sound);
    if (result != MA_SUCCESS)
    {
        g_audio.state = AUDIO_STATE_STOPPED;
        g_audio.currentPath[0] = L'\0';
        g_audio.fileSampleRate = 0;
        g_audio.fileSizeBytes = 0;
        return FALSE;
    }

    g_audio.soundLoaded = TRUE;
    ma_sound_set_pan(&g_audio.sound, g_audio.pan);
    /* Sound frames use the resource manager output rate, not necessarily the file rate. */
    g_audio.fileSampleRate = 0;
    ma_uint32 nativeRate = 0;
    if (ma_sound_get_data_format(&g_audio.sound, nullptr, nullptr, &nativeRate, nullptr, 0) == MA_SUCCESS)
        g_audio.sourceSampleRate = static_cast<int>(nativeRate);
    ma_decoder probe;
    if (ma_decoder_init_file_w(path, nullptr, &probe) == MA_SUCCESS) {
        if (ma_decoder_get_data_format(&probe, nullptr, nullptr, &nativeRate, nullptr, 0) == MA_SUCCESS)
            g_audio.fileSampleRate = static_cast<int>(nativeRate);
        // MP3 length queries scan and seek. Only use an independent decoder,
        // before playback, rather than racing the device's active decoder.
        ma_uint64 frames = 0;
        if (nativeRate && ma_decoder_get_length_in_pcm_frames(&probe,&frames)==MA_SUCCESS)
            g_audio.durationSeconds = static_cast<double>(frames)/nativeRate;
        ma_decoder_uninit(&probe);
    }
    // Unicode Vorbis push mode reports no length; metadata reads the final
    // granule using another independent decoder and bounded file buffers.
    if (g_audio.durationSeconds<=0)
        g_audio.durationSeconds=ReadTrackMetadata(path).duration;

    /* Rotear o sound pelo EQ; o tap opcional fica depois do EQ. */
    if (g_eqActive)
        ma_node_attach_output_bus(&g_audio.sound, 0, &g_eqNode.base, 0);

    /* Salvar caminho e tamanho do arquivo. */
    wcsncpy_s(g_audio.currentPath, ARRAYSIZE(g_audio.currentPath),
              path, _TRUNCATE);

    /* Tamanho do arquivo para cálculo de bitrate. */
    g_audio.fileSizeBytes = fileSize;

    /* Resetar estado. */
    g_audio.state = AUDIO_STATE_STOPPED;
    ma_sound_seek_to_pcm_frame(&g_audio.sound, 0);

    return TRUE;
}

double Audio_GetDurationSeconds(void)
{
    return g_audio.soundLoaded ? g_audio.durationSeconds : 0.0;
}

/* ------------------------------------------------------------------ */
/* Reprodução (M2-T3)                                                   */
/* ------------------------------------------------------------------ */

void Audio_Play(void)
{
    if (!g_audio.soundLoaded || g_audio.state == AUDIO_STATE_PLAYING)
        return;

    if (ma_sound_start(&g_audio.sound) == MA_SUCCESS) g_audio.state = AUDIO_STATE_PLAYING;
}

void Audio_Pause(void)
{
    if (!g_audio.soundLoaded || g_audio.state != AUDIO_STATE_PLAYING)
        return;

    ma_sound_stop(&g_audio.sound);
    /* miniaudio não tem pause nativo — paramos o sound mas mantemos a
     * posição. O estado PAUSED indica que devemos retomar de onde parou. */
    g_audio.state = AUDIO_STATE_PAUSED;
}

void Audio_Stop(void)
{
    g_audio.state = AUDIO_STATE_STOPPED;
    if (!g_audio.soundLoaded)
        return;

    ma_sound_stop(&g_audio.sound);
    ma_sound_seek_to_pcm_frame(&g_audio.sound, 0);
    g_audio.state = AUDIO_STATE_STOPPED;
    g_eq.RequestReset();
}

AudioState Audio_GetState(void)
{
    return g_audio.state;
}

BOOL Audio_IsAtEnd(void) {
    return g_audio.soundLoaded && ma_sound_at_end(&g_audio.sound);
}

double Audio_GetPositionSeconds(void)
{
    if (!g_audio.soundLoaded || g_audio.sourceSampleRate <= 0)
        return 0.0;

    ma_uint64 frameCount = 0;
    ma_sound_get_cursor_in_pcm_frames(&g_audio.sound, &frameCount);
    double position = (double)frameCount / g_audio.sourceSampleRate;
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

BOOL Audio_HasTrack(void)
{
    return g_audio.soundLoaded;
}

int Audio_GetFileSampleRate(void)
{
    return Audio_HasTrack() ? g_audio.fileSampleRate : 0;
}

void Audio_GetTrackName(wchar_t* out, int outLen)
{
    if (out == NULL || outLen <= 0)
        return;
    out[0] = L'\0';
    if (!Audio_HasTrack())
        return;

    const wchar_t* name = g_audio.currentPath;
    for (const wchar_t* p = name; *p; p++)
        if (*p == L'\\' || *p == L'/')
            name = p + 1;

    wcsncpy_s(out, (size_t)outLen, name, _TRUNCATE);

    wchar_t* dot = wcsrchr(out, L'.');
    if (dot != NULL && dot != out)
        *dot = L'\0';
}

int Audio_GetChannels(void)
{
    if (!g_audio.soundLoaded)
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
    if (!g_audio.soundLoaded)
        return;

    /* Limitar aos limites do arquivo. */
    double dur = Audio_GetDurationSeconds();
    if (dur <= 0.0)
        return;

    if (seconds < 0.0) seconds = 0.0;
    if (seconds > dur) seconds = dur;

    /* Converter segundos para frames PCM. */
    int sampleRate = g_audio.sourceSampleRate;
    if (sampleRate <= 0)
        return;

    ma_uint64 targetFrame = (ma_uint64)(seconds * sampleRate);
    ma_sound_seek_to_pcm_frame(&g_audio.sound, targetFrame);
    g_eq.RequestReset();
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

    g_audio.pan = pan;
    if (g_audio.soundLoaded) ma_sound_set_pan(&g_audio.sound, pan);
}
