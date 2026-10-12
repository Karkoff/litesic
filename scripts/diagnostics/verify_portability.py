"""Inspect the existing x64 PE and run only disposable --portable copies.

Run using .tools/gui-venv/Scripts/python.exe. Never builds, replaces the Release,
changes personal settings, switches the input desktop, or retries a failed run.
The PE graph describes this host Windows; delay imports are recorded separately.
"""
import configparser
import ctypes
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import time

import validate_release as ui

ROOT = Path(__file__).resolve().parents[2]
EXE = ROOT / 'build/64/release/LiteSic.exe'
WORK = ROOT / 'build/validation' / ('portability-' + time.strftime('%Y%m%d-%H%M%S') + '-' + str(os.getpid()))
EVIDENCE = ROOT / 'docs/evidence/portability-2026-10-12'
DYNAMIC_NAMES = ['advapi32.dll', 'avrt.dll', 'dsound.dll', 'kernel32.dll',
                 'mmdevapi.dll', 'ole32.dll', 'user32.dll', 'winmm.dll']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def save(name, value):
    (EVIDENCE / name).write_text(json.dumps(value, ensure_ascii=False, indent=2), encoding='utf-8')


class PE:
    """Read imports, delay imports and export forwarders without loading code."""
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        pe = self.u32(0x3c)
        assert self.data[pe:pe+4] == b'PE\0\0', self.path
        self.machine, count = struct.unpack_from('<HH', self.data, pe+4)
        optional = pe+24
        magic = self.u16(optional)
        assert magic in (0x10b, 0x20b)
        self.bits = 64 if magic == 0x20b else 32
        self.base = struct.unpack_from('<Q' if self.bits == 64 else '<I', self.data,
                                       optional + (24 if self.bits == 64 else 28))[0]
        directory = optional + (112 if self.bits == 64 else 96)
        number = self.u32(directory-4)
        self.directories = [struct.unpack_from('<II', self.data, directory+8*i)
                            for i in range(min(number, 16))]
        section = optional+self.u16(pe+20)
        self.sections = []
        for i in range(count):
            at = section+40*i
            name = self.data[at:at+8].rstrip(b'\0').decode('ascii')
            size, rva, raw_size, raw = struct.unpack_from('<IIII', self.data, at+8)
            self.sections.append((name, rva, max(size, raw_size), raw, raw_size))

    def u16(self, offset):
        return struct.unpack_from('<H', self.data, offset)[0]

    def u32(self, offset):
        return struct.unpack_from('<I', self.data, offset)[0]

    def offset(self, rva):
        for _, start, size, raw, _ in self.sections:
            if start <= rva < start+size:
                return raw+rva-start
        if 0 <= rva < min(s[1] for s in self.sections):
            return rva
        raise ValueError((self.path, 'unmapped RVA', hex(rva)))

    def string(self, rva):
        at = self.offset(rva)
        return self.data[at:self.data.index(0, at)].decode('ascii')

    def imports(self, delay=False):
        rva, size = self.directories[13 if delay else 1]
        if not rva:
            return {}
        at = self.offset(rva)
        result = {}
        for _ in range(4096):
            values = struct.unpack_from('<8I' if delay else '<5I', self.data, at)
            if not any(values):
                return result
            if delay:
                attributes, name, _, iat, thunk, *_ = values
                adjust = 0 if attributes & 1 else self.base
                name -= adjust
                thunk = (thunk or iat)-adjust
            else:
                thunk, _, _, name, iat = values
                thunk = thunk or iat
            symbols = []
            step = self.bits//8
            position = self.offset(thunk)
            while True:
                value = struct.unpack_from('<Q' if self.bits == 64 else '<I', self.data, position)[0]
                if not value:
                    break
                if value & (1 << (self.bits-1)):
                    symbols.append('#'+str(value & 0xffff))
                else:
                    symbols.append(self.string(value+2))
                position += step
            result[self.string(name).lower()] = symbols
            at += 32 if delay else 20
        raise ValueError('unterminated import directory')

    def forwarders(self):
        rva, size = self.directories[0]
        if not rva:
            return {}
        at = self.offset(rva)
        base, count, names, functions, name_rvas, ordinals = struct.unpack_from('<6I', self.data, at+16)
        result = {}
        for i in range(count):
            target = self.u32(self.offset(functions)+4*i)
            if rva <= target < rva+size:
                result['#'+str(base+i)] = self.string(target)
        for i in range(names):
            ordinal = self.u16(self.offset(ordinals)+2*i)
            key = '#'+str(base+ordinal)
            if key in result:
                result[self.string(self.u32(self.offset(name_rvas)+4*i))] = result[key]
        return result


