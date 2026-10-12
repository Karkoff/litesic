// Audio continuity on the real output device (performance audit).
// Captures the default render endpoint through WASAPI loopback while LiteSic plays a
// continuous sine, and reports 2 ms windows whose RMS drops below half the median of
// the active region (inserted silence = underrun/dropout). Other applications can only
// add energy to the mix, so they cannot create a false dropout.
// Usage: loopback-probe <seconds>  -> JSON summary on stdout.
// Build (MINGW64, after the release build):
//   g++ -std=c++17 -O2 -D_WIN32_WINNT=0x0601 -DUNICODE -D_UNICODE -Isrc -Ithird_party
//       scripts/diagnostics/loopback_probe.cpp build/internal/64/release/obj/miniaudio_impl.o
//       -static -lole32 -lwinmm -o build/perf/loopback-probe.exe
#include "ma_config.h"
#include "miniaudio.h"
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static std::vector<float> windows;
static volatile LONG count = 0;
static double sum = 0; static unsigned filled = 0, frameWindow = 96;
static void Data(ma_device* d, void*, const void* input, ma_uint32 frames) {
    const float* in = static_cast<const float*>(input); unsigned ch = d->capture.channels;
    for (ma_uint32 i = 0; i < frames; ++i) {
        float m = 0; for (unsigned c = 0; c < ch; ++c) m += in[i * ch + c]; m /= ch;
        sum += double(m) * m;
        if (++filled == frameWindow) {
            LONG n = count; if (size_t(n) < windows.size()) { windows[n] = float(std::sqrt(sum / frameWindow)); InterlockedIncrement(&count); }
            sum = 0; filled = 0;
        }
    }
}
int main(int argc, char** argv) {
    int seconds = argc > 1 ? std::atoi(argv[1]) : 60;
    ma_backend backends[] = {ma_backend_wasapi};
    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format = ma_format_f32; config.capture.channels = 2; config.sampleRate = 48000;
    config.dataCallback = Data;
    windows.assign(size_t(seconds) * 500 + 1000, 0.f);
    ma_device device;
    if (ma_device_init_ex(backends, 1, nullptr, &config, &device) != MA_SUCCESS) { std::printf("{\"error\":\"loopback init\"}\n"); return 1; }
    frameWindow = device.sampleRate / 500;
    ma_device_start(&device);
    std::fprintf(stderr, "capturing %d s at %u Hz\n", seconds, device.sampleRate);
    Sleep(DWORD(seconds) * 1000);
    ma_device_uninit(&device);
    int n = count; const float floor = 1e-4f;
    int first = -1, last = -1;
    for (int i = 0; i < n; ++i) if (windows[i] > floor) { if (first < 0) first = i; last = i; }
    if (first < 0) { std::printf("{\"error\":\"no signal\",\"windows\":%d}\n", n); return 1; }
    // Ignore the first and last 0.5 s of the active region (start/stop ramps).
    first += 250; last -= 250;
    std::vector<float> active(windows.begin() + first, windows.begin() + last + 1), sorted = active;
    std::sort(sorted.begin(), sorted.end()); float median = sorted[sorted.size() / 2];
    int dropWindows = 0, events = 0; bool in = false; std::vector<double> times;
    for (size_t i = 0; i < active.size(); ++i) {
        bool low = active[i] < median * 0.5f;
        if (low) { ++dropWindows; if (!in) { ++events; if (times.size() < 50) times.push_back((first + i) * 0.002); } }
        in = low;
    }
    std::printf("{\"seconds_requested\":%d,\"windows\":%d,\"active_seconds\":%.2f,\"median_rms\":%.6f,\"p001_rms\":%.6f,\"min_rms\":%.6f,\"dropout_windows_2ms\":%d,\"dropout_events\":%d,\"event_times_s\":[",
        seconds, n, active.size() * 0.002, median, sorted[sorted.size() / 1000], sorted.front(), dropWindows, events);
    for (size_t i = 0; i < times.size(); ++i) std::printf("%s%.3f", i ? "," : "", times[i]);
    std::printf("]}\n");
    return 0;
}
