"""Development-only GUI driver. Targets only the LiteSic process it launched.

Run on the user's unlocked interactive desktop. No server, remote endpoint,
desktop switching, elevation, arbitrary executable, or global key shortcuts.
"""
from __future__ import annotations
import argparse
import ctypes as c
from ctypes import wintypes as w
from datetime import datetime, timezone
import hashlib
import importlib.metadata
import json
from pathlib import Path
import re
import sys
import time
import wave

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build' / 'gui'
STATE = OUT / 'session.json'
CLASS = 'LiteSic.MainWnd'
user = c.WinDLL('user32', use_last_error=True)
kernel = c.WinDLL('kernel32', use_last_error=True)
user.GetProcessWindowStation.restype = w.HANDLE
user.GetThreadDesktop.argtypes = [w.DWORD]
user.GetThreadDesktop.restype = w.HANDLE
user.OpenInputDesktop.argtypes = [w.DWORD, w.BOOL, w.DWORD]
user.OpenInputDesktop.restype = w.HANDLE
user.CloseDesktop.argtypes = [w.HANDLE]
user.GetUserObjectInformationW.argtypes = [w.HANDLE, c.c_int, w.LPVOID, w.DWORD, c.POINTER(w.DWORD)]
user.PrintWindow.argtypes = [w.HWND, w.HDC, w.UINT]
user.PrintWindow.restype = w.BOOL
kernel.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)]
kernel.QueryFullProcessImageNameW.restype = w.BOOL

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def object_name(handle):
    buf = c.create_unicode_buffer(256)
    needed = w.DWORD()
    if not user.GetUserObjectInformationW(handle, 2, buf, c.sizeof(buf), c.byref(needed)):
        raise c.WinError(c.get_last_error())
    return buf.value

def doctor():
    session = w.DWORD()
    kernel.ProcessIdToSessionId(kernel.GetCurrentProcessId(), c.byref(session))
    desktop = object_name(user.GetThreadDesktop(kernel.GetCurrentThreadId()))
    station = object_name(user.GetProcessWindowStation())
    handle = user.OpenInputDesktop(0, False, 1)
    input_desktop = None
    error = None
    if handle:
        try: input_desktop = object_name(handle)
        finally: user.CloseDesktop(handle)
    else: error = c.get_last_error()
    return {'session': session.value, 'station': station, 'desktop': desktop,
            'input_desktop': input_desktop, 'input_error': error,
            'interactive': station == 'WinSta0' and desktop == input_desktop == 'Default',
            'python': sys.version.split()[0],
            'packages': {p: importlib.metadata.version(p) for p in
                         ('pywinauto', 'Pillow', 'pywin32', 'comtypes', 'six')}}

def require_desktop():
    info = doctor()
    if not info['interactive']:
        raise RuntimeError('Desktop não interativo: ' + json.dumps(info) +
                           '. Execute scripts/gui.ps1 no terminal local do usuário, com a tela desbloqueada.')

def modules():
    # Set DPI awareness before pywinauto initializes any UI APIs.
    user.SetProcessDPIAware()
    import win32api, win32con, win32gui, win32process, win32ui
    from pywinauto import Application
    return win32api, win32con, win32gui, win32process, win32ui, Application

def identity(pid):
    api, _, _, process, _, _ = modules()
    handle = api.OpenProcess(0x1000, False, pid)
    try:
        path = c.create_unicode_buffer(32768)
        length = w.DWORD(len(path))
        if not kernel.QueryFullProcessImageNameW(int(handle), 0, path, c.byref(length)):
            raise c.WinError(c.get_last_error())
        return {'pid': pid, 'exe': str(Path(path.value).resolve()),
                'created': process.GetProcessTimes(handle)['CreationTime'].isoformat()}
    finally: handle.Close()

def write_state(state):
    OUT.mkdir(parents=True, exist_ok=True)
    temp = STATE.with_suffix('.tmp')
    temp.write_text(json.dumps(state, indent=2), encoding='utf-8')
    temp.replace(STATE)

def validate_exe(path):
    path = Path(path).resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError('Executável fora do projeto.')
    allowed = [(ROOT / 'build' / arch / config / 'LiteSic.exe').resolve()
               for arch in ('32', '64') for config in ('debug', 'release')]
    if path not in allowed or not path.is_file():
        raise ValueError('Executável deve ser build/{32,64}/{debug,release}/LiteSic.exe deste projeto.')
    return path