def api_sets(system):
    """Read the observed Windows 11 v6 API namespace, never assume other layouts."""
    pe = PE(system/'apisetschema.dll')
    section = next(s for s in pe.sections if s[0] == '.apiset')
    data = pe.data[section[3]:section[3]+section[4]]
    version, size, _, count, offset, _, _ = struct.unpack_from('<7I', data)
    assert version == 6 and size <= len(data) and count < 10000
    text = lambda start, length: data[start:start+length].decode('utf-16-le')
    result = {}
    for i in range(count):
        _, name, length, hashed_length, values, value_count = struct.unpack_from('<6I', data, offset+24*i)
        targets = []
        for j in range(value_count):
            _, alias, alias_length, target, target_length = struct.unpack_from('<5I', data, values+20*j)
            targets.append((text(alias, alias_length).lower(), text(target, target_length).lower()))
        result[text(name, length).lower()+'.dll'] = targets
        # The namespace's hashed prefix excludes the revision suffix.
        result['@'+text(name, hashed_length).lower()] = targets
    return result


def dependency_graph(executable, loaded):
    system = Path(os.environ['SystemRoot'])/'System32'
    schema = api_sets(system)
    windows = Path(os.environ['SystemRoot']).resolve()
    observed = {Path(p).name.lower(): Path(p) for p in loaded
                if Path(p).resolve().is_relative_to(windows)}
    nodes, edges, aliases, missing = {}, [], {}, []
    visited_symbols = set()
    queue = [(executable, [])]

    def resolve(name, parent):
        name = name.lower()
        if not name.endswith('.dll'):
            name += '.dll'
        if name.startswith(('api-', 'ext-')):
            choices = schema.get(name) or schema.get('@'+name.rsplit('-', 1)[0], [])
            targets = [v for alias, v in choices if alias == parent.name.lower()]
            targets = targets or [v for alias, v in choices if not alias]
            targets = targets or [v for _, v in choices]
            aliases[name] = dict(importer=parent.name, hosts=choices, selected=targets[:1])
            if not targets or not targets[0]:
                missing.append(dict(parent=str(parent), dependency=name, reason='API set has no host'))
                return None
            name = targets[0]
        path = observed.get(name, system/name)
        if not path.is_file():
            missing.append(dict(parent=str(parent), dependency=name, reason='file absent'))
            return None
        return path.resolve()

    while queue:
        path, requested = queue.pop(0)
        key = str(path).lower()
        if key not in nodes:
            pe = PE(path)
            assert pe.bits == 64, path
            normal = pe.imports()
            nodes[key] = dict(path=str(path), sha256=digest(path), imports=normal,
                              delay_imports=pe.imports(True), forwarders=pe.forwarders())
            for name, symbols in normal.items():
                target = resolve(name, path)
                edges.append(dict(parent=str(path), dependency=name, target=str(target) if target else None,
                                  kind='import'))
                if target:
                    queue.append((target, symbols))
        for symbol in requested:
            if (key, symbol) in visited_symbols:
                continue
            visited_symbols.add((key, symbol))
            forward = nodes[key]['forwarders'].get(symbol)
            if forward:
                module, destination = forward.rsplit('.', 1)
                target = resolve(module, path)
                edges.append(dict(parent=str(path), symbol=symbol, dependency=forward,
                                  target=str(target) if target else None, kind='export-forwarder'))
                if target:
                    queue.append((target, [destination]))
    return dict(direct=sorted(PE(executable).imports()), root_delay_imports=PE(executable).imports(True),
                api_set_schema=dict(path=str(system/'apisetschema.dll'), version=6, sha256=digest(system/'apisetschema.dll')),
                nodes=nodes, edges=edges, api_set_resolutions=aliases, unresolved=missing,
                dynamic_names_in_compiled_windows_backends=DYNAMIC_NAMES,
                scope='Required normal imports and referenced export forwarders. Windows DLL delay imports are listed, not asserted mandatory.')


def read_ini(path):
    data = path.read_bytes()
    text = data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')
    ini = configparser.ConfigParser()
    ini.read_string(text)
    return ini


