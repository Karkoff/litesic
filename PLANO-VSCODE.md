#LiteSic — plano para desenvolver localmente no VS Code

Player estilo Winamp, em C++ com Win32 nativo, rodando no Windows 7, 8, 10 e 11.
Três telas: player principal, equalizador de 10 bandas e lista de reprodução.
Streaming e extras ficam para depois.

## 1. Decisões técnicas (e por quê)

| Item | Escolha | Motivo |
|---|---|---|
| Linguagem / UI | C++17 + Win32 API + GDI | Nenhum runtime extra (.NET, VC++ Redist, Qt); um `.exe` único que roda do Win7 ao Win11 |
| Áudio | [miniaudio](https://miniaud.io) (1 header) + stb_vorbis | Decodifica MP3, FLAC, WAV e OGG; saída WASAPI com fallback DirectSound/WinMM; filtros de EQ prontos |
| Compilador | MinGW-w64 via MSYS2 (ambiente **MINGW64**/**MINGW32**) | Gratuito, integra bem com VS Code, gera exe estático usando `msvcrt.dll` (existe no Win7) |
| Alvo mínimo | `_WIN32_WINNT=0x0601` (Windows 7) | Impede o uso acidental de APIs do Win8+ |
| Build | `Makefile` + tasks do VS Code | Simples; um comando compila tudo |

> Evite o ambiente **UCRT64** do MSYS2: ele depende do Universal CRT, que o Windows 7 só tem com atualização (KB2999226).

## 2. Preparar a máquina (uma vez)

1. **VS Code**: https://code.visualstudio.com
2. **MSYS2**: https://www.msys2.org — instale em `C:\msys64` (o padrão).
3. Abra o terminal **"MSYS2 MINGW64"** e rode:
   ```bash
   pacman -Syu            # atualiza (pode pedir para fechar e abrir de novo)
   pacman -S --needed make mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb
   # opcional, para gerar a versão 32 bits (Win7 32-bit):
   pacman -S --needed mingw-w64-i686-gcc mingw-w64-i686-gdb
   ```
4. Extensões do VS Code (o projeto já sugere ao abrir):
   - **C/C++** (`ms-vscode.cpptools`) — IntelliSense e depurador
   - **Makefile Tools** (`ms-vscode.makefile-tools`) — opcional
5. (Recomendado) **Git** para versionar: https://git-scm.com

## 3. Estrutura do projeto

```
Litesic/
├─ .vscode/
│  ├─ tasks.json            # Ctrl+Shift+B compila (Debug / Release 64 / Release 32)
│  ├─ launch.json           # F5 compila e depura com gdb
│  ├─ c_cpp_properties.json # IntelliSense apontando para o MinGW
│  └─ extensions.json
├─ src/
│  ├─ app.cpp               # WinMain, comandos, atalhos, instância única
│  ├─ mainwnd.cpp           # tela do player (tempo LCD, visualizador, botões, volume)
│  ├─ eqwnd.cpp             # tela do equalizador (10 bandas + pré-amp + presets)
│  ├─ plwnd.cpp             # tela da lista (adicionar, remover, ordenar, M3U)
│  ├─ skin.h / skin.cpp     # base das janelas desenhadas, widgets, encaixe (snap)
│  ├─ audio.h / audio.cpp   # motor de áudio (miniaudio), EQ, volume, visualização
│  ├─ playlist.h / .cpp     # modelo da lista, tags ID3/FLAC/OGG, M3U
│  ├─ config.h / .cpp       # configurações em INI (modo portátil suportado)
│  ├─ util.cpp, common.h, ma_config.h
│  └─ miniaudio_impl.c      # implementação do miniaudio + stb_vorbis
├─ third_party/             # miniaudio.h, stb_vorbis.c
├─ res/                     # ícone, manifest (DPI, compatibilidade Win7-11), app.rc
├─ Makefile
└─ README.md
```

## 4. Compilar e depurar no VS Code

- **Abrir**: `Arquivo > Abrir Pasta...` na pasta `LiteSic`.
- **Compilar**: `Ctrl+Shift+B` → "Compilar (Debug)". Saída em `build\LiteSic.exe`.
- **Depurar**: `F5` ("Depurar LiteSic (gdb)"). Breakpoints funcionam normalmente.
- **Versão final**: `Terminal > Executar Tarefa...` → "Compilar (Release 64 bits)" ou "(Release 32 bits)".
- Pelo terminal MSYS2 também funciona: `make`, `make DEBUG=1`, `make ARCH=32`, `make clean`.

As tasks chamam `C:\msys64\usr\bin\bash.exe -lc "make ..."`; se instalou o MSYS2 em outro lugar,
ajuste esse caminho em `tasks.json`, `launch.json` e `c_cpp_properties.json`.

## 5. Etapas de desenvolvimento

Cada etapa termina com algo que dá para abrir e testar.

1. **Esqueleto** — janela principal sem borda, arrastável pela barra de título, ícone, fechar/minimizar.
2. **Tocar um arquivo** — motor de áudio (miniaudio): abrir, tocar, pausar, parar, posição e duração.
3. **Player principal** — tempo em LCD (clique alterna decorrido/restante), título rolando,
   kbps/kHz/estéreo, barra de busca, volume, balanço, anterior/próxima, aleatório/repetir.
4. **Visualizador** — espectro (FFT) e osciloscópio; clique alterna.
5. **Lista de reprodução** — adicionar arquivos/pasta, arrastar e soltar, duplo clique toca,
   Delete remove, seleção múltipla, reordenar arrastando, ordenar/embaralhar, salvar/abrir M3U,
   duração e tags lidas em segundo plano, janela redimensionável.
6. **Equalizador** — liga/desliga, pré-amp, 10 bandas (60 Hz a 16 kHz, ±12 dB), curva, presets
   (Rock, Pop, Jazz, Clássica, Graves...), aplicados em tempo real.
7. **Integração das janelas** — EQ e lista encaixam (snap) e se movem junto com o player;
   mostrar/ocultar pelos botões EQ/PL.
8. **Acabamento** — atalhos estilo Winamp (Z X C V B, setas, L, Ctrl+D), teclas de mídia,
   "sempre no topo", zoom da interface, instância única (abrir arquivo pelo Explorer adiciona à lista),
   configurações e lista salvas ao fechar.
9. **Testes de compatibilidade** — ver seção 6.
10. **Distribuição** — exe 32 e 64 bits em um .zip (modo portátil: crie `LiteSic.ini` vazio ao lado do exe);
    depois, se quiser, instalador com Inno Setup (roda no Win7).

**Futuro**: streaming (rádios HTTP/Icecast), skins, mini-player, bandeja do sistema, associação de arquivos,
suporte a mais formatos (AAC/M4A via Media Foundation, Opus).

## 6. Testar no Windows 7

- Máquina virtual (VirtualBox ou Hyper-V) com Windows 7 SP1, 32 e 64 bits.
- Verifique: abre sem pedir DLL, toca MP3/FLAC/OGG/WAV, EQ funciona, arrastar e soltar, dispositivos de áudio USB/HDMI.
- Conferir que o exe não importa APIs novas:
  `objdump -p build/LiteSic.exe | grep "DLL Name"` → só DLLs de sistema (KERNEL32, USER32, GDI32, msvcrt, etc.).

## 7. Dicas

- Use sempre as funções `...W` (Unicode) do Windows para nomes de arquivo com acento.
- Nada de `std::thread`/`std::mutex` no código: usamos `CreateThread`/`CRITICAL_SECTION`, que funcionam em qualquer
  Windows sem depender da `winpthread`.
- O callback de áudio roda em outra thread: ele nunca deve travar, alocar memória ou mexer em janelas
  (só `PostMessage`).
