# Auditoria de implementação do roadmap — LiteSic

Data: 11/10/2026. Base efetiva: `master`, HEAD `82d28ce` (posterior ao
`3ca057a` informado no pedido); working tree inicialmente limpa na rodada histórica.
Esta matriz foi atualizada no encerramento formal da M7. Baseline histórica x64 Release: 136 casos/23 binários,
PASS, sem capturas. Histórico M6: 544 aprovações nas quatro variantes.

## Critério e fontes

Baseline: 48 tarefas M0-T1…M10-T3. Total atual: 51 tarefas, com
M8-T6/T7/T8 acrescentadas sem renumerar as anteriores; checklists detalham
sua aceitação e não são contados novamente. Todos os 11 arquivos de milestone,
ROADMAP, README, AGENT, PLANO-VSCODE, relatórios M5/M6, documentos UI/automação,
fontes, Makefile e testes foram inspecionados. ALREADY_ACCEPTED preserva aceite
documentado, sem alegar repetição manual nesta rodada. Existência de código
sem cobertura do contrato completo permanece PARTIALLY_IMPLEMENTED ou
IMPLEMENTED_NOT_VERIFIED. BLOCKED abaixo descreve ensaio indisponível, nunca
reabertura da M9. Sem percentuais arbitrários.

A matriz distingue o estado documental de entrada e o estado técnico após
o trabalho M7. A evidência final e os limites ficam em
[M7-INTEGRATED-VALIDATION.md](M7-INTEGRATED-VALIDATION.md).

## Matriz por requisito

