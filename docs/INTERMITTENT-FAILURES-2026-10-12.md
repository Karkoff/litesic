# LiteSic — investigação das falhas intermitentes

Data: 12/10/2026, America/Sao_Paulo. Base: `609cb38a2f6fa73cde22e56354b51ead08ee5da0`, branch `master`.

Foram identificados **cinco cenários de falha em quatro executáveis de teste**:
dois já constavam da reconstrução anterior e três apareceram na investigação.
As correções são de automação, isolamento e medição (**B**). Não foi demonstrado
defeito funcional da aplicação (**A**), nem atribuído um problema ambiental (**C**)
sem evidência. O componente interno responsável por parte do aumento de commit
privado continua indeterminado; isso não foi convertido em diagnóstico de
vazamento do produto.

Resultado final: **92/92 amostras dirigidas PASS**, quatro builds concluídas e
quatro suítes completas **31/31 PASS** cada. Esses números descrevem as condições
testadas, não certificam ausência universal de falhas nem uso contínuo por dias.
Nenhuma falha anterior foi apagada ou considerada corrigida apenas por passar
novamente.

## Resultado por falha

| Falha | Causa | Correção | Resultado |
| ----- | ----- | -------- | --------- |
| `topmost_owns_all_panels` | **B:** fixture compartilha o desktop/foco do usuário; a elevação do player depende da permissão de foreground do Windows. | Executar a fixture numa thread Win32 em desktop próprio, sem trocar o desktop visível. | Antes: 5/5 falhas no desktop interativo. Depois: 5/5 PASS no mesmo contexto de lançamento; 20/20 nas quatro variantes; 6/6 concorrentes. |
| `audio_and_incremental_resources` do tooltip | **B:** contagens de memória/GDI/USER incluem inicialização assíncrona dos serviços de entrada do Windows dentro do processo. | Desktop próprio e `ImmDisableIME` antes da primeira janela da thread da fixture. Manter as assertions originais de recursos e o limite de 512 KiB. | Antes: 7/8 falhas x64 e 3/5 x86 no sandbox. Depois: 20/20 PASS nas quatro variantes, 5/5 no sandbox x64 e 8/8 concorrentes. |
| `repeated_switches_release_catalogs_and_measure_cost` | **B:** commit privado do processo era usado como medida de alocações vivas dos catálogos. | Contar bytes de blocos ocupados nos heaps, preservar métricas de commit, limite de 512 KiB e verificação de payload/descarregamento; acrescentar controle positivo de retenção. | Falha reproduzida na cópia instrumentada, mesmo com bytes/blocos vivos constantes. Depois: 20/20 PASS e 6/6 concorrentes. |
| `repeated_live_changes_cost_and_memory` | **B:** usa a mesma métrica inadequada de commit privado. | Aplicar a mesma medição de heap vivo e controle positivo, mantendo o limite. | Falhou nas suítes iniciais x64 Debug e x86 Release. Depois: 20/20 PASS; incluído nas 6/6 amostras concorrentes da seleção de idioma. |
| Preferência manual de idioma perdida entre processos | **B:** todos os processos usavam `build/language-selection-test.ini`, podendo sobrescrever ou apagar o cenário do outro. | Usar `language-selection-test-<PID>.ini`. | Antes: 1/6 falhas concorrentes em `Active()=="pt-PT"`, contra 0/5 sequenciais x64 Debug. Depois: 0/6 concorrentes e 0/20 sequenciais. |

## Evidências da reconstrução anterior

Lidos o [relatório de reconstrução](CLEAN-RUNTIME-2026-10-12.md), os logs de
`docs/evidence/clean-runtime/`, os correspondentes em `build/validation/clean-runtime/`,
os sumários JSON, o Makefile, o helper de build e os fontes envolvidos.

Há **seis execuções com falha preservadas**, em dois testes:

