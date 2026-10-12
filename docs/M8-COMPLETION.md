# M8 — conclusão dos requisitos restantes

**M8 — COMPLETE; aceite humano PASS (12/10/2026).**
Decisão expressa do usuário: **M8-DEC-2026-10-12**, solicitação
“LITESIC — ENCERRAMENTO DEFINITIVO DA M8”. Resultados técnicos, evidências
originais e aceites anteriores preservados. Validações físicas, assistivas e
linguísticas não realizadas são observações futuras não bloqueantes e não
reabrem automaticamente a milestone; não são convertidas em PASS técnico.
M0–M7 e M9 permanecem COMPLETE. Próxima atividade: **Auditoria Geral de Desempenho**,
antes da **M10 — Distribuição**; ambas ainda não iniciadas.
Neste encerramento não foram repetidos testes ou auditorias nem implementadas
funcionalidades. Consolidação, commit e push para `origin/master` autorizados.

Base técnica `6ae73fd`; resultados abaixo obtidos antes deste encerramento.

## Contratos implementados

| Requisito | Implementação e infraestrutura reutilizada | Verificação |
|---|---|---|
| T1 — teclado | Despacho único para Z/X/C/V/B/L/Ctrl+D nos três painéis; setas fazem seek de 5 s no player/lista, preservando sliders e navegação do EQ; EDIT mantém edição normal. Ações chamam PlaybackController/Audio/PlWnd existentes. | `test_m8_remaining`, testes anteriores de player/EQ/playlist e inspeção real X/V. |
| T2 — mídia/global | Quatro WM_APPCOMMAND encaminhados ao player. RegisterHotKey opcional Ctrl+Alt+F9 play/pause, F10 stop, F11 anterior, F12 próxima; desativado por padrão. Falha em qualquer registro libera o conjunto e informa conflito. Encerramento libera registros. | Mensagens nos três painéis, conflito real, rollback e liberação. Teclado físico de mídia continua ensaio humano. |
| T3 — topo/zoom | Menu de contexto continua como UI definitiva; Sempre no topo aplica HWND_TOPMOST/NOTOPMOST ao conjunto e persiste. Zoom existente preservado. | Normal/topmost nos três painéis, preferência real em INI, regressões de geometria/hit-test. |
| T4 — perfil/instância | Mutex nomeado Local por caminho de perfil normalizado; propriedade publicada apenas após playlist pronta; IPC WM_COPYDATA UTF-16 limitado a 1 MiB, com confirmação e espera limitada. Segunda instância acrescenta os arquivos na ordem pedida, restaura/ativa a principal e termina. | Dois processos, x86→x64 com nome Unicode, pacote inválido, reinício, crash e perfis independentes. |
| T5 — dados/configuração | Mesmo settings.ini e sessão M3U/sidecar; novos pan, shuffle, repeat, topo, preferência global; posição principal em seção independente, com parser estrito. Estado EQ, tema, idioma, volume, favoritos e docking reutilizados. | Round-trip/bordas/INI inválido/caminhos Unicode, reinício real e suites anteriores. |
| DPI | Fator local de EQ/playlist × zoom comum; Aparência/Informações recalculam tamanho, fontes e filhos enquanto abertas; zoom respeita área útil negativa; limite de escala pela área útil preserva controles essenciais. | 96/120/144/192 DPI, zoom 100/125/150, 24 amostras de produção x86/x64; regressões de docking. |
| Acessibilidade | MSAA IAccessible + IEnumVARIANT para Canvas, nomes/descrições dos tooltips, papéis, estados, ações, valores e foco; eventos WinEvent; nomes de EDIT por IAccPropServices. Tab/Shift+Tab/ativação/foco da playlist completos, com favoritos da seleção. | COM real, enumeração, ações, sliders/valores inválidos, desconexão do provider e árvore nativa Computer Use. |
| Legibilidade | Ajuda completa do EQ e rodapé completo da playlist disponíveis por acessibilidade/tooltips; nomes completos não dependem da abreviação visual. Fontes/scripts/RTL e contraste dos 36 temas preservados. | Catálogos, renderização/contraste existentes e capturas reais representativas. |

## Dados instalados, portáteis e migração

