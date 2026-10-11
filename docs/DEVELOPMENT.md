# Ambiente de desenvolvimento Windows

Atualização UI/UX: player e playlist migrados parcialmente para Direct2D/DirectWrite,
EQ explicitamente indisponível até DSP real. A suíte atual tem dezesseis binários,
89 verificações por variante (356 execuções nas quatro), incluindo sete casos
de temas; 27 guardas de automação. Ver [sistema de temas](THEMES.md) e
[relatório UI/UX histórico](design/UI-VALIDATION-2026-10-11.md)
para capturas, aumento de memória, fases pendentes e limites DPI/Windows 7.

Este procedimento mantém o ambiente usado na M5. A playlist está implementada;
seu checkpoint manual passou. Os novos temas aguardam aceite visual humano.
Equalizador (M6) e integração das
janelas (M7) permanecem planejados. Validação atual:
[M5-CLOSEOUT-2026-10-11.md](M5-CLOSEOUT-2026-10-11.md), [THEMES.md](THEMES.md)
e [playlist/menus atuais](PLAYLIST-UI-2026-10-11.md). Histórico do ambiente:
[VALIDATION-2026-10-10.md](VALIDATION-2026-10-10.md).

## Requisitos e escolha da toolchain

- Máquina de desenvolvimento: Windows 10/11 x64, Git e VS Code.
- MSYS2 x64, GNU Make, GCC/G++, binutils (`ld`, `windres`, `objdump`) e GDB.
- MINGW64/MSVCRT para x64; MINGW32/MSVCRT para x86. Não usar UCRT64 neste
  projeto. Os compiladores C e C++ vêm no mesmo pacote GCC.
- C17 para as unidades C e C++17 para C++; Unicode e `_WIN32_WINNT=0x0601`.
- Extensão VS Code `ms-vscode.cpptools`; Makefile Tools é opcional.

