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

**Estado atual: M0–M5 concluídas.** A playlist está implementada e integrada,
com testes, validação gráfica e PASS manual do usuário para o visual,
arraste do Explorer e M3U externo. A sessão persiste após fechar/reabrir e
Play destaca e centraliza a faixa selecionada. A seleção primária também
volta destacada ao reabrir, pronta para Play. **Adicionar ▾**, **Remover** e
**Lista ▾** ficam no rodapé; Arquivos/Pasta estão agrupados em Adicionar.
Equalizador e a integração completa das três janelas permanecem planejados.
Player e playlist já possuem encaixe magnético e movimento conjunto.
Veja o [encerramento técnico da M5](docs/M5-CLOSEOUT-2026-10-11.md).
Modernização UI/UX **IN_PROGRESS**: player e playlist já usam Direct2D/DirectWrite,
com pesquisa que preserva a fila e os cinco transportes separados. O painel EQ
é explicitamente indisponível até a implementação do DSP. Baseline repetida:
292 aprovações; suíte atual: **356 (89 por variante)**. A investigação de memória
reproduziu o custo de inicialização gráfica, sem crescimento contínuo na carga
curta do backend atual. UI-00..08 ainda não estão concluídas; M6 não iniciada.
Veja [implementação, capturas e pendências](docs/design/UI-VALIDATION-2026-10-11.md)
e [plano por fase](docs/design/LITESIC-UI-IMPLEMENTATION-PLAN.md).

**Temas visuais:** 36 opções em **botão direito no player → Aparência > Temas...**,
com categorias, prévias, aplicação imediata e preferência persistente.
LiteSic Original continua sendo o padrão; player e playlist compartilham a
aparência sem interromper a reprodução. Validação técnica PASS; aceite visual
humano dos novos temas pendente. Seletor e menus próprios usam a paleta ativa.
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
| **Visualizador** | Osciloscópio em tempo real + espectro FFT com escala logarítmica |
| **Equalizador** | Planejado para M6: 10 bandas, pré-amp e presets |
| **Compatibilidade** | Alvo Windows 7–11, x86/x64; validação Win7 pendente na M9 |
| **DPI** | Suporte a alta densidade de pixels |
| **Runtime** | Runtime GCC/C++ estático; depende das DLLs de sistema Windows |

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
| **M6** | Equalizador — 10 bandas, presets, tempo real | 📝 Planejada |
| **M7** | Integração — snap/dock entre janelas, mostrar/ocultar EQ/PL | 📝 Planejada |
| **M8** | Acabamento — atalhos, teclas de mídia, instância única | 📝 Planejada |
| **M9** | Testes de compatibilidade (VM Win7) | 📝 Planejada |
| **M10** | Distribuição — builds 32/64, zip portátil, instalador | 📝 Planejada |

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
Parte dos testes antigos de DSP ainda usa cópias dos algoritmos. INI e
equalizador continuam fora desta etapa.

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
