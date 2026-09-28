# ROADMAP — LiteSic

Cada milestone (M0–M10) corresponde às etapas da seção 5 do
`PLANO-VSCODE.md`. Cada tarefa tem um ID (`M<n>-T<n>`), uma descrição do
que fazer, e um critério de "pronto". Use os IDs para pedir a execução de
uma tarefa (ex.: "executa M2-T3").

Convenção de commit: `M<n>-T<n>: <resumo curto>` (ex.: `M2-T3: pausa e stop
no motor de áudio`).

Milestones marcadas com 🖥️ terminam em algo que dá para abrir e olhar —
ao concluir a última tarefa da milestone, a skill deve sugerir rodar o
`.exe` para validação manual antes de seguir para a próxima milestone.

---

## M0 — Fundação do projeto

Sem isso, nada compila. Não é "testável abrindo o app" (ainda não há app),
mas cada tarefa é verificável.

- **M0-T1** — Criar estrutura de pastas (`src/`, `third_party/`, `res/`,
  `.vscode/`, `docs/`, `tests/`) conforme seção 3 do plano.
  Pronto quando: pastas existem, README.md inicial criado.
- **M0-T2** — Baixar/vendorizar `miniaudio.h` e `stb_vorbis.c` em
  `third_party/`, com nota de versão/licença.
  Pronto quando: arquivos presentes e compiláveis isoladamente (um
  `.c`/`.cpp` mínimo que só inclui os headers compila sem erro).
- **M0-T3** — `Makefile` com alvos `all` (Release), `DEBUG=1`, `ARCH=32`,
  `clean`, `test`, definindo `_WIN32_WINNT=0x0601` e linkando estático
  contra `msvcrt`.
  Pronto quando: `make` gera um `.exe` mínimo (WinMain vazio) em `build/`.
- **M0-T4** — `.vscode/tasks.json`, `launch.json`, `c_cpp_properties.json`,
  `extensions.json` apontando para o MSYS2 (`C:\msys64` configurável).
  Pronto quando: `Ctrl+Shift+B` compila e `F5` inicia o gdb.
- **M0-T5** — `res/app.rc` + manifest (DPI aware, compatibilidade
  Win7–Win11) + ícone placeholder.
  Pronto quando: o `.exe` embute ícone e manifest (checar com
  `objdump`/Explorer).

## M1 — Esqueleto 🖥️

- **M1-T1** — Janela principal sem borda (`WS_POPUP`), tamanho fixo estilo
  Winamp, registra classe e mensagens básicas (`WM_DESTROY`, `WM_PAINT`
  com fundo sólido).
- **M1-T2** — Barra de título customizada arrastável (`WM_NCHITTEST` ou
  `WM_LBUTTONDOWN` + `SendMessage(WM_SYSCOMMAND, SC_MOVE...)`).
- **M1-T3** — Botões de fechar/minimizar desenhados (hit-test manual,
  feedback visual de hover/pressed).
- **M1-T4** — Ícone da aplicação na taskbar e na janela.

Pronto (milestone): app abre, mostra janela sem borda arrastável, título,
fechar e minimizar funcionam.

## M2 — Tocar um arquivo 🖥️

- **M2-T1** — `audio.h/.cpp`: inicializar miniaudio, abrir device WASAPI
  com fallback automático (miniaudio já faz isso).
- **M2-T2** — Abrir/decodificar um arquivo (MP3/WAV via miniaudio,
  OGG via stb_vorbis) e tocar.
  Teste unitário: dado um arquivo de teste curto, `duration()` retorna
  valor esperado (tolerância).
- **M2-T3** — Pausar/parar/retomar; posição atual e duração expostas para
  a UI via `PostMessage`.
- **M2-T4** — Botões mínimos (play/pause/stop) na janela do M1 ligados ao
  motor de áudio, com um "abrir arquivo" via `GetOpenFileNameW`.

Pronto (milestone): abre um arquivo pelo diálogo, toca, pausa, para.

## M3 — Player principal 🖥️

- **M3-T1** — Display LCD de tempo (dígitos desenhados), clique alterna
  decorrido/restante.
- **M3-T2** — Título rolando (scrolling text) com nome do arquivo/tags.
- **M3-T3** — Indicador de kbps/kHz/estéreo lido do decoder.
- **M3-T4** — Barra de busca (seek) arrastável, sincronizada com a
  posição de playback.
- **M3-T5** — Volume e balanço (sliders), aplicados no motor de áudio.
- **M3-T6** — Anterior/próxima, aleatório/repetir (estado, sem lista real
  ainda — pode operar sobre uma lista interna de 1 item até M5).

Pronto (milestone): controles completos do player principal funcionando
com um arquivo tocando.

## M4 — Visualizador 🖥️

- **M4-T1** — Buffer de amostras do callback de áudio para a UI (lock-free
  ou `CRITICAL_SECTION` curta, nunca alocando no callback).
