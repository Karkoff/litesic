# Preparação e validação técnica — 10/10/2026

## Resultado

Ambiente principal preparado, builds e testes aprovados em x64/x86,
Debug/Release. Os executáveis inicializam e o motor reproduz os quatro
formatos. A validação integral da interface e do acionamento de F5 no VS Code
permanece manual: o helper de controle Windows retornou
`Computer Use native pipe is unavailable` em duas tentativas.

Nenhuma funcionalidade de M5–M10 foi implementada. Não houve commit ou push.
Procedimento de instalação e uso: [DEVELOPMENT.md](DEVELOPMENT.md).

## A. Ambiente preparado

Inspeção inicial: branch `master`, commit `0440a5d` (29/09/2026), working tree
limpo. Foram examinados README, AGENT, plano, roadmap/milestones, Makefile,
configurações VS Code, fontes, recursos, bibliotecas incorporadas e testes.

- Windows x64, build **10.0.26200** (Windows 11).
- Git existente: **2.53.0**.
- MSYS2 oficial **20260927**, isolado em `.tools/msys64`; base atualizada com
  pacman. Download conferido com o SHA-256 publicado pelo projeto.
- GCC/G++ **16.2.0**, pacote **16.2.0-5**, em MINGW64 e MINGW32, ambos MSVCRT.
- GNU Make **4.4.1**, pacote **4.4.1-3**.
- GNU GDB **18.1**, pacote **18.1-3**, x64/x86.
- GNU binutils **2.47.20260726**, pacote **2.47-4**, incluindo ld/windres/objdump.
- MinGW-w64 CRT/headers **14.0.0.r426.g4564ee4b5-1**.
- Extensões já presentes: C/C++ **1.34.4**, Makefile Tools **0.12.17**.
- Visual Studio/Build Tools e Windows SDK já existentes foram preservados;
  não são necessários para este build MinGW.

Não foi encontrada uma toolchain MSYS2/MinGW funcional no PATH, registros de
instalação ou locais convencionais e diretórios de desenvolvimento examinados.
O Git traz DLLs MinGW, mas não um compilador. A instalação isolada evita
conflitos com MSVC, CUDA, Git Bash e o Bash do Windows/WSL.
Não foram alterados PATH global, configurações globais do VS Code ou política
global de execução. Lista completa de pacotes locais: `build/validation/toolchain-packages.txt`.

## B. Dependências

Instalados pelo pacman: Make, GCC/G++, GDB x64/x86 e suas dependências
transitivas assinadas, incluindo binutils, CRT e headers Win32.

Preservados: miniaudio **0.11.25**, stb_vorbis **1.22** e seu header local.
Nenhuma atualização das bibliotecas incorporadas foi necessária para compilar
ou decodificar os formatos previstos. FFT, espectro e buffer são código próprio;
não há dependência de FFTW, Qt, SDL, .NET ou framework de testes.

WAV/MP3/FLAC usam os decoders internos do miniaudio; OGG Vorbis usa stb_vorbis.
O Makefile explicita import libs de Windows e runtime matemático. Os executáveis
Release x64 e x86 importam diretamente apenas `comdlg32.dll`, `GDI32.dll`,
`KERNEL32.dll`, `msvcrt.dll` e `USER32.dll`. Não há import direto de libstdc++,
libgcc, winpthread, MSYS ou UCRT. Backends de áudio podem carregar DLLs de
sistema dinamicamente; inspeção de imports não certifica compatibilidade Win7.

## C. Alterações realizadas

| Arquivo | Justificativa |
|---|---|
| `.gitignore` | Excluir a instalação local `.tools/` do versionamento |
| `Makefile` | Separar variantes, rastrear headers, explicitar ferramentas/linking, corrigir falhas de testes, incluir otimização Release e alvo run |
| `.vscode/tasks.json` | Oito tarefas de build/test, Debug/Release, x64/x86 |
| `.vscode/launch.json` | Corrigir `cpp-gdb` para `cppdbg`, selecionar GDB/executável/preLaunchTask por variante |
| `.vscode/c_cpp_properties.json` | IntelliSense x64/x86 com caminhos do workspace |
| `scripts/dev.ps1` | Selecionar MSYSTEM, localizar toolchain, fornecer temporários graváveis e propagar erros sem PATH global |
| `src/ma_config.h` | Habilitar os decoders necessários e os três backends Windows; remover definição de debug que era ativada mesmo com valor 0 |
| `src/miniaudio_impl.c` | Compartilhar a mesma configuração usada por audio.cpp |
| `tests/build_third_party_smoke.c` | Vincular implementação real e verificar decodificação WAV; aceitar arquivos opcionais |
| `README.md` | Atualizar caminhos/uso, cobertura real e distinção entre recursos atuais e planejados |
| `docs/DEVELOPMENT.md` | Procedimento reproduzível de instalação, build, testes e debug |
| Este relatório | Registrar evidências e limites da validação |

As macros `MA_NO_WAV`, `MA_NO_MP3` e `MA_NO_FLAC` desabilitavam os decoders
nas declarações, mas não na implementação, que não incluía ma_config.h.
A configuração anterior também restringia as declarações a WASAPI, enquanto a
implementação utilizava os defaults. A correção mantém WAV/MP3/FLAC habilitados,
OGG via stb e WASAPI/DirectSound/WinMM consistentes entre as unidades.

Não houve alterações em audio.cpp, mainwnd.cpp, visualizer.cpp ou util.cpp.

## D. Compilação

