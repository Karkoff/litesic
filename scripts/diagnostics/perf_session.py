"""External measurements of the real LiteSic executable (performance audit).

Every launch uses --profile=<build/perf/profiles/...>; no personal profile,
playlist or setting is read or written. Control uses only the player's own
window messages (WM_APP+97 commands, WM_CLOSE). Results are JSON lines.

Usage:
  python perf_session.py startup <exe> <label> [runs] [session_tracks]
  python perf_session.py scenarios <exe> <label> [seconds] [eq:0|1]
  python perf_session.py soak <exe> <label> <minutes> <track>
"""
import ctypes
import ctypes.wintypes as wt
import json
import os
import shutil
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
PERF = os.path.join(ROOT, "build", "perf")
FIX = os.path.join(PERF, "fixtures")
user32 = ctypes.WinDLL("user32", use_last_error=True)
kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
psapi = ctypes.WinDLL("psapi")

PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010
WM_APP = 0x8000
PLAYBACK = WM_APP + 97
WM_CLOSE = 0x0010
WM_NULL = 0


class PMC(ctypes.Structure):
    _fields_ = [("cb", wt.DWORD), ("PageFaultCount", wt.DWORD), ("PeakWorkingSetSize", ctypes.c_size_t),
                ("WorkingSetSize", ctypes.c_size_t), ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
                ("QuotaPagedPoolUsage", ctypes.c_size_t), ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
                ("QuotaNonPagedPoolUsage", ctypes.c_size_t), ("PagefileUsage", ctypes.c_size_t),
                ("PeakPagefileUsage", ctypes.c_size_t), ("PrivateUsage", ctypes.c_size_t)]


class THREADENTRY32(ctypes.Structure):
    _fields_ = [("dwSize", wt.DWORD), ("cntUsage", wt.DWORD), ("th32ThreadID", wt.DWORD),
                ("th32OwnerProcessID", wt.DWORD), ("tpBasePri", wt.LONG), ("tpDeltaPri", wt.LONG), ("dwFlags", wt.DWORD)]


kernel32.OpenProcess.restype = wt.HANDLE
kernel32.CreateToolhelp32Snapshot.restype = wt.HANDLE
user32.SendMessageTimeoutW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM, wt.UINT, wt.UINT, ctypes.POINTER(ctypes.c_size_t)]
user32.PostMessageW.argtypes = [wt.HWND, wt.UINT, wt.WPARAM, wt.LPARAM]
EnumProc = ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)