def connect(dialog=False, target='main', menu_index=None):
    require_desktop()
    state = json.loads(STATE.read_text(encoding='utf-8'))
    path = validate_exe(state['exe'])
    current = identity(state['pid'])
    if current != {key: state[key] for key in current} or digest(path) != state['sha256']:
        raise RuntimeError('Identidade do processo/executável mudou. Inicie uma nova sessão de teste.')
    _, _, gui, _, _, Application = modules()
    app = Application(backend='win32').connect(process=state['pid'], timeout=5)
    candidates = app.windows(class_name=CLASS, visible_only=True)
    if len(candidates) != 1:
        raise RuntimeError('Esperada exatamente uma janela principal do processo lançado.')
    main = candidates[0]
    if dialog:
        candidates = app.windows(class_name='#32770', visible_only=True)
        owned = []
        for window in candidates:
            owner = gui.GetWindow(window.handle, 4)
            for _ in range(8):
                if owner == main.handle:
                    owned.append(window); break
                if not owner: break
                owner = gui.GetWindow(owner, 4)
        if len(owned) != 1:
            raise RuntimeError('Esperado exatamente um diálogo pertencente à janela LiteSic.')
        window = owned[0]
    else:
        window = main
        if target in ('playlist', 'eq'):
            class_name = 'LiteSic.PlaylistWnd' if target == 'playlist' else 'LiteSic.EqualizerWnd'
            candidates = app.windows(class_name=class_name, visible_only=True)
            candidates = [p for p in candidates if gui.GetWindow(p.handle, 4) == main.handle]
            if len(candidates) != 1: raise RuntimeError('Esperado um painel pertencente ao player lançado.')
            window = candidates[0]
        elif target == 'menu':
            candidates = app.windows(class_name='#32768', visible_only=True)
            if menu_index is None and len(candidates) != 1: raise RuntimeError('Menu ambíguo; selecione --menu-index após inspect.')
            index = 0 if menu_index is None else menu_index
            if not 0 <= index < min(len(candidates),4): raise RuntimeError('Menu indisponível.')
            window = candidates[index]
        elif target != 'main': raise ValueError('Alvo não permitido.')
        if not window.is_enabled():
            raise RuntimeError('Janela bloqueada por diálogo; capture/use --dialog primeiro.')
    return state, window

def foreground(window):
    _, _, gui, process, _, _ = modules()
    if window.is_minimized(): raise RuntimeError('Restaure a janela de teste antes de interagir.')
    if window.class_name() != '#32768': window.set_focus()
    time.sleep(0.12)
    hwnd = gui.GetForegroundWindow()
    same_process = process.GetWindowThreadProcessId(hwnd)[1] == window.process_id()
    menu_owner = window.class_name() == '#32768' and gui.GetClassName(hwnd) in (CLASS, 'LiteSic.PlaylistWnd')
    if not same_process or (hwnd != window.handle and not menu_owner):
        raise RuntimeError('Foco não confirmado na janela alvo. Nenhuma entrada foi enviada.')

def window_info(window):
    rect = window.rectangle()
    client = window.client_rect()
    return {'hwnd': window.handle, 'class': window.class_name(), 'title': window.window_text(),
            'rect': [rect.left, rect.top, rect.right, rect.bottom],
            'client': [client.width(), client.height()], 'child_count': len(window.children())}

def capture(state, window, label):
    if not re.fullmatch(r'[a-zA-Z0-9_-]{1,64}', label): raise ValueError('Nome de captura inválido.')
    _, _, gui, _, ui, _ = modules()
    from PIL import Image
    rect = window.rectangle()
    width, height = rect.width(), rect.height()
    if not 1 <= width <= 8192 or not 1 <= height <= 8192: raise RuntimeError('Dimensões inválidas.')
    dc_handle = gui.GetWindowDC(window.handle)
    source = ui.CreateDCFromHandle(dc_handle)
    memory = source.CreateCompatibleDC()
    bitmap = ui.CreateBitmap()
    bitmap.CreateCompatibleBitmap(source, width, height)
    old = memory.SelectObject(bitmap)
    try:
        if not user.PrintWindow(window.handle, memory.GetSafeHdc(), 0):
            raise RuntimeError('PrintWindow falhou; não há evidência visual.')
        image = Image.frombuffer('RGB', (width, height), bitmap.GetBitmapBits(True), 'raw', 'BGRX', 0, 1)
        if len(image.getcolors(width * height) or []) < 4:
            raise RuntimeError('Captura uniforme/vazia; não há evidência visual válida.')
        path = OUT / (label + '.png')
        image.save(path)
    finally:
        memory.SelectObject(old)
        gui.DeleteObject(bitmap.GetHandle())
        memory.DeleteDC(); source.DeleteDC(); gui.ReleaseDC(window.handle, dc_handle)
    info = window_info(window)
    state['observation'] = {'hwnd': window.handle, 'rect': info['rect'], 'time': time.time()}
    write_state(state)
    return {**info, 'image': str(path), 'sha256': digest(path), 'method': 'PrintWindow/GDI',
            'process': {key: state[key] for key in ('pid','exe','created','sha256')}}

