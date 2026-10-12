// Pre-distribution performance audit probe. Links the production objects (with
// the offline audio test entry points, as eq_probe/memory_probe do). No user
// profile is opened: ThemeConfigure(nullptr) and null session paths only, except
// mode "session", which uses an explicit directory under build/perf.
// Output: one JSON object per line on stdout.
#include "audio.h"
#include "eq_dsp.h"
#include "eq_state.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "ui_theme.h"
#include "ui_draw.h"
#include "ui_player.h"
#include "ui_tooltip.h"
#include "visualizer.h"
#include "playback.h"
#include "tags.h"
#include "m3u.h"
#include "i18n.h"
#include "language_selection.h"
#include "skin.h"
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {
void Require(bool ok, const char* what) { if (!ok) { std::fprintf(stderr, "FAIL %s\n", what); std::fflush(stderr); ExitProcess(2); } }
double Ms() { static LARGE_INTEGER f{}; if (!f.QuadPart) QueryPerformanceFrequency(&f); LARGE_INTEGER t; QueryPerformanceCounter(&t); return double(t.QuadPart) * 1000.0 / double(f.QuadPart); }
unsigned long long Cpu100ns() {
    FILETIME c, e, k, u; GetProcessTimes(GetCurrentProcess(), &c, &e, &k, &u);
    ULARGE_INTEGER a{}, b{}; a.LowPart = k.dwLowDateTime; a.HighPart = k.dwHighDateTime; b.LowPart = u.dwLowDateTime; b.HighPart = u.dwHighDateTime;
    return a.QuadPart + b.QuadPart;
}
unsigned Threads() {
    unsigned count = 0; HANDLE s = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); if (s == INVALID_HANDLE_VALUE) return 0;
    THREADENTRY32 e{}; e.dwSize = sizeof(e);
    if (Thread32First(s, &e)) do { if (e.th32OwnerProcessID == GetCurrentProcessId()) ++count; } while (Thread32Next(s, &e));
    CloseHandle(s); return count;
}
// Copy-on-write pages of DLLs injected by third-party tools (e.g. an overlay hook)
// are charged as private bytes. Count committed image pages outside Windows and this
// executable so results can be reported with and without that machine-specific charge.
unsigned long long ForeignImageCommit() {
    wchar_t self[MAX_PATH]{}, windows[MAX_PATH]{}; GetModuleFileNameW(nullptr, self, MAX_PATH); GetWindowsDirectoryW(windows, MAX_PATH);
    unsigned long long total = 0; MEMORY_BASIC_INFORMATION m; char* a = nullptr;
    while (VirtualQuery(a, &m, sizeof(m))) {
        if (m.State == MEM_COMMIT && m.Type == MEM_IMAGE && (m.Protect & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY))) {
            wchar_t name[MAX_PATH]{};
            if (GetModuleFileNameW(static_cast<HMODULE>(m.AllocationBase), name, MAX_PATH) && _wcsicmp(name, self) &&
                _wcsnicmp(name, windows, wcslen(windows))) total += m.RegionSize;
        }
        a = static_cast<char*>(m.BaseAddress) + m.RegionSize;
    }
    return total;
}
struct Mem { unsigned long long ws = 0, priv = 0, foreign = 0; unsigned long handles = 0, gdi = 0, user = 0, threads = 0; };
Mem Snap() {
    Mem m; m.foreign = ForeignImageCommit(); PROCESS_MEMORY_COUNTERS_EX p{}; p.cb = sizeof(p);
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&p), sizeof(p));
    m.ws = p.WorkingSetSize; m.priv = p.PrivateUsage; DWORD h = 0; GetProcessHandleCount(GetCurrentProcess(), &h); m.handles = h;
    m.gdi = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS); m.user = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS); m.threads = Threads();
    return m;
}
void Print(const char* mode, const char* stage, const Mem& m, const char* extra = "") {
    std::printf("{\"mode\":\"%s\",\"stage\":\"%s\",\"arch\":%u,\"working_set\":%llu,\"private_bytes\":%llu,\"foreign_image_commit\":%llu,\"handles\":%lu,\"gdi\":%lu,\"user\":%lu,\"threads\":%lu%s%s}\n",
        mode, stage, unsigned(sizeof(void*) * 8), m.ws, m.priv, m.foreign, m.handles, m.gdi, m.user, m.threads, *extra ? "," : "", extra);
    std::fflush(stdout);
}
struct Stats { double mean = 0, p50 = 0, p95 = 0, p99 = 0, max = 0; size_t n = 0; };
Stats Summ(std::vector<double> v) {
    Stats s; if (v.empty()) return s; std::sort(v.begin(), v.end()); double sum = 0; for (double x : v) sum += x;
    s.n = v.size(); s.mean = sum / v.size(); s.p50 = v[v.size() / 2]; s.p95 = v[std::min(v.size() - 1, v.size() * 95 / 100)];
    s.p99 = v[std::min(v.size() - 1, v.size() * 99 / 100)]; s.max = v.back(); return s;
}
std::string StatJson(const char* prefix, const Stats& s) {
    char b[400]; std::snprintf(b, sizeof(b), "\"%s_n\":%zu,\"%s_mean\":%.4f,\"%s_p50\":%.4f,\"%s_p95\":%.4f,\"%s_p99\":%.4f,\"%s_max\":%.4f",
        prefix, s.n, prefix, s.mean, prefix, s.p50, prefix, s.p95, prefix, s.p99, prefix, s.max);
    return b;
}
// Pump with the longest single dispatch: the UI-thread blocking time seen by input/paint.
double Pump(double* longest = nullptr) {
    MSG m; double worst = 0, total = 0;
    while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) {
        double a = Ms(); TranslateMessage(&m); DispatchMessageW(&m); double d = Ms() - a; total += d; worst = std::max(worst, d);
    }
    if (longest) *longest = std::max(*longest, worst);
    return total;
}
HWND mainWindow, playlistWindow, eqWindow;
BOOL CALLBACK FindWindows(HWND h, LPARAM) {
    wchar_t c[80]{}; GetClassNameW(h, c, 80);
    if (!wcscmp(c, L"LiteSic.PlaylistWnd")) playlistWindow = h;
    if (!wcscmp(c, L"LiteSic.EqualizerWnd")) eqWindow = h;
    return TRUE;
}
void Redraw(HWND h) { RedrawWindow(h, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN); }
std::wstring Wide(const char* s) { std::wstring w; while (*s) w.push_back(wchar_t(static_cast<unsigned char>(*s++))); return w; }
std::wstring Full(const std::wstring& p) { return FullPath(p); }