O MSYS2 atual exige Windows recente para executar suas ferramentas, mas ainda
permite mirar Windows 7 com MINGW64/MINGW32. Esses ambientes são legados;
registrar as versões usadas antes de atualizar. Isso não certifica os binários
no Windows 7: a validação em VM e de APIs continua na M9.
Fontes: [suporte Windows](https://www.msys2.org/docs/windows_support/) e
[ambientes](https://www.msys2.org/docs/environments/).

Não é necessário instalar MSVC, Windows SDK separado, CMake, Qt, FFTW ou .NET.
Os headers e bibliotecas de importação Win32 vêm com MinGW-w64. O build usa
`comdlg32`, `user32`, `kernel32`, `gdi32`, `ole32`, `winmm` e funções matemáticas
do runtime. As bibliotecas GCC/C++ são vinculadas estaticamente; DLLs de sistema
Windows continuam necessárias. Verifique os imports após trocar a toolchain.

## Instalação isolada e verificável

Por padrão, o projeto usa `.tools/msys64` (ignorado pelo Git), sem modificar
PATH global nem as configurações globais do VS Code. Use caminho NTFS curto,
ASCII e sem espaços para o repositório/toolchain.

1. Obtenha o arquivo oficial `msys2-base-x86_64-20260927.sfx.exe` em
   [releases MSYS2](https://github.com/msys2/msys2-installer/releases/tag/2026-09-27).
   Baixe também o `.sha256` correspondente e compare com
   `Get-FileHash -Algorithm SHA256`. Consulte o
   [guia oficial](https://www.msys2.org/docs/installer/) para assinatura GPG.
2. Extraia o arquivo com `-y -o<raiz-do-projeto>/.tools`. O pacote cria
   `msys64` dentro dessa pasta. O SHA-256 validado nesta preparação foi
   `ad336cccfda47758b5e15cda993fbba421115cb0b126697daef1ee4dfe37209f`.
3. Em PowerShell na raiz do projeto:

   ```powershell
   $env:MSYSTEM = 'MINGW64'
   $env:CHERE_INVOKING = '1'
   & .\.tools\msys64\usr\bin\bash.exe -lc 'pacman -Syu --noconfirm'
   # Se a atualização encerrar o shell, execute de novo em um novo processo:
   & .\.tools\msys64\usr\bin\bash.exe -lc 'pacman -Syu --noconfirm'
   & .\.tools\msys64\usr\bin\bash.exe -lc 'pacman -S --needed --noconfirm make mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb mingw-w64-i686-gcc mingw-w64-i686-gdb'
   ```

As dependências transitivas são resolvidas e verificadas pelo pacman. Os
pacotes i686 são necessários somente para desenvolver/validar x86. Não misture
objetos ou arquivos de runtime de UCRT64, Git Bash, MSVC e MINGW64/MINGW32.
O Bash do Git e o `bash.exe` do Windows/WSL não substituem o Bash do MSYS2.

Para reutilizar outra instalação, passe `-Msys2Root C:\msys64` ao script ou
defina `LITESIC_MSYS2_ROOT` no ambiente do processo. Ajuste também
`compilerPath` e `miDebuggerPath` dos arquivos `.vscode` para essa instalação;
eles apontam por padrão para a cópia local do workspace.

## Compilar, testar e executar

PowerShell, na raiz do projeto:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -DebugBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Architecture 32
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Architecture 32 -DebugBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Action test
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Action test -DebugBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Action run -DebugBuild
```

Acrescente `-Architecture 32` para testar/executar x86. A opção `Bypass` vale
somente para esse processo PowerShell; não altera a política global.
O script seleciona MSYSTEM e temporários graváveis, restaura o ambiente do
processo e propaga o código de saída do Make.

No shell MSYS2 também funcionam `make`, `make DEBUG=1`, `make ARCH=32`,
`make ARCH=32 DEBUG=1`, `make test` e `make run`. Combine ARCH/DEBUG para
selecionar a variante. Saídas:

| Variante | Executável |
|---|---|
| x64 Release | `build/64/release/LiteSic.exe` |
| x64 Debug | `build/64/debug/LiteSic.exe` |
| x86 Release | `build/32/release/LiteSic.exe` |
| x86 Debug | `build/32/debug/LiteSic.exe` |

`make clean` remove somente a variante selecionada, por exemplo
`make ARCH=32 DEBUG=1 clean`. Feche o executável antes de recompilar.
Headers são rastreados por arquivos `.d`; recursos dependem explicitamente
do manifest e do ícone. Release usa `-O2`; Debug usa `-g -O0` sem `NDEBUG`.

## VS Code

Para captura e interação com a interface nativa, consulte
[GUI-AUTOMATION.md](GUI-AUTOMATION.md). `scripts/gui.ps1` é exclusivo de
desenvolvimento, tem dependências Python isoladas e foi validado com evidências
reais em [GUI-VALIDATION-2026-10-11.md](GUI-VALIDATION-2026-10-11.md).

- `Ctrl+Shift+B`: **Compilar (Debug 64 bits)**.
- **Terminal > Executar Tarefa**: escolher build ou testes Debug/Release,
  32/64 bits. As tarefas chamam `scripts/dev.ps1` com caminhos do workspace.
- **Executar e Depurar**: escolher **Depurar LiteSic (64 bits)** ou
  **(32 bits)** e pressionar F5. O `preLaunchTask` compila a variante correta.
- Coloque um breakpoint em `wWinMain` (`src/app.cpp`) para verificar símbolos
  e execução. Use F10/F11 para avançar e Shift+F5 para encerrar a depuração.
- **C/C++: Select IntelliSense Configuration** permite escolher 32/64 bits.

O depurador da extensão C/C++ usa `cppdbg`, `MIMode: gdb` e GDB da mesma
arquitetura do build. Nesta máquina, a sessão x86 apresentou uma exceção
inicial no carregador `ntdll!ZwMapViewOfSection` e uma pausa subsequente antes
de alcançar o breakpoint. Duas continuações permitiram iniciar normalmente.
Não foi configurada supressão global de exceções. A sessão x64 chegou ao
breakpoint diretamente. Isso é uma limitação observada do debug x86, não
uma certificação de ausência de falhas. Fonte de configuração:
[referência de launch.json](https://code.visualstudio.com/docs/cpp/launch-json-reference).

## Dependências incorporadas e limites dos testes

Preservadas sem atualização: miniaudio **0.11.25** e stb_vorbis **1.22** em
`third_party/`. WAV/MP3/FLAC usam decoders internos do miniaudio; OGG usa
stb_vorbis. `stb_vorbis.h` é uma adaptação local de declarações.
`src/ma_config.h` é compartilhado por `audio.cpp` e `miniaudio_impl.c`.
FFT, janela Hann, espectro e buffer circular são implementações do projeto.
Não há biblioteca externa de FFT, framework gráfico ou framework de testes.

O baseline anterior à UI/UX executava doze binários: 27 casos anteriores (tempo, FFT, amplitude,
buffer circular e smoke de dependências), 41 casos da M5 e cinco casos de
regressão para repetição/encaixe: 73 por variante, 292 nas quatro variantes.
Os novos cobrem playlist (11), M3U (6), tags (8), controlador (8), áudio real
offline (6), worker (3) e integração da janela Win32 (4).
O teste da janela cria e oculta sua própria janela, entrega `HDROP` no mesmo
processo e processa mensagens reais; não envia mouse/teclado ao desktop.
O smoke aceita caminhos de áudio como argumentos para testar outros formatos.
Qualquer binário que falhar interrompe a suíte e causa erro no Make.

Os testes de FFT/buffer/amplitude contêm cópias de algoritmos e não comprovam
integralmente o código de produção. Os novos testes da M5 vinculam componentes
de produção. INI e equalizador ainda não estão implementados. A GUI é validada
também pelo [auxiliar local](GUI-AUTOMATION.md); qualidade audível e Windows 7
continuam exigindo validação humana/VM.

Na M5 foram validados tempos/seek com arquivo 44,1 kHz e motor 48 kHz,
corrigidos os guards de áudio carregado, a espera do callback pelo lock da UI,
a navegação no fim da faixa e a atualização do seek no fim da lista.
As divergências dos testes antigos de DSP permanecem registradas. Não iniciar
M6 antes de cumprir o checkpoint manual da M5 e receber autorização.
