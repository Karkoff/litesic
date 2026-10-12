"""Rebuild and run each M8 variant sequentially (global hotkey tests share OS state)."""
import json
import subprocess
import time
from pathlib import Path

root = Path(__file__).resolve().parents[2]
out = root / 'docs/evidence/m8-completion'
out.mkdir(parents=True, exist_ok=True)
results = []
for arch, config in [('64', 'debug'), ('64', 'release'), ('32', 'debug'), ('32', 'release')]:
    for action in ['build', 'test']:
        command = ['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                   '-File', str(root / 'scripts/dev.ps1'), '-Action', action,
                   '-Architecture', arch]
        if config == 'debug':
            command.append('-DebugBuild')
        start = time.monotonic()
        with (out / f'final-{action}-{arch}-{config}.txt').open('w', encoding='utf-8') as log:
            result = subprocess.run(command, cwd=root, stdout=log, stderr=subprocess.STDOUT)
        entry = dict(action=action, arch=arch, config=config, exit=result.returncode,
                     seconds=round(time.monotonic() - start, 2))
        results.append(entry)
        print(json.dumps(entry), flush=True)
        (out / 'final-results.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
        if result.returncode:
            raise SystemExit(result.returncode)