class Proc:
    def __init__(self, popen):
        self.popen = popen
        self.pid = popen.pid
        self.handle = kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, self.pid)

    def cpu_seconds(self):
        c, e, k, u = (wt.FILETIME() for _ in range(4))
        kernel32.GetProcessTimes(self.handle, ctypes.byref(c), ctypes.byref(e), ctypes.byref(k), ctypes.byref(u))
        f = lambda t: ((t.dwHighDateTime << 32) | t.dwLowDateTime) / 1e7
        return f(k) + f(u)

    def threads(self):
        snap = kernel32.CreateToolhelp32Snapshot(4, 0)
        e = THREADENTRY32()
        e.dwSize = ctypes.sizeof(e)
        n = 0
        ok = kernel32.Thread32First(snap, ctypes.byref(e))
        while ok:
            if e.th32OwnerProcessID == self.pid:
                n += 1
            ok = kernel32.Thread32Next(snap, ctypes.byref(e))
        kernel32.CloseHandle(snap)
        return n

    def foreign_image_commit(self):
        """Committed copy-on-write image pages of DLLs outside Windows and LiteSic.

        Third-party injected hooks (e.g. an overlay) are charged as private bytes."""
        class MBI(ctypes.Structure):
            _fields_ = [("BaseAddress", ctypes.c_void_p), ("AllocationBase", ctypes.c_void_p), ("AllocationProtect", wt.DWORD),
                        ("PartitionId", wt.WORD), ("RegionSize", ctypes.c_size_t), ("State", wt.DWORD),
                        ("Protect", wt.DWORD), ("Type", wt.DWORD)]
        total, addr, m = 0, 0, MBI()
        name = ctypes.create_unicode_buffer(1024)
        while kernel32.VirtualQueryEx(self.handle, ctypes.c_void_p(addr), ctypes.byref(m), ctypes.sizeof(m)):
            if m.State == 0x1000 and m.Type == 0x1000000 and m.Protect & 0x88:  # commit, image, (execute_)writecopy
                if psapi.GetMappedFileNameW(self.handle, ctypes.c_void_p(m.BaseAddress), name, 1024):
                    n = name.value.lower()
                    if "\\windows\\" not in n and "litesic" not in n:
                        total += m.RegionSize
            addr = (m.BaseAddress or 0) + m.RegionSize
        return total

    def sample(self):
        m = PMC()
        m.cb = ctypes.sizeof(m)
        psapi.GetProcessMemoryInfo(self.handle, ctypes.byref(m), m.cb)
        h = wt.DWORD()
        kernel32.GetProcessHandleCount(self.handle, ctypes.byref(h))
        return {"working_set": m.WorkingSetSize, "private_bytes": m.PrivateUsage, "peak_working_set": m.PeakWorkingSetSize,
                "handles": h.value, "gdi": user32.GetGuiResources(self.handle, 0), "user": user32.GetGuiResources(self.handle, 1),
                "threads": self.threads(), "cpu_s": self.cpu_seconds(), "foreign_image_commit": self.foreign_image_commit()}

    def window(self, cls="LiteSic.MainWnd"):
        found = []

        def cb(hwnd, _):
            pid = wt.DWORD()
            user32.GetWindowThreadProcessId(hwnd, ctypes.byref(pid))
            if pid.value == self.pid:
                name = ctypes.create_unicode_buffer(80)
                user32.GetClassNameW(hwnd, name, 80)
                if name.value == cls and user32.IsWindowVisible(hwnd):
                    found.append(hwnd)
            return True
        user32.EnumWindows(EnumProc(cb), 0)
        return found[0] if found else None

    def responsive(self, hwnd, timeout_ms=5000):
        r = ctypes.c_size_t()
        return bool(user32.SendMessageTimeoutW(hwnd, WM_NULL, 0, 0, 0x0002, timeout_ms, ctypes.byref(r)))

    def command(self, action):
        hwnd = self.window()
        user32.PostMessageW(hwnd, PLAYBACK, action, 0)
        self.responsive(hwnd)

    def close(self, timeout=60):
        hwnd = self.window()
        t = time.perf_counter()
        if hwnd:
            user32.PostMessageW(hwnd, WM_CLOSE, 0, 0)
        try:
            code = self.popen.wait(timeout)
        except subprocess.TimeoutExpired:
            self.popen.kill()
            code = -999
        kernel32.CloseHandle(self.handle)
        return code, time.perf_counter() - t


def profile(name, settings=None, session=None):
    path = os.path.join(PERF, "profiles", name)
    shutil.rmtree(path, ignore_errors=True)
    os.makedirs(path)
    lines = ["[Appearance]", "VolumePermille=60", "PlaylistVisible=1", "VisualizerVisible=1", "EqualizerVisible=0", "Spectrum=0"]
    for key, value in (settings or {}).items():
        lines = [l for l in lines if not l.startswith(key + "=")] + [f"{key}={value}"]
    eq = [l for l in lines if l.startswith("EQ")]
    lines = [l for l in lines if not l.startswith("EQ")]
    if eq:
        lines += ["[Equalizer]"] + [l[2:] for l in eq]
    with open(os.path.join(path, "settings.ini"), "w", encoding="utf-16") as f:
        f.write("\r\n".join(lines) + "\r\n")
    if session:
        with open(os.path.join(path, "playlist.m3u8"), "w", encoding="utf-8", newline="") as f:
            f.write("#EXTM3U\r\n")
            for p in session:
                f.write(f"#EXTINF:1,{os.path.splitext(os.path.basename(p))[0]}\r\n{p}\r\n")
    return path


def launch(exe, prof, files=()):
    t0 = time.perf_counter()
    p = Proc(subprocess.Popen([exe, f"--profile={prof}", *files]))
    hwnd = None
    while time.perf_counter() - t0 < 60 and hwnd is None:
        hwnd = p.window()
        if hwnd is None:
            time.sleep(0.002)
    t_window = time.perf_counter() - t0
    ok = hwnd is not None and p.responsive(hwnd, 60000)
    t_resp = time.perf_counter() - t0
    return p, t_window, t_resp, ok