// Creates the three production windows in the isolated configuration.
void CreateWindows(bool withPlaylist = true, bool withEq = false) {
    Require(MainWnd_RegisterClass(GetModuleHandleW(nullptr)), "register");
    mainWindow = MainWnd_Create(GetModuleHandleW(nullptr), SW_SHOWNOACTIVATE); Require(mainWindow != nullptr, "main");
    if (withPlaylist) { Require(PlWnd_Create(GetModuleHandleW(nullptr), mainWindow), "playlist"); }
    if (withEq) EqWnd_Toggle(mainWindow);
    EnumThreadWindows(GetCurrentThreadId(), FindWindows, 0);
    Pump();
}
void ClickPlayer(int id) {
    auto r = PlayerControlRect(id); LPARAM p = MAKELPARAM(MainWnd_DPIScale(int(r.x + r.w / 2)), MainWnd_DPIScale(int(r.y + r.h / 2)));
    SendMessageW(mainWindow, WM_LBUTTONDOWN, MK_LBUTTON, p); SendMessageW(mainWindow, WM_LBUTTONUP, 0, p);
}

// ---------------------------------------------------------------- DSP
void ModeDsp() {
    struct Config { const char* name; bool enabled; int kind; };
    const Config configs[] = {{"disabled", false, 0}, {"enabled_flat", true, 0}, {"one_band", true, 1},
                              {"rock_preset", true, 2}, {"all_bands_extreme", true, 3}, {"transition_every_block", true, 4}};
    const unsigned rates[] = {44100, 48000, 96000}; const unsigned blocks[] = {480, 1024};
    std::vector<float> in(2 * 4096), out(2 * 4096); unsigned seed = 12345;
    for (auto& x : in) { seed = seed * 1664525u + 1013904223u; x = (int(seed >> 9) % 20000 - 10000) / 20000.f; }
    for (unsigned rate : rates) for (unsigned block : blocks) for (const auto& c : configs) {
        static Eq::Processor p; Eq::Parameters params; params.enabled = c.enabled;
        if (c.kind == 1) params.gains[4] = 6;
        if (c.kind == 2 || c.kind == 4) { const auto& r = Eq::PresetAt(1); params.preamp = r.preamp; for (unsigned b = 0; b < Eq::Bands; ++b) params.gains[b] = r.gains[b]; }
        if (c.kind == 3) { params.preamp = -12; for (unsigned b = 0; b < Eq::Bands; ++b) params.gains[b] = b % 2 ? 12.f : -12.f; }
        p.Initialize(rate, 2, params);
        std::vector<double> t; const int iterations = 3000;
        for (int i = 0; i < iterations; ++i) {
            if (c.kind == 4) { params.gains[i % Eq::Bands] = float(i % 25 - 12); p.Publish(params); }
            double a = Ms(); p.Process(out.data(), in.data(), block); double d = Ms() - a;
            if (i >= 200) t.push_back(d * 1000.0);
        }
        auto s = Summ(t); double budgetUs = block * 1e6 / rate;
        std::printf("{\"mode\":\"dsp\",\"arch\":%u,\"config\":\"%s\",\"rate\":%u,\"block\":%u,\"channels\":2,%s,\"ns_per_frame\":%.3f,\"budget_us\":%.2f,\"budget_pct_mean\":%.4f,\"budget_pct_max\":%.4f}\n",
            unsigned(sizeof(void*) * 8), c.name, rate, block, StatJson("us", s).c_str(), s.mean * 1000.0 / block, budgetUs, s.mean / budgetUs * 100, s.max / budgetUs * 100);
        std::fflush(stdout);
    }
}

