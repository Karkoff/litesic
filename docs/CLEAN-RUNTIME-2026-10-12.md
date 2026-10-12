# LiteSic — preparação para uso real, anterior à M10

Registro: 2026-10-12T14:30:00-03:00 (America/Sao_Paulo).
Builds geradas a partir da árvore local; nenhum fetch/pull, commit ou push.

## A. Limpeza e preservação

HEAD inicial e final: `609cb38a2f6fa73cde22e56354b51ead08ee5da0`.
Git estava limpo antes do trabalho. Antes de remover arquivos, foram examinados
Makefile, helpers, recursos, resolução de locales e perfil do runtime.

Removidos **474 arquivos regeneráveis**, somando 769,593,270 bytes:

| Pasta | Arquivos removidos |
|---|---:|
| build/64/release | 121 |
| build/32/release | 119 |
| build/64/debug | 117 |
| build/32/debug | 117 |

Objetos `.o`, dependências `.d`, `test-*.exe` correspondentes aos fontes de
testes, probes EQ/performance conhecidos e `build.log`.
[Inventário completo com caminho, tamanho e SHA-256](evidence/clean-runtime/removed.csv).
Nenhuma remoção recursiva: somente arquivos individuais classificados por
nomes de fontes/testes e nomes conhecidos de probes. Nenhum arquivo
desconhecido precisou ser removido. Os executáveis principais foram
substituídos pela linkedição; os 18 catálogos foram preservados e conferidos.

`src/`, `tests/`, `res/`, `locales/`, `third_party/` não têm alterações Git.
`.tools`, dependências, ambientes, arquivos pessoais e evidências antigas
(inclusive o baseline diagnóstico em `build/perf/baseline-src`) foram preservados.

## B. Organização

```text
build/
  64/release/ LiteSic.exe + locales/<18 idiomas>/messages.tsv
  32/release/ LiteSic.exe + locales/<18 idiomas>/messages.tsv
  64/debug/   LiteSic.exe + locales/<18 idiomas>/messages.tsv
  32/debug/   LiteSic.exe + locales/<18 idiomas>/messages.tsv
  internal/<32|64>/<debug|release>/
    obj/          objetos e dependências
    tests/        executáveis de teste, dependências e seus locales
    diagnostics/  probes e seus locales (quando construídos)
    logs/         saídas do helper dev.ps1
    identity/     build.json e patch da árvore local
  validation/clean-runtime/  inventários, perfis novos e verificações
```

Diretórios são criados conforme a necessidade. Fixtures, relatórios e perfis
das suítes existentes continuam em outras pastas de desenvolvimento sob
`build/`, sem entrar nas pastas do aplicativo.

## C. Makefile e scripts

- Quatro variantes preservadas. OBJ_DIR, TEST_DIR e DIAG_DIR separados de BUILD_DIR.
- `test` e probes recebem locales próprios; `run` prepara os recursos do aplicativo.
- Catálogos são copiados mesmo se a fonte restaurada tiver timestamp antigo.
- `clean` remove saídas conhecidas da variante, sem apagar pastas inteiras ou dados.
- `dev.ps1`: `-Rebuild` força recompilação de todas as unidades; `-Jobs` controla
  paralelismo; logs internos com preservação de erro pelo pipefail; identidade
  JSON com commit, alterações locais, compilador, hashes de inputs e executável.
- `clean-runtime.ps1`: migração conservadora do layout antigo, inventário antes
  da exclusão, suporte a WhatIf e interrupção diante de arquivos desconhecidos.
- Caminhos de objetos/testes atualizados nos scripts EQ, memória, auditoria M8,
  relatório M6 e exemplo de loopback. Helpers antigos EQ/memória receberam a
  lista atual de objetos/bibliotecas; parser EQ separa JSON da linha COUNTERS.
- `validate_release.py`: verificação reproduzível com perfis explícitos novos,
  PATH restrito ao Windows, títulos traduzidos, DLLs carregadas e persistência.

Sem mudanças no produto, áudio, instrumentação padrão ou configuração pessoal.

## D. Compilação

Quatro builds concluídas, com compilação de todas as unidades. Os diretórios
internos eram novos e não receberam objetos antigos. Releases e Debug x64
também foram recompiladas com `-Rebuild`. MSYS2 MINGW64/MINGW32, MSVCRT;
GCC/G++ `16.2.0 Rev5`; Release `-O2`, runtime GCC/C++ estático, sem DEBUG ou LITESIC_TEST.
Debug usa `-g -O0 -DDEBUG`.

| Release | Início (-03:00) | Fim (-03:00) | Bytes |
|---|---|---|---:|
| x64 | 2026-10-12T14:15:09.6384861-03:00 | 2026-10-12T14:15:37.9868511-03:00 | 4,767,936 |
| x86 | 2026-10-12T14:15:20.4406802-03:00 | 2026-10-12T14:15:47.6064249-03:00 | 4,694,637 |

Há avisos do código existente: conversão de GetProcAddress, funções GDI sem
uso e avisos do miniaudio na otimização Release. Logs completos preservados;
nenhuma correção funcional foi introduzida para esconder esses avisos.

## E. Testes e limites

| Variante | Executáveis executados | Resultado final |
|---|---:|---|
| x64 release | 31/31 | PASS |
| x64 debug | 31/31 | PASS |
| x86 release | 31/31 | PASS |
| x86 debug | 31/31 | PASS |