def emit(obj):
    print(json.dumps(obj), flush=True)


def window_metrics(p):
    return {"playlist_visible": bool(p.window("LiteSic.PlaylistWnd")), "eq_visible": bool(p.window("LiteSic.EqualizerWnd"))}


def startup(exe, label, runs=10, tracks=0):
    folder = os.path.join(FIX, "many", "10000")
    session = [os.path.join(folder, f"perf-{i:05d}.mp3") for i in range(tracks)] if tracks else None
    for run in range(runs):
        prof = profile(f"startup-{label}", session=session)
        p, tw, tr, ok = launch(exe, prof)
        time.sleep(3 if not tracks else 1)
        s1 = p.sample()
        settle = None
        if tracks:  # wait until metadata reading finishes: CPU stops increasing
            t0 = time.perf_counter()
            last = p.cpu_seconds()
            quiet = 0
            while time.perf_counter() - t0 < 600 and quiet < 4:
                time.sleep(0.5)
                c = p.cpu_seconds()
                quiet = quiet + 1 if c - last < 0.01 else 0
                last = c
            settle = time.perf_counter() - t0 - 2.0
        s2 = p.sample()
        code, tclose = p.close()
        emit({"kind": "startup", "label": label, "run": run, "session_tracks": tracks, "window_visible_s": tw,
              "responsive_s": tr, "ok": ok, "metadata_settle_s": settle, "after_3s": s1, "settled": s2,
              "close_s": tclose, "exit_code": code})


def scenarios(exe, label, seconds=60, eq=0):
    settings = {"EQEnabled": 1, "EQPreset": "Rock", "EQPreamp": -4,
                **{f"EQBand{i}": g for i, g in enumerate([4, 3, 1, -2, -1, 2, 3, 2, 1, 1])}} if eq else {}
    track = os.path.join(FIX, "track-4min.mp3")
    prof = profile(f"scen-{label}", settings)
    p, tw, tr, ok = launch(exe, prof)
    emit({"kind": "launch", "label": label, "window_visible_s": tw, "responsive_s": tr, "ok": ok})

    def measure(name, secs=seconds):
        time.sleep(2)
        a = p.sample()
        t = time.perf_counter()
        series = []
        while time.perf_counter() - t < secs:
            time.sleep(1)
            series.append(p.sample())
        b = p.sample()
        wall = time.perf_counter() - t
        per = [(series[i]["cpu_s"] - (series[i - 1] if i else a)["cpu_s"]) * 100 for i in range(len(series))]
        emit({"kind": "scenario", "label": label, "eq": eq, "scenario": name, "seconds": wall,
              "cpu_pct_one_core_mean": (b["cpu_s"] - a["cpu_s"]) / wall * 100, "cpu_pct_one_core_max_1s": max(per),
              "start": a, "end": b, **window_metrics(p)})

    measure("idle_empty")
    # Same path as a file opened from Explorer: the second instance forwards via WM_COPYDATA.
    subprocess.run([exe, f"--profile={prof}", track], timeout=30)
    time.sleep(1)
    measure("stopped_track_loaded_in_list")
    p.command(5)
    measure("playing_osc")
    p.command(13)
    measure("playing_fft")
    p.command(16)
    measure("playing_viz_hidden")
    p.command(16)
    p.command(12)
    measure("playing_fft_eq_window_open")
    p.command(6)
    measure("paused")
    p.command(5)
    user32.ShowWindow(p.window(), 6)  # SW_MINIMIZE
    measure("playing_minimized")
    user32.ShowWindow(p.window(), 9)
    p.command(12)
    p.command(7)
    measure("stopped_after_playback")
    code, tclose = p.close()
    emit({"kind": "close", "label": label, "close_s": tclose, "exit_code": code})


