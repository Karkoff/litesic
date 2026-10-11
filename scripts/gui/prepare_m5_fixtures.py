"""Original silent fixtures confined to build/gui; no third-party audio."""
from pathlib import Path
import shutil
import wave

def prepare():
    root = Path(__file__).resolve().parents[2]
    out = root / 'build' / 'gui'
    fixtures = out / 'm5-fixtures'
    large = out / 'm5-large'
    fixtures.mkdir(parents=True, exist_ok=True)
    large.mkdir(parents=True, exist_ok=True)
    for name in ('Alpha', 'Beta', 'Gamma'):
        shutil.copyfile(out / 'gui-silence.wav', fixtures / (name + '.wav'))
    for name in ('tagged.mp3', 'tagged.flac', 'tagged.ogg'):
        shutil.copyfile(root / 'tests' / 'fixtures' / name, fixtures / name)
    (fixtures / 'invalid.wav').write_bytes(b'not audio')
    for i in range(2000):
        with wave.open(str(large / f'Faixa-{i:04d}.wav'), 'wb') as f:
            f.setnchannels(2); f.setsampwidth(2); f.setframerate(48000)
            f.writeframes(b'\0' * 4800 * 4)
    return {'folder': str(fixtures), 'large_folder': str(large), 'large_count': 2000}
