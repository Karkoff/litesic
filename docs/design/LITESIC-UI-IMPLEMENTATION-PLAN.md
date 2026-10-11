# Modernização UI/UX — plano e evidências

Referência PNG aberta e inspecionada em 11/10/2026, antes de alterar fontes.
A especificação fornecida existe como `LITESIC-UI-UX-PROMPT-CODEX.md`;
`IMPLEMENTACAO-UI-UX-CODEX.md` não existe neste checkout. Aplicam-se as fases
UI-00 a UI-08 e os critérios do documento disponível, sem renomear o original.

## Baseline UI-00

Branch master, HEAD 0440a5d. Há trabalho local de infraestrutura, áudio,
playlist e docking. Inventário, diff e cópia dos fontes anteriores em
`build/ui-baseline/`. Nada revertido, nenhum commit/push autorizado.
Quatro builds e quatro suítes passaram: 73 casos/variante, 292 execuções.
Logs `build/ui-baseline/{build,test}-{32,64}-{debug,release}.log`.
Captura real `build/gui/ui-00-before.png`: 413 × 270 px, 150%, player
275 × 180 DIP. Player GDI, busca/volume/pan/transporte, FFT/OSC; playlist
em janela própria, seleção/worker/M3U e magnetismo já implementados.
Não há EQ/DSP nem persistência. Metadados do modelo têm título/duração;
artista/álbum não devem ser inventados.

## Matriz de execução

| Fase/requisito | Arquivos | Dependência | Teste/evidência | Status |
|---|---|---|---|---|
| UI-00 inventário/baseline | este plano, build/ui-baseline | nenhuma | 292 aprovações, ui-00-before.png | COMPLETE |
| UI-01 tokens/primitivas | ui_theme.h, ui_draw.* | UI-00 | quatro builds, guia de tokens, capturas reais | COMPLETE |
| UI-02 D2D/DirectWrite | ui_draw.*, Makefile | UI-01 | pintura real nas quatro variantes; resize/recriação em teste de produção | COMPLETE |
| UI-03 player | ui_player.*, mainwnd.* | UI-02 | transporte/seek/FFT, screenshots | IN_PROGRESS |
| UI-04 playlist | plwnd.*, playlist_view.* | UI-02 | filtro canônico, seleção, docking | IN_PROGRESS |
| UI-05 EQ | eqwnd.* | UI-02, M6 DSP | indisponível até DSP testado | IN_PROGRESS |
| UI-06 coerência/dock | mainwnd.*, plwnd.*, eqwnd.* | UI-03..05 | janelas reais, movimento | IN_PROGRESS |
| UI-07 acabamento | componentes/layout | UI-06 | foco, DPI, perf, títulos | IN_PROGRESS |
| UI-08 auditoria | UI-VALIDATION-2026-10-11.md | UI-07 | 320 aprovações; 142 auditorias; fechamento depende das pendências UI-03..07 | IN_PROGRESS |

Estado da entrega: **parcial e funcional**, sem declarar todas as fases concluídas.
UI-03/04 têm o novo desenho e integração real; faltam acabamento completo de
acessibilidade, tooltips da playlist e matriz visual em todos os DPIs.
UI-05 é somente o painel explicitamente indisponível permitido pela especificação;
DSP, presets reais e transições de ganho ainda não foram implementados/testados.
UI-06 tem prova real de movimento/desprendimento/reencaixe da playlist; fechamento
global das três janelas depende do EQ funcional. UI-07 requer investigação do
aumento de memória e ampliação das medições. UI-08 tem auditoria executada,
mas não significa aceite de produto. Nenhuma fase BLOCKED por falta de autorização.

Relatório, diferenças visuais e próximos critérios verificáveis:
[UI-VALIDATION-2026-10-11.md](UI-VALIDATION-2026-10-11.md).

## Relação com milestones e riscos

UI-03 moderniza M3/M4; UI-04 mantém M5 IN_PROGRESS e seu aceite manual.
UI-05 corresponde à M6: só disponibilizar EQ com DSP real comprovado,
ou apresentar painel explicitamente indisponível como permitido na especificação.
UI-06 contribui para M7 sem declarar toda a integração concluída.
UI-07 cruza M8 (persistência/instância única continuam fora deste corte).
M9 continua PENDENTE DE VALIDAÇÃO EM WINDOWS 7; M10 depende dos marcos anteriores.
Não alterar checkboxes históricos por aprovação visual do agente.

Renderização D2D 1.0 em ID2D1DCRenderTarget mantém PrintWindow e fallback
GDI durante migração; DC bind por pintura/tamanho, formatos DirectWrite em cache,
descarte de recursos no device-loss. APIs documentadas pela Microsoft:
[interop](https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-and-gdi-interoperation-overview),
[DC target](https://learn.microsoft.com/en-us/windows/win32/api/d2d1/nn-d2d1-id2d1dcrendertarget),
[DirectWrite](https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nn-dwrite-idwritefactory).
Isso não comprova compatibilidade em VM Win7 nem GPU acelerada.
Validação no desktop interativo somente pelo helper com revisão da plataforma.
Aceite estético humano permanece pendente.
