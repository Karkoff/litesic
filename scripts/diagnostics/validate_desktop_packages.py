"""Validate extracted desktop release ZIPs with disposable, silent profiles.

Uses the existing Win32/MSAA helpers. No desktop input or screenshots; never
launches the normal edition against the user's actual LocalAppData profile.
"""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time
import zipfile

import validate_release as ui
from verify_portability import read_ini


def seed(profile, locale='pt-BR'):
    profile.mkdir(parents=True, exist_ok=True)
    (profile/'settings.ini').write_text(
        '[Appearance]\nEqualizerVisible=1\nPlaylistVisible=1\nVolumePermille=0\nGlobalHotkeys=0\n'
        '[Language]\nMode=manual\nSelected='+locale+'\n'
        '[Equalizer]\nEnabled=1\nPreamp=-3\nBand0=2\n', encoding='utf-8')


def launch(exe, cwd, profile, portable, label, fixture=None, locale=None,
           play=False, set_volume=None, expected_volume=None, fallback=False):
    env = os.environ.copy()
    env['PATH'] = os.environ['SystemRoot']+'\\System32;'+os.environ['SystemRoot']
    args = [str(exe)] + ([] if portable else ['--profile='+str(profile)])
    if fixture:
        args.append(str(fixture))
    process = subprocess.Popen(args, cwd=cwd, env=env, creationflags=subprocess.CREATE_NO_WINDOW)
    try:
        deadline = time.monotonic()+15
        while True:
            assert process.poll() is None, (label, 'premature exit', process.returncode)
            found = ui.windows(process.pid)
            classes = ['LiteSic.MainWnd'] + (['LiteSic.PlaylistWnd','LiteSic.EqualizerWnd'] if locale else [])
            if all(cls in found for cls in classes):
                break
            assert time.monotonic() < deadline, (label, 'startup timeout', found)
            time.sleep(.05)
        row = dict(label=label, result='PASS')
        provider = ctypes.POINTER(ui.accessibility.IAccessible)()
        ui.oleacc.AccessibleObjectFromWindow(found['LiteSic.MainWnd'], 0xFFFFFFFC,
            ctypes.byref(ui.accessibility.IAccessible._iid_), ctypes.byref(provider))
        if locale:
            source = ui.ROOT/'locales'/locale/'messages.tsv'
            catalog = dict(line.split('\t',1) for line in source.read_text(encoding='utf-8-sig').splitlines()
                           if line and not line.startswith('#'))
            assert provider.accName[5] == catalog['player.play'], (label, provider.accName[5])
            for cls,key in [('LiteSic.PlaylistWnd','playlist.title'),('LiteSic.EqualizerWnd','equalizer.title')]:
                text = ctypes.create_unicode_buffer(1024)
                ui.user.GetWindowTextW(found[cls],text,len(text))
                assert text.value == catalog['window.caption'].replace('{title}',catalog[key]), (label,text.value)
        if expected_volume is not None:
            assert provider.accValue[9] == str(expected_volume), (label,provider.accValue[9])
        if play:
            row['playback'] = ui.playback(found['LiteSic.MainWnd'],process)
        if set_volume is not None:
            provider.accValue[9] = str(set_volume)
            assert provider.accValue[9] == str(set_volume)
        icon = ctypes.c_size_t()
        assert ui.user.SendMessageTimeoutW(found['LiteSic.MainWnd'],0x7F,1,0,2,5000,ctypes.byref(icon))
        assert icon.value, 'Embedded application icon missing'
        windows_root = Path(os.environ['SystemRoot']).resolve()
        loaded = ui.modules(process.pid)
        assert {'d2d1.dll','dwrite.dll'} <= {Path(path).name.lower() for path in loaded}
        row['external_host_modules'] = []
        for module in loaded:
            path = Path(module).resolve()
            # Host overlays and antivirus may inject DLLs into unrelated apps.
            # Record these separately from the audited PE imports and ZIP payload.
            assert '.tools' not in str(path).lower() and 'msys' not in str(path).lower() and 'mingw' not in str(path).lower(), module
            assert not path.is_relative_to(ui.ROOT.resolve()) or path == exe.resolve(), module
            if path != exe.resolve() and not path.is_relative_to(windows_root):
                row['external_host_modules'].append(path.name)
        assert ui.user.PostMessageW(found['LiteSic.MainWnd'],0x10,0,0)
        assert process.wait(timeout=15) == 0, label
        assert (profile/'settings.ini').is_file() and (profile/'playlist.m3u8').is_file(), label
        if fixture:
            assert str(fixture) in (profile/'playlist.m3u8').read_text(encoding='utf-8-sig')
        if locale and not fallback:
            settings = read_ini(profile/'settings.ini')
            assert settings['Language']['Selected'] == locale
            assert float(settings['Equalizer']['Preamp']) == -3
        print(label,'PASS',flush=True)
        return row
    finally:
        if process.poll() is None:
            process.kill()  # Only the disposable child owned by this check.
            process.wait(timeout=10)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory',type=Path)
    args = parser.parse_args()
    directory = args.directory.resolve()
    def check(*positional,**keywords):
        return launch(*positional,**keywords)
    manifest = json.loads((directory/'manifest.json').read_text(encoding='utf-8-sig'))
    work = directory/('validation-'+time.strftime('%H%M%S')+'-'+str(os.getpid()))
    work.mkdir()
    cwd = work/'Other working directory'
    cwd.mkdir()
    fixture = work/'silent.wav'
    shutil.copy2(ui.ROOT/'tests/fixtures/native-44100.wav',fixture)
    personal = Path(os.environ['LOCALAPPDATA'])/'LiteSic'
    personal_before = ui.snapshot(personal)
    rows = []
    for package in manifest['Packages']:
        asset = directory/'assets'/package['Asset']
        assert hashlib.sha256(asset.read_bytes()).hexdigest().upper() == package['SHA256']
        folder = work/('Música portátil - '+asset.stem)
        with zipfile.ZipFile(asset) as archive:
            assert sorted(archive.namelist()) == sorted(package['Files'])
            assert all(not Path(name).is_absolute() and '..' not in Path(name).parts for name in archive.namelist())
            archive.extractall(folder)
        assert sorted(path.name for path in folder.iterdir()) == ['LiteSic.exe','licenses','locales','resources']
        for source in (ui.ROOT/'locales').glob('*/messages.tsv'):
            assert ui.digest(source) == ui.digest(folder/'locales'/source.parent.name/source.name)
        assert len(list((folder/'locales').glob('*/messages.tsv'))) == 18
        exe = folder/'LiteSic.exe'
        assert ui.digest(exe).upper() == package['ExecutableSHA256']
        portable = package['Portable']
        assert (folder/'resources/portable.flag').exists() == portable
        profile = folder/'data' if portable else work/('profile-'+asset.stem)
        rows.append(check(exe,cwd,profile,portable,asset.stem+'-fresh'))
        seed(profile)
        rows.append(check(exe,cwd,profile,portable,asset.stem+'-play-pt',fixture,'pt-BR',True,25))
        rows.append(check(exe,cwd,profile,portable,asset.stem+'-restart',locale='pt-BR',expected_volume=25))
        if portable:
            moved = work/('Moved - á - '+asset.stem)
            assert folder.resolve().is_relative_to(work) and moved.resolve().is_relative_to(work)
            folder.rename(moved)
            rows.append(check(moved/'LiteSic.exe',cwd,moved/'data',True,
                               asset.stem+'-moved',locale='pt-BR',expected_volume=25))
        else:
            assert not (folder/'data').exists(), 'Normal edition unexpectedly created a portable profile'
            fallback_dir = work/('Fallback-'+package['Architecture'])
            fallback_dir.mkdir()
            shutil.copy2(exe,fallback_dir/'LiteSic.exe')
            fallback_profile = fallback_dir/'profile'
            seed(fallback_profile,'en-US')
            rows.append(check(fallback_dir/'LiteSic.exe',cwd,fallback_profile,False,
                               asset.stem+'-embedded-english',locale='en-US',fallback=True))
    assert not list(cwd.iterdir()), 'Application wrote to unrelated current directory'
    assert ui.snapshot(personal) == personal_before, 'Personal profile changed'
    result = dict(result='PASS',checks=rows,packages=4,launches=len(rows),
                  personal_profile_unchanged=True,unrelated_cwd_unchanged=True,
                  os_version=list(__import__('sys').getwindowsversion()))
    (directory/'validation.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('PASS:',len(rows),'launches; four extracted ZIPs; real playback; portable double-click behavior,',
          'Unicode relocation, settings persistence, 18 catalogs, English fallback and reviewed runtime modules.',flush=True)


if __name__ == '__main__':
    main()