def soak(exe, label, minutes, track):
    settings = {"EQEnabled": 1, "EQPreset": "Rock", "EQPreamp": -4, "Spectrum": 1, "EqualizerVisible": 1, "Repeat": 1,
                **{f"EQBand{i}": g for i, g in enumerate([4, 3, 1, -2, -1, 2, 3, 2, 1, 1])}}
    prof = profile(f"soak-{label}", settings)
    p, tw, tr, ok = launch(exe, prof, [track])
    p.command(5)
    t0 = time.perf_counter()
    prev = p.sample()
    emit({"kind": "soak", "label": label, "t": 0, **prev, **window_metrics(p)})
    while time.perf_counter() - t0 < minutes * 60:
        time.sleep(10)
        s = p.sample()
        s["cpu_pct_10s"] = (s["cpu_s"] - prev["cpu_s"]) / 10 * 100
        hwnd = p.window()
        tr = time.perf_counter()
        s["responsive"] = p.responsive(hwnd, 2000)
        s["response_ms"] = (time.perf_counter() - tr) * 1000
        emit({"kind": "soak", "label": label, "t": time.perf_counter() - t0, **s})
        prev = s
    code, tclose = p.close()
    emit({"kind": "close", "label": label, "close_s": tclose, "exit_code": code})


def _burn(stop):
    while not stop.is_set():
        pass


def continuity(exe, label, seconds, track, saturate=0):
    """Real-device continuity: loopback capture while UI (and optionally CPU) is stressed."""
    import multiprocessing
    import threading
    probe = os.path.join(PERF, "loopback-probe.exe")
    prof = profile(f"cont-{label}", {"VolumePermille": 200, "Spectrum": 1, "EqualizerVisible": 1, "EQEnabled": 1,
                                     "EQPreset": "Rock", "EQPreamp": -4,
                                     **{f"EQBand{i}": g for i, g in enumerate([4, 3, 1, -2, -1, 2, 3, 2, 1, 1])}})
    p, tw, tr, ok = launch(exe, prof, [track])
    cap = subprocess.Popen([probe, str(seconds + 4)], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
    time.sleep(1.5)
    p.command(5)
    stop = multiprocessing.Event()
    burners = [multiprocessing.Process(target=_burn, args=(stop,)) for _ in range(os.cpu_count() if saturate else 0)]
    for b in burners:
        b.start()
    t0, step, actions = time.perf_counter(), 0, 0
    folder = os.path.join(FIX, "many", "10000")
    while time.perf_counter() - t0 < seconds - 2:
        hwnd = p.window()
        k = step % 12
        if step == 5:
            subprocess.Popen([exe, f"--profile={prof}", folder])  # forwarded import of 10,000 files
        if k in (0, 1):
            p.command(13 if k == 0 else 14)
        elif k in (2, 3):
            p.command(12)
        elif k in (4, 5):
            p.command(11)
        elif k in (6, 7):
            p.command(16)
        elif k == 8:
            user32.ShowWindow(hwnd, 6)
        elif k == 9:
            user32.ShowWindow(hwnd, 9)
        else:
            r = wt.RECT()
            user32.GetWindowRect(hwnd, ctypes.byref(r))
            user32.SetWindowPos(hwnd, None, r.left + (40 if k == 10 else -40), r.top, 0, 0, 0x0001 | 0x0004 | 0x0010)
        actions += 1
        step += 1
        time.sleep(0.5)
    stop.set()
    for b in burners:
        b.join()
    out, _ = cap.communicate(timeout=60)
    s = p.sample()
    code, tclose = p.close()
    result = json.loads(out.strip().splitlines()[-1])
    emit({"kind": "continuity", "label": label, "seconds": seconds, "track": os.path.basename(track), "cpu_saturation_processes": len(burners),
          "ui_actions": actions, **result, "end": s, "exit_code": code})


if __name__ == "__main__":
    mode, exe, label = sys.argv[1], os.path.abspath(sys.argv[2]), sys.argv[3]
    if mode == "startup":
        startup(exe, label, int(sys.argv[4]) if len(sys.argv) > 4 else 10, int(sys.argv[5]) if len(sys.argv) > 5 else 0)
    elif mode == "scenarios":
        scenarios(exe, label, int(sys.argv[4]) if len(sys.argv) > 4 else 60, int(sys.argv[5]) if len(sys.argv) > 5 else 0)
    elif mode == "continuity":
        continuity(exe, label, int(sys.argv[4]), os.path.abspath(sys.argv[5]), int(sys.argv[6]) if len(sys.argv) > 6 else 0)
    elif mode == "soak":
        soak(exe, label, float(sys.argv[4]), os.path.abspath(sys.argv[5]))
