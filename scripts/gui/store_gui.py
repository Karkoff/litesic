"""Use the existing restricted GUI driver with the installed MSIX test identity.

Never accepts an arbitrary executable: WindowsApps path, publisher family and
the exact freshly built Release SHA-256 must all match before any UI input.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import json
import os
from pathlib import Path
import re
import sys
import time
from datetime import datetime, timezone

import litesic_gui as driver

FAMILY = 'LiteSic.LocalValidation_tk4hddz7kkqza'
OUT = driver.ROOT / 'build/store/gui'
STATE = OUT / 'session.json'


def validate_exe(path):
    path = Path(path).resolve()
    apps = (Path(os.environ['ProgramFiles']) / 'WindowsApps').resolve()
    if path.name != 'LiteSic.exe' or path.parent.parent != apps:
        raise ValueError('Only the installed WindowsApps validation executable is permitted.')
    match = re.fullmatch(r'LiteSic\.LocalValidation_\d+\.\d+\.\d+\.\d+_(x64|x86)__tk4hddz7kkqza', path.parent.name)
    if not match or not path.is_file():
        raise ValueError('Wrong package name, architecture or test publisher.')
    arch = '64' if match[1] == 'x64' else '32'
    built = driver.ROOT / f'build/store-native/{arch}/release/LiteSic.exe'
    if driver.digest(path) != driver.digest(built):
        raise ValueError('Installed executable differs from the current Store Release build.')
    return path


def package_family(pid):
    api, *_ = driver.modules()
    handle = api.OpenProcess(0x1000, False, pid)
    query = driver.kernel.GetPackageFamilyName
    query.argtypes = [w.HANDLE, c.POINTER(w.UINT), w.LPWSTR]
    query.restype = w.LONG
    try:
        size = w.UINT()
        if query(int(handle), c.byref(size), None) != 122 or not 2 <= size.value <= 128:
            raise RuntimeError('Process has no valid package identity.')
        name = c.create_unicode_buffer(size.value)
        if query(int(handle), c.byref(size), name) != 0:
            raise RuntimeError('Cannot read the process package family.')
        return name.value
    finally:
        handle.Close()


def check_family(pid):
    if package_family(pid) != FAMILY:
        raise ValueError('Process is not the separate LiteSic MSIX validation identity.')


def attach(pid):
    driver.require_desktop()
    current = driver.identity(pid)
    path = validate_exe(current['exe'])
    check_family(pid)
    driver.write_state({**current, 'sha256': driver.digest(path), 'validation': True, 'store_validation': True})


def capture_window(state, window, label, ordinary_capture):
    # Owner privacy requirement: file dialogs reveal personal shell folders.
    if window.class_name() == '#32770':
        info = driver.window_info(window)
        state['observation'] = {'hwnd': window.handle, 'rect': info['rect'], 'time': time.time()}
        driver.write_state(state)
        return {**info, 'image': None, 'method': 'SKIPPED_PRIVATE_FILE_DIALOG'}
    return ordinary_capture(state, window, label)


def capture_layout(label):
    from PIL import Image
    if not label or not re.fullmatch(r'[a-zA-Z0-9_-]{1,48}', label):
        raise ValueError('A bounded capture label is required.')
    captures = []
    for target in ('playlist', 'eq', 'main'):
        state, window = driver.connect(False, target)
        captures.append(driver.capture(state, window, label + '-' + target))
    left = min(item['rect'][0] for item in captures)
    top = min(item['rect'][1] for item in captures)
    width = max(item['rect'][2] for item in captures) - left
    height = max(item['rect'][3] for item in captures) - top
    size = (1920, 1080) if width <= 1800 and height <= 1020 else (3840, 2160)
    if width > size[0] or height > size[1]:
        raise ValueError('Actual window arrangement exceeds Store screenshot dimensions.')
    canvas = Image.new('RGB', size, '#18191c')
    dx, dy = (size[0] - width) // 2, (size[1] - height) // 2
    for item in captures:
        with Image.open(item['image']) as image:
            canvas.paste(image, (item['rect'][0] - left + dx, item['rect'][1] - top + dy))
    output = OUT / (label + '.png')
    canvas.save(output, optimize=True)
    event = {'utc': datetime.now(timezone.utc).isoformat(), 'action': 'capture-layout', 'ok': True,
             'image': str(output), 'sha256': driver.digest(output), 'size': size,
             'method': 'Actual PrintWindow/GDI surfaces at native resolution and relative positions; solid background; no overlays',
             'sources': captures}
    with (OUT / 'events.jsonl').open('a', encoding='utf-8') as stream:
        stream.write(json.dumps(event, ensure_ascii=False) + '\n')
    print(json.dumps(event, ensure_ascii=True))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, add_help=False)
    parser.add_argument('--pid', type=int)
    parser.add_argument('--label')
    args, remaining = parser.parse_known_args()
    if not remaining:
        raise ValueError('Specify attach, launch or a restricted GUI-driver action.')
    driver.OUT, driver.STATE = OUT, STATE
    driver.validate_exe = validate_exe
    original_connect = driver.connect
    original_capture = driver.capture

    def capture(state, window, label):
        return capture_window(state, window, label, original_capture)

    driver.capture = capture

    def connect(*args, **kwargs):
        driver.require_desktop()
        state = json.loads(STATE.read_text(encoding='utf-8'))
        if not state.get('store_validation') or not state.get('validation'):
            raise ValueError('Not a Store validation session.')
        check_family(state['pid'])
        return original_connect(*args, **kwargs)

    driver.connect = connect
    if remaining[0] == 'capture-layout':
        return capture_layout(args.label)
    if remaining[0] == 'attach':
        if args.pid is None:
            raise ValueError('attach requires an observed --pid.')
        attach(args.pid)
        remaining[0] = 'capture'
    elif remaining[0] == 'launch':
        driver.require_desktop()
        state = json.loads(STATE.read_text(encoding='utf-8'))
        path = validate_exe(state['exe'])
        try:
            current = driver.identity(state['pid'])
        except Exception:
            current = None
        if current and current['created'] == state['created']:
            raise RuntimeError('The validation session is still running; close it first.')
        *_, Application = driver.modules()
        app = Application(backend='win32').start('"' + str(path) + '"', work_dir=str(driver.ROOT), timeout=15)
        app.window(class_name=driver.CLASS).wait('visible', timeout=15)
        attach(app.process)
        remaining[0] = 'capture'
    elif args.pid is not None:
        raise ValueError('--pid is valid only when attaching a new observed session.')
    if remaining[0] == 'm5-fixtures':
        raise ValueError('Use fixture or copy only synthetic fixtures into build/store/gui.')
    sys.argv = [__file__, *remaining, *(['--label', args.label] if args.label else [])]
    return driver.main()


if __name__ == '__main__':
    sys.exit(main())
