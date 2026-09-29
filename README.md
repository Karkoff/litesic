# LiteSic

<div align="center">

**Player de áudio estilo Winamp em C++ nativo com Win32/GDI puro**

[![Platform](https://img.shields.io/badge/platform-Windows%207--11-blue)]()
[![Language](https://img.shields.io/badge/language-C%2B%2B-17?color=659ad2&logo=c%2B%2B)]()
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

</div>

---

## 🎵 Visão Geral

LiteSic é um player de áudio para Windows que recria a experiência clássica do Winamp, construído inteiramente em **C++17 nativo** com **Win32 API + GDI puro**. Sem Qt, sem .NET, sem MFC — apenas código Windows puro que roda desde o Windows 7 até o Windows 11.

A aplicação oferece três janelas independentes:
- 🎛️ **Player Principal** — LCD com tempo/duração, título rolando, controles de play/pause/stop, seek, volume, balanço, prev/next, shuffle/repeat e visualizador (osciloscópio/espectro FFT)
- 🔊 **Equalizador** — 10 bandas com pré-amp, presets e ajuste em tempo real
- 📋 **Lista de Reprodução** — drag & drop, reordenação, suporte a M3U

## ✨ Características

| Recurso | Detalhes |
|---|---|
| **Formatos** | MP3, WAV, FLAC, OGG Vorbis |
| **Motor de áudio** | [miniaudio](https://miniaud.io) — single-header, multi-backend |
| **Interface** | Janela sem borda arrastável, 3 janelas com snap/dock |
| **Visualizador** | Osciloscópio em tempo real + espectro FFT com escala logarítmica |
| **Equalizador** | 10 bandas, pré-amp ajustável, presets salvos em INI |
| **Compatibilidade** | Windows 7, 8, 10 e 11 (32 e 64 bits) |
| **DPI** | Suporte a alta densidade de pixels |
| **Runtime** | Zero dependências externas — `.exe` único e estático |

## 🏗️ Arquitetura

```
LiteSic/
├── src/                  # Código-fonte principal
│   ├── app.cpp           # Ponto de entrada (WinMain)
│   ├── mainwnd.cpp       # Janela do player principal
│   ├── audio.cpp         # Motor de áudio (miniaudio wrapper)
│   ├── visualizer.cpp    # Osciloscópio + espectro FFT
│   ├── util.cpp          # Funções auxiliares (INI, M3U, scroll)
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
- **`Ctrl+Shift+B`** — compilar
- **`F5`** — depurar (usa gdb do MSYS2 MINGW64)

## 📋 Roadmap

O projeto é dividido em milestones incrementais, cada uma com tarefas verificáveis:

| Milestone | Descrição | Status |
|---|---|---|
| **M0** | Fundação — estrutura de pastas, Makefile, third_party | ✅ Concluída |
| **M1** | Esqueleto — janela sem borda, arrastável, ícone | ✅ Concluída |
| **M2** | Motor de áudio — abrir/tocar/pausar/parar, posição/duração | ✅ Concluída |
| **M3** | Player principal — LCD, título rolando, seek, volume, shuffle/repeat | ✅ Concluída |
| **M4** | Visualizador — osciloscópio + espectro FFT | ✅ Concluída |
| **M5** | Lista de reprodução — add/drag&drop, reordenar, M3U | 📝 Planejada |
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

Testes unitários cobrem lógica pura sem dependência de janela:
- Parsing de M3U (playlist)
- Parsing de INI (configuração)
- Matemática do equalizador
- Modelo da playlist
- Funções utilitárias (scroll, mapeamento de amostras)

```bash
make test    # compila e roda todos os testes
```

## 📄 Licença

Este projeto está disponível sob a licença MIT. Veja o arquivo [`LICENSE`](LICENSE) para detalhes.

---

**LiteSic** — *Lightweight, native, Windows audio player.*
