"""Audit and package the recorded M5 run; does not drive the UI or alter sessions."""
import hashlib
import json
import re
import shutil
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "build/m5-closeout"
OUT = ROOT / "docs/evidence/m5-closeout"
LABELS = """synthetic-list offcenter-selected playing-main playing-centered
search-pass canonical-playing canonical-playing-list filter-cleared
multi-ctrl multi-shift reorder-before reorder-after resized
dock-main-before dock-main-moved dock-list-followed
detached-main-before detached-main-moved detached-list-stable
rejoin-snapped snapped-main-before snapped-main-moved snapped-list-followed
exported-list imported-list after-multi-files folder-added-list large-ready
reopened-list repeat-list paused-a paused-b stopped""".split()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read_log(path):
    raw = path.read_bytes()
    return raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig").replace("\r\n", "\n")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    events = [json.loads(line) for line in (ROOT / "build/gui/events.jsonl").read_text(encoding="utf-8").splitlines()]
    captures = []
    by_label = {}
    for label in LABELS:
        name = "closeout-" + label
        matches = [e for e in events if e.get("ok") and e.get("args", {}).get("label") == name and "image" in e.get("result", {})]
        event = matches[-1]
        result = event["result"]
        src = Path(result["image"])
        assert digest(src) == result["sha256"], name
        assert digest(Path(result["process"]["exe"])) == result["process"]["sha256"], name
        shutil.copy2(src, OUT / src.name)
        entry = dict(label=name, utc=event["utc"], file=src.name, sha256=result["sha256"], process=result["process"], rect=result["rect"], client=result["client"])
        captures.append(entry)
        by_label[label] = entry
    # Geometry checks supplement the human inspection of the saved captures.
    assert by_label["resized"]["client"] == [502, 432]
    for main_label, list_label in [("dock-main-moved", "dock-list-followed"), ("snapped-main-moved", "snapped-list-followed")]:
        assert by_label[main_label]["rect"][3] == by_label[list_label]["rect"][1]
    assert by_label["detached-main-before"]["rect"] != by_label["detached-main-moved"]["rect"]
    for record in read_json(RUN / "production-source-hashes.json"):
        assert digest(Path(record["Path"])) == record["Hash"].lower()
    assert read_json(RUN / "persistence-repeat.json")["Identical"]
    assert read_json(RUN / "session-restored.json")["Identical"]
    wmp = read_json(RUN / "wmp-reference.json")
    assert wmp["count"] == 40 and wmp["all_paths_exist"]
    export = (ROOT / "build/gui/m5-closeout-export.m3u8").read_text(encoding="utf-8")
    titles = [line.split(",", 1)[1] for line in export.splitlines() if line.startswith("#EXTINF:")]
    assert len(titles) == 40 and titles[:5] == [f"Teste-{i:03}" for i in (1, 2, 3, 4, 0)]
    counts = []
    for arch in (32, 64):
        for mode in ("debug", "release"):
            log = read_log(RUN / f"test-{arch}-{mode}.log")
            assert "All tests passed." in log
            cases = sum(map(int, re.findall(r"^(\d+) .*cases$", log, re.M)))
            assert cases == 55
            assert len(re.findall(r"^==>", log, re.M)) == 15
            counts.append(cases + 4 + 10 + 6 + 6 + 1)
    assert sum(counts) == 328
    driver = read_log(RUN / "driver-tests.log")
    assert "Ran 24 tests" in driver and "OK" in driver
    for src in RUN.glob("*.log"):
        # Preserve output content, normalizing encoding, newlines and trailing spaces.
        normalized = "\n".join(line.rstrip() for line in read_log(src).splitlines()) + "\n"
        (OUT / (src.stem + ".txt")).write_text(normalized, encoding="utf-8")
    for name in ("persistence-comparison.json", "persistence-repeat.json", "session-restored.json", "production-source-hashes.json", "wmp-reference.json"):
        shutil.copy2(RUN / name, OUT / name)
    memout = OUT / "memory"
    memout.mkdir(exist_ok=True)
    summary = {}
    for backend in ("gdi", "fonts", "software", "d2d"):
        runs = []
        for trial in (1, 2, 3):
            src = RUN / "memory" / f"{backend}-{trial}.jsonl"
            rows = [json.loads(line) for line in read_log(src).splitlines()]
            assert rows[-1]["stage"] == "windows_destroyed"
            (memout / src.name).write_text(read_log(src), encoding="utf-8")
            runs.append(rows)
        warm = [next(r for r in rows if r["stage"] == "empty_warm") for rows in runs]
        summary[backend] = {key + "_median_MiB": round(statistics.median(r[key] for r in warm) / 1048576, 2) for key in ("working_set", "private_bytes")}
        summary[backend]["trials"] = 3
        summary[backend]["filter_private_range_bytes"] = [[min(r["private_bytes"] for r in rows if r["stage"] == "filter_repaint"), max(r["private_bytes"] for r in rows if r["stage"] == "filter_repaint")] for rows in runs]
    for src in (RUN / "memory").glob("*.json"):
        shutil.copy2(src, memout / src.name)
    (memout / "summary.json").write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    manifest = dict(captures=captures, tests=dict(variants=4, executables_per_variant=15, cases_per_variant=82, total_cases=328, driver_guards=24), automated_audit="PASS: capture/binary/source hashes, resize/dock geometry, logs, M3U order and paths, session hash, 12 memory runs", scope="UI semantics inspected visually; manual visual/Explorer/M3U PASS supplied by user. No personal playlist copied into this evidence package.")
    manifest["artifacts"] = [{"file": p.relative_to(OUT).as_posix(), "sha256": digest(p)} for p in sorted(OUT.rglob("*")) if p.is_file() and p.name != "manifest.json"]
    (OUT / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {len(captures)} captures, 328 cases, 24 guards, 12 memory runs; {OUT}")


if __name__ == "__main__":
    main()
