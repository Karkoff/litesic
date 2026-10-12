# LiteSic — verificação de portabilidade, 12/10/2026

## Resultado

O x64 Release atual **não depende de MSYS2, MinGW, VS Code, Python ou ferramentas
de desenvolvimento para executar**. Possui modo portátil funcional por
`LiteSic.exe --portable`: configurações e sessão ficam em `data/` junto ao exe.
A cópia isolada passou em 21 inicializações, incluindo os 18 idiomas, reprodução,
reinício e mudança de pasta. A Release em avaliação não foi recompilada ou alterada.

Há uma limitação comprovada: a playlist grava caminhos absolutos das músicas.
Mover o aplicativo e a mídia juntos preserva as preferências, mas não atualiza
esses caminhos. Duplo clique sem `--portable` usa o perfil normal em LocalAppData.

## Executável inspecionado

- Caminho: `D:\Projetos\IA\litesic\build\64\release\LiteSic.exe`.
- Arquitetura PE: AMD64, 64 bits.
- Commit de compilação: `23310e32b5ddfa99e2014374d02e9e75f0df9348`.
- HEAD durante a inspeção: `883f10f4f69eefb4cefe3dfa945a2ed4571a9f08`.
- SHA-256: `E9C3963B0CC2D294E8AB9512E5A53AA1166B617D3147809707FD4A5C508FC4B1`.
- Host: Windows 11 Pro, 10.0.26200, x64.

A identificação fonte/build permanece a do
[relatório Release](PRE-DISTRIBUTION-RELEASE-2026-10-12.md). Esta tarefa adiciona
somente documentação, evidências e verificador diagnóstico; não inicia a M10.

## Dependências diretas

Tabela de importações PE, conferida também com objdump. **12 DLLs**, todas do Windows:

| DLLs | Uso |
| --- | --- |
| `COMCTL32.dll`, `comdlg32.dll` | Controles nativos e diálogos |
| `d2d1.dll`, `DWrite.dll` | Direct2D e DirectWrite |
| `GDI32.dll`, `USER32.dll` | Interface Win32 e desenho |
| `KERNEL32.dll` | Processos, arquivos, sincronização e APIs básicas |
| `msvcrt.dll` | Runtime C do Windows |
| `ole32.dll`, `OLEAUT32.dll`, `OLEACC.dll` | COM, automação e acessibilidade |
| `SHELL32.dll` | Integração com Shell e caminhos de dados |

O exe não tem importações PE diferidas (delay imports). Não importa
`msys-2.0.dll`, `libgcc_s_seh-1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`,
`VCRUNTIME140.dll` ou `MSVCP140.dll`. GCC/C++ são vinculados estaticamente;
miniaudio e os decodificadores WAV/MP3/FLAC/Vorbis são compilados no exe.
Não é necessário instalar toolchain ou redistribuível de desenvolvimento.

[Importações diretas independentes por objdump](evidence/portability-2026-10-12/direct-imports-objdump.txt).

## Dependências transitivas

As importações normais das DLLs diretas e os encaminhamentos de exportações
referenciadas acrescentam **12 DLLs físicas** neste Windows:

`ADVAPI32.dll`, `combase.dll`, `gdi32full.dll`, `KERNELBASE.dll`,
`msvcp_win.dll`, `ntdll.dll`, `RPCRT4.dll`, `sechost.dll`, `SHCore.dll`,
`SHLWAPI.dll`, `ucrtbase.dll`, `win32u.dll`.

São **24 DLLs na cadeia estática completa observada**, todas sob a pasta do
Windows, e todas presentes nos processos testados. `msvcp_win.dll` e
`ucrtbase.dll` são dependências dos próprios componentes do Windows 11
(por exemplo, comdlg32/DWrite/Shell32); não representam link direto do LiteSic
contra UCRT64 ou necessidade de redistribuir essas DLLs junto ao aplicativo.

