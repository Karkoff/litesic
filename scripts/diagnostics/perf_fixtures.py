"""Synthetic, isolated fixtures for the pre-distribution performance audit.

Everything is written below build/perf (ignored by git). No personal media,
playlist or settings file is read or modified. Requires ffmpeg on PATH.
"""
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, "build", "perf", "fixtures")
COUNT = 10000


def ffmpeg(*args):
    subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", *args], check=True)


# Music-like content: a chord with slow tremolo plus quiet pink noise. Pure tones
# would compress unrealistically well in FLAC and make decoding costs optimistic.
def music(seconds):
    chord = ("aevalsrc='0.18*sin(2*PI*220*t)*(0.6+0.4*sin(2*PI*0.5*t))+0.12*sin(2*PI*277.2*t)"
             "+0.10*sin(2*PI*329.6*t)+0.05*sin(2*PI*1760*t)*(0.5+0.5*sin(2*PI*3*t))"
             f"|0.18*sin(2*PI*220.5*t)+0.12*sin(2*PI*277*t)+0.1*sin(2*PI*330*t):s=44100:d={seconds}'")
    noise = f"anoisesrc=color=pink:amplitude=0.08:sample_rate=44100:d={seconds}"
    return ["-f", "lavfi", "-i", chord, "-f", "lavfi", "-i", noise,
            "-filter_complex", "[1]aformat=channel_layouts=stereo[n];[0][n]amix=inputs=2:normalize=0"]


def syncsafe(n):
    return bytes([(n >> 21) & 127, (n >> 14) & 127, (n >> 7) & 127, n & 127])


def tagged(audio, title, artist, album):
    # ID3v2.4 permits UTF-8 (encoding 3); frame sizes are syncsafe in v2.4.
    body = b""
    for name, text in (("TIT2", title), ("TPE1", artist), ("TALB", album)):
        payload = b"\x03" + text.encode("utf-8")
        body += name.encode("ascii") + syncsafe(len(payload)) + b"\x00\x00" + payload
    return b"ID3\x04\x00\x00" + syncsafe(len(body)) + body + audio


def main():
    os.makedirs(OUT, exist_ok=True)
    long_dir = os.path.join(OUT, "long")
    os.makedirs(long_dir, exist_ok=True)
    jobs = [
        ("track-4min.wav", 240, ["-c:a", "pcm_s16le"]),
        ("track-4min.mp3", 240, ["-c:a", "libmp3lame", "-b:a", "192k"]),
        ("track-4min.flac", 240, ["-c:a", "flac"]),
        ("track-4min.ogg", 240, ["-c:a", "libvorbis", "-q:a", "5"]),
        ("track-4min-48k.flac", 240, ["-ar", "48000", "-c:a", "flac"]),
        ("track-4min-96k24.flac", 240, ["-ar", "96000", "-sample_fmt", "s32", "-c:a", "flac"]),
    ]
    for name, seconds, codec in jobs:
        path = os.path.join(OUT, name)
        if not os.path.exists(path):
            ffmpeg(*music(seconds), *codec, path)
    for name, codec in (("session-60min.mp3", ["-c:a", "libmp3lame", "-b:a", "192k"]),
                        ("session-60min.flac", ["-c:a", "flac"])):
        path = os.path.join(long_dir, name)
        if not os.path.exists(path):
            ffmpeg(*music(3600), *codec, path)
    # Many short files exercise playlist scale and the metadata worker.
    many = os.path.join(OUT, "many")
    seed = os.path.join(OUT, "seed-1s.mp3")
    if not os.path.exists(seed):
        ffmpeg(*music(1), "-ac", "2", "-c:a", "libmp3lame", "-b:a", "96k", "-id3v2_version", "0",
               "-write_xing", "0", seed)
    with open(seed, "rb") as f:
        audio = f.read()
    for group in (100, 1000, 5000, 10000):
        os.makedirs(os.path.join(many, str(group)), exist_ok=True)
    existing = len(os.listdir(os.path.join(many, "10000")))
    if existing != COUNT:
        for group in (100, 1000, 5000, 10000):
            folder = os.path.join(many, str(group))
            shutil.rmtree(folder)
            os.makedirs(folder)
            for i in range(group):
                data = tagged(audio, f"Faixa sintética {i:05d} — teste", f"Artista {i % 97:02d}", f"Álbum {i % 400:03d}")
                with open(os.path.join(folder, f"perf-{i:05d}.mp3"), "wb") as f:
                    f.write(data)
    total = 0
    for dirpath, _, files in os.walk(OUT):
        for name in files:
            total += os.path.getsize(os.path.join(dirpath, name))
    print(f"fixtures={OUT} bytes={total}")


if __name__ == "__main__":
    sys.exit(main())