| Variante / log anterior | Teste e mensagem exata | Resultado registrado |
| --- | --- | --- |
| x64 Release — `test-x64-sandbox-failure.txt` | `test_tooltip.cpp:88: GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS)==user` | `make: ... Error 1` |
| x64 Release — `test-x64-desktop-failure.txt` | `test_m8_remaining.cpp:43: GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TOPMOST` | `make: ... Error 1` |
| x64 Debug — `test-64-debug-desktop.txt` | Mesma assertion de topmost, linha 43 | `make: ... Error 1`; sumário do helper: 2 |
| x86 Release — `test-32-release-desktop.txt` | Mesma assertion de topmost, linha 43 | `make: ... Error 1`; sumário do helper: 2 |
| x86 Debug — `test-32-debug-desktop.txt` | Mesma assertion de topmost, linha 43 | `make: ... Error 1`; sumário do helper: 2 |
| x86 Debug — `test-32-debug-tooltip-failure.txt` | `test_tooltip.cpp:88: GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)==gdi` | `make: ... Error 1`; sumário do helper: 2 |

`CHECK` termina o executável com `exit(1)`, explicando o `Error 1` do filho.
Os sumários [desktop](evidence/clean-runtime/suites.json) e
[sandbox](evidence/clean-runtime/suites-sandbox.json) guardam os códigos externos
disponíveis. Os dois logs de falha x64 Release não registram separadamente o
código bruto do wrapper; não se inventou esse dado.

| Variante | Falhas documentadas antes do log final PASS | Total mínimo de tentativas documentadas até PASS |
| --- | ---: | ---: |
| x64 Release | 2 | 3 |
| x64 Debug | 1 | 2 |
| x86 Release | 1 | 2 |
| x86 Debug | 2 | 3 |

São dez logs: seis falhas e quatro aprovações finais. Não existe um histórico
completo de todos os comandos para provar que nenhuma outra tentativa ocorreu,
nem para ordenar com precisão os dois logs x64 Release entre si. Os logs antigos
não contêm os valores antes/depois das contagens que falharam. Esses limites
foram preservados, em vez de preencher as lacunas por suposição.

As quatro falhas topmost são de contexto interativo. As duas de tooltip constam
das rodadas de sandbox. Os testes rodam sequencialmente dentro do alvo `make test`;
`-j` paraleliza a compilação, não o loop que executa a suíte.

## Reprodução e identificação das causas

Ambiente registrado em [environment.json](evidence/intermittent-failures/environment.json):
Windows kernel `10.0.26200`, processo de host x64, MSYS2 MINGW64/MINGW32, MSVCRT,
C++17. O identificador `Windows-10` retornado pelo Python é o rótulo da API,
não uma certificação da edição comercial do Windows. O sandbox usou Python
3.11 e os comandos interativos usaram Python 3.10; os filhos são os mesmos
executáveis nativos, identificados por SHA-256 nos manifestos.

Inicialmente foram executados **somente m8_remaining e tooltip**. Os binários
anteriores foram copiados para `build/validation/intermittent-failures/baseline/`,
com locales próprios. O executável da avaliação pessoal nunca foi executado ou
reescrito pela investigação.

### Topmost

- Binário original x64: sandbox **5 PASS / 0 FAIL**; desktop **0 PASS / 5 FAIL**.
- Original x86 Debug no sandbox: **5 PASS / 0 FAIL**.
- Instrumentação: `SetWindowPos` retornou 1 e `GetLastError` 0, mas o player
  continuou com estilo 0, enquanto playlist/EQ ficaram com o bit topmost.
  [Registro](evidence/intermittent-failures/trace-m8-desktop-01.txt).
- Mostrar o player e antecipar a chamada para o owner, isoladamente, não
  corrigiu: **0/2 PASS** em cada experimento. Não se alterou a ordem no produto.
- Com foreground adquirido, **2/2 PASS**. Depois, a própria tentativa de
  `SetForegroundWindow` foi negada em **5/5** amostras: isso não é uma preparação
  determinística para a suíte.
  [Foreground adquirido](evidence/intermittent-failures/trace-m8-foreground-desktop-01.txt),
  [permissão negada](evidence/intermittent-failures/foreground-m8-64-desktop-01.txt).