- **M4-T2** — Osciloscópio (desenho da forma de onda em tempo real).
- **M4-T3** — Espectro via FFT (implementação própria ou vendorizada em
  `third_party/`).
  Teste unitário: FFT de um sinal senoidal conhecido pico no bin esperado.
- **M4-T4** — Clique alterna entre osciloscópio e espectro.

Pronto (milestone): visualizador reage ao áudio tocando, alterna com clique.

## M5 — Lista de reprodução 🖥️

- **M5-T1** — `playlist.h/.cpp`: modelo de dados (lista de faixas, metadados
  básicos). Teste unitário: add/remove/reorder.
- **M5-T2** — Janela da lista (`plwnd.cpp`): desenho da lista, redimensionável.
- **M5-T3** — Adicionar arquivos/pasta (diálogo + varredura recursiva) e
  arrastar e soltar (`WM_DROPFILES`).
- **M5-T4** — Duplo clique toca, Delete remove, seleção múltipla,
  reordenar arrastando.
- **M5-T5** — Ordenar/embaralhar; salvar/abrir M3U.
  Teste unitário: parser de M3U (ida e volta, caminhos relativos/absolutos).
- **M5-T6** — Leitura de duração/tags (ID3/FLAC/OGG) em thread de segundo
  plano (`CreateThread`), sem travar a UI.
  Teste unitário: parser de tags contra arquivos de amostra.

Pronto (milestone): lista funcional, integrada ao player (próxima/anterior
usam a lista real), M3U funcionando.

## M6 — Equalizador 🖥️

- **M6-T1** — `eqwnd.cpp`: janela do equalizador, liga/desliga, pré-amp.
- **M6-T2** — 10 bandas (60 Hz–16 kHz, ±12 dB) com filtros aplicados no
  motor de áudio em tempo real.
  Teste unitário: resposta do filtro de uma banda isolada contra valores
  esperados (matemática do DSP, sem depender de janela).
- **M6-T3** — Curva desenhada conectando os sliders.
- **M6-T4** — Presets (Rock, Pop, Jazz, Clássica, Graves...), salvos/lidos
  de config.

Pronto (milestone): EQ altera o som audivelmente em tempo real, presets
aplicam valores corretos nos sliders.

## M7 — Integração das janelas 🖥️

- **M7-T1** — `skin.h/.cpp`: base comum de janela desenhada (usada por
  player/EQ/lista), se ainda não abstraída nas milestones anteriores.
- **M7-T2** — Encaixe (snap) entre janelas ao arrastar (detectar proximidade
  de bordas).
- **M7-T3** — Janelas se movem juntas quando o player é arrastado.
- **M7-T4** — Botões EQ/PL no player mostram/ocultam as respectivas janelas.

Pronto (milestone): as três janelas se comportam como um conjunto único,
estilo Winamp.

## M8 — Acabamento 🖥️

- **M8-T1** — Atalhos de teclado estilo Winamp (Z X C V B, setas, L,
  Ctrl+D).
- **M8-T2** — Teclas de mídia do teclado (`WM_APPCOMMAND`).
- **M8-T3** — "Sempre no topo" e zoom/escala da interface.
- **M8-T4** — Instância única (mutex nomeado) — abrir arquivo pelo
  Explorer adiciona à lista da instância já aberta.
- **M8-T5** — `config.h/.cpp`: configurações em INI (modo portátil se
  `LiteSic.ini` existir ao lado do exe), lista salva/restaurada ao
  fechar/abrir.
  Teste unitário: round-trip de leitura/escrita do INI.

Pronto (milestone): app "sente" como um player completo, configurações
persistem entre execuções.

## M9 — Testes de compatibilidade

Não é sobre código novo, é sobre validação. Sem 🖥️ automático — é manual
em VM.

- **M9-T1** — Checar DLLs importadas (`objdump -p build/LiteSic.exe | grep
  "DLL Name"`) — só DLLs de sistema.
- **M9-T2** — Rodar em VM Windows 7 SP1 (32 e 64 bits): abre sem pedir DLL,
  toca MP3/FLAC/OGG/WAV, EQ funciona, drag&drop funciona, dispositivos de
  áudio USB/HDMI reconhecidos.
- **M9-T3** — Registrar e corrigir divergências encontradas na VM.

## M10 — Distribuição

- **M10-T1** — Build Release 32 e 64 bits reproduzível via `make`.
- **M10-T2** — Empacotar `.zip` portátil (exe + `LiteSic.ini` vazio ao
  lado).
- **M10-T3** (opcional) — Instalador com Inno Setup, testado no Win7.

---

## Futuro (fora do roadmap atual)

Streaming (rádios HTTP/Icecast), skins, mini-player, bandeja do sistema,
associação de arquivos, suporte a AAC/M4A (Media Foundation) e Opus.