def observed(state, window):
    obs = state.get('observation', {})
    age = time.time() - obs.get('time', 0)
    if (obs.get('hwnd') != window.handle or obs.get('rect') != window_info(window)['rect'] or
            age < 0 or age > 120):
        raise RuntimeError('Capture e inspecione a janela atual antes de enviar entrada (validade: 120 s).')
    # Consume before input so an ambiguous failure cannot be retried blindly.
    state.pop('observation', None)
    write_state(state)

def click(state, window, x, y, button, modifier='', double=False):
    _, _, gui, process, _, _ = modules()
    observed(state, window)
    client = window.client_rect()
    if not (0 <= x < client.width() and 0 <= y < client.height()): raise ValueError('Ponto fora da janela.')
    foreground(window)
    point = window.client_to_screen((x, y))
    under = gui.WindowFromPoint(point)
    if process.GetWindowThreadProcessId(under)[1] != state['pid'] or gui.GetAncestor(under, 2) != window.handle:
        raise RuntimeError('Ponto coberto por outra janela. Nenhum clique enviado.')
    if modifier not in ('', 'ctrl', 'shift'): raise ValueError('Modificador não permitido.')
    if window.class_name() == '#32768':
        from pywinauto import mouse
        mouse.click(button=button, coords=point)
    else: window.click_input(coords=(x, y), button=button,
                             pressed='control' if modifier == 'ctrl' else modifier, double=double)

def fixture():
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / 'gui-silence.wav'
    # Real decodable, silent 120-second PCM fixture: no unexpected audible tone.
    with wave.open(str(path), 'wb') as f:
        f.setnchannels(2); f.setsampwidth(2); f.setframerate(48000)
        for _ in range(120): f.writeframesraw(b'\0' * 48000 * 4)
    return {'fixture': str(path), 'seconds': 120, 'rate': 48000, 'sha256': digest(path)}

def fixture_text(kind, title):
    allowed = {
        'silence': (('Abrir','Open'), [OUT/'gui-silence.wav']),
        'multi': (('Abrir','Open'), [OUT/'m5-fixtures'/name for name in ('Alpha.wav','Beta.wav','Gamma.wav')]),
        'tags': (('Abrir','Open'), [OUT/'m5-fixtures'/name for name in ('tagged.mp3','tagged.flac','tagged.ogg')]),
        'invalid': (('Abrir','Open'), [OUT/'m5-fixtures'/'invalid.wav']),
        'm3u-save': (('Salvar lista M3U',), [OUT/'m5-export-validated.m3u']),
        'm3u-open': (('Abrir lista M3U',), [OUT/'m5-export-validated.m3u']),
        'm3u-save-closeout': (('Salvar lista M3U',), [OUT/'m5-closeout-export.m3u8']),
        'm3u-open-closeout': (('Abrir lista M3U',), [OUT/'m5-closeout-export.m3u8']),
        'folder': (('Procurar Pasta','Browse For Folder'), [OUT/'m5-fixtures']),
        'large-folder': (('Procurar Pasta','Browse For Folder'), [OUT/'m5-large']),
    }
    if kind not in allowed: raise ValueError('Fixture não permitida.')
    titles, paths = allowed[kind]
    if title not in titles: raise ValueError('Fixture incompatível com o diálogo atual: '+title)
    for path in paths:
        if not path.resolve().is_relative_to(OUT.resolve()): raise ValueError('Fixture fora do diretório de evidências.')
        if kind not in ('m3u-save','m3u-save-closeout') and not path.exists(): raise RuntimeError('Fixture ausente: '+str(path))
    return str(paths[0]) if len(paths)==1 else ' '.join('"'+str(p)+'"' for p in paths)