// ---------------------------------------------------------------- offline graph (decode+EQ+tap)
// pace > 0 consumes each 10 ms block when due at pace x real time, as a device would.
void ModeGraph(const wchar_t* file, int seconds, int pace) {
    Ui::ThemeConfigure(nullptr);
    for (int variant = 0; variant < 3; ++variant) {
        Eq::ApplyPreset(variant == 2 ? 1 : 0); Eq::SetEnabled(variant >= 1);
        Require(Audio_TestInit(48000), "init"); Require(Audio_VizInit(), "viz");
        Mem before = Snap(); double l0 = Ms(); Require(Audio_Load(file), "load"); double loadMs = Ms() - l0; Mem after = Snap();
        Audio_Play(); float pcm[2 * 480]; std::vector<double> t;
        const int blocks = seconds * 100;
        int silent = 0; double energy = 0, due = Ms();
        if (pace) timeBeginPeriod(1);
        for (int i = 0; i < blocks; ++i) {
            if (pace) { due += 10.0 / pace; while (Ms() < due) Sleep(due - Ms() > 2 ? 1 : 0); }
            double a = Ms(); Require(Audio_TestRead(pcm, 480), "read"); t.push_back((Ms() - a) * 1000.0);
            float peak = 0; for (float x : pcm) { peak = std::max(peak, std::fabs(x)); energy += double(x) * x; }
            if (peak < 1e-5f) ++silent;
        }
        if (pace) timeEndPeriod(1);
        double cursor = double(Audio_TestCursor()) / 48000.0, rms = std::sqrt(energy / (blocks * 960.0));
        auto s = Summ(t);
        const char* names[] = {"eq_off", "eq_flat", "eq_rock"};
        char name[600]; WideCharToMultiByte(CP_UTF8, 0, file, -1, name, sizeof(name), nullptr, nullptr);
        for (char* c = name; *c; ++c) if (*c == '\\') *c = '/';
        std::printf("{\"mode\":\"graph\",\"arch\":%u,\"file\":\"%s\",\"variant\":\"%s\",\"load_ms\":%.3f,\"load_private_delta\":%lld,\"load_ws_delta\":%lld,%s,\"budget_us\":10000,\"budget_pct_mean\":%.4f,\"budget_pct_p99\":%.4f,\"budget_pct_max\":%.4f,\"pace\":%d,\"silent_blocks\":%d,\"rms\":%.5f,\"cursor_s\":%.3f}\n",
            unsigned(sizeof(void*) * 8), name, names[variant], loadMs, (long long)(after.priv - before.priv), (long long)(after.ws - before.ws), StatJson("us", s).c_str(), s.mean / 100, s.p99 / 100, s.max / 100,
            pace, silent, rms, cursor);
        std::fflush(stdout);
        Audio_Shutdown();
    }
}

// ---------------------------------------------------------------- metadata worker cost per file
void ModeMeta(int argc, char** argv) {
    for (int i = 2; i < argc; ++i) {
        std::wstring path = Full(Wide(argv[i])); std::vector<double> t; TrackMetadata m;
        for (int k = 0; k < 7; ++k) { double a = Ms(); m = ReadTrackMetadata(path); double d = Ms() - a; if (k) t.push_back(d); }
        auto s = Summ(t); std::string name = argv[i]; std::replace(name.begin(), name.end(), '\\', '/');
        std::printf("{\"mode\":\"meta\",\"arch\":%u,\"file\":\"%s\",\"bytes\":%llu,\"duration_s\":%.2f,%s}\n", unsigned(sizeof(void*) * 8), name.c_str(),
            (unsigned long long)m.fileBytes, m.duration, StatJson("ms", s).c_str());
        std::fflush(stdout);
    }
}

// ---------------------------------------------------------------- startup cost components
void ModeStartupParts() {
    double a = Ms(); CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); double com = Ms() - a;
    a = Ms(); Ui::ThemeConfigure(nullptr); I18n::InitializeLanguage(nullptr, [] { return std::vector<std::string>{"en-US"}; }); double ui = Ms() - a;
    HDC screen = GetDC(nullptr); HDC dc = CreateCompatibleDC(screen); HBITMAP b = CreateCompatibleBitmap(screen, 690, 435); SelectObject(dc, b);
    RECT rect{0, 0, 690, 435}; Ui::Canvas c;
    a = Ms(); Require(c.Begin(dc, rect, 1.5f), "begin"); c.Fill({0, 0, 460, 290}, Ui::Background); c.TextAt({10, 10, 300, 24}, L"LiteSic", Ui::Text); Require(c.End(), "end"); double first = Ms() - a;
    a = Ms(); Require(c.Begin(dc, rect, 1.5f), "begin"); c.Fill({0, 0, 460, 290}, Ui::Background); c.TextAt({10, 10, 300, 24}, L"LiteSic", Ui::Text); Require(c.End(), "end"); double second = Ms() - a;
    a = Ms(); BOOL audio = Audio_Init(); double audioMs = Ms() - a;
    a = Ms(); Audio_VizInit(); double viz = Ms() - a;
    a = Ms(); Audio_Shutdown(); double shutdown = Ms() - a;
    std::printf("{\"mode\":\"startup_parts\",\"arch\":%u,\"coinit_ms\":%.3f,\"theme_language_ms\":%.3f,\"first_d2d_frame_ms\":%.3f,\"second_d2d_frame_ms\":%.3f,\"audio_init_ms\":%.3f,\"audio_ok\":%d,\"viz_init_ms\":%.3f,\"audio_shutdown_ms\":%.3f}\n",
        unsigned(sizeof(void*) * 8), com, ui, first, second, audioMs, audio, viz, shutdown);
    c.Reset(); DeleteDC(dc); DeleteObject(b); ReleaseDC(nullptr, screen);
}

