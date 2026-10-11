"""Audit real M5 captures/events. No desktop input; not OCR or audible QA."""
from datetime import datetime
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build' / 'gui'

def main():
    events = [json.loads(line) for line in (OUT/'events.jsonl').read_text(encoding='utf-8').splitlines()]
    captures = {}
    for event in events:
        result = event.get('result', {})
        label = event.get('args', {}).get('label')
        if event.get('ok') and label and label.startswith('m5-') and 'image' in result:
            captures[label] = event
    manifest = {}
    for label, event in captures.items():
        result = event['result']; path = Path(result['image']).resolve()
        assert path.is_relative_to(OUT.resolve())
        assert hashlib.sha256(path.read_bytes()).hexdigest() == result['sha256'], label
        with Image.open(path) as image:
            assert image.width > 0 and image.height > 0
        manifest[label] = {'utc': event['utc'], 'image': path.relative_to(ROOT).as_posix(),
                           'sha256': result['sha256'], 'rect': result['rect'],
                           'process': result.get('process'),
                           'note': '' if result.get('process') else 'Historical iteration without per-capture binary identity.'}
    def img(label): return Image.open(OUT/(label+'.png')).convert('RGB')
    a, b = captures['m5-paused-a'], captures['m5-paused-b']
    pause = (datetime.fromisoformat(b['utc'])-datetime.fromisoformat(a['utc'])).total_seconds()
    selected = (90,74,42)
    control, shift = img('m5-control-two-confirmed'), img('m5-shift-selection')
    main = captures['m5-fixed-scale-150-main']['result']['rect']
    playlist = captures['m5-fixed-scale-150-list']['result']['rect']
    wmp = json.loads((OUT/'m5-reference-wmp-final.json').read_text(encoding='utf-16'))
    summary = json.loads((ROOT/'build'/'m5-validation-summary.json').read_text(encoding='utf-8'))
    checks = {
        'pause_interval_at_least_3_seconds': pause >= 3,
        'pause_images_identical': ImageChops.difference(img('m5-paused-a'),img('m5-paused-b')).getbbox() is None,
        'resume_clock_changed': ImageChops.difference(img('m5-paused-seek').crop((6,39,126,78)),img('m5-resume-progress').crop((6,39,126,78))).getbbox() is not None,
        'ctrl_two_noncontiguous_rows': control.getpixel((7,96))==selected and control.getpixel((7,150))==selected and control.getpixel((7,123))!=selected,
        'shift_three_rows': all(shift.getpixel((7,y))==selected for y in (96,123,150)),
        'drag_changed_list_image': ImageChops.difference(img('m5-alpha-selected'),img('m5-alpha-moved')).getbbox() is not None,
        'eof_seek_has_no_progress_fill': all(color!=(102,204,68) for _,color in img('m5-eof-regression-stopped').crop((6,81,407,105)).getcolors(401*24)),
        'scaled_windows_do_not_overlap': max(main[0],playlist[0])>=min(main[2],playlist[2]) or max(main[1],playlist[1])>=min(main[3],playlist[3]),
        'external_wmp_resolved_seven_paths': wmp['count']==7 and all(item['exists'] for item in wmp['items']),
    }
    labels = {('64','debug'):'m5-delivery-playing', ('64','release'):'m5-64-release-provenance',
              ('32','debug'):'m5-32-debug-list', ('32','release'):'m5-32-release-list'}
    for variant in summary:
        arch, cfg = variant['arch'], variant['configuration']
        process = captures[labels[(arch,cfg)]]['result']['process']
        exe = (ROOT/'build'/arch/cfg/'LiteSic.exe').resolve()
        checks[f'identity_{arch}_{cfg}'] = (Path(process['exe']).resolve()==exe and process['sha256']==variant['sha256']==hashlib.sha256(exe.read_bytes()).hexdigest())
        log = (ROOT/'build'/f'm5-final-tests-{arch}-{cfg}.log').read_text(encoding='utf-16')
        assert 'All tests passed.' in log
        assert variant['cases']==68 and variant['failed']==0
    result = {'checks':checks,'pause_seconds':pause,'capture_count':len(manifest), 'captures':manifest}
    (OUT/'m5-evidence.json').write_text(json.dumps(result,indent=2,ensure_ascii=False),encoding='utf-8')
    print(json.dumps({'checks':checks,'capture_count':len(manifest)},indent=2))
    assert all(checks.values()), 'Evidence check failed; inspect the captures.'

if __name__=='__main__': main()
