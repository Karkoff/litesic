// Output-device lifecycle: a scripted miniaudio backend runs the real device
// worker thread, so a failed data loop leaves the engine exactly as a WASAPI
// AUDCLNT_E_DEVICE_INVALIDATED does (e.g. a Bluetooth headset disconnecting):
// the render loop ends, the stop call fails and no notification restarts it.
#include "playback.h"
#include "check.h"
#include "ma_config.h"
#include "../third_party/miniaudio.h"
#include <tlhelp32.h>
#include <string>

static struct {
    HANDLE wake;
    volatile LONG credit;     // Frames the device may still render: a simulated clock.
    volatile LONG failLoop;   // Next data loop iteration fails like a lost endpoint.
    volatile LONG failStart;  // Restarting the stale endpoint fails this many times.
    volatile LONG failInit;   // Opening a new endpoint fails this many times.
    volatile LONG lostStop;   // Stop after a lost endpoint fails, as on WASAPI.
    volatile LONG inits;
} fake;
static const ma_uint32 Period = 480;

static ma_result DeviceInit(ma_device*, const ma_device_config*, ma_device_descriptor* playback, ma_device_descriptor*) {
    if (fake.failInit > 0) { InterlockedDecrement(&fake.failInit); return MA_NO_DEVICE; }
    InterlockedIncrement(&fake.inits);
    playback->format = ma_format_f32; playback->channels = 2; playback->sampleRate = 48000;
    ma_channel_map_init_standard(ma_standard_channel_map_default, playback->channelMap, MA_MAX_CHANNELS, 2);
    playback->periodSizeInFrames = Period; playback->periodCount = 2;
    return MA_SUCCESS;
}
static ma_result DeviceStart(ma_device*) {
    if (fake.failStart > 0) { InterlockedDecrement(&fake.failStart); return MA_UNAVAILABLE; }
    return MA_SUCCESS;
}
static ma_result DeviceStop(ma_device*) { return InterlockedExchange(&fake.lostStop, 0) ? MA_UNAVAILABLE : MA_SUCCESS; }
static ma_result DeviceLoop(ma_device* device) {
    static float pcm[Period * 2];
    while (ma_device_get_state(device) == ma_device_state_started) {
        if (InterlockedExchange(&fake.failLoop, 0)) { fake.lostStop = 1; return MA_UNAVAILABLE; }
        if (fake.credit > 0) {
            ma_device_handle_backend_data_callback(device, pcm, nullptr, Period);
            InterlockedExchangeAdd(&fake.credit, -LONG(Period));
        } else WaitForSingleObject(fake.wake, 1);
    }
    return MA_SUCCESS;
}
static ma_result DeviceWake(ma_device*) { SetEvent(fake.wake); return MA_SUCCESS; }
static ma_result DeviceUninit(ma_device*) { return MA_SUCCESS; }
static ma_result Enumerate(ma_context* context, ma_enum_devices_callback_proc callback, void* user) {
    ma_device_info info{}; info.isDefault = MA_TRUE; lstrcpynA(info.name, "Scripted", sizeof(info.name));
    callback(context, ma_device_type_playback, &info, user); return MA_SUCCESS;
}
static ma_result Info(ma_context*, ma_device_type, const ma_device_id*, ma_device_info* info) {
    info->isDefault = MA_TRUE; return MA_SUCCESS;
}
static ma_result ContextUninit(ma_context*) { return MA_SUCCESS; }
static ma_result ContextInit(ma_context*, const ma_context_config*, ma_backend_callbacks* c) {
    c->onContextUninit = ContextUninit; c->onContextEnumerateDevices = Enumerate; c->onContextGetDeviceInfo = Info;
    c->onDeviceInit = DeviceInit; c->onDeviceUninit = DeviceUninit; c->onDeviceStart = DeviceStart;
    c->onDeviceStop = DeviceStop; c->onDeviceDataLoop = DeviceLoop; c->onDeviceDataLoopWakeup = DeviceWake;
    return MA_SUCCESS;
}
static ma_context context;