| Milestone | ID | Descrição | Documento anterior | Estado técnico | Arquivos/funções | Testes | Evidências | Lacunas | Ação | Impacto |
|---|---|---|---|---|---|---|---|---|---|---|
| M0 | M0-T1 | Estrutura de pastas | COMPLETE; [x] | ALREADY_ACCEPTED | Makefile, .vscode/, res/, src/, third_party/ | builds; configuração versionada | docs/milestones/M0.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M0 | M0-T2 | Vendorizar miniaudio e stb_vorbis | COMPLETE; [x] | ALREADY_ACCEPTED | third_party/README.md, miniaudio.h, stb_vorbis.c, src/miniaudio_impl.c | build_third_party_smoke.c | docs/milestones/M0.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M0 | M0-T3 | Makefile | COMPLETE; [x] | ALREADY_ACCEPTED | Makefile, scripts/dev.ps1 | quatro variantes make all test | docs/milestones/M0.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M0 | M0-T4 | Configuração do VS Code | COMPLETE; [x] | ALREADY_ACCEPTED | .vscode/tasks.json, launch.json, c_cpp_properties.json, extensions.json | aceite histórico M0; inspeção das tasks | docs/milestones/M0.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M0 | M0-T5 | Recursos: manifest, ícone, app.rc | COMPLETE; [x] | ALREADY_ACCEPTED | res/app.rc, app.manifest, icon.ico | aceite M0; binário PE | docs/milestones/M0.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M1 | M1-T1 | Janela principal sem borda | COMPLETE; [x] | ALREADY_ACCEPTED | src/app.cpp, mainwnd.cpp: MainWnd_Create | test_player_window.cpp | docs/milestones/M1.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M1 | M1-T2 | Barra de título arrastável | COMPLETE; [x] | ALREADY_ACCEPTED | src/mainwnd.cpp: WM_NCHITTEST | aceite M1; teste Win32 de movimentação | docs/milestones/M1.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M1 | M1-T3 | Botões fechar/minimizar desenhados | COMPLETE; [x] | ALREADY_ACCEPTED | src/mainwnd.cpp: botões de título | aceite M1; test_window_integration.cpp | docs/milestones/M1.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M1 | M1-T4 | Ícone na aplicação e na taskbar | COMPLETE; [x] | ALREADY_ACCEPTED | res/app.rc, src/mainwnd.cpp: registro da classe | aceite histórico M1 | docs/milestones/M1.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M2 | M2-T1 | Inicializar miniaudio (engine + device) | COMPLETE; [x] | ALREADY_ACCEPTED | src/audio.cpp: Audio_Init/Shutdown | test_audio_integration.cpp; aceite M2 | docs/milestones/M2.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M2 | M2-T2 | Abrir e decodificar um arquivo | COMPLETE; [x] | ALREADY_ACCEPTED | src/audio.cpp: Audio_Load; src/miniaudio_impl.c | test_audio_integration.cpp; smoke; quatro codecs | docs/milestones/M2.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M2 | M2-T3 | Play / pause / stop / posição | COMPLETE; [x] | ALREADY_ACCEPTED | src/audio.cpp: Audio_Play/Pause/Stop | test_audio_integration.cpp, test_playback.cpp | docs/milestones/M2.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M2 | M2-T4 | Ligar à UI: abrir arquivo, play/pause/stop | COMPLETE; [x] | ALREADY_ACCEPTED | src/mainwnd.cpp, plwnd.cpp: abrir/transportes | test_player_window.cpp; aceite histórico | docs/milestones/M2.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M3 | M3-T1 | Display LCD de tempo | COMPLETE; [x] | ALREADY_ACCEPTED | src/util.cpp, ui_player.cpp, mainwnd.cpp | test_format_time.c, test_player_window.cpp | docs/milestones/M3.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M3 | M3-T2 | Título rolando (scrolling text) | COMPLETE; [x] | ALREADY_ACCEPTED | src/title_scroll.h, mainwnd.cpp: UpdateScrollTimer | test_title_scroll.cpp | docs/milestones/M3.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M3 | M3-T3 | Indicador kbps/kHz/estéreo | COMPLETE; [x] | ALREADY_ACCEPTED | src/audio.cpp, tags.cpp, ui_player.cpp | test_audio_integration.cpp, test_tags.cpp | docs/milestones/M3.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M3 | M3-T4 | Barra de busca (seek) | COMPLETE; [x] | ALREADY_ACCEPTED | src/mainwnd.cpp, audio.cpp: seek | test_player_window.cpp | docs/milestones/M3.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M3 | M3-T5 | Volume e balanço | COMPLETE; [x] | ALREADY_ACCEPTED | src/mainwnd.cpp, audio.cpp: volume/pan | test_player_window.cpp, test_audio_integration.cpp | docs/milestones/M3.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M3 | M3-T6 | Anterior/próxima, aleatório/repetir | COMPLETE; [x] | ALREADY_ACCEPTED | src/playlist.cpp, playback.cpp, mainwnd.cpp | test_playlist.cpp, test_playback.cpp | docs/milestones/M3.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M4 | M4-T1 | Buffer de amostras do callback para a UI | COMPLETE; [x] | ALREADY_ACCEPTED | src/audio.cpp: tap e ring buffer | test_ring_buffer.c, test_audio_integration.cpp | docs/milestones/M4.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M4 | M4-T2 | Osciloscópio | COMPLETE; [x] | ALREADY_ACCEPTED | src/visualizer.cpp, ui_player.cpp | test_map_sample_to_y.c; aceite histórico M4 | docs/milestones/M4.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M4 | M4-T3 | Espectro via FFT | COMPLETE; [x] | ALREADY_ACCEPTED | src/visualizer.cpp | test_fft.c; aceite M4 | docs/milestones/M4.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M4 | M4-T4 | Alternar por clique | COMPLETE; [x] | ALREADY_ACCEPTED | src/mainwnd.cpp: FFT/OSC | test_player_window.cpp, test_appearance.cpp | docs/milestones/M4.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M5 | M5-T1 | Modelo de dados da playlist | COMPLETE; [x] | ALREADY_ACCEPTED | src/playlist.h/.cpp | test_playlist.cpp | docs/milestones/M5.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M5 | M5-T2 | Janela da lista (plwnd.cpp) | COMPLETE; [x] | ALREADY_ACCEPTED | src/plwnd.cpp: Proc/Paint | test_playlist_window.cpp | docs/milestones/M5.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M5 | M5-T3 | Adicionar arquivos/pasta e drag & drop | COMPLETE; [x] | ALREADY_ACCEPTED | src/plwnd.cpp, playlist_worker.cpp | test_playlist_window.cpp, test_playlist_worker.cpp; aceite Explorer | docs/milestones/M5.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M5 | M5-T4 | Interação: duplo clique, delete, seleção múltipla, reordenar | COMPLETE; [x] | ALREADY_ACCEPTED | src/plwnd.cpp, playlist.cpp | test_playlist_window.cpp, test_playlist.cpp; aceite M5 | docs/milestones/M5.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M5 | M5-T5 | Ordenar/embaralhar e M3U | COMPLETE; [x] | ALREADY_ACCEPTED | src/m3u.cpp, playlist.cpp | test_m3u.cpp; aceite M3U externo | docs/milestones/M5.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M5 | M5-T6 | Duração e tags em segundo plano | COMPLETE; [x] | ALREADY_ACCEPTED | src/tags.cpp, playlist_worker.cpp | test_tags.cpp, test_playlist_worker.cpp; regressão pesquisa grande | docs/milestones/M5.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M6 | M6-T1 | Janela do equalizador (eqwnd.cpp) | COMPLETE; [x] | ALREADY_ACCEPTED | src/eqwnd.cpp, ui_draw.cpp | test_eq_window.cpp; aceite final M6 | docs/milestones/M6.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M6 | M6-T2 | 10 bandas com filtros em tempo real | COMPLETE; [x] | ALREADY_ACCEPTED | src/eq_dsp.cpp, audio.cpp | test_eq_filter.cpp, test_eq_audit.cpp, test_eq_audio.cpp | docs/milestones/M6.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M6 | M6-T3 | Curva desenhada | COMPLETE; [x] | ALREADY_ACCEPTED | src/eqwnd.cpp: WM_PAINT | test_eq_window.cpp; aceite visual M6 | docs/milestones/M6.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M6 | M6-T4 | Presets | COMPLETE; [x] | ALREADY_ACCEPTED | src/eq_state.cpp, eqwnd.cpp | test_eq_filter.cpp, test_eq_audit.cpp; aceite M6 | docs/milestones/M6.md; M6-CLOSEOUT para regressões integradas | Sem lacuna bloqueante registrada para a milestone encerrada. | Preservar implementação e aceite; executar regressões. | Não reabrir milestones aceitas. |
| M7 | M7-T1 | Extrair base comum de janela (skin.h/.cpp) | [ ]; encaixe direto e toggles antecipados | ALREADY_ACCEPTED | src/skin.h/.cpp, ui_draw.h/.cpp | test_window_integration.cpp; suítes UI existentes | M7-INTEGRATED-VALIDATION; logs/testes Win32 atuais | Aceite humano final “TUDO PASS” em 11/10/2026. | Preservar aceite; ver M7-CLOSEOUT-2026-10-11. | M7 COMPLETE; M8 apenas planejada. |
| M7 | M7-T2 | Encaixe (snap) entre janelas | [ ]; encaixe direto e toggles antecipados | ALREADY_ACCEPTED | src/window_snap.h: Snap/Graph, skin.cpp: SnapToPeers | test_window_snap.cpp; test_window_integration.cpp | M7-INTEGRATED-VALIDATION; logs/testes Win32 atuais | Aceite humano final “TUDO PASS” em 11/10/2026. | Preservar aceite; ver M7-CLOSEOUT-2026-10-11. | M7 COMPLETE; M8 apenas planejada. |
| M7 | M7-T3 | Mover em conjunto | [ ]; encaixe direto e toggles antecipados | ALREADY_ACCEPTED | src/skin.cpp: Changed/Constrain/Reconcile | test_window_integration.cpp; test_eq_window.cpp | M7-INTEGRATED-VALIDATION; logs/testes Win32 atuais | Aceite humano final “TUDO PASS” em 11/10/2026. | Preservar aceite; ver M7-CLOSEOUT-2026-10-11. | M7 COMPLETE; M8 apenas planejada. |
| M7 | M7-T4 | Botões EQ/PL mostram/ocultam | [ ]; encaixe direto e toggles antecipados | ALREADY_ACCEPTED | src/mainwnd.cpp, eqwnd.cpp, plwnd.cpp: toggles | test_eq_window.cpp, test_player_window.cpp; sessão isolada | M7-INTEGRATED-VALIDATION; logs/testes Win32 atuais | Aceite humano final “TUDO PASS” em 11/10/2026. | Preservar aceite; ver M7-CLOSEOUT-2026-10-11. | M7 COMPLETE; M8 apenas planejada. |
| M8 | M8-T1 | Atalhos de teclado estilo Winamp | [ ]; milestone aberta | PARTIALLY_IMPLEMENTED | src/mainwnd.cpp: WM_KEYDOWN; eqwnd.cpp/plwnd.cpp | test_player_window.cpp (Tab/setas); sem teste ZXCVB global | Inspeção do código e testes listados; relatório M7 | Tab/Space/Enter e setas locais; faltam Z/X/C/V/B, L, Ctrl+D e despacho com foco nos três painéis. | Completar aceleradores na M8. | Não implementado nesta execução. |
| M8 | M8-T2 | Teclas de mídia do teclado | [ ]; milestone aberta | NOT_IMPLEMENTED | nenhum WM_APPCOMMAND no código | nenhum teste específico | Inspeção do código e testes listados; relatório M7 | Ausência de WM_APPCOMMAND e dos comandos multimídia. | Implementar na M8 e testar entrada. | Não altera a M7. |
| M8 | M8-T3 | Sempre no topo e zoom da interface | [ ]; milestone aberta | PARTIALLY_IMPLEMENTED | src/app.cpp: InitDpiAwareness; mainwnd.cpp: ApplyInterfaceScale | test_player_window.cpp, test_appearance.cpp, test_window_integration.cpp | Inspeção do código e testes listados; relatório M7 | DPI/zoom e sua persistência já existem; falta always-on-top para o conjunto e decisão de UI definitiva. | Reutilizar escala; completar toggle e aceite na M8. | DPI é reutilizado pela M7, sem concluir M8. |
| M8 | M8-T4 | Instância única | [ ]; milestone aberta | NOT_IMPLEMENTED | src/app.cpp: wWinMain sem mutex/WM_COPYDATA | nenhum teste específico | Inspeção do código e testes listados; relatório M7 | Sem CreateMutexW/WM_COPYDATA; argumentos normais não adicionam arquivos. | Implementar instância única e encaminhamento na M8. | Pré-requisito para experiência de distribuição. |
| M8 | M8-T5 | Configurações em INI e persistência da lista | [ ]; milestone aberta | PARTIALLY_IMPLEMENTED | src/ui_theme.cpp, eq_state.cpp, plwnd.cpp: Geometry/SaveM3U | test_appearance.cpp, test_eq_filter.cpp, test_playlist_window.cpp, test_window_integration.cpp | Inspeção do código e testes listados; relatório M7 | Volume, EQ, tema, zoom, visibilidades, favoritos, playlist e geometria dos painéis existem. Faltam modo portátil, balanço, shuffle/repeat e always-on-top; posição principal depende da sessão PL. Usa LOCALAPPDATA, não o contrato APPDATA/LiteSic.ini. | Consolidar a infraestrutura existente na M8; não criar INI concorrente. | Parent da M7 amplia as seções existentes; não implementa config.h ou modo portátil. |
| M8 | M8-T6 | Internacionalização integral, 18 idiomas, catálogo e DirectWrite/RTL | COMPLETE; [x]; 11/10/2026 | IMPLEMENTED_AND_VERIFIED | src/i18n.h/.cpp, i18n_english.h, locales, UI migrada | test_i18n.cpp, test_i18n_window.cpp, i18n_catalogs.py; 644 casos, 29 guardas e quatro builds PASS | M8-T6-I18N.md; evidence/m8-i18n (54 capturas inspecionadas) | Revisão linguística humana e RTL complexo pendentes; sem certificação Win7 prática | T7: detecção/menu/persistência/refresh global; T8: tooltips explicativos | 188/188 IDs por idioma; 36 temas × quatro escalas renderizados; M8 aberta; sem commit/push |
| M8 | M8-T7 | Detecção Windows, menu Idioma e seleção manual persistida/ao vivo | COMPLETE; [x]; 11/10/2026 | IMPLEMENTED_AND_VERIFIED | src/language_selection.h/.cpp, app.cpp, mainwnd.cpp, plwnd.cpp, eqwnd.cpp, theme_window.cpp; INI existente | test_language_selection.cpp (8), test_language_window.cpp (2), suites existentes; 684 casos, 32 guardas e quatro builds PASS | M8-T7-LANGUAGE-SELECTION.md; evidence/m8-language-selection (90 capturas, 50 inspecionadas representativamente) | Revisão linguística humana e ensaios físicos M9 pendentes | T8: tooltips explicativos; M8 permanece aberta | 18 escolhas nativas; 191 IDs; default Windows/manual; refresh cinco janelas, áudio/estado preservados; sem commit/push |
| M8 | M8-T8 | Tooltips multilíngues dos controles existentes | COMPLETE; [x]; aceite humano TUDO PASS | IMPLEMENTED_AND_VERIFIED | ui_tooltip.h/.cpp, Canvas::TextLayout, mainwnd/eqwnd/plwnd/theme_window, 18 catálogos | test_tooltip (8 + 2 gráficos), suítes 716 casos, 36 guardas, quatro builds PASS | M8-T8-TOOLTIPS.md; inventário; evidence/m8-tooltips, 10 imagens inspecionadas e 3.092 frames | Aceite visual humano T8 concedido; revisão linguística, UIA/MSAA Canvas e multi-DPI físico pendentes | M8-T1…T5 e checkpoint; M8 aberta | 46 IDs novos; 237 IDs × 18 idiomas; 36 temas; estados reais; áudio preservado; sem commit/push |
| M9 | M9-T1 | Checar DLLs importadas | [ ]; M9 administrativamente COMPLETE | IMPLEMENTED_AND_VERIFIED | Makefile: -static; res/app.manifest | objdump dos dois Release; evidência nesta rodada | Inspeção do código e testes listados; relatório M7 | Link estático e imports de sistema confirmados por objdump nesta rodada. | Manter checagem após troca de toolchain. | M9 permanece COMPLETE; checkpoint histórico não reescrito. |
| M9 | M9-T2 | Rodar em VM Windows 7 SP1 (32 e 64 bits) | [ ]; M9 administrativamente COMPLETE | BLOCKED | fontes compatíveis com alvo 0x0601; sem VM disponível | nenhum ensaio físico Win7 nesta rodada | Inspeção do código e testes listados; relatório M7 | VMs Win7 SP1 x86/x64 indisponíveis neste ambiente; ausência de evidência física. | Execução futura pelo usuário, conforme decisão M9. | Bloqueio do ensaio, não do status administrativo M9. |
| M9 | M9-T3 | Registrar e corrigir divergências | [ ]; M9 administrativamente COMPLETE | BLOCKED | docs/M9-CLOSEOUT-2026-10-11.md | nenhum diagnóstico novo entre sistemas | Inspeção do código e testes listados; relatório M7 | Comparação prática entre sistemas depende de ambientes ainda indisponíveis; ausência de falhas observadas não comprova compatibilidade. | Registrar divergências quando os ensaios futuros ocorrerem. | Não reabrir M9. |
| M10 | M10-T1 | Build Release reproduzível | [ ]; milestone aberta | PARTIALLY_IMPLEMENTED | Makefile: variantes isoladas e --no-insert-timestamp; README.md | builds atuais; hashes coincidentes na reconstrução pré-M7 | Inspeção do código e testes listados; relatório M7 | Build Release x64/x86 documentado e estático; linker sem timestamp. Ainda falta prova formal de duas compilações limpas independentes e sua comparação após M7. | Concluir ensaio de reprodutibilidade na M10. | Builds deste ciclo não encerram distribuição. |
| M10 | M10-T2 | Empacotar .zip portátil | [ ]; milestone aberta | NOT_IMPLEMENTED | nenhum script dist/package de produto; scripts/diagnostics empacotam evidências | nenhum zip portátil do produto validado | Inspeção do código e testes listados; relatório M7 | Sem zips do produto, pacote/licenças ou modo portátil conforme M8-T5. | Implementar empacotamento após modo portátil. | Depende do trabalho residual M8-T5. |
| M10 | M10-T3 | Instalador com Inno Setup (opcional) | [ ]; milestone aberta | NOT_IMPLEMENTED | nenhum installer/LiteSic.iss | nenhum ensaio de instalador | Inspeção do código e testes listados; relatório M7 | Instalador Inno Setup opcional ausente. | Decidir se será entregue na M10; só então implementar/testar. | Opcional; não é bloqueio da M7. |

