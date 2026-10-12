"""Bounded, fail-recording runs; never retries a failed sample or kills other processes.

Run from the repository root. Build with command-line BUILD_DIR/INTERNAL_DIR
overrides; this driver never builds or writes the user's runtime directories.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import shlex
import subprocess

ROOT = Path(__file__).resolve().parents[2]
EVIDENCE = ROOT / "docs/evidence/intermittent-failures"
WORK = ROOT / "build/validation/intermittent-failures"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def snapshot(label):
    files = []
    for arch in ("64", "32"):
        files.extend((ROOT / f"build/{arch}/release").rglob("*"))
    personal = Path(os.environ["LOCALAPPDATA"]) / "LiteSic"
    if personal.exists():
        files.extend(personal.rglob("*"))
    result = {str(p): {"bytes": p.stat().st_size, "sha256": digest(p)}
              for p in sorted(files) if p.is_file()}
    (EVIDENCE / f"preservation-{label}.json").write_text(
        json.dumps(result, indent=2), encoding="utf-8")
    if label == "before":
        for args, name in [(["status", "--short"], "initial-git-status.txt"),
                           (["diff", "--binary", "HEAD"], "initial-working-tree.patch")]:
            (EVIDENCE / name).write_bytes(subprocess.check_output(["git", *args], cwd=ROOT))
    else:
        before = json.loads((EVIDENCE / "preservation-before.json").read_text())
        assert result == before, "Runtime or personal data changed; inspect snapshots"
    print(f"{label}: {len(result)} files hashed")


def sample(task):
    executable, label, index, arguments = task
    log = EVIDENCE / f"{label}-{index:02d}.txt"
    started = datetime.now(timezone.utc).isoformat()
    env = dict(os.environ, LITESIC_NO_CAPTURE="1")
    timed_out = False
    with log.open("wb") as stream:
        process = subprocess.Popen([str(executable), *arguments], cwd=ROOT,
                                   env=env, stdout=stream, stderr=subprocess.STDOUT)
        try:
            code = process.wait(timeout=120)
        except subprocess.TimeoutExpired:
            timed_out = True
            process.kill()  # Only this exact test process, never a name-based kill.
            code = process.wait()
    return {"label": label, "sample": index, "executable": str(executable),
            "sha256": digest(executable), "arguments": arguments,
            "started_utc": started, "finished_utc": datetime.now(timezone.utc).isoformat(),
            "pid": process.pid, "exit_code": code, "timeout": timed_out,
            "log": log.name}


def run(args):
    executable = ROOT / args.executable
    tasks = [(executable, args.label, i, args.arguments) for i in range(1, args.count + 1)]
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        results = list(pool.map(sample, tasks))
    (EVIDENCE / f"{args.label}.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    failures = sum(r["exit_code"] != 0 for r in results)
    print(f"{args.label}: {len(results)-failures}/{len(results)} PASS, {failures} FAIL; workers={args.workers}")
    raise SystemExit(int(failures != 0))


def variant_paths(arch, configuration):
    base = WORK / arch / configuration
    return base / "runtime", base / "internal"


def build(args):
    runtime, internal = variant_paths(args.arch, args.configuration)
    relative = lambda p: p.relative_to(ROOT).as_posix()
    binaries = [relative(internal / "tests" / ("test-" + p.stem + ".exe"))
                for suffix in ("*.c", "*.cpp") for p in sorted((ROOT / "tests").glob(suffix))]
    command = ["make", "-j4", f"ARCH={args.arch}",
               f"DEBUG={int(args.configuration == 'debug')}",
               f"BUILD_DIR={relative(runtime)}", f"INTERNAL_DIR={relative(internal)}",
               "all", "test-locales", *binaries]
    prefix = "mingw32" if args.arch == "32" else "mingw64"
    env = dict(os.environ, MSYSTEM=prefix.upper(), CHERE_INVOKING="1")
    log = EVIDENCE / f"build-{args.arch}-{args.configuration}.txt"
    started = datetime.now(timezone.utc).isoformat()
    with log.open("wb") as stream:
        code = subprocess.call([str(ROOT / ".tools/msys64/usr/bin/bash.exe"),
                                "-lc", shlex.join(command)], cwd=ROOT, env=env,
                               stdout=stream, stderr=subprocess.STDOUT)
    result = {"command": command, "exit_code": code, "started_utc": started,
              "finished_utc": datetime.now(timezone.utc).isoformat(), "log": log.name}
    if code == 0:
        result["executable"] = str(runtime / "LiteSic.exe")
        result["sha256"] = digest(runtime / "LiteSic.exe")
    (EVIDENCE / f"build-{args.arch}-{args.configuration}.json").write_text(
        json.dumps(result, indent=2), encoding="utf-8")
    print(f"build {args.arch}/{args.configuration}: exit={code}", flush=True)
    raise SystemExit(code)


def suite(args):
    _, internal = variant_paths(args.arch, args.configuration)
    label = f"{args.label}-{args.arch}-{args.configuration}"
    results = []
    for suffix in ("*.c", "*.cpp"):
        for source in sorted((ROOT / "tests").glob(suffix)):
            binary = internal / "tests" / ("test-" + source.stem + ".exe")
            result = sample((binary, f"{label}-{source.stem}", 1, []))
            results.append(result)
            print(f"{label} {source.stem}: exit={result['exit_code']}", flush=True)
    (EVIDENCE / f"{label}.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    raise SystemExit(int(any(r["exit_code"] != 0 for r in results)))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    snap = sub.add_parser("snapshot")
    snap.add_argument("label", choices=["before", "after"])
    runs = sub.add_parser("run")
    runs.add_argument("executable")
    runs.add_argument("label")
    runs.add_argument("--count", type=int, default=5, choices=range(1, 21))
    runs.add_argument("--workers", type=int, default=1, choices=range(1, 5))
    runs.add_argument("arguments", nargs="*")
    for action in ("build", "suite"):
        matrix = sub.add_parser(action)
        matrix.add_argument("arch", choices=["32", "64"])
        matrix.add_argument("configuration", choices=["debug", "release"])
        if action == "suite":
            matrix.add_argument("--label", default="suite")
    args = parser.parse_args()
    EVIDENCE.mkdir(parents=True, exist_ok=True)
    WORK.mkdir(parents=True, exist_ok=True)
    if args.action == "snapshot":
        snapshot(args.label)
    elif args.action == "run":
        run(args)
    elif args.action == "build":
        build(args)
    else:
        suite(args)
