# LiteSic

<div align="center">

**Player de áudio estilo Winamp em C++ nativo com Win32 + Direct2D/DirectWrite**

[![Platform](https://img.shields.io/badge/platform-Windows%207--11-blue)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B-17?color=659ad2&logo=c%2B%2B)]()
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

</div>

---

## 🎵 Visão Geral

LiteSic é um player de áudio para Windows que recria a experiência clássica do Winamp, construído em **C++17 nativo** com **Win32 API + Direct2D/DirectWrite**, mantendo GDI como fallback durante a migração. Alvo de compatibilidade Windows 7–11. Sem Qt, sem .NET, sem MFC.

**Estado atual: M0–M6 COMPLETE.** A playlist está implementada e integrada,
com testes, validação gráfica e PASS manual do usuário para o visual,
arraste do Explorer e M3U externo. A sessão persiste após fechar/reabrir e
Play destaca a faixa selecionada e só a centraliza quando ela está fora da
área visível; faixas já visíveis mantêm sua posição. A seleção primária também
volta destacada ao reabrir, pronta para Play. **ADIC ▾**, **REMOVER** e
**LISTA ▾** ficam à direita da barra de status; Arquivos/Pasta estão agrupados
em Adicionar. A pesquisa tem um ícone de borracha para limpar.
Veja as [duas correções visuais recentes](docs/PLAYLIST-FOOTER-2026-10-11.md).
**M6 COMPLETE:** aceite final visual, audível, funcional e de uso prolongado
informado pelo usuário em 11/10/2026, após os ajustes integrados.
[Encerramento, quatro builds e validação final](docs/M6-CLOSEOUT-2026-10-11.md).
A [auditoria integrada anterior](docs/M6-INTEGRATED-VALIDATION-2026-10-11.md)
permanece como histórico, com suas medições e limitações originais.
[Ajustes visuais posteriores](docs/UI-BUTTON-STATES-2026-10-11.md): estados
ativos com fundo claro, indicador EQ e comandos do equalizador no rodapé.
[Atualização de volume e painéis](docs/VOLUME-DOCKING-2026-10-11.md): último
volume persistido, EQ do tamanho do player compacto, formação mantida ao mover
e posições da playlist e do equalizador preservadas ao reabrir.
[Favoritos e informações das músicas](docs/PLAYLIST-FAVORITES-2026-10-11.md):
estrelas persistentes, abas Geral/Favoritas, faixa em negrito e informações pelo
menu de contexto. Temas mantém a posição da lista ao aplicar; botões do EQ
têm largura uniforme.
Play iniciado em Favoritas toca somente as músicas marcadas, respeitando
shuffle e repetição; sem repetição, para no fim das favoritas.
[Correção da pesquisa em playlists grandes](docs/SEARCH-FREEZE-2026-10-11.md):
metadados são aplicados em lotes, sem acumular varreduras por notificações antigas.
Equalizador real implementado, com dez bandas, PRE,
ON/OFF suave, sete presets, curva, teclado e persistência. Continua funcionando
com o painel oculto. Na entrega original, quatro builds e **420 verificações** passaram; o usuário confirmou o funcionamento em 11/10/2026. [Relatório, medições e capturas](docs/M6-EQUALIZER-2026-10-11.md).
A integração M7 está **COMPLETE**, com aceite humano “TUDO PASS”:  encaixe entre quaisquer
dos três painéis, movimento transitivo, persistência dos vínculos e recuperação
de posições. [Validação M7](docs/M7-INTEGRATED-VALIDATION.md) e
[auditoria do roadmap](docs/ROADMAP-IMPLEMENTATION-AUDIT.md).
[Compactação por visibilidade](docs/M7-VISIBILITY-COMPACTION.md): ocultar um
painel intermediário fecha o espaço; reabrir recompõe a formação, respeitando
um novo posicionamento manual.
EQ e playlist encaixam no player com captura de 24 DIP e alinhamento
automático pelo início, centro ou fim de cada borda. Os painéis encaixados
acompanham movimento e escala. [Verificação](docs/WINDOW-SNAP-2026-10-11.md).
Títulos longos deslizam suavemente no LCD nos dois sentidos, com pausa nas
extremidades; títulos que cabem permanecem parados.
Corrigida a consulta concorrente de duração MP3 que podia travar e encerrar
o player durante a reprodução/transição. [Auditoria](docs/AUDIO-TRANSITION-AUDIT-2026-10-11.md).
**Aparência entre sessões:** visibilidade do visualizador (incluindo player
compacto), playlist e painel EQ, modo FFT/OSC, escala da interface e tema
são preservados. Veja [validação da persistência](docs/APPEARANCE-PERSISTENCE-2026-10-11.md).
Veja o [encerramento técnico da M5](docs/M5-CLOSEOUT-2026-10-11.md).
Modernização UI/UX **IN_PROGRESS**: player e playlist já usam Direct2D/DirectWrite,
com pesquisa que preserva a fila e os cinco transportes separados. O painel EQ
agora controla o DSP real. Histórico anterior à M6 — baseline repetida:
292 aprovações; rodada anterior: **356 (89 por variante)**. Após persistir a
aparência, a suíte x64 Debug tem **93 testes aprovados**. A investigação de memória
reproduziu o custo de inicialização gráfica, sem crescimento contínuo na carga
curta do backend atual. UI-00..08 ainda não estão concluídas; M6 teve o funcionamento confirmado pelo usuário.
Veja [implementação, capturas e pendências](docs/design/UI-VALIDATION-2026-10-11.md)
e [plano por fase](docs/design/LITESIC-UI-IMPLEMENTATION-PLAN.md).

**Temas visuais:** 36 opções em **botão direito no player → Aparência > Temas...**,
com categorias, prévias, aplicação imediata e preferência persistente.
LiteSic Original continua sendo o padrão; player, playlist e EQ compartilham a
aparência sem interromper a reprodução. Validação técnica PASS e aceite visual
final da M6 informado pelo usuário; não há registro de aprovação individual
dos 36 temas. Seletor e menus próprios usam a paleta ativa.
Veja [sistema de temas](docs/THEMES.md),
[relatório atual de playlist, menus e temas](docs/PLAYLIST-UI-2026-10-11.md) e
[galeria inicial dos oito temas representativos](docs/evidence/themes/index.html).
O ambiente e os comandos atualizados estão em
[`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md).

O produto prevê três janelas independentes:
- 🎛️ **Player Principal** — LCD com tempo/duração, título rolando, controles de play/pause/stop, seek, volume, balanço, prev/next, shuffle/repeat e visualizador (osciloscópio/espectro FFT)
- 🔊 **Equalizador** — 10 bandas com pré-amp, presets e ajuste em tempo real
- 📋 **Lista de Reprodução** — drag & drop, reordenação, suporte a M3U

## ✨ Características

| Recurso | Detalhes |
|---|---|
| **Formatos** | MP3, WAV, FLAC, OGG Vorbis |
| **Motor de áudio** | [miniaudio](https://miniaud.io) — single-header, multi-backend |
| **Interface** | Player e playlist sem borda; playlist redimensionável e com encaixe magnético ao player |
| **Repetição** | Desligada → lista (`REP`) → faixa atual (`REP 1`); Anterior/Próximo continuam navegando |
| **Playlist** | Arquivos/pastas, seleção múltipla, arraste para reordenar, ordenação, M3U, tags em background e recuperação automática ao reabrir |
| **Visualizador** | Osciloscópio em tempo real + espectro FFT com escala logarítmica; botão **VIZ** ao lado de LISTA/EQ para ocultar ou mostrar, reduzindo/restaurando a altura do player |
| **Equalizador** | DSP real: 10 bandas ±12 dB, PRE ±12 dB, ON/OFF suave, presets, curva e persistência |
| **Compatibilidade** | Alvo Windows 7–11, x86/x64; M9 COMPLETE por decisão do usuário. Windows 11 confirmado; testes práticos Windows 7/8/8.1/10 e validação específica x86/x64 pendentes |
| **DPI** | Suporte a alta densidade de pixels |
| **Runtime** | Runtime GCC/C++ estático; depende das DLLs de sistema Windows |

Encerramento M9 em 11/10/2026: os testes restantes serão realizados
futuramente pelo usuário, sem bloquear o avanço nem reabrir automaticamente
a milestone. Ver [PROJECT_STATE](PROJECT_STATE.md) e
[decisão M9-DEC-2026-10-11](docs/M9-CLOSEOUT-2026-10-11.md).

## 🏗️ Arquitetura

```
LiteSic/
├── src/                  # Código-fonte principal
│   ├── app.cpp           # Ponto de entrada (WinMain)
│   ├── mainwnd.cpp       # Janela do player principal
│   ├── plwnd.cpp         # Playlist Win32/Direct2D, busca e docking
│   ├── ui_draw.cpp       # Primitivas Direct2D/DirectWrite e cache
│   ├── ui_player.cpp     # Apresentação do player em DIPs
│   ├── playlist.cpp      # Modelo, seleção e navegação
│   ├── playback.cpp      # Integração da lista com o áudio
│   ├── playlist_worker.cpp # Fila nativa de I/O/metadados
│   ├── m3u.cpp           # Parser e gravação de M3U
│   ├── tags.cpp          # ID3, FLAC e Vorbis Comment
│   ├── audio.cpp         # Motor de áudio (miniaudio wrapper)
│   ├── visualizer.cpp    # Osciloscópio + espectro FFT
│   ├── util.cpp          # Tempo, scroll, bitrate e sliders
│   └── *.h               # Cabeçalhos
├── third_party/          # Bibliotecas de terceiros
│   ├── miniaudio.h       # Motor de áudio single-header
│   └── stb_vorbis.c      # Decodificador OGG Vorbis
├── res/                  # Recursos Windows
│   ├── app.rc            # Resource script (ícone, manifest)
│   └── *.manifest        # Manifesto DPI + compatibilidade
├── tests/                # Testes unitários
├── docs/                 # Documentação
│   ├── ROADMAP.md        # Roadmap de milestones
│   └── milestones/       # Detalhamento de cada milestone (M0–M10)
├── .vscode/              # Configurações VS Code
├── Makefile              # Build system (MinGW-w64)
└── AGENT.md              # Regras e convenções do projeto
```

## 🛠️ Compilação

### Pré-requisitos

Para preparar outro computador, consulte o
[procedimento de instalação e validação](docs/DEVELOPMENT.md).
As tarefas do VS Code usam `.tools/msys64` por padrão.

- **MSYS2** com ambiente MinGW-w64 (MINGW64 ou MINGW32)
  - ⚠️ Nunca use UCRT64 — quebra compatibilidade com Windows 7
- **Make** disponível no PATH

### Comandos

```bash
# Release 64 bits (padrão)
make

# Debug
make DEBUG=1

# Release 32 bits
make ARCH=32

# Limpar build
make clean

# Rodar testes unitários
make test
```

### VS Code

Abra a pasta do projeto e use:
- **`Ctrl+Shift+B`** — compilar Debug 64 bits
- **`F5`** — depurar (selecione 32 ou 64 bits)
- **Terminal > Executar Tarefa** — builds e testes Debug/Release, 32/64 bits

Executáveis em `build/<32|64>/<debug|release>/LiteSic.exe`.

## 📋 Roadmap

O projeto é dividido em milestones incrementais, cada uma com tarefas verificáveis:

| Milestone | Descrição | Status |
|---|---|---|
| **M0** | Fundação — estrutura de pastas, Makefile, third_party | ✅ Concluída |
| **M1** | Esqueleto — janela sem borda, arrastável, ícone | ✅ Concluída |
| **M2** | Motor de áudio — abrir/tocar/pausar/parar, posição/duração | ✅ Concluída |
| **M3** | Player principal — LCD, título rolando, seek, volume, shuffle/repeat | ✅ Concluída |
| **M4** | Visualizador — osciloscópio + espectro FFT | ✅ Concluída |
| **M5** | Lista de reprodução — add/drag&drop, reordenar, M3U | COMPLETE — implementação/testes e checkpoint manual PASS |
| **M6** | Equalizador — DSP, interface e testes entregues | COMPLETE — funcionamento confirmado pelo usuário |
| **M7** | Integração — snap/dock entre janelas, mostrar/ocultar EQ/PL | COMPLETE — implementação/testes e aceite humano TUDO PASS |
| **M8** | Acabamento — atalhos, mídia, DPI, persistência, 18 idiomas e tooltips | 📝 Planejada |
| **M9** | Compatibilidade — encerrada por decisão explícita do usuário | COMPLETE — testes futuros não bloqueantes |
| **M10** | Distribuição — builds 32/64, pacote limpo portátil e instalador opcional | 📝 Planejada |

> 🖥️ Milestones com indicador visual terminam em algo executável para validação manual.

## 🔧 Regras de Implementação

O projeto segue convenções rigorosas documentadas no [`AGENT.md`](AGENT.md):

- **Win32 Unicode**: sempre usar variantes `...W` das funções
- **Threads nativas**: `CreateThread` + `CRITICAL_SECTION` (proibido `std::thread`/`std::mutex`)
- **Callback de áudio seguro**: sem alocação, sem travamento, apenas `PostMessage` para UI
- **Compatibilidade Windows 7**: `_WIN32_WINNT=0x0601`, nenhuma API exclusiva do Win8+

## 📚 Documentação

| Arquivo | Descrição |
|---|---|
| [`AGENT.md`](AGENT.md) | Regras e convenções obrigatórias para implementação |
| [`PLANO-VSCODE.md`](PLANO-VSCODE.md) | Plano completo de arquitetura do projeto |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Índice das milestones |
| `docs/milestones/M*.md` | Detalhamento de cada milestone com tarefas e critérios de aceite |

## 🧪 Testes

Para inspecionar e operar a interface Windows real durante o desenvolvimento,
use o [auxiliar de automação gráfica](docs/GUI-AUTOMATION.md). Ele usa Python
isolado em `.tools/gui-venv`, captura a janela pelo DC Win32 e limita mouse/teclado à
instância de teste do LiteSic. A
[prova de conceito](docs/GUI-VALIDATION-2026-10-11.md) demonstrou Play, Pause,
retomada, Stop e teclado. A execução requer o desktop interativo desbloqueado;
o procedimento explica o isolamento do terminal Codex e a integração nativa.

Testes unitários cobrem lógica pura sem dependência de janela:
- Formatação de tempo
- FFT e janela Hann
- Mapeamento de amplitude
- Buffer circular
- Smoke de dependências com decodificação WAV real

Os testes novos da M5 usam o modelo, parser M3U, tags, controlador, worker,
áudio e mensagens de janela reais: 41 casos da M5 e cinco dos ajustes de
repetição/encaixe, além dos 27 anteriores.
Antes da modernização eram 73 casos por variante, 292 aprovações nas quatro variantes. A GUI foi
exercitada nas quatro, com cobertura detalhada em x64 Debug.
Parte dos testes antigos de DSP ainda usa cópias dos algoritmos. A M6 acrescentou
testes do DSP de produção, grafo de áudio com sinais nos quatro codecs,
INI e painel Win32: 20 executáveis, 105 verificações por variante.

Uso dos ajustes: clique em repetir para alternar os três modos. Arraste o
player para mover a playlist encaixada; arraste a barra da própria playlist
para soltá-la e aproxime uma borda do player para encaixar novamente.
Os controles ficam na ordem Anterior, Play, Pause, Stop, Próximo. Adicione
arquivos por **+ Arquivos** ou **+ Pasta** na playlist modernizada.
Ao fechar o programa, a playlist é salva em `%LOCALAPPDATA%\LiteSic\playlist.m3u8`
e recuperada na próxima abertura, preservando a ordem e as entradas repetidas.
Play toca a faixa selecionada e a destaca no centro da lista (limitado pelas
extremidades); um filtro que esconda a faixa é limpo para mostrá-la.
Veja [validação dos ajustes](docs/PLAYER-ADJUSTMENTS-2026-10-11.md).

```bash
make test    # compila e roda todos os testes
```

## 📄 Licença

Este projeto está disponível sob a licença MIT. Veja o arquivo [`LICENSE`](LICENSE) para detalhes.

---

**LiteSic** — *Lightweight, native, Windows audio player.*