## Resumo por milestone

Contagens atuais: 51 tarefas; 37 aceitas, três verificadas, quatro parciais,
cinco não implementadas e duas bloqueadas apenas no ensaio físico M9.
M7 incorpora o aceite humano final “TUDO PASS”.

| Milestone | Total | ALREADY_ACCEPTED | IMPLEMENTED_AND_VERIFIED | IMPLEMENTED_NOT_VERIFIED | PARTIALLY_IMPLEMENTED | NOT_IMPLEMENTED | BLOCKED | NOT_APPLICABLE |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| M0 | 5 | 5 | 0 | 0 | 0 | 0 | 0 | 0 |
| M1 | 4 | 4 | 0 | 0 | 0 | 0 | 0 | 0 |
| M2 | 4 | 4 | 0 | 0 | 0 | 0 | 0 | 0 |
| M3 | 6 | 6 | 0 | 0 | 0 | 0 | 0 | 0 |
| M4 | 4 | 4 | 0 | 0 | 0 | 0 | 0 | 0 |
| M5 | 6 | 6 | 0 | 0 | 0 | 0 | 0 | 0 |
| M6 | 4 | 4 | 0 | 0 | 0 | 0 | 0 | 0 |
| M7 | 4 | 4 | 0 | 0 | 0 | 0 | 0 | 0 |
| M8 | 8 | 0 | 2 | 0 | 3 | 3 | 0 | 0 |
| M9 | 3 | 0 | 1 | 0 | 0 | 0 | 2 | 0 |
| M10 | 3 | 0 | 0 | 0 | 1 | 2 | 0 | 0 |

