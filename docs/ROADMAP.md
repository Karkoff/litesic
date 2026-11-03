# ROADMAP — LiteSic

**Correção 1.0.1 (03/11/2026):** exportação Geral/Favoritas corrigida e
salvamentos aceitos preservados ao limpar/fechar. 372 casos e quatro pacotes
x64/x86 padrão/portátil validados. [Relatório](PLAYLIST-EXPORT-1.0.1.md).
Esta correção não reabre milestones encerradas.

**Atualização 03/11/2026:** M10 em andamento; pacote Microsoft Store validado
localmente e Submission 1 em rascunho, sem envio ou publicação.
Distribuição desktop 1.0.0 final estável publicada no GitHub: quatro ZIPs x64/x86 padrão/portátil
verificados; [procedimento e limites](DESKTOP-RELEASE-2026-11-03.md).
[Estado e procedimento](STORE-PUBLICATION.md). Os registros abaixo preservam
os encerramentos e planejamentos anteriores.

Índice das milestones. Cada uma tem um arquivo próprio em
`docs/milestones/` com a lista detalhada de tarefas (cada tarefa é um
checkbox `- [ ]`, marcado `- [x]` quando concluída).

**M8 — COMPLETE; aceite humano PASS (12/10/2026).**
Decisão expressa do usuário: **M8-DEC-2026-10-12**, solicitação
“LITESIC — ENCERRAMENTO DEFINITIVO DA M8”. Resultados técnicos, evidências
originais e aceites anteriores preservados. Validações físicas, assistivas e
linguísticas não realizadas são observações futuras não bloqueantes e não
reabrem automaticamente a milestone; não são convertidas em PASS técnico.
M0–M7 e M9 permanecem COMPLETE. **Auditoria Geral de Desempenho:
COMPLETE_WITH_CAVEATS** (12/10/2026, PERF-DEC-2026-10-12;
[relatório](PERFORMANCE-AUDIT.md)). Próxima atividade: **M10 — Distribuição**,
não iniciada; será autorizada separadamente.
Neste encerramento não foram repetidos testes ou auditorias nem implementadas
funcionalidades. Consolidação, commit e push para `origin/master` autorizados.

Os baselines e pendências abaixo são históricos; o encerramento acima prevalece
para o estado atual. [Relatório final M8](M8-COMPLETION.md).

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
| M6 | [docs/milestones/M6.md](milestones/M6.md) | Equalizador real — COMPLETE, aceite final visual, audível e funcional informado pelo usuário | sim |
| M7 | [docs/milestones/M7.md](milestones/M7.md) | Integração: pares completos, movimento transitivo e compactação — COMPLETE, aceite humano TUDO PASS | sim |
| M8 | [docs/milestones/M8.md](milestones/M8.md) | Acabamento — COMPLETE; aceite humano PASS; observações futuras não bloqueantes | sim |
| M9 | [docs/milestones/M9.md](milestones/M9.md) | Compatibilidade — COMPLETE por decisão explícita; Windows 11 confirmado; testes Windows 7/8/8.1/10 e x86/x64 pendentes, não bloqueantes | encerrada por decisão |
| M10 | [docs/milestones/M10.md](milestones/M10.md) | Distribuição: builds 32/64, pacote limpo reproduzível, portátil e instalador opcional — aberta | build final |

## M9 — encerramento por decisão do usuário

**Status: COMPLETE, por decisão explícita em 11/10/2026.**
Alvo de compatibilidade: **Windows 7–11, x86/x64**.
Windows 11: funcionamento confirmado pelo usuário, sem atribuir essa
confirmação a uma arquitetura específica. Windows 7, 8, 8.1 e 10:
testes práticos pendentes. Validação específica x86/x64: pendente.

O usuário realizará os testes restantes futuramente. Essas pendências não
bloqueiam o avanço do projeto nem reabrem automaticamente a M9. COMPLETE
registra o encerramento da milestone, não comprovação de compatibilidade
em ambientes ainda não testados. Nenhum código foi alterado e nenhum teste
foi executado nesta tarefa de documentação.

Decisão rastreável: **M9-DEC-2026-10-11**, solicitação do usuário
“LiteSic — Persistência do encerramento da M9 (Compatibilidade)”.
Ver [estado canônico](../PROJECT_STATE.md) e
[registro de encerramento](M9-CLOSEOUT-2026-10-11.md).

## Modernização transversal UI/UX — IN_PROGRESS