// ---------------------------------------------------------------- memory by startup stage (app order)
void ModeMemoryStages(const wchar_t* track) {
    Print("memory_stages", "process", Snap());
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Print("memory_stages", "com_sta", Snap());
    Ui::ThemeConfigure(nullptr); I18n::InitializeLanguage(nullptr, [] { return std::vector<std::string>{"en-US"}; });
    Print("memory_stages", "theme_language", Snap());
    Require(MainWnd_RegisterClass(GetModuleHandleW(nullptr)), "register");
    mainWindow = MainWnd_Create(GetModuleHandleW(nullptr), SW_SHOWNOACTIVATE); Require(mainWindow != nullptr, "main"); Pump();
    Print("memory_stages", "player_window_audio_device_d2d", Snap());
    Require(PlWnd_Create(GetModuleHandleW(nullptr), mainWindow), "playlist"); Pump();
    Print("memory_stages", "playlist_window", Snap());
    EqWnd_Toggle(mainWindow); Pump(); Print("memory_stages", "equalizer_window", Snap());
    Audio_SetVolume(0.05f); PlayerPlaylist().Add(track); Require(PlayerPlayback().PlayIndex(0), "play"); PlWnd_Refresh();
    for (int i = 0; i < 100; ++i) { SendMessageW(mainWindow, WM_TIMER, VIZ_TIMER_ID, 0); Pump(); Sleep(10); }
    Print("memory_stages", "playing_1s_device", Snap());
    DestroyWindow(mainWindow); Pump(); Print("memory_stages", "windows_destroyed", Snap());
}

// ---------------------------------------------------------------- Direct2D resources per canvas
void ModeCanvas() {
    Ui::ThemeConfigure(nullptr);
    Print("canvas", "process", Snap());
    HDC screen = GetDC(nullptr); std::vector<HDC> dcs; std::vector<HBITMAP> bitmaps; std::vector<Ui::Canvas*> canvases;
    RECT rect{0, 0, 690, 435};
    for (int i = 1; i <= 6; ++i) {
        HDC dc = CreateCompatibleDC(screen); HBITMAP b = CreateCompatibleBitmap(screen, 690, 435); SelectObject(dc, b);
        auto c = new Ui::Canvas; dcs.push_back(dc); bitmaps.push_back(b); canvases.push_back(c);
        for (int f = 0; f < 30; ++f) {
            Require(c->Begin(dc, rect, 1.5f), "begin");
            c->Fill({0, 0, 460, 290}, Ui::Background); c->TextAt({10, 10, 300, 24}, L"LiteSic — 音楽 العربية", Ui::Text, Ui::Font::Heading);
            Require(c->End(), "end");
        }
        char stage[32]; std::snprintf(stage, sizeof(stage), "canvases_%d", i); Print("canvas", stage, Snap());
    }
    for (auto c : canvases) delete c;
    Print("canvas", "canvases_deleted", Snap());
    for (size_t i = 0; i < dcs.size(); ++i) { DeleteDC(dcs[i]); DeleteObject(bitmaps[i]); }
    ReleaseDC(nullptr, screen);
}

// ---------------------------------------------------------------- paint cost of real windows
void TimePaint(const char* label, HWND h, int frames, const char* extra = "") {
    std::vector<double> t; auto c0 = Cpu100ns(); double w0 = Ms();
    for (int i = 0; i < frames; ++i) { InvalidateRect(h, nullptr, FALSE); double a = Ms(); UpdateWindow(h); t.push_back(Ms() - a); }
    double wall = Ms() - w0; auto cpu = (Cpu100ns() - c0) / 10000.0;
    auto s = Summ(t);
    std::printf("{\"mode\":\"paint\",\"arch\":%u,\"case\":\"%s\",%s,\"cpu_ms_per_frame\":%.4f,\"wall_ms\":%.1f%s%s}\n", unsigned(sizeof(void*) * 8), label,
        StatJson("ms", s).c_str(), cpu / frames, wall, *extra ? "," : "", extra);
    std::fflush(stdout);
}
void TimeVizRect(const char* label, int frames) {
    std::vector<double> t; auto c0 = Cpu100ns(); float pcm[960];
    for (int i = 0; i < frames; ++i) {
        Audio_TestRead(pcm, 480); Audio_TestRead(pcm, 480); Audio_TestRead(pcm, 480);
        SendMessageW(mainWindow, WM_TIMER, VIZ_TIMER_ID, 0); double a = Ms(); UpdateWindow(mainWindow); t.push_back(Ms() - a);
    }
    auto cpu = (Cpu100ns() - c0) / 10000.0; auto s = Summ(t);
    std::printf("{\"mode\":\"paint\",\"arch\":%u,\"case\":\"%s\",%s,\"cpu_ms_per_frame_incl_offline_audio\":%.4f}\n", unsigned(sizeof(void*) * 8), label, StatJson("ms", s).c_str(), cpu / frames);
    std::fflush(stdout);
}
void ModePaint(const wchar_t* track, const wchar_t* folder) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Ui::ThemeConfigure(nullptr);
    Print("paint", "process", Snap());
    CreateWindows(true, true);
    Audio_Shutdown(); Require(Audio_TestInit(48000), "init"); Require(Audio_VizInit(), "viz");
    Print("paint", "windows_created", Snap());
    TimePaint("player_idle_no_track", mainWindow, 300);
    TimePaint("playlist_empty", playlistWindow, 300);
    TimePaint("equalizer", eqWindow, 300);
    PlayerPlaylist().Add(track); Require(PlayerPlayback().PlayIndex(0), "play"); PlWnd_Refresh(); Pump();
    float pcm[960]; for (int i = 0; i < 20; ++i) Audio_TestRead(pcm, 480);
    TimePaint("player_playing_osc_full", mainWindow, 300);
    TimeVizRect("player_playing_osc_viz_rect", 300);
    ClickPlayer(13); Pump();
    TimePaint("player_playing_fft_full", mainWindow, 300);
    TimeVizRect("player_playing_fft_viz_rect", 300);
    ClickPlayer(16); Pump();
    TimePaint("player_playing_viz_hidden_full", mainWindow, 300);
    ClickPlayer(16); Pump();
    Print("paint", "after_paint_cases", Snap());
    if (folder && *folder) {
        PlWnd_AppendPaths({folder});
        double longest = 0; double t0 = Ms();
        while (Ms() - t0 < 300000) {
            Pump(&longest); bool pending = false;
            for (const auto& t : PlayerPlaylist().Tracks()) if (t.metadataPending) { pending = true; break; }
            if (PlayerPlaylist().Count() > 1 && !pending) break;
            Sleep(5);
        }
        Pump();
        char extra[64]; std::snprintf(extra, sizeof(extra), "\"tracks\":%d", PlayerPlaylist().Count());
        TimePaint("playlist_loaded_top", playlistWindow, 300, extra);
        SendMessageW(playlistWindow, WM_KEYDOWN, VK_END, 0); Pump();
        TimePaint("playlist_loaded_end", playlistWindow, 300, extra);
        TimePaint("player_with_large_playlist", mainWindow, 300, extra);
    }
    Print("paint", "final", Snap());
    DestroyWindow(mainWindow); Pump(); Audio_Shutdown();
}

