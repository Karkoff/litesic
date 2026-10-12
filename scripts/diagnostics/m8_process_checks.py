import ctypes,subprocess,time,json,shutil,sys,configparser
from ctypes import wintypes
from pathlib import Path
root=Path(__file__).resolve().parents[2];work=root/'build/m8-completion';exe=root/'build/64/release/LiteSic.exe'
user=ctypes.WinDLL('user32',use_last_error=True)
user.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
user.GetClassNameW.argtypes=[wintypes.HWND,wintypes.LPWSTR,ctypes.c_int]
user.PostMessageW.argtypes=[wintypes.HWND,wintypes.UINT,wintypes.WPARAM,wintypes.LPARAM]
user.ShowWindow.argtypes=[wintypes.HWND,ctypes.c_int]
user.IsIconic.argtypes=[wintypes.HWND]
user.GetWindowLongW.argtypes=[wintypes.HWND,ctypes.c_int]
user.RegisterHotKey.argtypes=[wintypes.HWND,ctypes.c_int,wintypes.UINT,wintypes.UINT]
user.UnregisterHotKey.argtypes=[wintypes.HWND,ctypes.c_int]
callback=ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
user.EnumWindows.argtypes=[callback,wintypes.LPARAM]
def windows(pid):
    found={}
    @callback
    def visit(hwnd,param):
        owner=wintypes.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(owner))
        if owner.value==pid:
            name=ctypes.create_unicode_buffer(100);user.GetClassNameW(hwnd,name,100);found[name.value]=hwnd
        return True
    user.EnumWindows(visit,0);return found

def wait_ready(process):
    start=time.monotonic()
    while time.monotonic()-start<15:
        found=windows(process.pid)
        if 'LiteSic.PlaylistWnd' in found:return found
        assert process.poll() is None,('premature exit',process.returncode)
        time.sleep(.05)
    raise AssertionError('startup timeout')
def close(process):
    found=windows(process.pid);assert 'LiteSic.MainWnd' in found
    user.PostMessageW(found['LiteSic.MainWnd'],0x10,0,0);assert process.wait(timeout=15)==0