**M0–M5: COMPLETE. M6: COMPLETE. M9: COMPLETE.**
O usuário informou o aceite final visual, audível, funcional e de uso
prolongado da M6 em 11/10/2026, substituindo READY_FOR_HUMAN_ACCEPTANCE.
[Encerramento e validação final](M6-CLOSEOUT-2026-10-11.md).
A [auditoria sobre a35c993](M6-INTEGRATED-VALIDATION-2026-10-11.md) mantém
suas medições e pendências históricas, sem atribuir medições ao aceite humano.
**M7 — Integração das Janelas: COMPLETE**, com aceite humano final
**“TUDO PASS”** concedido pelo usuário em 11/10/2026.
[Encerramento formal e testes atuais](M7-CLOSEOUT-2026-10-11.md).
Ver [validação M7](M7-INTEGRATED-VALIDATION.md) e
[matriz de requisitos, com baseline histórico de 48 tarefas](ROADMAP-IMPLEMENTATION-AUDIT.md).
Histórico anterior ao encerramento: M8-T6 COMPLETE: [18 catálogos e tipografia](M8-T6-I18N.md),
quatro builds e 644 casos PASS no baseline T6. **M8-T7 COMPLETE:**
[seleção Windows/manual e atualização ao vivo](M8-T7-LANGUAGE-SELECTION.md),
191 IDs por catálogo, quatro builds, 684 casos e 32 guardas PASS.
**M8-T8 COMPLETE, aceite visual humano TUDO PASS:** [tooltips](M8-T8-TOOLTIPS.md),
46 chaves novas, 237 IDs × 18 idiomas, 36 temas, quatro builds, 716 casos,
36 guardas e dois casos gráficos adicionais PASS (3.092 frames). Aceite visual
humano T8 concedido pelo usuário; T7 tem PASS humano explícito. M8-T1…T5 e checkpoint
continuavam pendentes nesse baseline; revisão linguística humana como observação futura não bloqueante. O aceite visual
não substitui a revisão profissional dos 18 idiomas. [Consolidação](M8-CONSOLIDATION.md).

As fases UI-00..08 mantêm as milestones acima. Player/playlist já usam
Direct2D/DirectWrite; pesquisa preserva índices canônicos e docking da M5.
UI-00..02 concluídas; UI-03..08 ainda IN_PROGRESS. M6 tem DSP real e painel
operável, com funcionamento confirmado pelo usuário. M5 tem checkpoint humano PASS; M9 está COMPLETE por decisão explícita do usuário; os testes práticos
de Windows 7/8/8.1/10 e a validação específica x86/x64 permanecem pendentes,
sem bloquear o avanço ou reabrir automaticamente a milestone. Ver [plano](design/LITESIC-UI-IMPLEMENTATION-PLAN.md)
e [relatório com testes/capturas/limitações](design/UI-VALIDATION-2026-10-11.md).
Encerramento atualizado da M5, investigação de memória e preparação da M6:
[relatório técnico](M5-CLOSEOUT-2026-10-11.md). Atualização da M6:
[implementação, testes e medições](M6-EQUALIZER-2026-10-11.md).

## Ampliação visual — temas

36 temas implementados, com aplicação imediata, persistência e seletor de
categorias/prévias. Quatro builds e 356 verificações passaram; 27 guardas de
automação e carga sustentada de 1.980 trocas por variante passaram. Aceite
visual humano dos novos temas estava pendente nesse relatório histórico;
o aceite visual final da M6 foi posteriormente informado pelo usuário, sem
registro individual dos 36 temas. M5 funcional mantém seu aceite anterior. M6 implementada e aceita pelo usuário; ajustes de encaixe documentados em [WINDOW-SNAP](WINDOW-SNAP-2026-10-11.md).
[Arquitetura, catálogo e evidências](THEMES.md).
Complemento entregue: seleção primária restaurada ao reabrir, comandos da
playlist no rodapé, dropdown Adicionar e seletor/menus com tema compartilhado.
[Relatório e nova validação nas quatro variantes](PLAYLIST-UI-2026-10-11.md).

## Auditoria das entregas antecipadas — 11/10/2026

Na auditoria de 11/10/2026, M8 estava aberta: zoom/DPI e persistência parcial já existem; faltam
atalhos globais, teclas de mídia, sempre no topo, instância única e conclusão
da persistência/modo portátil. Acrescentados M8-T6…T8: internacionalização
integral em 18 idiomas, detecção Windows/manual, troca ao vivo e tooltips.
[Escopo e aceitação detalhados](M8-I18N-REQUIREMENTS.md).
M10 permanece aberta: builds existem, mas
reprodutibilidade formal, zips portáteis e eventual instalador não foram
concluídos. M10-T2 exige separar build interno e distribuição, somente
LiteSic.exe na raiz, recursos/locales/licenças/dados necessários em subpastas,
validação do pacote e execução final portátil/instalada. Ver [M10](milestones/M10.md).
M9 continua COMPLETE por decisão anterior; os ensaios físicos
pendentes não foram inventados nem executados neste ciclo.
Detalhes por tarefa na [auditoria](ROADMAP-IMPLEMENTATION-AUDIT.md).

## Futuro (fora do roadmap atual)

Streaming (rádios HTTP/Icecast), skins, mini-player, bandeja do sistema,
associação de arquivos, suporte a AAC/M4A (Media Foundation) e Opus.

[Auditoria integral restante da M8 e plano de conclusão](M8-REMAINING-REQUIREMENTS-AUDIT.md):
T1–T8 COMPLETE; aceite humano final PASS. Consolidação T6–T8 publicada em
`1aa560f`; resultados e baselines históricos preservados.