// Lets the device render `frames` (simulated time) and reports whether it did.
static bool Pump(LONG frames) {
    InterlockedExchange(&fake.credit, frames);
    const DWORD limit = 5000 + DWORD(frames / 48000) * 20;
    for (DWORD start = GetTickCount(); fake.credit > 0 && GetTickCount() - start < limit;) Sleep(1);
    bool consumed = fake.credit <= 0; InterlockedExchange(&fake.credit, 0); return consumed;
}
static void LoseEndpoint() {
    fake.failLoop = 1; SetEvent(fake.wake);
    for (int i = 0; i < 2000 && Audio_TestOutputStarted(); ++i) Sleep(1);
    CHECK(!Audio_TestOutputStarted());
}
static bool Advances() { double before = Audio_GetPositionSeconds(); return Pump(4800) && Audio_GetPositionSeconds() > before + .05; }
static PlaybackBackend Real() {
    return {Audio_Load, Audio_Play, Audio_Stop, Audio_Unload, Audio_SeekSeconds, Audio_GetPositionSeconds, Audio_GetState, Audio_IsAtEnd, Audio_IsActive};
}
static void Fresh() {
    Audio_Shutdown(); fake.credit = fake.failLoop = fake.failStart = fake.failInit = fake.lostStop = fake.inits = 0;
    CHECK(Audio_TestInitContext(&context)); CHECK(Audio_TestOutputStarted()); CHECK(fake.inits == 1);
    Audio_TestAdvanceClock(60000); // Independent cases: no recent recovery.
}
static const wchar_t* const T48 = L"tests\\fixtures\\native-48000.wav";
static const wchar_t* const T44 = L"tests\\fixtures\\native-44100.wav";