## Funcionalidades antecipadas e limites de responsabilidade

- **M7:** matemática de snap/âncoras, docking direto, botões EQ/LISTA,
  janela independente e movimento direto existiam. O residual era base comum
  de arraste, pares playlist↔EQ, propagação transitiva e preservação dessas
  relações na infraestrutura já existente.
- **M8-T3:** DPI por monitor, conversão DIP/pixel, zoom manual e persistência
  da escala já implementados. Falta sempre no topo; não refazer zoom.
- **M8-T5:** aparência/volume/EQ e sessão M3U já persistem. Reutilizar
  ui_theme/eq_state e sidecar da playlist; faltam campos e contrato portátil.
- **M9-T1 / M10-T1:** variantes, recursos e link estático já disponíveis.
  Verificar imports não equivale a teste físico Win7 ou distribuição concluída.
- **Temas, favoritos e informações:** extensões aceitas na M6, além das
  tarefas originais; não constituem milestone futura a ser recriada.
- **Monitores/DPI na M7:** preservar acessibilidade e relações durante
  movimento, escala e restauração; topologias negativas/contíguas/com lacunas
  testadas por simulação, sem declarar ensaio físico multi-DPI.
- **UI-00…08:** modernização transversal conserva pendências de acessibilidade,
  tooltips e cobertura visual ampla; fechamento M7 não conclui essas fases.