A documentação de [SetWindowPos](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowpos)
descreve a permissão de foreground para elevar uma janela. A dependência do
desktop e os experimentos acima sustentam a classificação da fixture como B;
não demonstram defeito no comando funcional invocado pelo usuário no player.
Um probe com janelas `STATIC` também foi preservado: o comportamento não foi
generalizado para toda classe de janela ou toda versão do Windows.

`isolated_desktop.h` cria um desktop com nome por PID e executa a fixture numa
thread `CreateThread`, antes de COM e da criação das janelas. Não chama
`SwitchDesktop` nem usa o foco do usuário. O chamador espera a thread terminar,
confere o código de saída e fecha o handle da thread e o desktop. Falhas nessas
operações continuam sendo assertions. Os testes de hotkeys/atalhos/IPC/DPI do
mesmo executável permanecem ativos.

### Recursos de tooltip

| Binário original / condição | PASS | FAIL |
| --- | ---: | ---: |
| x64 Release, sandbox, sequencial | 1 | 7 |
| x64 Release, sandbox, dois processos | 6 | 2 |
| x64 Release, desktop, sequencial | 5 | 0 |
| x86 Debug, sandbox, sequencial | 2 | 3 |

A instrumentação completa revelou uma thread com janelas `CicLoaderWndClass`,
`CiceroUIWndFrame`, `Input Switch Message Worker Window`, `Touch Tooltip Window`
e outras classes do Windows. Elas são distintas de `LiteSic.TooltipWnd`.
[Inventário](evidence/intermittent-failures/trace-tooltip-01.txt).
As contagens de [GetGuiResources](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getguiresources)
são do processo inteiro, não apenas do código de descrição do tooltip.

A instrumentação lenta alterava a taxa de reprodução. Por isso foi criada uma
cópia do teste original que só imprime os valores **depois** da medição.
Em oito amostras ela reprovou três vezes:

- USER **52 → 51**, delta privado 0: a igualdade falhou mesmo com redução.
- GDI **35 → 42**, USER **42 → 50**, delta privado **+1.241.088 bytes**.
- GDI **36 → 42**, USER **49 → 50**, delta privado **+290.816 bytes**.

[Código diagnóstico](evidence/intermittent-failures/tooltip-shared-diagnostic.cpp),
[primeira medição](evidence/intermittent-failures/shared-diagnostic-sandbox-01.txt),
[sétima](evidence/intermittent-failures/shared-diagnostic-sandbox-07.txt),
[oitava](evidence/intermittent-failures/shared-diagnostic-sandbox-08.txt).