// Scenarios 1-5: inactivity alone (hours of device time) is harmless.
static void idle_hour_then_select_plays() {
    Fresh(); Playlist p; p.Add(T48); p.Add(T44); PlaybackController c(p, Real());
    CHECK(c.PlayIndex(0)); CHECK(Advances()); Audio_Stop();
    CHECK(Pump(48000 * 3600));  // One hour of device time rendering silence.
    CHECK(c.PlayIndex(1)); CHECK(Audio_GetState() == AUDIO_STATE_PLAYING); CHECK(Advances());
    Audio_Pause(); double at = Audio_GetPositionSeconds(); CHECK(Pump(48000 * 600));
    CHECK(c.Play()); CHECK(Audio_GetPositionSeconds() >= at - .001); CHECK(Advances());
    c.Clear();
}
static wchar_t logPath[MAX_PATH];
static std::string ReadLog() {
    std::string text; HANDLE f = CreateFileW(logPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return text;
    char buffer[4096]; DWORD n = 0;
    while (ReadFile(f, buffer, sizeof(buffer), &n, nullptr) && n) text.append(buffer, n);
    CloseHandle(f); return text;
}
// The incident: endpoint lost while idle (miniaudio neither rerouted nor restarted).
static void endpoint_lost_while_idle_then_select_plays() {
    Fresh(); Playlist p; p.Add(T48); p.Add(T44); PlaybackController c(p, Real());
    CHECK(c.PlayIndex(0)); CHECK(Advances()); Audio_Stop();
    CHECK(GetFileAttributesW(logPath) == INVALID_FILE_ATTRIBUTES); // Normal use writes nothing.
    LoseEndpoint();
    CHECK(c.PlayIndex(1)); CHECK(Audio_GetState() == AUDIO_STATE_PLAYING);
    CHECK(Advances());  // Before the fix: PLAYING reported, but nothing rendered.
    CHECK(fake.inits == 1); // The existing device restarted; no rebuild needed.
    std::string log = ReadLog();
    CHECK(log.find("output not running (play)") != std::string::npos); CHECK(log.find("output restarted") != std::string::npos);
    c.Clear();
}
// Scenario 4 with a stale endpoint that cannot restart: the engine is
// recreated and the paused track resumes at the same position.
static void paused_track_resumes_after_engine_rebuild() {
    Fresh(); CHECK(Audio_VizInit()); Audio_SetVolume(.5f);
    Playlist p; p.Add(T48); p.Add(L"tests\\eq_fixtures\\eq-tone-2.wav"); // 1 kHz, -34 dBFS
    PlaybackController c(p, Real());
    CHECK(c.PlayIndex(1)); CHECK(Pump(24000)); Audio_Pause(); double at = Audio_GetPositionSeconds(); CHECK(at > .4);
    fake.failStart = 1; LoseEndpoint();
    CHECK(c.Play()); CHECK(fake.inits == 2); CHECK(Audio_GetState() == AUDIO_STATE_PLAYING);
    CHECK(Audio_HasTrack()); CHECK(p.GetCurrentIndex() == 1);
    CHECK(Audio_GetPositionSeconds() > at - .01 && Audio_GetPositionSeconds() < at + .05);
    CHECK(Advances());
    float samples[512]; CHECK(Audio_GetVisualizationSamples(samples, 512) == 512);
    float peak = 0; for (float v : samples) peak = v > peak ? v : -v > peak ? -v : peak;
    CHECK(peak > .01f); // Visualization tap reattached to the new graph.
    c.Clear(); Audio_VizShutdown();
}
// Scenario 3 after a loss: STOPPED, then Play restarts the same track from 0.
static void stopped_track_plays_from_start_after_loss() {
    Fresh(); Playlist p; p.Add(T48); PlaybackController c(p, Real());
    CHECK(c.Play()); CHECK(Pump(9600)); Audio_Stop();
    fake.failStart = 1; LoseEndpoint();
    CHECK(c.Play()); CHECK(fake.inits == 2); CHECK(Audio_GetPositionSeconds() < .05); CHECK(Advances());
    c.Clear();
}
// Scenarios 6 and 9: lost while playing; the UI timer check recovers it.
static void lost_while_playing_recovers_from_timer_check() {
    Fresh(); Playlist p; p.Add(T48); PlaybackController c(p, Real());
    CHECK(c.Play()); CHECK(Advances());
    LoseEndpoint(); double at = Audio_GetPositionSeconds();
    CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_OK);  // One observation may be a reroute.
    CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_RECOVERED);
    CHECK(Audio_GetState() == AUDIO_STATE_PLAYING); CHECK(Audio_GetPositionSeconds() >= at - .001); CHECK(Advances());
    CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_OK);
    c.Clear();
}
// A restart that does not hold (fails again within 30 s) escalates to a rebuild.
static void repeated_loss_escalates_to_rebuild() {
    Fresh(); Playlist p; p.Add(T48); PlaybackController c(p, Real());
    CHECK(c.Play()); CHECK(Advances());
    LoseEndpoint(); Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_RECOVERED); CHECK(fake.inits == 1);
    Audio_TestAdvanceClock(5000);
    LoseEndpoint(); Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_RECOVERED); CHECK(fake.inits == 2);
    CHECK(Advances());
    Audio_TestAdvanceClock(31000);
    LoseEndpoint(); Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_RECOVERED); CHECK(fake.inits == 2);
    CHECK(Advances());
    c.Clear();
}
// No output at all: reported, bounded, nothing marked broken, and playback
// works again once a device exists, without restarting the application.
static void no_output_reported_bounded_and_retried() {
    Fresh(); Playlist p; p.Add(T48); p.Add(T44); p.Add(T48); PlaybackController c(p, Real());
    CHECK(c.Play()); CHECK(Advances());
    fake.failStart = fake.failInit = 1000; LoseEndpoint();
    Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_FAILED);
    CHECK(Audio_GetState() == AUDIO_STATE_PAUSED); CHECK(!Audio_IsActive());
    LONG before = fake.failInit;
    for (int i = 0; i < 20; ++i) CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_OK); // No retry loop.
    CHECK(fake.failInit == before);
    CHECK(!c.PlayIndex(1)); CHECK(Audio_GetState() != AUDIO_STATE_PLAYING);
    CHECK(!c.Next(false, true)); CHECK(before - fake.failInit <= 4); // One attempt per command.
    for (int i = 0; i < p.Count(); ++i) CHECK(!p.At(i)->failed);
    CHECK(p.Count() == 3);
    fake.failStart = fake.failInit = 0; // The device is back.
    CHECK(c.Play()); CHECK(Audio_GetState() == AUDIO_STATE_PLAYING); CHECK(Advances());
    c.Clear();
}
// A track lost by a failed rebuild is reopened at its position by the next Play.
static void failed_rebuild_keeps_track_for_next_play() {
    Fresh(); Playlist p; p.Add(T44); PlaybackController c(p, Real());
    CHECK(c.Play()); CHECK(Pump(24000)); double at = Audio_GetPositionSeconds();
    fake.failStart = 1; fake.failInit = 1; LoseEndpoint();
    Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_FAILED); CHECK(!Audio_HasTrack());
    CHECK(c.Play()); CHECK(Audio_HasTrack()); CHECK(Audio_GetPositionSeconds() > at - .01); CHECK(Advances());
    c.Clear();
}
// Automatic attempts are budgeted per play command.
static void automatic_recovery_budget_is_bounded() {
    Fresh(); Playlist p; p.Add(T48); PlaybackController c(p, Real());
    CHECK(c.Play());
    for (int i = 0; i < 3; ++i) { LoseEndpoint(); Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_RECOVERED); }
    LoseEndpoint(); Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_FAILED);
    CHECK(Audio_GetState() == AUDIO_STATE_PAUSED);
    CHECK(c.Play()); CHECK(Advances()); // A user command starts a new budget.
    LoseEndpoint(); Audio_CheckOutput(); CHECK(Audio_CheckOutput() == AUDIO_OUTPUT_RECOVERED);
    c.Clear();
}
// Scenario 7: load/decode errors are file failures; the output stays usable.
static void load_and_decode_errors_keep_output() {
    Fresh(); Playlist p; p.Add(L"tests\\fixtures\\missing.wav"); p.Add(L"README.md"); p.Add(T48);
    PlaybackController c(p, Real());
    CHECK(!c.PlayIndex(0)); CHECK(p.At(0)->failed); CHECK(Audio_IsActive()); CHECK(Audio_TestOutputStarted());
    CHECK(!c.PlayIndex(1)); CHECK(p.At(1)->failed);
    CHECK(c.Next(false, false)); CHECK(p.GetCurrentIndex() == 2); CHECK(Advances()); CHECK(fake.inits == 1);
    c.Clear();
}
static DWORD Threads() {
    DWORD count = 0; HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); THREADENTRY32 t{}; t.dwSize = sizeof(t);
    for (BOOL ok = Thread32First(snap, &t); ok; ok = Thread32Next(snap, &t)) count += t.th32OwnerProcessID == GetCurrentProcessId();
    CloseHandle(snap); return count;
}
// Scenarios 8-10: many cycles with losses, restarts and rebuilds; the device
// worker keeps running and handles/threads do not grow.
static void cycles_with_losses_do_not_leak() {
    Fresh(); CHECK(Audio_VizInit()); Playlist p; p.Add(T48); p.Add(T44); PlaybackController c(p, Real());
    DWORD handles0 = 0, threads0 = 0;
    for (int i = 0; i < 60; ++i) {
        if (i == 10) { GetProcessHandleCount(GetCurrentProcess(), &handles0); threads0 = Threads(); }
        CHECK(c.PlayIndex(i % 2)); CHECK(Pump(960));
        Audio_Pause(); CHECK(c.Play()); CHECK(Pump(960)); Audio_Stop();
        if (i % 3 == 0) { Audio_TestAdvanceClock(31000); fake.failStart = i % 2; LoseEndpoint(); }
    }
    CHECK(c.Play()); CHECK(Advances());
    DWORD handles1 = 0; GetProcessHandleCount(GetCurrentProcess(), &handles1); DWORD threads1 = Threads();
    std::printf("  handles %lu -> %lu, threads %lu -> %lu, device inits %ld\n", handles0, handles1, threads0, threads1, fake.inits);
    CHECK(handles1 <= handles0 + 4); CHECK(threads1 <= threads0 + 1); CHECK(fake.inits > 5);
    c.Clear(); Audio_VizShutdown();
}