def launch(executable, cwd, label, locale=None, media=None, playback=False, volume=None):
    env = {k: v for k, v in os.environ.items()
           if not k.startswith(('MSYS', 'MINGW', 'VSCODE', 'LITESIC_')) and k != 'CHERE_INVOKING'}
    env['PATH'] = os.environ['SystemRoot']+'\\System32;'+os.environ['SystemRoot']
    env['TMP'] = env['TEMP'] = str(WORK/'temp')
    command = [str(executable), '--portable'] + ([str(media)] if media else [])
    started = datetime.now(timezone.utc).isoformat()
    p = subprocess.Popen(command, cwd=cwd, env=env, creationflags=subprocess.CREATE_NO_WINDOW)
    row = dict(label=label, pid=p.pid, command=command, cwd=str(cwd), path=env['PATH'],
               started_utc=started, locale=locale)
    try:
        deadline = time.monotonic()+15
        while True:
            assert p.poll() is None, ('premature exit', label, p.returncode)
            found = ui.windows(p.pid)
            required = ['LiteSic.MainWnd'] + (['LiteSic.PlaylistWnd', 'LiteSic.EqualizerWnd'] if locale else [])
            if all(name in found for name in required):
                break
            assert time.monotonic() < deadline, ('startup timeout', label, found)
            time.sleep(.05)
        row['captions'] = {}
        if locale:
            catalog = dict(line.split('\t', 1) for line in
                (executable.parent/'locales'/locale/'messages.tsv').read_text(encoding='utf-8-sig').splitlines()
                if line and not line.startswith('#'))
            for cls, key in [('LiteSic.PlaylistWnd', 'playlist.title'), ('LiteSic.EqualizerWnd', 'equalizer.title')]:
                caption = ctypes.create_unicode_buffer(1024)
                ui.user.GetWindowTextW(found[cls], caption, len(caption))
                title = catalog[key]
                if locale == 'ar-SA':
                    title = '\u202a'+title+'\u202c'
                assert caption.value == catalog['window.caption'].replace('{title}', title), (label, caption.value)
                row['captions'][cls] = caption.value
            provider = ctypes.POINTER(ui.accessibility.IAccessible)()
            ui.oleacc.AccessibleObjectFromWindow(found['LiteSic.MainWnd'], 0xFFFFFFFC,
                ctypes.byref(ui.accessibility.IAccessible._iid_), ctypes.byref(provider))
            assert provider.accName[5] == catalog['player.play']
            row['initial_volume_percent'] = provider.accValue[9]
            if playback:
                row['playback'] = ui.playback(found['LiteSic.MainWnd'], p)
            if volume is not None:
                provider.accValue[9] = str(volume)
                assert provider.accValue[9] == str(volume)
        row['modules'] = ui.modules(p.pid)
        assert all('.tools' not in path.lower() and 'msys' not in path.lower() and
                   'mingw' not in path.lower() for path in row['modules'])
        assert ui.user.PostMessageW(found['LiteSic.MainWnd'], 0x10, 0, 0)
        assert p.wait(timeout=15) == 0
        row.update(result='PASS', exit_code=p.returncode, finished_utc=datetime.now(timezone.utc).isoformat())
        print(label, 'PASS', flush=True)
        return row
    except BaseException as error:
        row.update(result='FAIL', error=repr(error), exit_code=p.poll())
        save('failed-sample.json', row)
        raise
    finally:
        if p.poll() is None:
            p.kill()  # Only the exact disposable child owned by this sample.
            p.wait(timeout=10)