Logs finais: [x64 Release](evidence/clean-runtime/test-64-release.txt),
[x86 Release](evidence/clean-runtime/test-32-release.txt),
[x64 Debug](evidence/clean-runtime/test-64-debug.txt),
[x86 Debug](evidence/clean-runtime/test-32-debug.txt).
As suítes cobrem decodificação real, transporte, playlist, pesquisa, sessão,
DSP/EQ, persistência, idiomas, temas, docking, DPI e acessibilidade estrutural.
Foram usadas sem capturas (`LITESIC_NO_CAPTURE=1`): assertions de pixels são
explicitamente PENDING, não PASS; não foi feita avaliação auditiva/visual humana.

**38 inicializações Release PASS**: os 18 idiomas em cada arquitetura, mais
uma cópia contendo apenas runtime e fixture por arquitetura. Executados
diretamente da pasta Release, com cwd nessa pasta, sem toolchain no PATH,
sempre usando `--profile=<perfil novo>`. Títulos nativos de playlist/EQ
conferidos contra cada catálogo, incluindo composição bidi em árabe;
playlist, idioma, volume e EQ persistidos. Perfil pessoal e árvore Release
idênticos antes/depois. [Resultados e módulos carregados](evidence/clean-runtime/runtime-checks.json).

36 guardas de automação PASS. Probes de performance x64/x86 construídos
nas pastas internas. Helper EQ x64 (uma rodada), memória GDI/D2D (uma rodada)
e auditoria M8 de layout/DPI executados com sucesso, sem alterar o aplicativo.

Tentativas anteriores falharam nas assertions de topmost em contexto de
desktop interativo e de memória/recursos GDI/USER em tooltip; os resultados
variaram entre execuções/contextos. Logs dessas falhas foram preservados
em `evidence/clean-runtime/`. Não foi determinada/corrigida a causa e nenhum
teste foi removido ou flexibilizado. O teste de uso real de 1–2 dias permanece
para o usuário; esta execução não certifica esse período ou outras versões do Windows.

## F. Identificação exata

Commit-base: `609cb38a2f6fa73cde22e56354b51ead08ee5da0`.
**Builds incluem alterações locais não commitadas no sistema de build**;
o código de produção e os catálogos permanecem iguais ao HEAD.
Todos os hashes de inputs das quatro identidades foram reconferidos contra
a árvore final. Hashes dos executáveis também foram reconferidos após testes.
Identificação considera revisão, inputs e SHA-256, além do intervalo da build.

**x64 Release**

- Caminho: `D:\Projetos\IA\litesic\build\64\release\LiteSic.exe`
- SHA-256: `E9C3963B0CC2D294E8AB9512E5A53AA1166B617D3147809707FD4A5C508FC4B1`
- [Identidade e hashes dos inputs](evidence/clean-runtime/identity-64-release.json).

**x86 Release**

- Caminho: `D:\Projetos\IA\litesic\build\32\release\LiteSic.exe`
- SHA-256: `93908A174F9B0F9FA5AA35A9F06F9A7DA2D04C07BA460EC44993065EDD82C899`
- [Identidade e hashes dos inputs](evidence/clean-runtime/identity-32-release.json).

[Debug x64](evidence/clean-runtime/identity-64-debug.json) e
[Debug x86](evidence/clean-runtime/identity-32-debug.json) também têm
tamanho, SHA-256, timestamps e inputs registrados.
Patches da árvore na compilação, patch final e snapshots dos arquivos de build
alterados estão em `docs/evidence/clean-runtime/`; registros internos originais
continuam em `build/internal/<arquitetura>/<configuração>/identity/`.

## G. Runtime final

Em cada Release há **somente `LiteSic.exe` como arquivo na raiz** e a pasta
`locales/`, com 18 catálogos idênticos às fontes. O mesmo vale para Debug.
Nenhum teste/probe/objeto/dependência/log foi depositado ali pelos novos comandos.
Ícone e manifest são recursos embutidos no executável.

Imports PE: COMCTL32, COMDLG32, d2d1, DWrite, GDI32, KERNEL32, msvcrt,
ole32, OLEACC, OLEAUT32, SHELL32 e USER32, todos do Windows.
Sem DLLs GCC, libstdc++, winpthread, MSYS2 ou outras DLLs de desenvolvimento.
Módulos carregados nos 38 lançamentos não incluíram arquivos de `.tools` ou
objetos/fontes do repositório. Nenhuma DLL precisou ser movida ou adicionada;
nenhuma política de carregamento do produto foi alterada.
[Imports x64](evidence/clean-runtime/imports-x64.txt) e
[x86](evidence/clean-runtime/imports-x86.txt).

## H. Git e preservação

Árvore inicialmente limpa; árvore final contém somente alterações desta tarefa:
Makefile, helper dev, scripts diagnósticos ajustados, novo limpador/verificador,
documentação e evidências. Build continua ignorado pelo Git. Fontes e testes
preservados. Nenhum commit, push, instalador ou pacote de distribuição.
M10 não iniciada. [Arquivos alterados e hashes](evidence/clean-runtime/modified-files.json).

## I. Executar a versão principal

`D:\Projetos\IA\litesic\build\64\release\LiteSic.exe`

Abrir normalmente, sem flags de validação/perfil, usa a persistência normal
em `%LOCALAPPDATA%\LiteSic`. A versão alternativa é
`D:\Projetos\IA\litesic\build\32\release\LiteSic.exe`.
Não abrir os executáveis de baseline ou de diagnóstico para o uso diário.