Sem flag, continua `%LOCALAPPDATA%\LiteSic\settings.ini` e `playlist.m3u8`.
Não há migração para APPDATA: o perfil instalado existente permanece no local atual.
`LiteSic.exe --portable` usa `data\` ao lado do executável, independente do cwd.
`--profile=<pasta>` seleciona explicitamente outro perfil, inclusive para testes;
se relativa, essa escolha explícita é resolvida antes da inicialização. `--validation`
continua usando a fixture de desenvolvimento existente. Argumentos de arquivo
são resolvidos pelo processo remetente, antes do IPC; `--` encerra opções.

A pasta precisa permitir gravação. A inicialização verifica a pasta e atributos
read-only de INI/sessão, sem substituir por outro perfil silenciosamente. Falha
mostra mensagem localizada e termina; não exige administração. Perfis portáteis
legados não são movidos, apagados ou importados automaticamente. Para migrar,
fechar o aplicativo e copiar voluntariamente os arquivos de dados existentes para
`data\` (incluindo sidecar e caminhos de mídia válidos); conservar os originais.
Empacotamento, instalador e associações de arquivo são M10 e não foram implementados.

## Falhas e limites

Mutex com proprietário encerrado é recuperado; a instância só publica seu HWND
quando pode receber arquivos. Inicialização concorrente espera até 5 s; IPC usa
SendMessageTimeout de 2 s e exige confirmação. Principal travada, perfis sem acesso,
UIPI/integridade distinta, opções inválidas ou arquivos recusados apresentam erro;
não iniciam uma segunda instância escritora nem descartam o pedido silenciosamente.
Arquivos não suportados/inexistentes são recusados antes de acrescentar o lote.
Pastas reutilizam o scanner existente e seu status de erro. Não há serviço residente.

Atalhos globais não substituem atalhos do Windows, não registram teclas simples ou
teclas de mídia indiscriminadamente e nunca forçam um registro em conflito.
A preferência ativa é salva; conflito no início é informado e o usuário pode tentar
ativar de novo pelo menu. WM_APPCOMMAND desconhecido segue o comportamento Win32.

DPI simulado não equivale a alterar o DPI real do sistema ou movimentar entre dois
monitores físicos. Quando o conjunto excede a área útil, cada superfície mantém
controles dentro da escala calculada e o docking usa sua recuperação existente;
não há novo motor de escala nem alteração de resolução/DPI do Windows.
Ensaio físico multi-DPI, desconexão/reconexão, leitores de tela e legibilidade ampla
permanecem como observações futuras não bloqueantes. Revisão linguística integral/naturalidade/plurais/RTL
dos 18 idiomas permanece separada; quatro novas mensagens por idioma, **241 IDs**.

## APIs e alvo Windows 7

Mantidos C++17/Win32/Direct2D/DirectWrite, variantes W, MSVCRT, alvo 0x0601 e ausência
de threads/mutex C++ no player. Novas bibliotecas são componentes de sistema
oleacc/oleaut32/uuid, sem runtime distribuído adicional. APIs consultadas na
Microsoft: [RegisterHotKey](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerhotkey),
[WM_COPYDATA](https://learn.microsoft.com/en-us/windows/win32/dataxchg/wm-copydata),
[SendMessageTimeoutW](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendmessagetimeoutw),
[LresultFromObject](https://learn.microsoft.com/en-us/windows/win32/api/oleacc/nf-oleacc-lresultfromobject),
[IAccPropServices::SetHwndPropStr](https://learn.microsoft.com/en-us/windows/win32/api/oleacc/nf-oleacc-iaccpropservices-sethwndpropstr).
Esses contratos atendem ao alvo; isso não substitui execução prática nos sistemas
pendentes da M9, que permanece COMPLETE por decisão anterior.

## Resultados e evidências

Validação retomada e concluída em 12/10/2026. Logs finais, hashes dos
executáveis/fontes, capturas e resultados estão em
[evidence/m8-completion](evidence/m8-completion/README.md).
A inspeção inicial por Computer Use confirmou exposição dos três painéis e
alteração acessível do volume 80→55%. Um comando de slider do driver direcionado
pelo índice apresentado como 1 kHz alterou 3 kHz; a captura identificou isso e
não foi declarada validação daquele índice. Um cliente COM separado confirmou por enumeração/nome o child ID 9 = 1 kHz
e alterou/consultou corretamente o ganho de +3 dB através do proxy entre processos.
A regressão COM verifica diretamente
child ID 9 = 1 kHz, sem depender do índice transitório do driver. A seleção pelo
driver e a interoperabilidade assistiva entram nos ensaios finais/humanos.

## Recuperação e conclusão da validação

A entrada já continha T1–T5, runtime de perfis/IPC, DPI local e MSAA implementados.
Foram preservados código, catálogos, aceites T6–T8 e evidências anteriores.
Os logs em `build/m8-completion` mostravam testes anteriores e uma última falha
de link x64 Release por executável aberto; alterações de DPI eram posteriores
à regressão registrada. Por isso as quatro variantes foram recompiladas e
testadas, sequencialmente, com perfis de teste isolados. Não houve reimplementação.

| Variante final | Build | Suíte completa |
|---|---|---|
| x64 Debug | PASS | PASS — 31 executáveis |
| x64 Release | PASS | PASS — 31 executáveis |
| x86 Debug | PASS | PASS — 31 executáveis |
| x86 Release | PASS | PASS — 31 executáveis |

Cada log contém 166 linhas de casos nomeados `PASS`, além das verificações C
que usam outro formato; não somar essa contagem às contagens históricas.
Incluídos testes de áudio offline, DSP/EQ, playlist, integração de janelas,
idioma, tooltips, configuração e nove casos de `test_m8_remaining`.
Nenhum teste foi removido. `git diff --check` PASS.

**Correção desta retomada:** zoom ao vivo deixava pixels do EQ na escala anterior,
pois `SetWindowPos` invalidava apenas faixas recém-expostas. Reproduzido visualmente
em duas capturas; corrigido com invalidação integral em `EqWnd_ScaleChanged`.
O teste de região de atualização falha nos objetos anteriores em
`PtInRegion(dirty,30,30)` e passa nas quatro variantes corrigidas.
Capturas antes/depois e mudança real de zoom 125→150% confirmam o resultado.
Não foram alterados áudio, DSP, modelo da playlist nem motor de docking.

**Atalhos e mídia:** ZXCVB e quatro WM_APPCOMMAND passam nos três painéis;
EDIT preservado; conflitos reais de RegisterHotKey, rollback e desativação PASS.
Computer Use confirmou X/B/C/V e Ctrl+Alt+F9/F10/F11/F12 com uma janela vazia
de outro processo em primeiro plano. Play/Pause, Stop, Próxima e Anterior
observados no player real. Anterior reinicia a faixa após alguns segundos;
no início volta à anterior, conforme PlaybackController existente.
Atalhos desativados não executam Play. Teste de processo confirmou liberação
dos quatro registros no encerramento e persistência de GlobalHotkeys.
Teclas físicas de mídia não foram acionadas: cobertura técnica por WM_APPCOMMAND,
sem alegar certificado de roteamento de todo teclado/driver ou aplicativo externo.

**Topo e perfis:** ativação/desativação pelo menu, flags nos três painéis,
persistência, minimização/restauração real e alternância de aplicação PASS.
As capturas do Computer Use são por janela; não provam sozinhas z-order global.
O estado Win32 complementa essa evidência. Onze cenários de processos PASS:
primeira abertura, segunda instância/arquivos, IPC x86→x64 Unicode e caminho
relativo, restauração minimizada, liberação de hotkeys, três arquivos persistidos,
reinício com outro cwd, preferências, recuperação após kill, perfis independentes,
portátil `data/` e perfil relativo (alguns cenários agrupam estas operações).
Modo instalado validado pelo contrato automatizado LOCALAPPDATA, sem executar
o aplicativo sobre o perfil pessoal. INI read-only e escrita/round-trip PASS.
O perfil portátil de teste é cópia temporária de runtime em build, não pacote M10.

**DPI:** 24 amostras x86/x64: DPI 96/120/144/192 × zoom 100/125/150%,
hit-tests de player/EQ, área negativa, fator independente e área compacta PASS.
Informações e Aparência abertas têm relayout/fontes verificados pela integração.
Host físico: um monitor 3840×2160, DPI 144 (150%), confirmado por GetDpiForWindow.
Inspeção real de controles, textos, menus, tooltip, foco e zoom 100/125/150%
nessa escala; 100/125/200% de DPI são simulados, sem mudar configurações Windows.
Não há segundo monitor exposto para mudança física, reconexão ou origem negativa.

**MSAA:** cliente COM externo consultou todos os filhos expostos nos três painéis
em 18 idiomas: 828 ocorrências na fixture inicial e 900 com três faixas.
Nomes não vazios, papéis, estados, retângulos e contagem/enumeração PASS;
nomes atualizam imediatamente e EDIT de pesquisa permanece nomeado.
Ocultar/reabrir EQ/playlist atualiza INVISIBLE; child 9 do EQ altera 1000 Hz
corretamente, sem depender de índice transitório Computer Use.
Tab apresentou foco visível; COM confirmou child 1/Anterior e STATE_SYSTEM_FOCUSED.
Computer Use apresenta o painel no campo resumido de foco: limite do bridge,
não evidência de ausência de foco MSAA. Provider desconectado, valores inválidos,
ações e estados checked cobertos pela regressão. Não certifica leitores de tela,
controles fora da enumeração ou todos os estados possíveis apenas por ter IAccessible.

**Gráfica e i18n:** Computer Use recuperado; capturas direcionadas de transporte,
menus, atalhos em segundo plano, foco, restart e correção de zoom.
36 guardas do auxiliar local PASS. Teste gráfico de tooltips final PASS:
dez casos, dois adicionais de GUI, 3.092 frames, 500 ciclos, 18 idiomas ×
36 temas × quatro escalas de tooltip; mesmos testes preservados.
Catálogos/inventário/fallback/literais/placeholder: 18 × 241 IDs PASS;
quatro mensagens novas por idioma cobrem topo, atalhos e erros de perfil/instância.
Menus e nomes acessíveis usam os catálogos; revisão semântica humana não comprovada.

## Tentativas, limites e aceite humano

O shell padrão usa `CodexSandboxDesktop-…`, diferente do desktop de entrada
`Default`: SetCursorPos falhou e Computer Use não via suas janelas.
A execução autorizada no desktop interativo resolveu esse impedimento.
O primeiro ensaio interativo de tooltip falhou no hover com a fixture topmost
aberta; sem ela houve uma falha transitória de atualização do tooltip EQ.
Reexecução dos mesmos testes, sem alteração de expectativa ou remoção, passou.
Os logs dessas tentativas foram preservados; não foi demonstrado defeito
persistente de produção nesse tooltip. A falha de repaint EQ foi reproduzível
e recebeu correção e regressão próprias. Warnings existentes de compilação
permanecem nos logs; nenhum erro final de build/teste.

Não há defeito técnico obrigatório conhecido aberto nos cenários executáveis
neste host. **M8 encerrada como COMPLETE, com aceite humano final PASS.**
Observações futuras não bloqueantes: revisão linguística integral/naturalidade/
plurais/RTL dos 18 idiomas, leitor de tela, teclado multimídia físico,
legibilidade ampla e multi-monitor/multi-DPI físicos. Esses ensaios não foram
realizados e não recebem PASS pelo aceite; não bloqueiam nem reabrem a M8.
Aceites T7/T8/M6/M7 e limites históricos da M9 preservados.

## Arquivos e Git

Produto: app/mainwnd/plwnd/eqwnd/theme_window/ui_theme; novos runtime,
ui_scale e ui_accessibility. Makefile e testes config/m8_remaining/i18n,
18 catálogos, fallback incorporado, inventário/exceções; probe de DPI;
relatório, matrizes, roadmap, estado canônico, README e evidências.
Motor de áudio, DSP, visualizadores, modelo da playlist e Skin/window_snap intactos.

Consolidação autorizada de código, testes, catálogos, documentação, diagnósticos
e evidências da M8 em commit sobre `6ae73fd`, para publicação em `origin/master`.
Binários, objetos, perfis e temporários em `build/` permanecem ignorados.
O hash publicado é confirmado no fechamento por Git, sem inserir o próprio hash
no documento que integra o mesmo commit. Nenhuma M10, auditoria geral de
desempenho ou reabertura de M0–M7/M9 integra este encerramento.
