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

#include <cstdarg>
#include <cstdint>
#include <cwchar>
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
    float      volume = 1.0f;

    /* Output recovery: see RecoverOutput(). */
    ma_engine_config config;    /* Recreates the engine on the current default device. */
    BOOL       configured;
    wchar_t    restorePath[MAX_PATH]; /* Track to reopen once an output exists again. */
    ma_uint64  restoreFrame;
    DWORD      lastRecovery;
    int        stalledChecks;
    int        autoRecoveries;

    /* M4-T1: Ring buffer de visualização. */
    float        vizBuffer[VISUALIZATION_BUFFER_SIZE]; /* Buffer circular */
    int          vizWritePos;      /* Próxima posição de escrita     */
    int          vizCount;         /* Amostras disponíveis para leitura */
    CRITICAL_SECTION vizLock;      /* Protege acesso concorrente     */
    BOOL         vizActive;        /* TRUE se viz buffer inicializado */
} g_audio{};

/* ------------------------------------------------------------------ */
/* Diagnostics: output failures and recoveries only, never per period.  */
/* ------------------------------------------------------------------ */

static wchar_t g_logPath[MAX_PATH];

void Audio_SetLogPath(const wchar_t* path)
{
    wcsncpy_s(g_logPath, ARRAYSIZE(g_logPath), path ? path : L"", _TRUNCATE);
}