| Arquitetura | Configuração | Resultado | Tamanho do EXE |
|---|---|---|---|
| x64 | Debug | Aprovado | 2.441.060 bytes |
| x64 | Release | Aprovado | 934.538 bytes |
| x86 | Debug | Aprovado | 2.273.929 bytes |
| x86 | Release | Aprovado | 1.036.803 bytes |

Executáveis em `build/<32|64>/<debug|release>/LiteSic.exe`. Os quatro builds
foram compilados de fato. `objdump -f` confirmou PE x86-64 e PE i386.

Warnings preservados: dois casts de ponteiros de funções de DPI e duas funções
de desenho não utilizadas por variante. Nos builds Release, GCC também reportou
três `-Wstringop-overflow` em operações atômicas internas do miniaudio. Não foram
suprimidos nem classificados como falsos positivos; merecem investigação.

Dependências de headers verificadas com `make -n DEBUG=1 -W src/util.h` e
`-W src/ma_config.h`: os objetos afetados e o executável são reconstruídos.
Objetos de arquiteturas/configurações diferentes permanecem em pastas distintas.

Uma recompilação completa x64 Release com `make -B` produziu SHA-256 idêntico:
`51e0357783def9c1925888cd36149576012df2b525cb92f55b920a802fab8aa4`.
Isso verifica repetibilidade nesta máquina/toolchain, sem prometer identidade
binária entre ambientes diferentes. O timestamp PE foi removido pelo linker.

## E. Testes

| Variante | Binários aprovados | Casos aprovados | Reprovados |
|---|---|---|---|
| x64 Debug | 5/5 | 27/27 | 0 |
| x64 Release | 5/5 | 27/27 | 0 |
| x86 Debug | 5/5 | 27/27 | 0 |
| x86 Release | 5/5 | 27/27 | 0 |

Cada execução cobre 10 casos de tempo, 4 FFT, 6 amplitude, 6 buffer e
1 smoke WAV real: **108 aprovações** nas quatro variantes. A suíte x64 Debug
também foi repetida após o ajuste final do comentário do smoke, com sucesso.

O bug do executor foi reproduzido usando `TEST_BINS="/usr/bin/false /usr/bin/true"`.
O Makefile original retornou **0**, apesar da falha intermediária. O corrigido
retornou **2** e interrompeu a execução. Essa falha deliberada é uma verificação
do executor, não uma reprovação dos testes funcionais.

O FFmpeg **n5.1.6-16-g6e63e49496-20250131**, já instalado, gerou localmente
arquivos de 30 segundos em WAV/MP3/FLAC/OGG; nenhum áudio foi baixado.
O smoke x64 Debug decodificou amostras dos quatro arquivos com sucesso.

## F. Execução e depuração

- Os quatro executáveis criaram janela `LiteSic`, com handle não nulo,
  `WaitForInputIdle=True` e processo responsivo; foram encerrados após a checagem.
- Um helper temporário vinculado aos objetos reais de cada variante validou
  carregamento, play, avanço da posição, pause estável, retomada, stop e
  512 amostras não silenciosas de visualização, para cada um dos quatro formatos:
  **16 cenários aprovados**. Arquivos de 44.100 Hz, engine de 48.000 Hz.
- O adaptador **OpenDebugAD7** da extensão C/C++ foi exercitado diretamente via
  DAP com as configurações finais de launch.json. O breakpoint em `app.cpp:59`
  foi atingido em x64 e x86; `hwnd != NULL`, `g_audio.active == 1` e
  `g_audio.vizActive == 1` foram confirmados.
- No debug x86 houve exceção inicial em `ntdll!ZwMapViewOfSection`, seguida de
  uma pausa; duas continuações chegaram ao breakpoint. Isso foi reproduzido e
  documentado. Não se adicionou supressão de SIGSEGV/exceções.
- A abertura do diálogo de arquivos, cliques nos controles, aparência e reação
  visual do desenho não foram inspecionados por UI. F5 não foi acionado na
  interface do VS Code. Esses pontos e a qualidade audível permanecem manuais.

Logs e fixtures ficam em `build/validation/`; helpers temporários em `.tools/`.
Esses diretórios são ignorados pelo Git. O teste de áudio adicional exercita
as APIs do motor; não equivale a testar cliques ou todos os caminhos da UI.

## G. Pendências

Preservadas para estabilização: conversão de frames/taxas de amostragem;
lock no callback de áudio; sincronização no final da faixa; erros de reprodução;
cópias/divergências entre testes e produção. Adicionalmente: warnings Release
do miniaudio, paradas iniciais do debug x86 e revisão visual/manual.

Windows 7 x86/x64 não foi executado em VM. M9 continua pendente. O arquivo
LICENSE citado pelo README continua ausente. Nenhuma milestone foi marcada
como concluída por esta intervenção.

## H. Git

Branch `master`, HEAD `0440a5d`, sem commit/push. Nove arquivos rastreados
modificados e três arquivos novos (script e dois documentos), listados em C.
Working tree com alterações disponíveis para revisão. `.tools/` e `build/`
permanecem ignorados. `git diff --check` passou.

## I. Continuidade

O ambiente principal x64 está tecnicamente preparado para continuar a M5;
build e testes não têm bloqueios. x86 também compila/executa, com a ressalva
do debugger descrita acima. Antes de declarar validação integral, executar
a revisão manual de UI/F5 e reprodução audível. As pendências de estabilização
e compatibilidade continuam registradas, sem ampliação do escopo funcional.
