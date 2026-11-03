"""Original synthesized audio for Store QA/screenshots; no third-party music."""
from array import array
import math
import json
import random
from pathlib import Path
import wave

root = Path(__file__).resolve().parents[2]
output = root / 'build/store/gui/LiteSic - Audio demo.wav'
output.parent.mkdir(parents=True, exist_ok=True)
rate, seconds = 48000, 120
temporary = output.with_suffix('.generating.wav')
rng = random.Random(20261103)
peak, squares, count = 0.0, 0.0, 0
notes = (220.0, 261.6256, 329.6276, 391.9954, 329.6276, 261.6256, 196.0, 246.9417)
with wave.open(str(temporary), 'wb') as stream:
    stream.setnchannels(2)
    stream.setsampwidth(2)
    stream.setframerate(rate)
    for second in range(seconds):
        samples = array('h')
        for frame in range(rate):
            t = second + frame / rate
            fade = min(1.0, t / .3, (seconds - t) / .5)
            beat = t % .5
            note = notes[int(t * 2) % len(notes)]
            bass = .34 * math.sin(2 * math.pi * 55 * t) + .12 * math.sin(2 * math.pi * 110 * t)
            melody = sum(.19 / harmonic * math.sin(2 * math.pi * note * harmonic * t)
                         for harmonic in range(1, 9))
            kick = .38 * math.exp(-beat * 17) * math.sin(2 * math.pi * 65 * t)
            noise = rng.uniform(-1, 1) * (.055 + .22 * math.exp(-(t % .25) * 38))
            signal = fade * .85 * math.tanh(1.6 * (bass + melody + kick + noise))
            right = signal * .96 + fade * .025 * math.sin(2 * math.pi * note * 2 * t)
            peak = max(peak, abs(signal), abs(right))
            squares += signal * signal + right * right
            count += 2
            samples.extend((int(signal * 32767), int(right * 32767)))
        stream.writeframesraw(samples.tobytes())
temporary.replace(output)  # Never expose a partly written demo to the player.
output.with_suffix('.json').write_text(json.dumps({'seconds': seconds, 'rate': rate,
    'peak': peak, 'rms': math.sqrt(squares / count), 'origin': 'Original deterministic synthesized audio; no third-party music'}, indent=2))
print(output)