## Sequência recomendada

1. M7 COMPLETE: aceite humano “TUDO PASS” já concedido; não solicitar novamente.
2. M8: completar atalhos globais e teclas de mídia; sempre no topo; instância
   única; consolidar persistência/modo portátil usando a infraestrutura existente.
3. M10: reprodutibilidade formal, zips e eventual instalador após modo portátil.
4. Testes físicos M9 continuam com o usuário, sem reabertura administrativa.

## Consolidação de encerramento — 11/10/2026

M7 COMPLETE: aceite humano e testes atuais no [encerramento](M7-CLOSEOUT-2026-10-11.md).
As 48 tarefas originais foram preservadas, com três novas tarefas M8 (51 atuais).
M8 continua aberta: DPI por monitor, zoom/persistência e refinamentos existentes
são reutilizados; atalhos parciais, mídia, topo, instância única e configuração
residual continuam pendentes. Na auditoria original, os 18 idiomas, seleção
automática/manual e tooltips ainda eram planejamento. Atualização posterior:
M8-T6, M8-T7 e M8-T8 foram implementadas e verificadas em 11/10/2026; seguem
a conclusão dos requisitos residuais M8-T1…T5 e checkpoint. Ver [entrega T6](M8-T6-I18N.md)
e [entrega T7](M8-T7-LANGUAGE-SELECTION.md). Os números T6 são históricos;
o estado atual T8 tem 237 IDs por catálogo e 716 casos funcionais nas quatro variantes,
36 guardas e dois casos gráficos adicionais. [Entrega T8](M8-T8-TOOLTIPS.md).
Aceite humano T7 PASS preservado; aceite visual humano T8 **TUDO PASS** concedido pelo usuário. Revisão linguística integral não foi concedida.

M10-T1 permanece parcial: builds não comprovam empacotamento reproduzível.
M10-T2 permanece NOT_IMPLEMENTED: pacote limpo, somente exe na raiz,
locales/resources/licenses/data necessários, 18 catálogos, manifesto e execução
final; nenhum objeto, dependência, teste, log ou temporário distribuído.
M10-T3 permanece opcional/NOT_IMPLEMENTED: organização lógica instalada e dados
do usuário fora de Program Files; futura Store/MSIX sem acoplar o player.
Contrato antigo de INI na raiz substituído por data/, com migração a planejar
na M8-T5. Ver [M10](milestones/M10.md). M9 permanece COMPLETE por decisão;
limitações práticas anteriores preservadas.

## Nova consolidação T6–T8

T6, T7 e T8 COMPLETE; aceite visual T8 TUDO PASS registrado.
[Regressão executada novamente e publicação](M8-CONSOLIDATION.md).
Resultados anteriores nesta matriz são históricos; não são contados como execuções novas.
