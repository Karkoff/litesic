"""Audit saved UI evidence and test logs; never controls the desktop or user files."""
import hashlib
import json
from pathlib import Path

from PIL import Image
from package_themes import read

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/evidence/playlist-ui"
CURRENT = ["main", "playlist", "main-menu", "themes", "categories", "classic-filter",
           "light-selector", "restore-original", "carbon-restored"]
EARLIER = ["add-menu", "list-menu", "selection", "restored-selection", "play-restored"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    events = [json.loads(line) for line in read(ROOT / "build/gui/events.jsonl").splitlines()]
    final_exe = digest(ROOT / "build/64/debug/LiteSic.exe")
    captures = []
    for prefix, labels in [("adjust-verified-", CURRENT), ("adjust-final-", EARLIER)]:
        for short in labels:
            label = prefix + short
            event = [e for e in events if e.get("ok") and e.get("args", {}).get("label") == label][-1]
            result = event["result"]
            source = Path(result["image"])
            assert digest(source) == result["sha256"], label
            if prefix == "adjust-verified-":
                assert result["process"]["sha256"] == final_exe, label
            with Image.open(source) as image:
                assert image.size == (result["rect"][2] - result["rect"][0], result["rect"][3] - result["rect"][1])
                assert len(image.getcolors(image.width * image.height) or []) > 20
            (OUT / source.name).write_bytes(source.read_bytes())
            captures.append({"file": source.name, "sha256": result["sha256"], "utc": event["utc"],
                             "exe_sha256": result["process"]["sha256"], "class": result["class"],
                             "client": result["client"], "final_binary": prefix == "adjust-verified-"})
    memory = {}
    for arch in (32, 64):
        for mode in ("debug", "release"):
            variant = f"{arch}-{mode}"
            for kind in ("build", "test"):
                content = read(ROOT / f"build/adjust-{kind}-{variant}.log")
                if kind == "test":
                    assert "All tests passed." in content
                    assert "PASS session_survives_window_restart" in content
                    assert "PASS sustained_switch_memory" in content
                else:
                    assert "error:" not in content.lower()
                (OUT / f"{kind}-{variant}.txt").write_text(
                    "\n".join(line.rstrip() for line in content.splitlines()) + "\n", encoding="utf-8")
            samples = [json.loads(line) for line in read(ROOT / f"build/themes-memory-{variant}.jsonl").splitlines()]
            assert len(samples) == 11 and samples[-1]["switches"] == 1980
            first, last = samples[0], samples[-1]
            assert last["private_bytes"] <= first["private_bytes"] + 8 * 1024 * 1024
            assert last["gdi"] <= first["gdi"] + 2 and last["user"] <= first["user"] + 2
            memory[variant] = {"first": first, "last": last, "samples": samples}
    guards = read(ROOT / "build/adjust-driver-tests.log")
    assert "Ran 27 tests" in guards and "OK" in guards
    (OUT / "driver-tests.txt").write_text(
        "\n".join(line.rstrip() for line in guards.splitlines()) + "\n", encoding="utf-8")
    (OUT / "memory.json").write_text(json.dumps(memory, indent=2) + "\n", encoding="utf-8")
    manifest = {"captures": captures, "checks": 356, "driver_guards": 27,
                "note": "Five complementary captures preceded only Unicode sidecar and GDI fallback fixes; nine use the final binary.",
                "sources": {str(p.relative_to(ROOT)): digest(p) for p in sorted((ROOT / "src").glob("*")) if p.is_file()},
                "artifacts": {p.name: digest(p) for p in sorted(OUT.iterdir()) if p.is_file() and p.name != "manifest.json"}}
    (OUT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {len(captures)} captures; 356 checks; 27 guards; four memory runs; {OUT}")


if __name__ == "__main__":
    main()
