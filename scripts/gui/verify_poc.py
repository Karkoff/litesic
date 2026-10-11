"""Verify the recorded proof's provenance and image changes, without UI input.

Run after the documented PoC; this compares actual captures, not mock images.
Pixel checks supplement the agent's visual inspection; they are not OCR/audio QA.
"""
import hashlib
import json
from pathlib import Path
from datetime import datetime
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'build' / 'gui'

def main():
    events = [json.loads(line) for line in (OUT / 'events.jsonl').read_text(encoding='utf-8').splitlines()]
    names = ['initial', 'loaded', 'play-click', 'playing-a', 'paused-a', 'paused-b',
             'resumed', 'resumed-later', 'stopped', 'reloaded', 'escape-return']
    images, records = {}, {}
    for name in names:
        path = OUT / (name + '.png')
        checksum = hashlib.sha256(path.read_bytes()).hexdigest()
        matched = [e for e in events if e['ok'] and e.get('result', {}).get('sha256') == checksum
                   and e.get('result', {}).get('image') == str(path)]
        if not matched: raise AssertionError('Sem registro de captura correspondente: ' + name)
        records[name] = matched[-1]
        images[name] = Image.open(path).convert('RGB')
    size = images['loaded'].size
    assert all(im.size == size for im in images.values()), 'Dimensões mudaram: refaça a prova.'
    scale_x, scale_y = size[0] / 275, size[1] / 180
    clock = tuple(round(v * (scale_x if i % 2 == 0 else scale_y)) for i, v in enumerate((4, 26, 84, 52)))
    def different(a, b, crop=False):
        left, right = images[a], images[b]
        if crop: left, right = left.crop(clock), right.crop(clock)
        return ImageChops.difference(left, right).getbbox() is not None
    pause_seconds = (datetime.fromisoformat(records['paused-b']['utc']) -
                     datetime.fromisoformat(records['paused-a']['utc'])).total_seconds()
    checks = {
        'loaded_title_changed': different('initial', 'loaded'),
        'clock_advanced_after_play': different('loaded', 'playing-a', True),
        'pause_observed_for_at_least_3_seconds': pause_seconds >= 3,
        'paused_images_identical': not different('paused-a', 'paused-b'),
        'clock_advanced_after_resume': different('resumed', 'resumed-later', True),
        'stop_restored_loaded_screen': not different('loaded', 'stopped'),
        'keyboard_file_open_restored_loaded_screen': not different('loaded', 'reloaded'),
        'escape_returned_to_player': not different('loaded', 'escape-return'),
    }
    # A later session may use another binary. Never attribute historical
    # captures to the executable currently recorded in session.json.
    hashes = {e['result'].get('process', {}).get('sha256') for e in records.values()}
    executable_hash = next(iter(hashes)) if len(hashes) == 1 and None not in hashes else None
    result = {'checks': checks, 'pause_interval_seconds': pause_seconds,
              'dimensions': list(size), 'executable_sha256': executable_hash,
              'provenance_note': 'Legacy captures without process metadata do not prove the current binary.',
              'captures': {name: {'sha256': records[name]['result']['sha256'], 'utc': records[name]['utc']}
                           for name in names}}
    (OUT / 'poc-verification.json').write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(json.dumps(result, indent=2))
    assert all(checks.values()), 'Verificação visual automática falhou; inspecione as capturas.'

if __name__ == '__main__': main()
