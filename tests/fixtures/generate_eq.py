"""Original 1 kHz, -34 dBFS tones; FFmpeg only used when regenerating codecs."""
from pathlib import Path
import math, struct, wave, subprocess, sys
root=Path(__file__).resolve().parents[1]/'eq_fixtures'
root.mkdir(exist_ok=True)
ffmpeg=sys.argv[1] if len(sys.argv)>1 else 'ffmpeg'
for channels in (1,2):
    path=root/f'eq-tone-{channels}.wav'
    with wave.open(str(path),'wb') as f:
        f.setnchannels(channels);f.setsampwidth(2);f.setframerate(44100)
        f.writeframes(b''.join(struct.pack('<h',round(655*math.sin(2*math.pi*1000*i/44100)))*channels for i in range(44100*2)))
for extension in ('mp3','flac','ogg'):
    subprocess.run([ffmpeg,'-hide_banner','-loglevel','error','-y','-i',str(root/'eq-tone-2.wav'),str(root/f'eq-tone.{extension}')],check=True)

