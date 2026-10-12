"""Read-only window/DPI state for an explicitly supplied isolated LiteSic PID."""
import ctypes as c
import json
import sys
from ctypes import wintypes as w

user = c.WinDLL('user32', use_last_error=True)
user.SetProcessDPIAware()
pid = int(sys.argv[1])
user.GetWindowThreadProcessId.argtypes = [w.HWND, c.POINTER(w.DWORD)]
user.GetClassNameW.argtypes = [w.HWND, w.LPWSTR, c.c_int]
user.GetWindowLongW.argtypes = [w.HWND, c.c_int]
user.GetWindowRect.argtypes = [w.HWND, c.POINTER(w.RECT)]
user.IsIconic.argtypes = [w.HWND]
user.IsWindowVisible.argtypes = [w.HWND]
user.GetDpiForWindow.argtypes = [w.HWND]
user.GetForegroundWindow.restype = w.HWND
visit_type = c.WINFUNCTYPE(w.BOOL, w.HWND, w.LPARAM)
user.EnumWindows.argtypes = [visit_type, w.LPARAM]
windows = []

@visit_type
def visit(hwnd, unused):
    owner = w.DWORD()
    user.GetWindowThreadProcessId(hwnd, c.byref(owner))
    if owner.value == pid:
        name = c.create_unicode_buffer(100)
        user.GetClassNameW(hwnd, name, 100)
        if name.value.startswith('LiteSic.'):
            rect = w.RECT()
            user.GetWindowRect(hwnd, c.byref(rect))
            windows.append(dict(class_name=name.value, hwnd=hwnd,
                                topmost=bool(user.GetWindowLongW(hwnd, -20) & 8),
                                visible=bool(user.IsWindowVisible(hwnd)),
                                minimized=bool(user.IsIconic(hwnd)),
                                dpi=user.GetDpiForWindow(hwnd),
                                rect=[rect.left, rect.top, rect.right, rect.bottom]))
    return True

user.EnumWindows(visit, 0)
foreground = w.DWORD()
user.GetWindowThreadProcessId(user.GetForegroundWindow(), c.byref(foreground))
monitor_type = c.WINFUNCTYPE(w.BOOL, w.HANDLE, w.HDC, c.POINTER(w.RECT), w.LPARAM)
user.EnumDisplayMonitors.argtypes = [w.HDC, c.POINTER(w.RECT), monitor_type, w.LPARAM]
monitors = []

@monitor_type
def monitor(handle, dc, rect, unused):
    r = rect.contents
    monitors.append([r.left, r.top, r.right, r.bottom])
    return True

user.EnumDisplayMonitors(None, None, monitor, 0)
print(json.dumps(dict(pid=pid, foreground_pid=foreground.value,
                      monitor_rects=monitors, windows=windows), indent=2))
if not any(v['class_name'] == 'LiteSic.MainWnd' for v in windows):
    raise SystemExit('No main window for supplied test PID')