// ---------------------------------------------------------------- playlist scale
int Pending() { int n = 0; for (const auto& t : PlayerPlaylist().Tracks()) if (t.metadataPending) ++n; return n; }
void Op(const char* label, int tracks, double ms, double longest = -1, const char* extra = "") {
    std::printf("{\"mode\":\"playlist\",\"arch\":%u,\"tracks\":%d,\"op\":\"%s\",\"ms\":%.3f,\"longest_dispatch_ms\":%.3f%s%s}\n", unsigned(sizeof(void*) * 8),
        tracks, label, ms, longest, *extra ? "," : "", extra);
    std::fflush(stdout);
}
void ModePlaylist(const wchar_t* folder, int expected) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Ui::ThemeConfigure(nullptr);
    CreateWindows(true, false);
    Audio_Shutdown(); Require(Audio_TestInit(48000), "init"); Require(Audio_VizInit(), "viz");
    Mem m0 = Snap(); auto c0 = Cpu100ns();
    double longest = 0, t0 = Ms(); Require(PlWnd_AppendPaths({folder}), "append");
    double scanned = -1;
    while (Ms() - t0 < 600000) {
        Pump(&longest); int count = PlayerPlaylist().Count();
        if (count == expected && scanned < 0) scanned = Ms() - t0;
        if (count == expected && !Pending()) break;
        Sleep(2);
    }
    double total = Ms() - t0; Pump(&longest); Mem m1 = Snap(); auto cpu = (Cpu100ns() - c0) / 10000.0;
    Require(PlayerPlaylist().Count() == expected, "count");
    char extra[300]; std::snprintf(extra, sizeof(extra), "\"scan_complete_ms\":%.1f,\"cpu_ms\":%.1f,\"private_delta\":%lld,\"ws_delta\":%lld,\"private_after\":%llu",
        scanned, cpu, (long long)(m1.priv - m0.priv), (long long)(m1.ws - m0.ws), m1.priv);
    Op("import_folder_until_metadata_done", expected, total, longest, extra);
    auto timed = [&](const char* label, auto fn) { double lg = 0; double a = Ms(); fn(); double d = Ms() - a; Pump(&lg); Op(label, expected, d, lg); };
    timed("refresh", [] { PlWnd_Refresh(); });
    timed("paint", [] { Redraw(playlistWindow); });
    timed("scroll_wheel_x10", [] { for (int i = 0; i < 10; ++i) SendMessageW(playlistWindow, WM_MOUSEWHEEL, MAKEWPARAM(0, -WHEEL_DELTA), 0); Redraw(playlistWindow); });
    timed("key_end", [] { SendMessageW(playlistWindow, WM_KEYDOWN, VK_END, 0); Redraw(playlistWindow); });
    HWND search = GetDlgItem(playlistWindow, 520);
    timed("search_keystroke_match_few", [&] { SetWindowTextW(search, L"00042"); Redraw(playlistWindow); });
    timed("search_keystroke_match_all", [&] { SetWindowTextW(search, L"faixa"); Redraw(playlistWindow); });
    timed("search_clear", [&] { SetWindowTextW(search, L""); Redraw(playlistWindow); });
    timed("sort_name_desc", [] { SendMessageW(playlistWindow, WM_COMMAND, 507, 0); Redraw(playlistWindow); });
    timed("sort_duration_asc", [] { SendMessageW(playlistWindow, WM_COMMAND, 508, 0); Redraw(playlistWindow); });
    timed("mix", [] { SendMessageW(playlistWindow, WM_COMMAND, 510, 0); Redraw(playlistWindow); });
    timed("play_index_middle", [&] { PlayerPlayback().PlayIndex(expected / 2); PlWnd_Refresh(true); Redraw(playlistWindow); Redraw(mainWindow); });
    timed("player_paint_with_list", [] { Redraw(mainWindow); });
    timed("next_track", [] { SendMessageW(mainWindow, WM_APP + 97, 2, 0); Redraw(mainWindow); });
    timed("save_m3u", [&] { std::wstring e; Require(SaveM3U(Full(L"build\\perf\\probe-save.m3u8"), PlayerPlaylist().Tracks(), e), "save"); });
    timed("load_m3u_parse", [&] { std::vector<Track> t; std::wstring e; Require(LoadM3U(Full(L"build\\perf\\probe-save.m3u8"), t, e), "load"); });
    timed("select_all_ctrl_a", [] {
        BYTE keys[256]{}; GetKeyboardState(keys); keys[VK_CONTROL] = 0x80; SetKeyboardState(keys);
        SendMessageW(playlistWindow, WM_KEYDOWN, 'A', 0); keys[VK_CONTROL] = 0; SetKeyboardState(keys); Redraw(playlistWindow); });
    Mem m2 = Snap();
    timed("remove_all_selected_delete", [] { SendMessageW(playlistWindow, WM_KEYDOWN, VK_DELETE, 0); Redraw(playlistWindow); });
    Mem m3 = Snap();
    char tail[200]; std::snprintf(tail, sizeof(tail), "\"private_before_delete\":%llu,\"private_after_delete\":%llu,\"remaining\":%d", m2.priv, m3.priv, PlayerPlaylist().Count());
    Op("summary", expected, 0, -1, tail);
    DestroyWindow(mainWindow); Pump(); Audio_Shutdown();
}

