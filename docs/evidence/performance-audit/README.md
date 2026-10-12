# Evidências — Auditoria Geral de Desempenho (12/10/2026)

Resultados brutos (JSON Lines) usados em [PERFORMANCE-AUDIT.md](../../PERFORMANCE-AUDIT.md).
"before" = referência `9b2689b` reconstruída; "after"/"final" = correções aprovadas.

| Arquivos | Origem | Conteúdo |
|---|---|---|
| `startup-*`, `scen-*`, `soak-*` | `perf_session.py` (executável real) | Inicialização, CPU/memória por cenário, sessão prolongada (34,8 min) |
| `continuity*`, `seekgap` | `perf_session.py continuity` + `loopback_probe` | Quedas de áudio no dispositivo real; lacuna por seek |
| `canvas-*`, `paint-*`, `memory-stages-*` | `perf_probe` | Custo do Direct2D por canvas, pintura, memória por estágio |
| `dsp-64`, `graph-*`, `meta-64-before` | `perf_probe` | Equalizador, grafo de áudio por formato, metadados |
| `graph-64-stream` | `perf_probe` (variante rejeitada) | Streaming para todos os arquivos, leitura sem ritmo: blocos silenciosos |
| `stress-64-before` | `perf_probe stress` (dispositivo simulado) | Áudio × UI sob carga |
| `playlist-*`, `session-*`, `cycles-*`, `languages100-*` | `perf_probe` | Playlist, sessão, temas/idiomas/janelas/tooltips |
| `title-scroll*.json` | `perf_session.py` (executável real) | CPU com título longo |

`foreign_image_commit` = commit de DLLs de terceiros injetadas (RTSS neste computador).