int main() {
    fake.wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    ma_context_config config = ma_context_config_init(); config.custom.onContextInit = ContextInit;
    ma_backend backend = ma_backend_custom;
    CHECK(ma_context_init(&backend, 1, &config, &context) == MA_SUCCESS);
    GetTempPathW(MAX_PATH, logPath); lstrcatW(logPath, L"litesic-audio-recovery-test.log");
    DeleteFileW(logPath); Audio_SetLogPath(logPath);
    int cases = 0;
    CASE(idle_hour_then_select_plays); CASE(endpoint_lost_while_idle_then_select_plays);
    CASE(paused_track_resumes_after_engine_rebuild); CASE(stopped_track_plays_from_start_after_loss);
    CASE(lost_while_playing_recovers_from_timer_check); CASE(repeated_loss_escalates_to_rebuild);
    CASE(no_output_reported_bounded_and_retried); CASE(failed_rebuild_keeps_track_for_next_play);
    CASE(automatic_recovery_budget_is_bounded); CASE(load_and_decode_errors_keep_output);
    CASE(cycles_with_losses_do_not_leak);
    std::printf("%s", ReadLog().substr(0, 1200).c_str()); // Sample of the diagnostic records.
    Audio_Shutdown(); ma_context_uninit(&context); CloseHandle(fake.wake); DeleteFileW(logPath);
    std::printf("%d recovery cases\n", cases);
}