// ---------------------------------------------------------------- session startup/shutdown with N tracks
void ModeSession(const wchar_t* profile, const wchar_t* folder, int expected) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::wstring dir = Full(profile); CreateDirectoryW(dir.c_str(), nullptr);
    std::wstring ini = dir + L"\\settings.ini", session = dir + L"\\playlist.m3u8";
    Ui::ThemeConfigure(ini.c_str());
    Require(MainWnd_RegisterClass(GetModuleHandleW(nullptr)), "register");
    double a = Ms(); mainWindow = MainWnd_Create(GetModuleHandleW(nullptr), SW_SHOWNOACTIVATE); double mainMs = Ms() - a;
    a = Ms(); Require(PlWnd_Create(GetModuleHandleW(nullptr), mainWindow, session.c_str()), "playlist"); double listMs = Ms() - a;
    EqWnd_Restore(mainWindow);
    double longest = 0; double t0 = Ms(); Pump(&longest);
    int count = PlayerPlaylist().Count();
    if (count == 0 && folder && *folder) PlWnd_AppendPaths({folder});
    while (Ms() - t0 < 600000) { Pump(&longest); if (PlayerPlaylist().Count() == expected && !Pending()) break; Sleep(2); }
    double settle = Ms() - t0; Mem m = Snap();
    a = Ms(); DestroyWindow(mainWindow); double destroyMs = Ms() - a; Pump();
    char extra[400]; std::snprintf(extra, sizeof(extra),
        "\"restored_tracks\":%d,\"main_create_ms\":%.3f,\"playlist_create_with_session_ms\":%.3f,\"metadata_settle_ms\":%.1f,\"longest_dispatch_ms\":%.3f,\"destroy_with_save_ms\":%.3f",
        count, mainMs, listMs, settle, longest, destroyMs);
    Print("session", count ? "restored" : "created", m, extra);
}