def run(args):
    if args.action == 'doctor': return doctor()
    if args.action == 'fixture': return fixture()
    if args.action == 'm5-fixtures':
        from prepare_m5_fixtures import prepare
        fixture()
        return prepare()
    if args.action == 'launch':
        require_desktop()
        if STATE.exists():
            prior = json.loads(STATE.read_text(encoding='utf-8'))
            try: current = identity(prior['pid'])
            except Exception: current = None
            if current and current['created'] == prior['created']:
                raise RuntimeError('A sessão anterior ainda está ativa. Use close antes de launch.')
        path = validate_exe(ROOT / 'build' / args.arch / args.config / 'LiteSic.exe')
        *_, Application = modules()
        app = Application(backend='win32').start('"' + str(path) + '"', work_dir=str(ROOT), timeout=15)
        try:
            state = {**identity(app.process), 'sha256': digest(path)}
        except Exception:
            app.kill()
            raise
        write_state(state)
        app.window(class_name=CLASS).wait('visible', timeout=15)
        state, window = connect()
        return capture(state, window, args.label or 'initial')
    state, window = connect(args.dialog, args.target, args.menu_index)
    if args.action == 'inspect': return window_info(window)
    if args.action == 'capture': return capture(state, window, args.label or 'capture')
    if args.action == 'click':
        if args.x is None or args.y is None: raise ValueError('click requer --x e --y em pixels do cliente.')
        click(state, window, args.x, args.y, args.button, args.modifier, args.double)
    elif args.action == 'search':
        if args.target != 'playlist' or args.dialog: raise ValueError('Pesquisa somente na playlist.')
        if args.query not in ('Teste-020', 'Alpha', 'inexistente-validacao'):
            raise ValueError('Consulta fora das fixtures de validação.')
        observed(state, window)
        # Do not focus the parent: the production edit hides on empty blur.
        _, _, gui, process, _, _ = modules()
        focused = gui.GetForegroundWindow()
        if process.GetWindowThreadProcessId(focused)[1] != state['pid']:
            raise RuntimeError('Playlist não está em primeiro plano; clique na pesquisa novamente.')
        edits = [e for e in window.children(class_name='Edit') if e.control_id() == 520]
        if len(edits) != 1: raise RuntimeError('Campo de pesquisa ambíguo.')
        # The production edit starts hidden until its painted placeholder is clicked.
        if not edits[0].is_visible(): raise RuntimeError('Clique na pesquisa antes de digitar.')
        if not edits[0].has_keyboard_focus(): raise RuntimeError('Foco não confirmado na pesquisa.')
        edits[0].type_keys('^a', set_foreground=False)
        edits[0].type_keys(args.query, with_spaces=True, set_foreground=False, pause=0.02)
        deadline = time.monotonic() + 3
        while edits[0].window_text() != args.query and time.monotonic() < deadline:
            time.sleep(0.05)
        if edits[0].window_text() != args.query: raise RuntimeError('Pesquisa digitada não confirmada.')
    elif args.action == 'resize':
        if args.target != 'playlist' or args.dialog: raise ValueError('Redimensionamento somente da playlist.')
        if not 275 <= args.width <= 1200 or not 180 <= args.height <= 1000: raise ValueError('Dimensões fora dos limites.')
        observed(state, window); foreground(window)
        window.move_window(width=args.width, height=args.height, repaint=True)
    elif args.action == 'drag':
        observed(state, window); foreground(window)
        _, _, gui, process, _, _ = modules()
        for x,y in ((args.x,args.y),(args.end_x,args.end_y)):
            client=window.client_rect()
            if not (0 <= x < client.width() and 0 <= y < client.height()): raise ValueError('Ponto fora da janela.')
            under=gui.WindowFromPoint(window.client_to_screen((x,y)))
            if process.GetWindowThreadProcessId(under)[1] != state['pid'] or gui.GetAncestor(under,2) != window.handle:
                raise RuntimeError('Ponto coberto por outra janela.')
        from pywinauto import mouse
        start=window.client_to_screen((args.x,args.y)); end=window.client_to_screen((args.end_x,args.end_y))
        mouse.press(coords=start)
        last=start
        try:
            for step in range(1,11):
                point=(round(start[0]+(end[0]-start[0])*step/10),round(start[1]+(end[1]-start[1])*step/10))
                under=gui.WindowFromPoint(point)
                if process.GetWindowThreadProcessId(under)[1] != state['pid'] or gui.GetAncestor(under,2) != window.handle:
                    raise RuntimeError('Arraste interrompido: ponto coberto por outra janela.')
                mouse.move(coords=point); last=point
        finally: mouse.release(coords=last)
    elif args.action == 'key':
        observed(state, window); foreground(window)
        keys = {'escape': '{ESC}', 'enter': '{ENTER}', 'tab': '{TAB}', 'space': '{SPACE}',
                'left': '{LEFT}', 'right': '{RIGHT}', 'home': '{HOME}', 'end': '{END}',
                'up': '{UP}', 'down': '{DOWN}', 'delete': '{DELETE}', 'ctrl-a': '^a'}
        if args.key not in keys: raise ValueError('Tecla não permitida.')
        window.type_keys(keys[args.key], set_foreground=False, pause=0.05)
    elif args.action == 'open-fixture':
        if not args.dialog or window.class_name() != '#32770': raise ValueError('Requer --dialog.')
        text = fixture_text(args.fixture, window.window_text())
        observed(state, window); foreground(window)
        edits = [e for e in window.descendants(class_name='Edit') if e.is_visible() and e.is_enabled()]
        if len(edits) != 1: raise RuntimeError('Campo de nome do arquivo ambíguo.')
        edits[0].set_focus()
        if not edits[0].has_keyboard_focus(): raise RuntimeError('Foco não confirmado no campo de arquivo.')
        edits[0].type_keys('^a', set_foreground=False)
        edits[0].type_keys(text, with_spaces=True, set_foreground=False, pause=0.001)
        deadline = time.monotonic() + 3
        while edits[0].window_text() != text and time.monotonic() < deadline:
            time.sleep(0.05)
        if edits[0].window_text() != text: raise RuntimeError('Digitação do caminho não confirmada.')
        foreground(window)
        window.type_keys('{ENTER}', set_foreground=False)
    elif args.action == 'close':
        if args.dialog or args.target != 'main': raise ValueError('close só permite a janela principal lançada pelo driver.')
        window.close()
        return {'closed': state['pid']}
    time.sleep(0.25)
    deadline = time.monotonic() + 5
    while True:
        try:
            state, window = connect(False, 'playlist' if args.target == 'menu' else args.target)
            break
        except RuntimeError as exc:
            if 'bloqueada por diálogo' not in str(exc): raise
            try:
                state, window = connect(True, args.target)
                break
            except RuntimeError:
                if time.monotonic() >= deadline: raise
                time.sleep(0.1)
    return capture(state, window, args.label or args.action)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['doctor', 'fixture', 'm5-fixtures', 'launch', 'inspect', 'capture', 'click', 'key', 'open-fixture', 'close', 'resize', 'drag', 'search'])
    parser.add_argument('--query')
    parser.add_argument('--target', choices=['main','playlist','eq','menu'], default='main')
    parser.add_argument('--fixture', default='silence')
    parser.add_argument('--menu-index',type=int,choices=range(4))
    parser.add_argument('--modifier', choices=['','ctrl','shift'], default='')
    parser.add_argument('--double', action='store_true')
    parser.add_argument('--width',type=int); parser.add_argument('--height',type=int)
    parser.add_argument('--end-x',type=int); parser.add_argument('--end-y',type=int)
    parser.add_argument('--arch', choices=['32', '64'], default='64')
    parser.add_argument('--config', choices=['debug', 'release'], default='debug')
    parser.add_argument('--dialog', action='store_true')
    parser.add_argument('--label')
    parser.add_argument('--x', type=int); parser.add_argument('--y', type=int)
    parser.add_argument('--button', choices=['left', 'right'], default='left')
    parser.add_argument('--key')
    args = parser.parse_args()
    event = {'utc': datetime.now(timezone.utc).isoformat(), 'action': args.action, 'args': vars(args)}
    try:
        result = run(args); event.update(ok=True, result=result)
    except Exception as exc:
        event.update(ok=False, error=f'{type(exc).__name__}: {exc}')
    OUT.mkdir(parents=True, exist_ok=True)
    with (OUT / 'events.jsonl').open('a', encoding='utf-8') as f:
        f.write(json.dumps(event, ensure_ascii=False) + '\n')
    print(json.dumps(event, ensure_ascii=True))
    return 0 if event['ok'] else 1

if __name__ == '__main__': sys.exit(main())