Desabilitar IME na thread da fixture, antes de criar janelas, deixou GDI/USER em
**9 → 9 / 9 → 9**, com delta privado 0 nas amostras representativas. A intervenção
isolada passou **8/8 no sandbox e 5/5 no desktop**. Depois foi combinada com o
desktop próprio. A API [ImmDisableIME](https://learn.microsoft.com/en-us/windows/desktop/api/imm/nf-imm-immdisableime)
exige a chamada antes da primeira janela e existe desde Windows XP. A biblioteca
`imm32` foi adicionada somente ao alvo do teste.

As comparações GDI/USER continuam exatas e o teto de memória continua em 512 KiB.
Os modos explícitos `--gui`/`--matrix` continuam usando o desktop interativo,
porque verificam hover real; não foram incluídos nas amostras sem captura.

### Memória de i18n e seleção de idioma

A primeira matriz completa após corrigir os dois testes históricos revelou:

| Suíte inicial desta investigação | Resultado | Novas falhas |
| --- | --- | --- |
| x64 Release | 30/31 | i18n, linha 118: `final<=warm+512*1024` |
| x64 Debug | 29/31 | i18n, mesma assertion; language_selection, linha 83: `final<=warm+512*1024` |
| x86 Release | 30/31 | language_selection, mesma assertion |
| x86 Debug | 31/31 | Nenhuma |

Todos esses filhos retornaram 1. As aprovações das rodadas seguintes não foram
usadas como justificativa de correção: i18n passou 5/5 na reprodução sem
instrumentação, e cada variante dirigida de language_selection passou 5/5.

Uma cópia diagnóstica de i18n enumerou os heaps com `HeapLock`/`HeapWalk`, sem
alocar ou imprimir durante a enumeração. **1/8 falhou**: commit privado cresceu
**1.241.088 bytes**, mas os blocos ocupados permaneceram em **2.568 → 2.568**,
e seus bytes em **332.329 → 332.329**. O commit enumerado dos heaps cresceu
somente **688.128 → 737.280**.
[Evidência](evidence/intermittent-failures/diagnostic-i18n-desktop-01.txt),
[fonte](evidence/intermittent-failures/i18n-heap-diagnostic.cpp).

Uma segunda cópia, incluindo contagens GUI, também reprovou **1/8**; GDI
permaneceu **0 → 0** e USER **1 → 1**. Portanto, **não foi atribuída a causa
de i18n ao IME/Direct2D** por analogia com tooltip.
[Registro](evidence/intermittent-failures/diagnostic-i18n-services-desktop-02.txt).
Não foi identificado o componente exato que adicionou todo o commit privado.

O defeito comprovado do teste é equiparar commit total a memória viva retida
pelo carregador. O carregador usa containers C++ e fecha o handle de cada
catálogo; `Install` troca o catálogo ativo e `Unload` libera o anterior.
O teste já limita o payload retido; essa verificação foi preservada.

A correção mede bytes ocupados em **todos os heaps do processo**, incluindo o
heap eventualmente separado do CRT. O limite permanece **512 KiB**. O commit
privado continua nos logs como diagnóstico. Não foi aumentado o aquecimento,
repetida a medição até caber no limite nem compactado o heap para obter PASS.
O controle `heap_accounting_detects_retention` aloca 1 MiB, prova que a medição
ultrapassa o limite, libera o bloco e verifica a recuperação. Assim, um bloco
real retido continua detectável. [HeapWalk](https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapwalk)
e as APIs auxiliares usadas estão disponíveis no alvo Windows 7.

### INI compartilhado

Dois processos do binário original x64 Debug usavam o mesmo nome de arquivo.
Em seis amostras com dois workers ocorreu **1 FAIL** na linha 38,
`Active()=="pt-PT"`, durante a restauração da preferência manual.
[Evidência](evidence/intermittent-failures/before-language-64-debug-concurrent-02.txt).
As cinco amostras sequenciais da mesma variante passaram. Esse é um defeito
de isolamento de cenário, distinto do limite de memória. O nome por PID elimina
a disputa e o teste continua verificando persistência, reinicialização e
preservação da seção de aparência no seu próprio INI.

## Verificações direcionadas adicionais

- `playlist_worker.cpp`: encerramento sinaliza o evento de parada, espera a
  thread e fecha handles. Nenhuma das falhas registradas aponta para worker
  sobrevivente ou acesso concorrente ao modelo de playlist.
- `audio.cpp`: testes offline e casos de transporte/decodificação foram
  preservados. Não houve falha de áudio demonstrada nas suítes finais.
- `runtime.cpp`/`app.cpp`: identidade por perfil, mutex de instância e IPC
  limitado foram examinados. Não se encontrou relação com as assertions
  investigadas. O contrato existente de aguardar a instância inicial publicar
  sua janela não foi modificado.
- `ui_tooltip.cpp`/`ui_draw.cpp`: lifecycle de popup, Canvas e objetos de
  DirectWrite examinado. Não houve evidência para alterar inicialização,
  recuperação ou liberação no produto.
- `i18n.cpp`/`language_selection.cpp`: carga relativa ao executável, catálogos
  temporários, troca/descarregamento, proteção do serviço e mudança na thread
  de UI examinados. Os 18 catálogos permaneceram intactos.
- Não foram alterados timers de UI, timeouts, waits fixos, código de IPC,
  processamento de áudio ou contratos funcionais. Nenhum retry foi adicionado.

## Validação final e reprodução

Todos os testes usaram `LITESIC_NO_CAPTURE=1`; assertions de imagem continuam
explicitamente **PENDING**, não foram contabilizadas como PASS visual.

| Variante | Build isolada | m8_remaining | tooltip | i18n | language_selection | Suíte final |
| --- | --- | --- | --- | --- | --- | --- |
| x64 Release | exit 0 | 5/5 | 5/5 | 5/5 | 5/5 | 31/31, exit 0 |
| x86 Release | exit 0 | 5/5 | 5/5 | 5/5 | 5/5 | 31/31, exit 0 |
| x64 Debug | exit 0 | 5/5 | 5/5 | 5/5 | 5/5 | 31/31, exit 0 |
| x86 Debug | exit 0 | 5/5 | 5/5 | 5/5 | 5/5 | 31/31, exit 0 |

São **80 execuções sequenciais dirigidas + 12 concorrentes finais**: seis de
i18n x64 Release e seis de seleção de idioma x64 Debug, com dois workers.
Todas passaram. Os experimentos adicionais de m8_remaining e tooltip em dois
workers também passaram (6/6 e 8/8); a rodada final no sandbox x64 passou 5/5
para cada um desses dois testes. Essas amostras adicionais não foram somadas
novamente às 92 finais.

As quatro suítes finais foram executadas em sequência, uma execução de cada
um dos 31 binários por variante, na ordem de fontes do Makefile. O driver
continua após uma falha para registrar os demais binários, mas retorna erro
se algum falhar; **não reexecuta o binário reprovado**. As suítes iniciais que
descobriram novas falhas permanecem em manifestos `suite-*`; as finais estão
em `final-suite-*`.

O contrato de saída do driver foi verificado com `false.exe` (filho e driver
retornam 1) e `true.exe` (ambos retornam 0).
[Controle](evidence/intermittent-failures/driver-contract.json). A reprovação
intencional desse controle não é uma falha do LiteSic e não integra as 92 amostras.

Logs, PIDs, início/fim UTC, hashes e códigos de saída por amostra estão em
[evidence/intermittent-failures/](evidence/intermittent-failures/).
O [sumário](evidence/intermittent-failures/run-summary.json) inclui rodadas
anteriores e diagnósticas, inclusive reprovações. A contagem não presume que
uma reexecução positiva apague uma falha.

Comandos reproduzíveis, a partir da raiz:

```powershell
python scripts/diagnostics/intermittent_failures.py build 64 release
python scripts/diagnostics/intermittent_failures.py build 32 release
python scripts/diagnostics/intermittent_failures.py build 64 debug
python scripts/diagnostics/intermittent_failures.py build 32 debug
python scripts/diagnostics/intermittent_failures.py run build/validation/intermittent-failures/64/release/internal/tests/test-test_tooltip.exe nova-tooltip --count 5
python scripts/diagnostics/intermittent_failures.py run build/validation/intermittent-failures/64/debug/internal/tests/test-test_language_selection.exe nova-language-concurrent --count 6 --workers 2
python scripts/diagnostics/intermittent_failures.py suite 64 release --label nova-suite
python scripts/diagnostics/intermittent_failures.py suite 32 release --label nova-suite
python scripts/diagnostics/intermittent_failures.py suite 64 debug --label nova-suite
python scripts/diagnostics/intermittent_failures.py suite 32 debug --label nova-suite
```

Usar labels novos para não sobrescrever evidências. O driver passa `BUILD_DIR`
e `INTERNAL_DIR` isolados ao Makefile; não usar o helper padrão de build para
esta investigação, pois ele aponta para o runtime em avaliação. Não executar
suítes completas em paralelo: outros testes históricos ainda usam nomes de
fixtures comuns e não foram auditados para concorrência arbitrária.

## Preservação e estado do Git

Os [inventários antes](evidence/intermittent-failures/preservation-before.json)
e [depois](evidence/intermittent-failures/preservation-after.json) são idênticos:
**41 arquivos**, incluindo os dois executáveis, 36 cópias de catálogos e três
arquivos pessoais (settings, playlist e dados associados). Nenhum processo
do usuário foi encerrado. Os manifestos de amostras não registram timeout.

| Executável preservado | SHA-256 antes e depois |
| --- | --- |
| `build/64/release/LiteSic.exe` | `E9C3963B0CC2D294E8AB9512E5A53AA1166B617D3147809707FD4A5C508FC4B1` |
| `build/32/release/LiteSic.exe` | `93908A174F9B0F9FA5AA35A9F06F9A7DA2D04C07BA460EC44993065EDD82C899` |

As novas Releases isoladas têm esses mesmos hashes. Não foram copiadas de volta.
Em ambas as pastas do usuário há apenas `LiteSic.exe` na raiz e `locales/` com
18 catálogos idênticos às fontes. [Verificação](evidence/intermittent-failures/release-layout.json).
Objetos e executáveis novos ficaram em `build/validation/intermittent-failures/`;
logs e manifestos ficaram em `docs/evidence/intermittent-failures/`. Fixtures
históricas das suítes continuam sob `build/`; o INI temporário por PID também
é criado ali e removido pelos casos de teste. Nada disso entrou nas Releases.

Arquivos desta tarefa:

- `Makefile`: somente `-limm32` no alvo de tooltip, sobre as alterações locais anteriores.
- `tests/test_m8_remaining.cpp`, `tests/test_tooltip.cpp`: fixtures isoladas; diagnóstico dos deltas de tooltip.
- `tests/test_i18n.cpp`, `tests/test_language_selection.cpp`: heap vivo/controle positivo; INI por PID no segundo.
- Novos `tests/isolated_desktop.h`, `tests/heap_usage.h`.
- Novo `scripts/diagnostics/intermittent_failures.py`.
- Este relatório e `docs/evidence/intermittent-failures/`.

A instrumentação temporária de `src/mainwnd.cpp` foi removida, inclusive
restaurando seus bytes/CRLF pelo hash da identidade anterior. `src/`, `locales/`,
`res/` e `third_party/` não têm diff funcional ou de conteúdo contra HEAD.

O Git começou com oito arquivos versionados modificados e quatro entradas
não versionadas da preparação anterior. As sete alterações versionadas
anteriores fora do Makefile foram comparadas byte a byte como patches e
[preservadas](evidence/intermittent-failures/preserved-local-changes.json).
No Makefile, a alteração adicional é a biblioteca específica do teste.
[Estado inicial](evidence/intermittent-failures/initial-git-status.txt),
[estado final](evidence/intermittent-failures/final-git-status.txt),
[patch final](evidence/intermittent-failures/final-working-tree.patch) e
[hashes de inputs](evidence/intermittent-failures/final-inputs.json).
Nenhum commit, push ou início da M10.

## Limites e questões restantes

Não restou reprovação nas amostras e suítes finais. Isso não constitui prova
de estabilidade integral da aplicação em qualquer ambiente. Permanecem fora
da validação: uso contínuo de dias, outros Windows, IME real, modos gráficos
opcionais de hover/captura e avaliação visual/auditiva humana.

O mecanismo interno exato de todo aumento de commit privado observado nas
rodadas antigas não foi determinado. Foi demonstrado que esse aumento não
é equivalente a crescimento dos blocos vivos dos catálogos; não foi atribuído
a um subsistema específico nem escondido dos logs. A medição de heap testa
as alocações C++ usadas por esses caminhos, não pretende substituir uma
auditoria geral de toda memória virtual ou de drivers do Windows.

Não foi necessário corrigir o código funcional do LiteSic. A avaliação
prolongada pode continuar com os mesmos executáveis preservados.