// ---------------------------------------------------------------- themes / languages / windows / tooltips
void ModeThemes(int cycles) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Ui::ThemeConfigure(nullptr);
    CreateWindows(true, true); Audio_Shutdown(); Require(Audio_TestInit(48000), "init"); Require(Audio_VizInit(), "viz");
    for (int i = 0; i < 3; ++i) PlayerPlaylist().Add(Full(L"tests\\fixtures\\native-44100.wav"));
    PlWnd_Refresh(); Pump();
    std::vector<double> t; Print("themes", "warm", Snap());
    for (int cycle = 1; cycle <= cycles; ++cycle) {
        for (size_t i = 0; i < Ui::ThemeCount(); ++i) {
            double a = Ms(); Ui::SelectTheme(Ui::ThemeAt(i).id, false); Redraw(mainWindow); Redraw(playlistWindow); Redraw(eqWindow); Pump(); t.push_back(Ms() - a);
        }
        if (cycle == 1 || cycle % 5 == 0) { char s[32]; std::snprintf(s, sizeof(s), "cycle_%d", cycle); Print("themes", s, Snap()); }
    }
    auto s = Summ(t); std::string extra = StatJson("switch_and_paint_ms", s);
    Print("themes", "summary", Snap(), extra.c_str());
    DestroyWindow(mainWindow); Pump(); Print("themes", "windows_destroyed", Snap()); Audio_Shutdown();
}
void ModeLanguages(int cycles) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Ui::ThemeConfigure(nullptr);
    I18n::InitializeLanguage(nullptr, [] { return std::vector<std::string>{"en-US"}; });
    CreateWindows(true, true); Audio_Shutdown(); Require(Audio_TestInit(48000), "init"); Require(Audio_VizInit(), "viz");
    std::vector<double> t; std::string error; Print("languages", "warm", Snap());
    const int count = int(I18n::Languages().size());
    for (int cycle = 1; cycle <= cycles; ++cycle) {
        for (int i = 0; i < count; ++i) {
            double a = Ms(); Require(I18n::ExecuteLanguageCommand(I18n::LanguageFirstCommand + i, error), "language");
            Redraw(mainWindow); Redraw(playlistWindow); Redraw(eqWindow); Pump(); t.push_back(Ms() - a);
        }
        if (cycle == 1 || cycle % 5 == 0) { char s[32]; std::snprintf(s, sizeof(s), "cycle_%d", cycle); Print("languages", s, Snap()); }
    }
    auto s = Summ(t); char extra[400]; std::snprintf(extra, sizeof(extra), "%s,\"languages\":%d", StatJson("switch_and_paint_ms", s).c_str(), count);
    Print("languages", "summary", Snap(), extra);
    DestroyWindow(mainWindow); Pump(); Audio_Shutdown();
}
void ModeWindows(int cycles) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Ui::ThemeConfigure(nullptr);
    CreateWindows(true, true); Audio_Shutdown(); Require(Audio_TestInit(48000), "init"); Require(Audio_VizInit(), "viz");
    RECT start{}; GetWindowRect(mainWindow, &start);
    std::vector<double> move, toggle, minimize; Print("windows", "warm", Snap());
    for (int cycle = 1; cycle <= cycles; ++cycle) {
        double a = Ms();
        for (int step = 0; step < 20; ++step) {
            int dx = (step % 10) * 6, dy = (step % 5) * 4;
            SetWindowPos(mainWindow, nullptr, start.left + dx, start.top + dy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            SendMessageW(mainWindow, WM_EXITSIZEMOVE, 0, 0); Pump();
        }
        move.push_back((Ms() - a) / 20);
        a = Ms(); ClickPlayer(11); Pump(); ClickPlayer(11); Pump(); ClickPlayer(12); Pump(); ClickPlayer(12); Pump(); ClickPlayer(16); Pump(); ClickPlayer(16); Pump();
        toggle.push_back((Ms() - a) / 6);
        a = Ms(); ShowWindow(mainWindow, SW_MINIMIZE); Pump(); ShowWindow(mainWindow, SW_RESTORE); Pump(); minimize.push_back(Ms() - a);
        if (cycle == 1 || cycle % 25 == 0) { char s[32]; std::snprintf(s, sizeof(s), "cycle_%d", cycle); Print("windows", s, Snap()); }
    }
    SetWindowPos(mainWindow, nullptr, start.left, start.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    std::string extra = StatJson("move_ms", Summ(move)) + "," + StatJson("toggle_ms", Summ(toggle)) + "," + StatJson("minimize_restore_ms", Summ(minimize));
    Print("windows", "summary", Snap(), extra.c_str());
    DestroyWindow(mainWindow); Pump(); Audio_Shutdown();
}

// ---------------------------------------------------------------- audio independence under UI stress
// An offline "device" thread consumes 480-frame blocks every 10 ms (48 kHz), as the
// WASAPI callback would, while the UI thread performs heavy production work.
struct AudioSim { volatile LONG stop = 0, phase = 0; std::vector<float> durations[8]; std::vector<float> lateness[8]; LONG overruns[8]{}; };
DWORD WINAPI AudioThread(void* p) {
    auto& s = *static_cast<AudioSim*>(p); float pcm[960];
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    timeBeginPeriod(1);
    double next = Ms();
    while (!InterlockedCompareExchange(&s.stop, 0, 0)) {
        double now = Ms(); if (now < next) { Sleep(DWORD(std::max(0.0, next - now - 1.0))); while (Ms() < next) {} }
        int ph = int(InterlockedCompareExchange(&s.phase, 0, 0));
        double a = Ms(); Audio_TestRead(pcm, 480); double d = Ms() - a;
        if (s.durations[ph].size() < 2000000) { s.durations[ph].push_back(float(d * 1000)); s.lateness[ph].push_back(float((a - next) * 1000)); }
        if (d > 10.0) InterlockedIncrement(&s.overruns[ph]);
        next += 10.0;
        if (Ms() > next + 50) next = Ms(); // never spiral after a debugger break
    }
    timeEndPeriod(1);
    return 0;
}
void ModeStress(const wchar_t* track, const wchar_t* folder, int phaseSeconds) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Ui::ThemeConfigure(nullptr);
    I18n::InitializeLanguage(nullptr, [] { return std::vector<std::string>{"en-US"}; });
    CreateWindows(true, true); Audio_Shutdown(); Eq::ApplyPreset(1); Eq::SetEnabled(true);
    Require(Audio_TestInit(48000), "init"); Require(Audio_VizInit(), "viz");
    PlayerPlaylist().Add(track); Require(PlayerPlayback().PlayIndex(0), "play"); PlWnd_Refresh(); Pump();
    ClickPlayer(13); Pump();
    static AudioSim sim; HANDLE th = CreateThread(nullptr, 0, AudioThread, &sim, 0, nullptr); Require(th != nullptr, "thread");
    const char* names[] = {"idle_ui", "viz_30fps", "themes", "languages", "playlist_import", "windows", "eq_changes", "search_sort"};
    std::string error;
    for (int ph = 0; ph < 8; ++ph) {
        InterlockedExchange(&sim.phase, ph); double t0 = Ms(); int iter = 0; double longest = 0;
        if (ph == 4) PlWnd_AppendPaths({folder});
        RECT start{}; GetWindowRect(mainWindow, &start);
        while (Ms() - t0 < phaseSeconds * 1000.0) {
            switch (ph) {
            case 0: Pump(&longest); Sleep(10); break;
            case 1: SendMessageW(mainWindow, WM_TIMER, VIZ_TIMER_ID, 0); Pump(&longest); Sleep(33); break;
            case 2: Ui::SelectTheme(Ui::ThemeAt(iter % Ui::ThemeCount()).id, false); Redraw(mainWindow); Redraw(playlistWindow); Redraw(eqWindow); Pump(&longest); break;
            case 3: I18n::ExecuteLanguageCommand(I18n::LanguageFirstCommand + iter % int(I18n::Languages().size()), error); Redraw(mainWindow); Redraw(playlistWindow); Redraw(eqWindow); Pump(&longest); break;
            case 4: Pump(&longest); Sleep(2); break;
            case 5: SetWindowPos(mainWindow, nullptr, start.left + (iter % 20) * 5, start.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                    SendMessageW(mainWindow, WM_EXITSIZEMOVE, 0, 0); if (iter % 10 == 0) { ClickPlayer(11); ClickPlayer(12); } Pump(&longest); break;
            case 6: Eq::ApplyPreset(iter % Eq::PresetCount()); Eq::SetEnabled(iter % 3 != 0); Redraw(eqWindow); Pump(&longest); Sleep(5); break;
            case 7: { HWND s = GetDlgItem(playlistWindow, 520); SetWindowTextW(s, iter % 2 ? L"faixa 0" : L""); SendMessageW(playlistWindow, WM_COMMAND, 506 + iter % 4, 0); Pump(&longest); break; }
            }
            ++iter;
        }
        SetWindowPos(mainWindow, nullptr, start.left, start.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        std::vector<double> d(sim.durations[ph].begin(), sim.durations[ph].end()), l(sim.lateness[ph].begin(), sim.lateness[ph].end());
        char extra[900]; std::snprintf(extra, sizeof(extra), "\"phase\":\"%s\",\"ui_iterations\":%d,\"ui_longest_dispatch_ms\":%.3f,%s,%s,\"block_overruns_over_10ms\":%ld,\"tracks\":%d",
            names[ph], iter, longest, StatJson("block_us", Summ(d)).c_str(), StatJson("lateness_us", Summ(l)).c_str(), sim.overruns[ph], PlayerPlaylist().Count());
        Print("stress", names[ph], Snap(), extra);
    }
    InterlockedExchange(&sim.stop, 1); WaitForSingleObject(th, INFINITE); CloseHandle(th);
    DestroyWindow(mainWindow); Pump(); Audio_Shutdown();
}

// ---------------------------------------------------------------- tooltips
void ModeTooltips(int cycles) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); Ui::ThemeConfigure(nullptr);
    CreateWindows(true, true);
    std::vector<double> t; Print("tooltips", "warm", Snap());
    for (int i = 1; i <= cycles; ++i) {
        double a = Ms();
        for (int id = 1; id <= 16; ++id) { auto text = Ui::Tooltips::Description(mainWindow, id); (void)text; }
        Ui::Tooltips::Refresh(); Ui::Tooltips::Hide(mainWindow); Pump(); t.push_back(Ms() - a);
        if (i % 1000 == 0) { char s[32]; std::snprintf(s, sizeof(s), "cycle_%d", i); Print("tooltips", s, Snap()); }
    }
    std::string extra = StatJson("describe16_ms", Summ(t)) + ",\"attached\":" + std::to_string(Ui::Tooltips::AttachedCount());
    Print("tooltips", "summary", Snap(), extra.c_str());
    DestroyWindow(mainWindow); Pump();
}
}

