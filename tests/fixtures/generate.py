"""Regenerate original silent audio fixtures. Requires ffmpeg only to regenerate."""
from pathlib import Path
import subprocess
import sys
import wave
import tempfile
ROOT = Path(__file__).resolve().parent
ffmpeg = sys.argv[1] if len(sys.argv) > 1 else 'ffmpeg'
for name, rate in [('native-44100.wav', 44100), ('native-48000.wav', 48000)]:
    with wave.open(str(ROOT / name), 'wb') as f:
        f.setnchannels(2); f.setsampwidth(2); f.setframerate(rate)
        f.writeframes(b'\0' * rate * 2 * 4)
for name, extra in [('tagged.mp3', ['-id3v2_version', '3']),
                    ('tagged-v4.mp3', ['-id3v2_version', '4']),
                    ('tagged.flac', []), ('tagged.ogg', [])]:
    with tempfile.TemporaryDirectory() as temp:
        metadata = Path(temp) / 'metadata.txt'
        metadata.write_text(';FFMETADATA1\nTITLE=Canção M5\nARTIST=LiteSic\n' +
                            ('DESCRIPTION=' + 'x' * 70000 + '\n' if name.endswith('.ogg') else ''), encoding='utf-8')
        subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-y', '-i', str(ROOT / 'native-44100.wav'),
                        '-f', 'ffmetadata', '-i', str(metadata), '-map_metadata', '1', '-t', '0.25',
                        *extra, str(ROOT / name)], check=True)
data = (ROOT / 'tagged.mp3').read_bytes()
size = ((data[6]&127)<<21)|((data[7]&127)<<14)|((data[8]&127)<<7)|(data[9]&127)
frames = data[10+size:]
tag = b'TAG' + b'Faixa antiga'.ljust(30, b'\0') + b'LiteSic'.ljust(30,b'\0') + b'\0' * 65
(ROOT / 'tagged-v1.mp3').write_bytes(frames + tag)
