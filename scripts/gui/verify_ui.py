"""Audit real UI migration evidence; no window input or fabricated screenshots."""
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build' / 'gui'
events = [json.loads(line) for line in (OUT / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
chosen = {e['args']['label']: e['result'] for e in events
          if e.get('ok') and (e.get('args', {}).get('label') or '').startswith('ui-') and 'image' in e.get('result', {})}
checks = []
def check(name, condition):
    checks.append({'name': name, 'passed': bool(condition)})
def shot(label):
    return Image.open(chosen[label]['image']).convert('RGB')
def rect(label):
    return chosen[label]['rect']
def move(a, b):
    ra, rb = rect(a), rect(b)
    return rb[0]-ra[0], rb[1]-ra[1]

for label, evidence in chosen.items():
    path = Path(evidence['image'])
    check(label + ': PNG hash', hashlib.sha256(path.read_bytes()).hexdigest() == evidence['sha256'])
    check(label + ': process provenance', all(evidence.get('process', {}).get(k) for k in ('pid','created','exe','sha256')))
    check(label + ': nonempty pixels', len(shot(label).getcolors(1000000) or []) > 10)

check('docked movement', move('ui-06-dock-before-main','ui-06-dock-main-moved') ==
      move('ui-07-search-empty','ui-06-dock-playlist-followed') == (60,30))
check('detached playlist stays', rect('ui-06-playlist-detached') == rect('ui-06-independent-playlist-still'))
check('reattached movement', move('ui-06-reattached-main-before','ui-06-reattached-main-moved') ==
      move('ui-06-playlist-reattached','ui-06-reattached-playlist-followed') == (60,30))
check('reattached facing edge', rect('ui-06-playlist-reattached')[1] == rect('ui-06-reattached-main-before')[3])
lcd = (18,63,180,126)  # Observed 150% main window; never used to send input.
check('paused LCD stable', ImageChops.difference(shot('ui-08-paused').crop(lcd),shot('ui-08-paused-stable').crop(lcd)).getbbox() is None)
repeat = (114,294,210,336)
check('repeat three distinct states', len({shot(label).crop(repeat).tobytes() for label in
      ('ui-08-repeat-all','ui-08-repeat-one','ui-08-repeat-off')}) == 3)
for arch, config in [('64','debug'),('64','release'),('32','debug'),('32','release')]:
    label=f'ui-08-{arch}-{config}'
    if label in chosen:
        evidence=chosen[label]
        check(label + ': current binary', hashlib.sha256(Path(evidence['process']['exe']).read_bytes()).hexdigest() == evidence['process']['sha256'])
report = {'checks': checks, 'passed': sum(c['passed'] for c in checks), 'total': len(checks),
          'human_visual_approval': 'PENDING', 'evidence': chosen}
(OUT / 'ui-evidence.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(f"{report['passed']}/{report['total']} UI provenance/pixel/geometry checks")
raise SystemExit(0 if all(c['passed'] for c in checks) else 1)