int main(int argc, char** argv) {
    // Same Per-Monitor V2 opt-in as app.cpp, resolved dynamically for Windows 7.
    if (auto set = reinterpret_cast<BOOL(WINAPI*)(HANDLE)>(reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))) set(reinterpret_cast<HANDLE>(-4));
    std::string mode = argc > 1 ? argv[1] : "";
    auto arg = [&](int i, const char* fallback) { return Full(Wide(argc > i ? argv[i] : fallback)); };
    auto num = [&](int i, int fallback) { return argc > i ? std::atoi(argv[i]) : fallback; };
    if (mode == "dsp") ModeDsp();
    else if (mode == "graph") ModeGraph(arg(2, "build\\perf\\fixtures\\track-4min.mp3").c_str(), num(3, 30), num(4, 0));
    else if (mode == "meta") ModeMeta(argc, argv);
    else if (mode == "canvas") ModeCanvas();
    else if (mode == "startup-parts") ModeStartupParts();
    else if (mode == "memory-stages") ModeMemoryStages(arg(2, "build\\perf\\fixtures\\track-4min.mp3").c_str());
    else if (mode == "paint") ModePaint(arg(2, "build\\perf\\fixtures\\track-4min.mp3").c_str(), argc > 3 ? arg(3, "").c_str() : L"");
    else if (mode == "playlist") ModePlaylist(arg(2, "build\\perf\\fixtures\\many\\1000").c_str(), num(3, 1000));
    else if (mode == "session") ModeSession(arg(2, "build\\perf\\session").c_str(), argc > 3 ? arg(3, "").c_str() : L"", num(4, 0));
    else if (mode == "themes") ModeThemes(num(2, 10));
    else if (mode == "languages") ModeLanguages(num(2, 10));
    else if (mode == "windows") ModeWindows(num(2, 100));
    else if (mode == "stress") ModeStress(arg(2, "build\\perf\\fixtures\\track-4min.mp3").c_str(), arg(3, "build\\perf\\fixtures\\many\\5000").c_str(), num(4, 20));
    else if (mode == "tooltips") ModeTooltips(num(2, 5000));
    else { std::fprintf(stderr, "usage: perf-probe dsp|graph|meta|canvas|paint|playlist|session|themes|languages|windows|stress|tooltips ...\n"); return 1; }
    return 0;
}