Os nomes `api-ms-*`/`ext-ms-*` são contratos virtuais que o carregador mapeia
para hosts físicos. O grafo usa o namespace API-set do próprio host, incluindo
prefixos sem revisão e redirecionamentos específicos do importador. Foram
conferidas **174 resoluções padrão** contra `GetModuleHandleW`; duas entradas
possuem redirecionamento próprio de kernel32 para kernelbase, registrado na
tabela do Windows. Nenhuma dependência normal ficou sem resolução.
[Explicação oficial dos aliases](https://learn.microsoft.com/en-us/windows/win32/apiindex/api-set-loader-operation).

As DLLs do Windows também declaram 315 nomes distintos de importações diferidas;
a evidência lista essas declarações separadamente. Elas são caminhos condicionais
internos do sistema, não 315 dependências obrigatórias adicionais do LiteSic.
A lista física e os hosts API-set variam com a versão do Windows.

[Grafo completo, símbolos importados, forwarders e delay imports](evidence/portability-2026-10-12/dependencies.json),
[conferência nativa API-set](evidence/portability-2026-10-12/api-set-native-checks.json).
A leitura PE segue a
[especificação da Microsoft](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).

## Carregamento dinâmico

O código compilado dos backends Windows do miniaudio contém cargas por nome
para `user32.dll`, `advapi32.dll`, `ole32.dll`, `kernel32.dll`,
`mmdevapi.dll`, `avrt.dll`, `dsound.dll` e `winmm.dll`.
WASAPI é preferido; DirectSound/WinMM são alternativas. Não foram forçadas
falhas de WASAPI para exercitar todos os backends. A aplicação também resolve
opcionalmente APIs de DPI em user32, preservando os fallbacks existentes.

A união das 21 amostras observou **72 DLLs físicas distintas**, deduplicadas
sem diferenciar maiúsculas/minúsculas:

| Grupo observado | Exemplos |
| --- | --- |
| Áudio e dispositivos | `MMDevAPI.dll`, `AUDIOSES.dll`, `avrt.dll`, `WINMM.dll`, `DEVOBJ.dll`, `cfgmgr32.dll` |
| Desenho e driver gráfico | `d3d11.dll`, `dxgi.dll`, `dxcore.dll`, DLLs NVIDIA em DriverStore |
| Fontes, entrada e imagens | `MSCTF.dll`, `IMM32.dll`, `TextShaping.dll`, `textinputframework.dll`, `WindowsCodecs.dll` |
| Shell, segurança e serviços do Windows | `windows.storage.dll`, `uxtheme.dll`, `CRYPT32.dll`, `bcrypt.dll`, `wintrust.dll` |
| Módulos do ambiente externo ao Windows | `RTSSHooks64.dll` (RivaTuner Statistics Server), `wslbscr64.dll` (Topaz OFD/Warsaw) |

**70** estão sob a pasta do Windows, incluindo seus drivers; **2** vêm dos
programas externos indicados. Estes dois não aparecem na cadeia estática nem
nos nomes carregados pelo código do LiteSic: são módulos ambientais observados,
não dependências a distribuir. O mecanismo de entrada desses módulos não foi
rastreado nesta verificação e os programas do usuário não foram desativados.

Nenhuma DLL veio de `.tools/`, MSYS2, MinGW, VS Code ou diretórios de desenvolvimento.
Os caminhos completos por processo e a união de módulos estão no
[registro de runtime](evidence/portability-2026-10-12/runtime.json).
Essa é a carga observada nos cenários executados; outros recursos do Windows
podem carregar DLLs adicionais quando acionados.

## Modo portátil e armazenamento

Implementação existente:

- `src/runtime.cpp`: `Parse`, `ProfileDirectory`, `PrepareProfile`.
- `src/app.cpp`: seleciona o perfil e passa os mesmos caminhos ao tema, EQ,
  idioma e playlist.
- `src/i18n.cpp`: procura catálogos relativos ao caminho do exe, via
  `GetModuleFileNameW`, e não relativos ao diretório corrente.

| Forma de execução | Perfil |
| --- | --- |
| `LiteSic.exe` sem flags | `%LOCALAPPDATA%\LiteSic` |
| `LiteSic.exe --portable` | `<pasta do exe>\data` |
| `LiteSic.exe --profile=<pasta>` | Perfil explícito; não pode combinar com `--portable` |

No perfil são gravados `settings.ini`, `playlist.m3u8` e
`playlist.m3u8.ini` (seleção/geometria). `data/` é criado automaticamente.
A pasta precisa permitir escrita. Se não conseguir preparar o perfil, o
programa exibe erro; não há fallback silencioso do portátil para o perfil pessoal.
A presença de `data/` sozinha não ativa o modo portátil: a flag é necessária.

Estrutura para execução portátil:

```text
LiteSic/
├── LiteSic.exe
├── locales/
│   └── <18 idiomas>/messages.tsv
└── data/                         # criado pelo aplicativo com --portable
    ├── settings.ini
    ├── playlist.m3u8
    └── playlist.m3u8.ini
```

Não copiar DLLs do Windows, do compilador, do driver ou de ferramentas locais.

## Teste da cópia isolada

A cópia foi criada em pasta descartável com espaços e caractere Unicode,
contendo inicialmente apenas `LiteSic.exe` e `locales/`. O diretório corrente
foi outra pasta vazia, sem relação com a pasta do exe. Todas as inicializações
usaram `--portable`, sem `--profile` ou `--validation`; PATH do filho continha
somente `%SystemRoot%\System32` e `%SystemRoot%`.

| Cenário | Resultado |
| --- | --- |
| Primeira abertura sem data/settings prévios | PASS: criou os três arquivos no data junto ao exe |
| Uma abertura por idioma | 18/18 PASS: títulos de playlist/EQ e nome acessível de Play conferidos |
| Reprodução silenciosa de WAV real | PASS: seek percentual avançou, volume zero |
| Volume alterado pelo controle MSAA e reinício | PASS: 25% persistido e recuperado |
| Mudança da pasta do aplicativo, com data e mídia | PASS para idioma, volume e parâmetros EQ; limitação da playlist confirmada |
| Diretório corrente independente | Permaneceu vazio, sem configurações |
| Encerramento dos processos descartáveis | 21/21 exit 0 |

Os **18 arquivos de catálogo** foram comparados por SHA-256 com as fontes da
Release. Os **18 idiomas** funcionaram: **17 usam catálogo externo** e
**inglês usa o catálogo embutido**, conforme `Service::Select("en-US")`.
Não se afirma que o arquivo externo inglês tenha sido lido pelo aplicativo.

Após a mudança de pasta: idioma `zh-TW`, volume `250` permille, preamp
`-3.000000` e Band0 `2.000000`, todos no novo `data/settings.ini`.
A mídia estava presente na nova pasta, mas a M3U continuou contendo o caminho
absoluto anterior, agora inexistente. `WriteM3U` chama `FullPath(track.path)`;
o leitor aceita caminhos relativos, mas o salvamento os converte em absolutos.
Isso demonstra a limitação sem atribuir uma falha ao carregamento de idiomas
ou às configurações portáteis.

O host continua tendo ferramentas instaladas. A independência dessas ferramentas
foi verificada pela cadeia de imports, módulos carregados e execução com PATH
restrito; **não foi usada uma VM limpa com ferramentas desinstaladas**.
Não se certifica nesta execução Windows 7/8/8.1/10 ou qualidade visual/auditiva.

## Integridade e verificador

Os inventários [antes](evidence/portability-2026-10-12/preservation-before.json)
e [depois](evidence/portability-2026-10-12/preservation-after.json) são iguais:
38 arquivos das duas Releases e três arquivos do perfil pessoal. Nenhum
executável, catálogo, configuração pessoal ou playlist pessoal foi modificado.
As cópias permanecem sob `build/validation/`; nada foi copiado de volta.

O verificador diagnóstico está em `scripts/diagnostics/verify_portability.py`.
Durante sua preparação ocorreram dois erros do próprio verificador, preservados
em [registro](evidence/portability-2026-10-12/inspection-checks.json):

1. Comparação textual exigia `-3`/`2`, embora o app gravasse valores numéricos
   válidos `-3.000000`/`2.000000`. Corrigida para comparação numérica.
2. A análise PE inicialmente buscava revisão API-set exata; passou a usar o
   prefixo da tabela do Windows. A análise foi refeita separadamente, sem
   repetir as 21 amostras de runtime aprovadas.

Nenhum retry foi introduzido e nenhum defeito do aplicativo foi escondido.
O teste inicial do verificador executou duas aberturas antes da interrupção;
as 21 amostras completas são contadas separadamente. Não houve reprovação de
processo LiteSic nessas amostras. O
[erro inicial](evidence/portability-2026-10-12/harness-initial-failure.txt) foi preservado.

```powershell
# Nova cópia descartável e novos testes; não compila nem altera a Release:
./.tools/gui-venv/Scripts/python.exe scripts/diagnostics/verify_portability.py
# Somente inspeção PE, usando o inventário de módulos existente:
./.tools/gui-venv/Scripts/python.exe scripts/diagnostics/verify_portability.py --dependencies-only
```

O parser do namespace API-set é diagnóstico e exige o formato v6 observado
no host; não é código do aplicativo nem modifica sua compatibilidade.

## Ajustes necessários

**Para usar como portátil com flag explícita:** nenhum ajuste funcional
necessário. Copiar exe + locales para uma pasta gravável e executar com
`--portable`; levar `data/` para preservar as preferências.

**Para uma entrega portátil inteiramente relocável e automática:** ainda seria
necessário definir uma ativação sem flags (por exemplo, launcher ou marcador)
e preservar/recalcular caminhos relativos da mídia na sessão portátil.
Esses ajustes não foram implementados. Até lá, abrir sem a flag usa LocalAppData,
e mover as músicas exige atualizar/reabrir suas entradas na playlist.

Nenhum instalador, pacote, build nova ou M10 foi iniciado.