def main():
    WORK.mkdir(parents=True)
    (WORK/'temp').mkdir()
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    original = {arch: ui.snapshot(ROOT/f'build/{arch}/release') for arch in ('64', '32')}
    personal = Path(os.environ['LOCALAPPDATA'])/'LiteSic'
    personal_before = ui.snapshot(personal)
    save('preservation-before.json', dict(releases=original, personal_hashes=personal_before))
    folder = WORK/'Copia portatil - \u00e1udio'
    folder.mkdir()
    shutil.copy2(EXE, folder/'LiteSic.exe')
    shutil.copytree(EXE.parent/'locales', folder/'locales')
    executable = folder/'LiteSic.exe'
    assert sorted(p.name for p in folder.iterdir()) == ['LiteSic.exe', 'locales']
    assert digest(executable) == digest(EXE)
    cwd = WORK/'another-working-directory'
    cwd.mkdir()
    rows = [launch(executable, cwd, 'fresh-portable-no-data')]
    profile = folder/'data'
    assert {p.name for p in profile.iterdir()} == {'settings.ini', 'playlist.m3u8', 'playlist.m3u8.ini'}
    (folder/'media').mkdir()
    media = folder/'media/sample.wav'
    shutil.copy2(ROOT/'tests/fixtures/native-44100.wav', media)
    catalogs = sorted((folder/'locales').glob('*/messages.tsv'))
    assert len(catalogs) == 18
    for source in catalogs:
        code = source.parent.name
        assert digest(source) == digest(EXE.parent/'locales'/code/source.name)
        (profile/'settings.ini').write_text('[Appearance]\nEqualizerVisible=1\nPlaylistVisible=1\n'
            'VolumePermille=0\nGlobalHotkeys=0\n[Language]\nMode=manual\nSelected='+code+
            '\n[Equalizer]\nEnabled=1\nPreamp=-3\nBand0=2\n', encoding='utf-8')
        rows.append(launch(executable, cwd, 'portable-'+code, code,
                           media if code == 'en-US' else None, playback=code == 'en-US',
                           volume=25 if code == catalogs[-1].parent.name else None))
        ini = read_ini(profile/'settings.ini')
        assert ini['Language']['Selected'] == code
        assert float(ini['Equalizer']['Preamp']) == -3 and float(ini['Equalizer']['Band0']) == 2
        assert ini['Appearance']['VolumePermille'] == ('250' if code == catalogs[-1].parent.name else '0')
    code = catalogs[-1].parent.name
    rows.append(launch(executable, cwd, 'portable-restart-persistence', code))
    assert rows[-1]['initial_volume_percent'] == '25'
    session = (profile/'playlist.m3u8').read_text(encoding='utf-8-sig')
    assert str(media) in session
    moved = WORK/'Moved portable copy'
    assert folder.resolve().is_relative_to(WORK.resolve()) and moved.resolve().is_relative_to(WORK.resolve())
    folder.rename(moved)  # Only this generated, stopped fixture; never a user directory.
    rows.append(launch(moved/'LiteSic.exe', cwd, 'portable-moved-directory', code))
    assert rows[-1]['initial_volume_percent'] == '25'
    assert not media.exists() and (moved/'media/sample.wav').is_file()
    moved_session = (moved/'data/playlist.m3u8').read_text(encoding='utf-8-sig')
    assert str(media) in moved_session  # Demonstrates the existing relocation limitation.
    assert not list(cwd.iterdir())
    loaded = sorted({str(Path(path).resolve()).lower(): path for row in rows for path in row['modules']
                     if Path(path).suffix.lower() == '.dll'}.values(), key=str.lower)
    graph = dependency_graph(EXE, loaded)
    save('dependencies.json', graph)
    windows = Path(os.environ['SystemRoot']).resolve()
    external = [p for p in loaded if not Path(p).resolve().is_relative_to(windows)]
    identity_path = ROOT/'build/internal/64/release/identity/build.json'
    identity = json.loads(identity_path.read_text(encoding='utf-8-sig')) if identity_path.is_file() else {}
    source_commit = identity.get('Commit') if identity.get('SHA256') == digest(EXE) else None
    result = dict(executable=str(EXE), sha256=digest(EXE), source_commit=source_commit,
                  inspected_head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                  work=str(WORK), checks=rows, locales_passed=18, launches_passed=len(rows),
                  loaded_dlls=loaded, external_loaded_dlls=external,
                  profile_created_beside_executable=True, settings_and_playlist_in_data=True,
                  restart_and_moved_settings_preserved=True, unrelated_cwd_unchanged=True,
                  playlist_relocation=dict(result='LIMITATION CONFIRMED', stored_path=str(media),
                      moved_media=str(moved/'media/sample.wav'), stored_path_exists=False, moved_media_exists=True),
                  english_uses_embedded_catalog=True, dev_tools_installed_on_host=True,
                  clean_vm_without_tools_tested=False, os_version=list(__import__('sys').getwindowsversion()))
    save('runtime.json', result)
    after = {arch: ui.snapshot(ROOT/f'build/{arch}/release') for arch in ('64', '32')}
    assert original == after and personal_before == ui.snapshot(personal)
    save('preservation-after.json', dict(releases=after, personal_hashes=ui.snapshot(personal), unchanged=True))
    assert not graph['unresolved'], graph['unresolved']
    print('PASS:', len(rows), 'portable launches, 18 languages, settings relocation; playlist absolute-path limitation confirmed.', flush=True)
    print('PE graph:', len(graph['direct']), 'direct DLLs;', len(graph['nodes'])-1, 'physical dependency DLLs;', len(external), 'external runtime modules.', flush=True)


if __name__ == '__main__':
    import sys
    if sys.argv[1:] == ['--dependencies-only']:
        runtime = json.loads((EVIDENCE/'runtime.json').read_text(encoding='utf-8'))
        graph = dependency_graph(EXE, runtime['loaded_dlls'])
        save('dependencies.json', graph)
        assert not graph['unresolved'], graph['unresolved']
        print('PASS: static dependency graph;', len(graph['nodes'])-1, 'physical DLLs; no runtime repetitions.')
    else:
        assert not sys.argv[1:], 'Supported option: --dependencies-only'
        main()
