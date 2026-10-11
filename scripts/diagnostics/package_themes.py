"""Audit recorded theme evidence. No desktop input or user configuration writes."""
import hashlib
import json
import re
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/evidence/themes"
TITLES = [
    ("original", "LiteSic Original"), ("dark", "Escuro"), ("light", "Claro"),
    ("inverse", "Inverso"), ("summer", "Verão"), ("christmas", "Natal"),
    ("halloween", "Halloween"), ("cyberpunk", "Cyberpunk"),
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    raw = path.read_bytes()
    return raw.decode("utf-16" if raw.startswith((b"\xff\xfe", b"\xfe\xff")) else "utf-8-sig").replace("\r\n", "\n")


def write_json(name, value):
    (OUT / name).write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    catalog = json.loads(read(ROOT / "build/themes-catalog-64-debug.json"))
    assert len(catalog) == 36 and len({t["id"] for t in catalog}) == 36
    palettes = {t["id"]: t["palette"] for t in catalog}
    write_json("palettes.json", catalog)
    names = {key: (name, category) for key, name, category in re.findall(r'Make\(L"([^"]+)",L"([^"]+)",L"([^"]+)"', read(ROOT / "src/ui_theme.cpp"))}
    names["original"] = ("LiteSic Original", "Essenciais")
    table = ["# Catálogo de temas", "", "Valores efetivos exportados pelos testes do código de produção. Paleta completa: [palettes.json](palettes.json).", "",
             "| Categoria | Tema / código estável | Fonte da interface | Fundo / painel | Texto / destaque | Seleção |",
             "|---|---|---|---|---|---|"]
    for theme in catalog:
        key, p = theme["id"], theme["palette"]
        name, category = names[key]
        table.append(f'| {category} | {name} (`{key}`) | {theme["body"]} | `{p["background"]}` / `{p["surface"]}` | `{p["text"]}` / `{p["green"]}` | `{p["selection"]}` |')
    table += ["", "Cronômetros: Consolas; corpo/pequeno/título/digital: 12/10/14/27 DIP; pesos: 400/400/600/600. Sakura usa título em itálico. Fallback de família ausente: Segoe UI, com fallback de glifos do DirectWrite."]
    (OUT / "catalog.md").write_text("\n".join(table) + "\n", encoding="utf-8")
    events = [json.loads(line) for line in read(ROOT / "build/gui/events.jsonl").splitlines()]
    labels = [f"themes-final2-{key}-{target}" for key, _ in TITLES for target in ("main", "playlist")]
    labels += ["themes-final2-selector-original", "themes-final2-cyberpunk-themes", "themes-final2-restore-original"]
    supplemental = ["themes-playing-cyberpunk", "themes-playing-light-main", "themes-playing-light-playlist",
                    "themes-restored-light-main", "themes-restored-light-playlist", "themes-final-restore-pass"]
    exe_hash = digest(ROOT / "build/64/debug/LiteSic.exe")
    initial_hash = digest(ROOT / "build/themes/captured-initial-LiteSic.exe")
    captures = []
    for label in labels + supplemental:
        event = [e for e in events if e.get("ok") and e.get("args", {}).get("label") == label and "image" in e.get("result", {})][-1]
        result = event["result"]
        src = Path(result["image"])
        assert digest(src) == result["sha256"], label
        assert result["process"]["sha256"] == (exe_hash if label in labels else initial_hash), label
        (OUT / src.name).write_bytes(src.read_bytes())
        captures.append(dict(label=label, file=src.name, sha256=result["sha256"], process=result["process"],
                             rect=result["rect"], client=result["client"], utc=event["utc"],
                             stage="final" if label in labels else "before final contrast adjustments"))
    for key, _ in TITLES:
        for target, size in (("main", [690, 435]), ("playlist", [690, 540])):
            entry = next(c for c in captures if c["label"] == f"themes-final2-{key}-{target}")
            assert entry["client"] == size
            im = Image.open(OUT / entry["file"]).convert("RGB")
            expected = tuple(bytes.fromhex(palettes[key]["background"][1:]))
            assert im.getpixel((3, 52)) == expected, (key, target)
            assert len(im.getcolors(im.width * im.height)) > 20
    # Compare the exact empty-player baseline recorded in the previous session.
    baseline = [e for e in events if e.get("ok") and e.get("args", {}).get("label") == "closeout-repeat-open"][-1]
    current = next(c for c in captures if c["label"] == "themes-final2-original-main")
    assert current["sha256"] == baseline["result"]["sha256"]
    summary = {}
    for arch in (32, 64):
        for mode in ("debug", "release"):
            variant = f"{arch}-{mode}"
            log = read(ROOT / f"build/themes-test-{variant}.log")
            assert "All tests passed." in log and "7 theme cases" in log
            cases = sum(map(int, re.findall(r"^(\d+) .*cases$", log, re.M))) + 27
            assert cases == 89 and len(re.findall(r"^==>", log, re.M)) == 16
            for kind in ("build", "test"):
                src = ROOT / f"build/themes-{kind}-{variant}.log"
                normalized = "\n".join(line.rstrip() for line in read(src).splitlines()) + "\n"
                (OUT / f"{kind}-{variant}.txt").write_text(normalized, encoding="utf-8")
            memory = [json.loads(line) for line in read(ROOT / f"build/themes-memory-{variant}.jsonl").splitlines()]
            assert len(memory) == 11 and memory[-1]["switches"] == 1980
            assert memory[-1]["private_bytes"] <= memory[0]["private_bytes"] + 8 * 1048576
            assert max(r["gdi"] for r in memory) <= memory[0]["gdi"] + 2
            assert max(r["user"] for r in memory) <= memory[0]["user"] + 2
            (OUT / f"memory-{variant}.jsonl").write_text("\n".join(json.dumps(r) for r in memory) + "\n", encoding="utf-8")
            summary[variant] = dict(first=memory[0], last=memory[-1], min_private=min(r["private_bytes"] for r in memory), max_private=max(r["private_bytes"] for r in memory))
            assert json.loads(read(ROOT / f"build/themes-catalog-{variant}.json")) == catalog
    guards = read(ROOT / "build/themes-driver-tests.log")
    assert "Ran 27 tests" in guards and "OK" in guards
    (OUT / "driver-tests.txt").write_text("\n".join(line.rstrip() for line in guards.splitlines()) + "\n", encoding="utf-8")
    restore = json.loads(read(ROOT / "build/themes/restoration.json"))
    assert restore["PlaylistRestored"] and restore["NewPreferenceIsOriginal"]
    write_json("restoration.json", restore)
    write_json("memory-summary.json", summary)
    sources = [{"file": p.relative_to(ROOT).as_posix(), "sha256": digest(p)} for p in sorted((ROOT / "src").glob("*")) if p.is_file()]
    write_json("source-hashes.json", sources)
    gallery = ['<!doctype html><html lang="pt-BR"><meta charset="utf-8"><title>LiteSic — temas</title>',
               '<style>body{font:16px Segoe UI,sans-serif;background:#181818;color:#eee;margin:24px}section{margin:32px 0}img{max-width:48%;vertical-align:top;margin-right:1%}a{color:#9cf}</style>',
               '<h1>LiteSic — oito temas representativos</h1><p>Capturas reais do executável final. Aceite visual humano pendente. <a href="palettes.json">36 paletas</a></p>']
    for key, name in TITLES:
        gallery.append(f'<section><h2>{name}</h2><img alt="Player: {name}" src="themes-final2-{key}-main.png"><img alt="Playlist: {name}" src="themes-final2-{key}-playlist.png"></section>')
    (OUT / "index.html").write_text("\n".join(gallery) + "\n", encoding="utf-8")
    manifest = dict(result="PASS", tests_per_variant=89, total_tests=356, driver_guards=27,
                    sustained_switches_per_variant=1980, captures=captures,
                    original_empty_player_matches_baseline=True, human_visual_acceptance="pending",
                    binary_sha256=exe_hash, supplemental_binary_sha256=initial_hash,
                    artifacts=[{"file": p.name, "sha256": digest(p)} for p in sorted(OUT.iterdir()) if p.is_file() and p.name != "manifest.json"])
    write_json("manifest.json", manifest)
    print(f"PASS: {len(captures)} captures; 356 checks; 27 guards; 4 x 1,980 switches; {OUT}")


if __name__ == "__main__":
    main()