static void AudioTrace(const wchar_t* format, ...)
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    wchar_t line[640] = {};
    const int room = (int)ARRAYSIZE(line) - 3; /* CR, LF, terminator */
    int n = _snwprintf(line, room, L"%04u-%02u-%02u %02u:%02u:%02u.%03u audio: ",
                       t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
    if (n < 0) return;
    va_list args;
    va_start(args, format);
    int m = _vsnwprintf(line + n, room - n, format, args);
    va_end(args);
    n = m < 0 ? room : n + m;
    line[n++] = L'\r'; line[n++] = L'\n'; line[n] = 0;
    OutputDebugStringW(line);
    if (!g_logPath[0]) return;

    /* Created on the first event only; restarted beyond 256 KiB. */
    HANDLE file = CreateFileW(g_logPath, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                              OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    LARGE_INTEGER size{};
    if (file != INVALID_HANDLE_VALUE && GetFileSizeEx(file, &size) && size.QuadPart > 256 * 1024) {
        CloseHandle(file);
        file = CreateFileW(g_logPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    }
    if (file == INVALID_HANDLE_VALUE) return;
    char utf8[2048];
    int bytes = WideCharToMultiByte(CP_UTF8, 0, line, n, utf8, sizeof(utf8), nullptr, nullptr);
    DWORD written = 0;
    if (bytes > 0) WriteFile(file, utf8, (DWORD)bytes, &written, nullptr);
    CloseHandle(file);
}

/* ------------------------------------------------------------------ */
/* Interface pública                                                    */
/* ------------------------------------------------------------------ */

static BOOL InitEngine(const ma_engine_config* config)
{
    ma_result result;

    if (g_audio.active)
        return TRUE; /* Já inicializado. */

    /* Kept so a lost output can be recreated with the same settings. */
    if (config != &g_audio.config)
        g_audio.config = config ? *config : ma_engine_config_init();
    g_audio.configured = TRUE;

    /* Inicializar engine com configurações padrão — usa o dispositivo de
     * saída padrão do SO (WASAPI no Windows 7+, fallback automático para
     * DirectSound/WinMM se WASAPI não estiver disponível). */
    result = ma_engine_init(&g_audio.config, &g_audio.engine);
    if (result != MA_SUCCESS)
    {
        /* Não crashar: registrar e retornar FALSE; Play tenta de novo. */
        AudioTrace(L"engine init failed: %d", (int)result);
        return FALSE;
    }

    if(!InitEq(&g_audio.engine)) { AudioTrace(L"equalizer node init failed");ma_engine_uninit(&g_audio.engine);return FALSE; }
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
BOOL Audio_TestInitContext(void* context) {
    ma_engine_config config = ma_engine_config_init();
    config.pContext = static_cast<ma_context*>(context);
    config.channels = 2;
    config.sampleRate = 48000;
    return InitEngine(&config);
}
BOOL Audio_TestOutputStarted(void) {
    ma_device* device = g_audio.active ? ma_engine_get_device(&g_audio.engine) : nullptr;
    return device && ma_device_get_state(device) == ma_device_state_started;
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

/* The tap node belongs to the engine's graph; the ring buffer outlives
 * an engine rebuilt by output recovery. */
static void AttachVizTap(void)
{
    if (!g_audio.active || !g_audio.vizActive || g_vizTapActive)
        return;

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
}

static void DetachVizTap(void)
{
    if (g_vizTapActive)
    {
        if(g_eqActive) ma_node_attach_output_bus(&g_eqNode.base,0,ma_engine_get_endpoint(&g_audio.engine),0);
        ma_node_uninit(&g_vizTap.base, NULL);
        g_vizTapActive = FALSE;
    }
}

BOOL Audio_VizInit(void)
{
    if (!g_audio.vizActive)
    {
        memset(g_audio.vizBuffer, 0, sizeof(g_audio.vizBuffer));
        g_audio.vizWritePos = 0;
        g_audio.vizCount    = 0;
        InitializeCriticalSection(&g_audio.vizLock);
        g_audio.vizActive   = TRUE;
    }
    AttachVizTap();
    return TRUE;
}

void Audio_VizShutdown(void)
{
    DetachVizTap();

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

/* Releases the sound, the graph nodes and the engine. Recovery calls it
 * without ma_engine_stop: the device has already stopped, and miniaudio's
 * uninit deliberately avoids stopping a backend that may no longer exist. */
static void TeardownEngine(void)
{
    /* Parar e liberar o sound antes de desligar a engine. */
    if (g_audio.soundLoaded) ma_sound_uninit(&g_audio.sound);
    g_audio.soundLoaded = FALSE;
    g_audio.durationSeconds = 0;
    g_audio.currentPath[0] = 0;

    DetachVizTap();

    if(g_eqActive) {ma_node_uninit(&g_eqNode.base,nullptr);g_eqActive=FALSE;}

    ma_engine_uninit(&g_audio.engine);
    g_audio.active   = FALSE;
}

void Audio_Shutdown(void)
{
    if (g_audio.active)
    {
        ma_engine_stop(&g_audio.engine);
        TeardownEngine();
    }

    /* Encerrar visualização. */
    Audio_VizShutdown();

    /* An intentional shutdown is not an output failure to recover from. */
    g_audio.configured = FALSE;
    g_audio.restorePath[0] = 0;
    g_audio.lastRecovery = 0;
    g_audio.stalledChecks = g_audio.autoRecoveries = 0;
    g_audio.state    = AUDIO_STATE_STOPPED;
}

BOOL Audio_IsActive(void)
{
    return g_audio.active;
}

void Audio_Unload(void) {
    g_eq.RequestReset();
    g_audio.restorePath[0] = 0;
    if (g_audio.soundLoaded) ma_sound_uninit(&g_audio.sound);
    g_audio.soundLoaded = FALSE;
    g_audio.durationSeconds = 0;
    g_audio.currentPath[0] = 0;
    g_audio.fileSampleRate = 0;
    g_audio.fileSizeBytes = 0;
    g_audio.state = AUDIO_STATE_STOPPED;
}

static BOOL RecoverOutput(const wchar_t* reason);

/* ------------------------------------------------------------------ */
/* Carregar arquivo (M2-T2)                                             */
/* ------------------------------------------------------------------ */

BOOL Audio_Load(const wchar_t* path)
{
    ma_result result;

    if (path == NULL)
        return FALSE;

    /* A new track replaces whatever a failed recovery still had to reopen. */
    g_audio.restorePath[0] = L'\0';
    if (!g_audio.active && !RecoverOutput(L"load"))
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
/* Output recovery                                                      */
/*                                                                      */
/* The engine's device runs from startup to exit, even while nothing    */
/* plays. When its WASAPI endpoint disappears (Bluetooth headset        */
/* disconnected, driver reset, resume from sleep), the render call      */
/* fails and miniaudio's device loop ends. If that error wins the race  */
/* against the endpoint notifications, miniaudio never restarts the     */
/* device: the engine stays stopped while ma_sound_start still succeeds,*/
/* so the player reports PLAYING and renders nothing. Recovery runs     */
/* only for that observed state, on a play request or while playing.    */
/* ------------------------------------------------------------------ */

static DWORD g_clockOffset; /* Simulated elapsed time (tests only). */
static DWORD Now(void) { return GetTickCount() + g_clockOffset; }

static BOOL OutputRunning(void)
{
    if (!g_audio.active)
        return FALSE;
    ma_device* device = ma_engine_get_device(&g_audio.engine);
    /* No device: the offline engine of the tests reads frames directly. */
    return !device || ma_device_get_state(device) == ma_device_state_started;
}

/* Reopens the track a recreated engine lost, at its previous position. */
static BOOL ReopenTrack(void)
{
    if (!g_audio.restorePath[0])
        return TRUE;
    wchar_t path[MAX_PATH];
    wcsncpy_s(path, ARRAYSIZE(path), g_audio.restorePath, _TRUNCATE);
    const ma_uint64 frame = g_audio.restoreFrame;
    if (!Audio_Load(path)) /* Clears restorePath. */
    {
        AudioTrace(L"track reopen failed after output recovery");
        return FALSE;
    }
    ma_sound_seek_to_pcm_frame(&g_audio.sound, frame);
    return TRUE;
}

/* First, restart the existing device: miniaudio may already have rerouted
 * it to the new default endpoint. If it cannot start, or the output failed
 * again within 30 s of the previous recovery, the endpoint itself is gone:
 * recreate the engine on the current default device and reopen the track.
 * Volume, pan, EQ, visualization, playlist and selection are preserved. */
static BOOL RecoverOutput(const wchar_t* reason)
{
    if (!g_audio.configured)
        return FALSE; /* Never initialized, or shut down on purpose. */

    const DWORD now = Now();
    const BOOL repeated = g_audio.lastRecovery && now - g_audio.lastRecovery < 30000;
    g_audio.lastRecovery = now ? now : 1;

    if (g_audio.active)
    {
        ma_device* device = ma_engine_get_device(&g_audio.engine);
        AudioTrace(L"output not running (%ls): device state %d, transport %d, track %ls",
                   reason, device ? (int)ma_device_get_state(device) : -1, (int)g_audio.state,
                   g_audio.soundLoaded ? L"loaded" : L"none");
        if (device && !repeated)
        {
            ma_result result = ma_device_start(device);
            if (result == MA_SUCCESS)
            {
                AudioTrace(L"output restarted");
                return TRUE;
            }
            AudioTrace(L"output restart failed: %d", (int)result);
        }
        if (g_audio.soundLoaded)
        {
            wcsncpy_s(g_audio.restorePath, ARRAYSIZE(g_audio.restorePath), g_audio.currentPath, _TRUNCATE);
            g_audio.restoreFrame = 0;
            ma_sound_get_cursor_in_pcm_frames(&g_audio.sound, &g_audio.restoreFrame);
        }
        TeardownEngine();
    }
    else
        AudioTrace(L"no output engine (%ls)", reason);

    if (!InitEngine(&g_audio.config))
    {
        AudioTrace(L"output recreation failed; playback stays unavailable until the next attempt");
        return FALSE;
    }
    ma_engine_set_volume(&g_audio.engine, g_audio.volume);
    AttachVizTap();
    AudioTrace(L"output recreated: %u Hz, %u channels",
               ma_engine_get_sample_rate(&g_audio.engine), ma_engine_get_channels(&g_audio.engine));
    return ReopenTrack();
}

AudioOutputCheck Audio_CheckOutput(void)
{
    if (g_audio.state != AUDIO_STATE_PLAYING || OutputRunning())
    {
        g_audio.stalledChecks = 0;
        return AUDIO_OUTPUT_OK;
    }
    /* miniaudio reroutes a default-device change on its notification thread
     * by stopping, reinitializing and restarting the device: one stopped
     * observation may be that window. Two consecutive ones are not. */
    if (++g_audio.stalledChecks < 2)
        return AUDIO_OUTPUT_OK;
    g_audio.stalledChecks = 0;

    /* Bounded: Audio_Play resets the budget, so only a user command or a new
     * track can start another series of automatic attempts. */
    if (g_audio.autoRecoveries < 3)
    {
        ++g_audio.autoRecoveries;
        if (RecoverOutput(L"stalled while playing"))
        {
            /* A recreated engine reopens the track stopped. */
            if (g_audio.state != AUDIO_STATE_PLAYING && g_audio.soundLoaded &&
                ma_sound_start(&g_audio.sound) == MA_SUCCESS)
                g_audio.state = AUDIO_STATE_PLAYING;
            if (g_audio.state == AUDIO_STATE_PLAYING && OutputRunning())
                return AUDIO_OUTPUT_RECOVERED;
        }
    }

    /* The transport must not keep claiming playback; the next Play retries. */
    if (g_audio.soundLoaded)
        ma_sound_stop(&g_audio.sound);
    g_audio.state = (g_audio.soundLoaded || g_audio.restorePath[0]) ? AUDIO_STATE_PAUSED : AUDIO_STATE_STOPPED;
    AudioTrace(L"playback paused: no working output");
    return AUDIO_OUTPUT_FAILED;
}

#ifdef LITESIC_TEST
void Audio_TestAdvanceClock(unsigned ms) { g_clockOffset += ms; }
void Audio_TestStopOutput(void) { if (g_audio.active) ma_engine_stop(&g_audio.engine); }
#endif

/* ------------------------------------------------------------------ */
/* Reprodução (M2-T3)                                                   */
/* ------------------------------------------------------------------ */

void Audio_Play(void)
{
    /* A play request repairs an output that stopped while idle; nothing
     * polls a silent player. A lost track is reopened at its position. */
    if (!OutputRunning() || (!g_audio.soundLoaded && g_audio.restorePath[0]))
    {
        if (!RecoverOutput(L"play"))
            return;
    }

    if (!g_audio.soundLoaded || g_audio.state == AUDIO_STATE_PLAYING)
        return;

    ma_result result = ma_sound_start(&g_audio.sound);
    if (result == MA_SUCCESS)
    {
        g_audio.state = AUDIO_STATE_PLAYING;
        g_audio.autoRecoveries = 0;
    }
    else
        AudioTrace(L"sound start failed: %d", (int)result);
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
    g_audio.restoreFrame = 0; /* A lost track reopens from the start. */
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
    /* Clamp entre 0.0 e 1.0. */
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;

    /* Kept for an engine created later (recovery or a late first init). */
    g_audio.volume = volume;
    if (!g_audio.active)
        return;

    ma_engine_set_volume(&g_audio.engine, volume);
}

void Audio_SetPan(float pan)
{
    /* Clamp entre -1.0 e 1.0. */
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f)  pan =  1.0f;

    g_audio.pan = pan; /* Applied by Audio_Load, also after recovery. */
    if (g_audio.soundLoaded) ma_sound_set_pan(&g_audio.sound, pan);
}
