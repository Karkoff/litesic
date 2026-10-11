# ROADMAP — LiteSic

Índice das milestones. Cada uma tem um arquivo próprio em
`docs/milestones/` com a lista detalhada de tarefas (cada tarefa é um
checkbox `- [ ]`, marcado `- [x]` quando concluída).

Convenção de ID de tarefa: `M<n>-T<n>` (ex.: `M2-T3`). Convenção de commit:
`M<n>-T<n>: <resumo curto>`.

Milestones marcadas com 🖥️ terminam em algo que dá para abrir e olhar —
ao concluir a última tarefa da milestone, a skill de execução de tarefas
deve sugerir rodar o `.exe` para validação manual antes de seguir para a
próxima.

| Milestone | Arquivo | Conteúdo | 🖥️ |
|---|---|---|---|
| M0 | [docs/milestones/M0.md](milestones/M0.md) | Fundação: estrutura de pastas, Makefile, `.vscode/*`, manifest, third_party | não |
| M1 | [docs/milestones/M1.md](milestones/M1.md) | Esqueleto: janela sem borda, arrastável, ícone, fechar/minimizar | sim |
| M2 | [docs/milestones/M2.md](milestones/M2.md) | Motor de áudio: abrir/tocar/pausar/parar, posição/duração | sim |
| M3 | [docs/milestones/M3.md](milestones/M3.md) | Player principal: LCD, título rolando, kbps/kHz/estéreo, seek, volume, balanço, prev/next, shuffle/repeat | sim |
| M4 | [docs/milestones/M4.md](milestones/M4.md) | Visualizador: espectro FFT + osciloscópio | sim (validada) |
| M5 | [docs/milestones/M5.md](milestones/M5.md) | Lista de reprodução: add/drag&drop, reordenar, M3U, tags em background — COMPLETE, checkpoint manual PASS | sim |
| M6 | [docs/milestones/M6.md](milestones/M6.md) | Equalizador: 10 bandas, pré-amp, presets, tempo real | sim |
| M7 | [docs/milestones/M7.md](milestones/M7.md) | Integração: snap/dock entre janelas, mostrar/ocultar EQ/PL | sim |
| M8 | [docs/milestones/M8.md](milestones/M8.md) | Acabamento: atalhos estilo Winamp, teclas de mídia, always-on-top, instância única, persistência | sim |
| M9 | [docs/milestones/M9.md](milestones/M9.md) | Testes de compatibilidade (VM Win7 32/64, checagem de DLLs importadas) | manual/VM |
| M10 | [docs/milestones/M10.md](milestones/M10.md) | Distribuição: builds 32/64, zip portátil, instalador Inno Setup | build final |

## Modernização transversal UI/UX — IN_PROGRESS

As fases UI-00..08 mantêm as milestones acima. Player/playlist já usam
Direct2D/DirectWrite; pesquisa preserva índices canônicos e docking da M5.
UI-00..02 concluídas; UI-03..08 ainda IN_PROGRESS. M6/DSP não implementado:
EQ permanece indisponível. M5 tem checkpoint humano PASS; M9 permanece
pendente de VM Windows 7. Ver [plano](design/LITESIC-UI-IMPLEMENTATION-PLAN.md)
e [relatório com testes/capturas/limitações](design/UI-VALIDATION-2026-10-11.md).
Encerramento atualizado da M5, investigação de memória e preparação da M6:
[relatório técnico](M5-CLOSEOUT-2026-10-11.md). M6 não iniciada.

## Ampliação visual — temas

36 temas implementados, com aplicação imediata, persistência e seletor de
categorias/prévias. Quatro builds e 356 verificações passaram; 27 guardas de
automação e carga sustentada de 1.980 trocas por variante passaram. Aceite
visual humano dos novos temas pendente; M5 funcional mantém seu aceite
anterior. M6/DSP não iniciada. [Arquitetura, catálogo e evidências](THEMES.md).
Complemento entregue: seleção primária restaurada ao reabrir, comandos da
playlist no rodapé, dropdown Adicionar e seletor/menus com tema compartilhado.
[Relatório e nova validação nas quatro variantes](PLAYLIST-UI-2026-10-11.md).

## Futuro (fora do roadmap atual)

Streaming (rádios HTTP/Icecast), skins, mini-player, bandeja do sistema,
associação de arquivos, suporte a AAC/M4A (Media Foundation) e Opus.
