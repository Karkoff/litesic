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
| M8 | M8-T1 | Atalhos Winamp e despacho nos três painéis | [ ]; M8 aberta | PARTIALLY_IMPLEMENTED | mainwnd/eqwnd/plwnd WM_KEYDOWN; app loop | Testes locais de teclado PASS; sem ZXCVB/L/Ctrl+D global | M8-REMAINING-REQUIREMENTS-AUDIT.md; regressão nova | Faltam aceleradores globais e seek independente do slider; conservar edição da pesquisa. | T1: reutilizar ações e centralizar despacho. | Não implementado neste ciclo. |
| M8 | M8-T2 | Quatro teclas de mídia | [ ]; M8 aberta | NOT_IMPLEMENTED | WM_APPCOMMAND ausente | Sem teste de despacho multimídia existente | Auditoria residual, busca src | Play/Pause, Stop, Next e Previous não têm handlers. | T2: despachar mensagens, testar e conferir teclado. | Sem reabrir milestones anteriores. |
| M8 | M8-T3 | Sempre no topo, zoom, DPI e posicionamento | [ ]; M8 aberta | PARTIALLY_IMPLEMENTED | InitDpiAwareness; ApplyInterfaceScale; Canvas; WM_DPICHANGED | Tests existentes PASS; probe x86/x64, 24 amostras | Auditoria residual; remaining-probe-{32,64}.txt | Topo ausente; zoom negativo desloca para zero; DPI auxiliar/local incompleto; área útil pequena excedida. | Reutilizar pipeline DIP/pixel; completar topo e lacunas específicas. | Não foi observada dupla escala no caminho player/EQ testado; matriz física pendente. |
| M8 | M8-T4 | Instância única e arquivos pela linha de comando | [ ]; M8 aberta | NOT_IMPLEMENTED | app sem mutex/WM_COPYDATA/parsing normal | Sem ensaio de instância única existente | Auditoria residual; app.cpp | Falta encaminhar/acrescentar arquivos e ativar instância. | T4: mutex e IPC Unicode limitado. | --validation não é modo de produto. |
| M8 | M8-T5 | Configuração, sessão e runtime portátil | [ ]; M8 aberta | PARTIALLY_IMPLEMENTED | ui_theme/eq_state/plwnd/M3U/Language | Round-trips atuais PASS; regressão 716 casos | Auditoria residual, matriz por campo | Faltam balanço, shuffle/repeat/topo, portátil/data/migração e origem principal independente da sessão PL. | Reutilizar INI LOCALAPPDATA documentado; completar campos/contrato. | Não recriar persistência nem empacotar M10. |
| M8 | M8-T6 | Catálogos, cobertura, Unicode e tipografia | COMPLETE; [x] | IMPLEMENTED_AND_VERIFIED | i18n/Canvas, 18 catálogos e inventário | Validador fonte e testes atuais PASS | M8-CONSOLIDATION.md; auditoria residual | Revisão linguística integral e legibilidade/RTL geral ficam no checkpoint. | Revisar achados concretos, sem regenerar catálogos. | 237 IDs atuais por idioma, 46 tooltip.*; T6 técnica preservada. |
| M8 | M8-T7 | Windows automático/manual, persistência e atualização | COMPLETE; [x]; PASS humano | ALREADY_ACCEPTED | language_selection e cinco janelas | Testes de preferência/troca/áudio/modelo/geometria PASS novamente | Consolidação e evidências novas; T7 histórica | Revisão linguística não foi concedida pelo aceite visual. | Preservar entrega aceita, sem nova aprovação T7. | Publicada em 1aa560f. |
| M8 | M8-T8 | Tooltips multilíngues explicativos | COMPLETE; [x]; TUDO PASS humano | ALREADY_ACCEPTED | ui_tooltip, estado real e tokens de tema | 8 casos/variante e 2 gráficos adicionais PASS; 3.092 frames | Consolidação/evidence/m8-consolidation; inventário T8 | UIA/MSAA Canvas e revisão linguística não equivalem a tooltips aceitos. | Preservar T8; tratar lacunas gerais no checkpoint M8. | Publicada em 1aa560f; 18 idiomas/36 temas. |
| M9 | M9-T1 | Checar DLLs importadas | [ ]; M9 administrativamente COMPLETE | IMPLEMENTED_AND_VERIFIED | Makefile: -static; res/app.manifest | objdump dos dois Release; evidência nesta rodada | Inspeção do código e testes listados; relatório M7 | Link estático e imports de sistema confirmados por objdump nesta rodada. | Manter checagem após troca de toolchain. | M9 permanece COMPLETE; checkpoint histórico não reescrito. |
| M9 | M9-T2 | Rodar em VM Windows 7 SP1 (32 e 64 bits) | [ ]; M9 administrativamente COMPLETE | BLOCKED | fontes compatíveis com alvo 0x0601; sem VM disponível | nenhum ensaio físico Win7 nesta rodada | Inspeção do código e testes listados; relatório M7 | VMs Win7 SP1 x86/x64 indisponíveis neste ambiente; ausência de evidência física. | Execução futura pelo usuário, conforme decisão M9. | Bloqueio do ensaio, não do status administrativo M9. |
| M9 | M9-T3 | Registrar e corrigir divergências | [ ]; M9 administrativamente COMPLETE | BLOCKED | docs/M9-CLOSEOUT-2026-10-11.md | nenhum diagnóstico novo entre sistemas | Inspeção do código e testes listados; relatório M7 | Comparação prática entre sistemas depende de ambientes ainda indisponíveis; ausência de falhas observadas não comprova compatibilidade. | Registrar divergências quando os ensaios futuros ocorrerem. | Não reabrir M9. |
| M10 | M10-T1 | Build Release reproduzível | [ ]; milestone aberta | PARTIALLY_IMPLEMENTED | Makefile: variantes isoladas e --no-insert-timestamp; README.md | builds atuais; hashes coincidentes na reconstrução pré-M7 | Inspeção do código e testes listados; relatório M7 | Build Release x64/x86 documentado e estático; linker sem timestamp. Ainda falta prova formal de duas compilações limpas independentes e sua comparação após M7. | Concluir ensaio de reprodutibilidade na M10. | Builds deste ciclo não encerram distribuição. |
| M10 | M10-T2 | Empacotar .zip portátil | [ ]; milestone aberta | NOT_IMPLEMENTED | nenhum script dist/package de produto; scripts/diagnostics empacotam evidências | nenhum zip portátil do produto validado | Inspeção do código e testes listados; relatório M7 | Sem zips do produto, pacote/licenças ou modo portátil conforme M8-T5. | Implementar empacotamento após modo portátil. | Depende do trabalho residual M8-T5. |
| M10 | M10-T3 | Instalador com Inno Setup (opcional) | [ ]; milestone aberta | NOT_IMPLEMENTED | nenhum installer/LiteSic.iss | nenhum ensaio de instalador | Inspeção do código e testes listados; relatório M7 | Instalador Inno Setup opcional ausente. | Decidir se será entregue na M10; só então implementar/testar. | Opcional; não é bloqueio da M7. |