results=[]
# One unique fresh profile, no dependency on previous validation sessions.
profile=work/('process-profile-'+str(int(time.time())));profile.mkdir()
(profile/'settings.ini').write_text('[Appearance]\nVisualizerVisible=0\nEqualizerVisible=1\nPlaylistVisible=1\nVolumePermille=310\nPanPermille=250\nShuffle=1\nRepeat=2\nTopmost=1\nGlobalHotkeys=1\nScalePercent=90\nSpectrum=1\n[Language]\nMode=manual\nSelected=en-US\n',encoding='utf-8')
fixture=str(root/'tests/fixtures/native-44100.wav')
first=subprocess.Popen([str(exe),'--profile='+str(profile),fixture],cwd=root/'build')
try:
    found=wait_ready(first)
    second=subprocess.Popen([str(exe),'--profile='+str(profile),fixture],cwd=root/'tests');assert second.wait(timeout=10)==0;assert not windows(second.pid)
    results.append({'case':'same_profile_forwards_audio_arguments','result':'PASS','primary_pid':first.pid,'secondary_exit':second.returncode})
    unicode_file=work/'音楽 á.wav';shutil.copy2(fixture,unicode_file)
    second=subprocess.Popen([str(root/'build/32/release/LiteSic.exe'),'--profile='+str(profile),unicode_file.name],cwd=work)
    assert second.wait(timeout=10)==0
    results.append({'case':'x86_to_x64_unicode_relative_file','result':'PASS'})
    found=windows(first.pid)  # Startup readiness initially precedes EQ restoration.
    for cls in ('LiteSic.MainWnd','LiteSic.PlaylistWnd','LiteSic.EqualizerWnd'):
        assert user.GetWindowLongW(found[cls],-20)&8,cls
    user.ShowWindow(found['LiteSic.MainWnd'],6);assert user.IsIconic(found['LiteSic.MainWnd'])
    second=subprocess.Popen([str(exe),'--profile='+str(profile)],cwd=root/'tests');assert second.wait(timeout=10)==0
    assert not user.IsIconic(found['LiteSic.MainWnd'])
    assert user.GetWindowLongW(found['LiteSic.MainWnd'],-20)&8
    results.append({'case':'second_instance_restores_minimized_topmost_primary','result':'PASS'})
    close(first)
    for offset in range(4):
        assert user.RegisterHotKey(None,6300+offset,3,0x78+offset),'shortcut not released on close'
        assert user.UnregisterHotKey(None,6300+offset)
    results.append({'case':'all_four_global_hotkeys_released_on_process_close','result':'PASS'})
    session=(profile/'playlist.m3u8').read_text(encoding='utf-8-sig');assert session.count('native-44100.wav')==2 and '音楽 á.wav' in session,session
    results.append({'case':'all_requested_files_persist','result':'PASS','tracks':3})
    # Reopening preserves additive session, independent of cwd.
    first=subprocess.Popen([str(exe),'--profile='+str(profile)],cwd=root/'tests');wait_ready(first);close(first)
    assert (profile/'playlist.m3u8').read_text(encoding='utf-8-sig').count('native-44100.wav')==2
    results.append({'case':'session_restart_from_different_cwd','result':'PASS'})
    settings=configparser.ConfigParser();settings.read(profile/'settings.ini',encoding='utf-8-sig')
    for key,value in {'VolumePermille':'310','PanPermille':'250','Shuffle':'1','Repeat':'2','Topmost':'1','GlobalHotkeys':'1','ScalePercent':'90','Spectrum':'1','VisualizerVisible':'0'}.items():assert settings['Appearance'][key]==value,(key,settings['Appearance'][key])
    assert settings['PlayerWindow']['Version']=='1' and settings['Language']['Selected']=='en-US'
    results.append({'case':'complete_preferences_survive_restart','result':'PASS'})
    # Abrupt primary termination releases the mutex; no background service.
    first=subprocess.Popen([str(exe),'--profile='+str(profile)],cwd=root);wait_ready(first);first.kill();first.wait(timeout=10)
    first=subprocess.Popen([str(exe),'--profile='+str(profile)],cwd=root);wait_ready(first);close(first)
    results.append({'case':'crashed_primary_profile_recovery','result':'PASS'})
    # Different selected profiles may run independently.
    separate=work/('separate-profile-'+str(int(time.time())));separate.mkdir()
    first=subprocess.Popen([str(exe),'--profile='+str(profile)],cwd=root);wait_ready(first)
    other=subprocess.Popen([str(exe),'--profile='+str(separate)],cwd=root);wait_ready(other);assert other.poll() is None and first.poll() is None;close(other);close(first)
    results.append({'case':'different_profiles_are_independent','result':'PASS'})
    # Explicit portable data belong next to the executable even with an unrelated cwd.
    portable=work/('portable-'+str(int(time.time())));portable.mkdir();shutil.copy2(exe,portable/'LiteSic.exe');shutil.copytree(exe.parent/'locales',portable/'locales')
    first=subprocess.Popen([str(portable/'LiteSic.exe'),'--portable',fixture],cwd=root/'tests');wait_ready(first);close(first)
    assert (portable/'data/settings.ini').exists() and 'native-44100.wav' in (portable/'data/playlist.m3u8').read_text(encoding='utf-8-sig')
    results.append({'case':'portable_executable_relative_data','result':'PASS'})
    relative=work/('relative-profile-'+str(int(time.time())))
    first=subprocess.Popen([str(exe),'--profile='+relative.name],cwd=work);wait_ready(first);close(first)
    assert (relative/'settings.ini').is_file()
    results.append({'case':'explicit_relative_profile_resolves_from_cwd','result':'PASS'})
finally:
    if first.poll() is None:first.kill();first.wait(timeout=10)
(work/'process-checks.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8');print(json.dumps(results,ensure_ascii=False),flush=True)
# Optional, isolated GUI fixture for bounded Computer Use inspection.
if '--gui' not in sys.argv:raise SystemExit(0)
gui=work/('gui-profile-final-'+str(int(time.time())));gui.mkdir(exist_ok=True)
(gui/'settings.ini').write_text('[Appearance]\nEqualizerVisible=1\nPlaylistVisible=1\n[Language]\nMode=manual\nSelected=pt-BR\n',encoding='utf-8')
opened=subprocess.Popen([str(exe),'--profile='+str(gui),fixture],cwd=root);wait_ready(opened)
(work/'gui-pid.json').write_text(json.dumps({'pid':opened.pid,'profile':str(gui),'exe':str(exe)}),encoding='utf-8');print('GUI fixture PID',opened.pid,flush=True)
