"""Markdown tables for docs/PERFORMANCE-AUDIT.md from build/perf/results (JSON lines)."""
import glob
import json
import os
import statistics as st

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RES = os.path.join(ROOT, "build", "perf", "results")
MiB = 2 ** 20


def rows(name):
    path = os.path.join(RES, name)
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8") as f:
        return [json.loads(l) for l in f if l.strip()]


def own_private(s):
    return (s["private_bytes"] - s.get("foreign_image_commit", 0)) / MiB


def startup():
    print("\n### Inicialização (mediana de 10 processos)\n")
    print("| Build | Janela visível (s) | Responde (s) | WS (MiB) | Privados* (MiB) | Threads | Handles | GDI/USER | Fechamento (s) |")
    print("|---|---:|---:|---:|---:|---:|---:|---:|---:|")
    for label in ["x64-before", "x64-after", "x64-final", "x86-before", "x86-after"]:
        r = rows(f"startup-{label}.jsonl")
        if not r:
            continue
        m = lambda f: st.median(f(x) for x in r)
        print(f"| {label} | {m(lambda x: x['window_visible_s']):.3f} | {m(lambda x: x['responsive_s']):.3f} | "
              f"{m(lambda x: x['settled']['working_set']) / MiB:.1f} | {m(lambda x: own_private(x['settled'])):.1f} | "
              f"{m(lambda x: x['settled']['threads']):.0f} | {m(lambda x: x['settled']['handles']):.0f} | "
              f"{m(lambda x: x['settled']['gdi']):.0f}/{m(lambda x: x['settled']['user']):.0f} | {m(lambda x: x['close_s']):.3f} |")


def scenarios():
    print("\n### CPU e memória por cenário (60 s cada; CPU em % de um núcleo)\n")
    files = sorted(glob.glob(os.path.join(RES, "scen-*.jsonl")))
    data = {os.path.basename(f)[5:-6]: [r for r in rows(os.path.basename(f)) if r["kind"] == "scenario"] for f in files}
    names = [r["scenario"] for r in next(iter(data.values()), [])]
    labels = list(data)
    print("| Cenário | " + " | ".join(labels) + " |")
    print("|---|" + "---:|" * len(labels))
    for n in names:
        cells = []
        for l in labels:
            r = next((x for x in data[l] if x["scenario"] == n), None)
            cells.append("—" if not r else f"{r['cpu_pct_one_core_mean']:.2f}% · {r['end']['working_set'] / MiB:.1f} WS · "
                         f"{own_private(r['end']):.1f} P* · {r['end']['threads']} thr")
        print(f"| {n} | " + " | ".join(cells) + " |")


def continuity():
    print("\n### Continuidade no dispositivo real (loopback WASAPI, janelas de 2 ms)\n")
    print("| Rodada | Arquivo | Saturação CPU | Ativo (s) | Ações UI | Janelas < 50% | Eventos |")
    print("|---|---|---:|---:|---:|---:|---:|")
    for name in ["continuity.jsonl", "continuity-stream.jsonl"]:
        for r in rows(name):
            print(f"| {r['label']} | {r['track']} | {r['cpu_saturation_processes']} | {r['active_seconds']:.0f} | "
                  f"{r['ui_actions']} | {r['dropout_windows_2ms']} | {r['dropout_events']} |")


def soak():
    for name in sorted(glob.glob(os.path.join(RES, "soak-*.jsonl"))):
        r = [x for x in rows(os.path.basename(name)) if x["kind"] == "soak"]
        if len(r) < 3:
            continue
        print(f"\n### Sessão prolongada: {os.path.basename(name)}\n")
        print("| Minuto | WS (MiB) | Privados* (MiB) | Handles | GDI | USER | Threads | CPU 10 s (%) | Resposta (ms) |")
        print("|---:|---:|---:|---:|---:|---:|---:|---:|---:|")
        for x in r:
            minute = x["t"] / 60
            if x is r[0] or x is r[-1] or abs(minute - round(minute / 10) * 10) < 0.09:
                print(f"| {minute:.1f} | {x['working_set'] / MiB:.1f} | {own_private(x):.1f} | {x['handles']} | {x['gdi']} | "
                      f"{x['user']} | {x['threads']} | {x.get('cpu_pct_10s', 0):.2f} | {x.get('response_ms', 0):.1f} |")
        tail = r[len(r) // 6:]
        cpu = [x["cpu_pct_10s"] for x in tail if "cpu_pct_10s" in x]
        print(f"\nCPU média (após 1/6 inicial): {st.mean(cpu):.2f}% · máx 10 s: {max(cpu):.2f}% · "
              f"respostas falhas: {sum(1 for x in r if x.get('responsive') is False)} · amostras: {len(r)}")
        first, last = tail[0], r[-1]
        print(f"Variação WS {first['working_set'] / MiB:.1f} → {last['working_set'] / MiB:.1f} MiB; "
              f"privados* {own_private(first):.1f} → {own_private(last):.1f} MiB; handles {first['handles']} → {last['handles']}; "
              f"GDI {first['gdi']} → {last['gdi']}; USER {first['user']} → {last['user']}; threads {first['threads']} → {last['threads']}")


if __name__ == "__main__":
    startup()
    scenarios()
    continuity()
    soak()