## Resumo por milestone

Contagens atuais: 51 tarefas; 39 aceitas, duas verificadas, quatro parciais,
quatro não implementadas e duas bloqueadas apenas no ensaio físico M9.
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
| M8 | 8 | 2 | 1 | 0 | 3 | 2 | 0 | 0 |
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
3. Após concluir M8: auditoria geral de desempenho aprovada pelo usuário.
4. M10: distribuição limpa/reproduzível, zips e instalador opcional.
5. Testes físicos M9 continuam com o usuário, sem reabertura administrativa.

## Consolidação de encerramento — 11/10/2026

M7 COMPLETE: aceite humano e testes atuais no [encerramento](M7-CLOSEOUT-2026-10-11.md).
As 48 tarefas originais foram preservadas, com três novas tarefas M8 (51 atuais).
M8 continua aberta: DPI por monitor, zoom/persistência e refinamentos existentes
são reutilizados; atalhos parciais, mídia, topo, instância única, configuração
residual e lacunas específicas DPI/acessibilidade continuam pendentes. Na auditoria original, os 18 idiomas, seleção
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

## Auditoria integral atual da M8

[Matriz completa e plano para conclusão](M8-REMAINING-REQUIREMENTS-AUDIT.md):
oito tarefas, todos os 15 critérios i18n, campos de configuração, DPI/monitores
e refinamento/acessibilidade. M8 permanece OPEN/IN_PROGRESS; T6–T8 COMPLETE.
M7/M9 não reabertas; nenhuma lacuna implementada neste ciclo. As linhas M8
acima expressam o estado atual; medições anteriores do documento são históricas.
Consolidação 1aa560f publicada, local/remoto conferidos.
