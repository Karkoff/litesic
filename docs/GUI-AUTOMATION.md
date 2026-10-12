## M8-T8 — hover e popup temático

`-Action hover -Target main|playlist|eq|themes|info -X <px> -Y <px>` exige
sessão isolada, observação recente inspecionada, PID/hash/ownership e ponto
não coberto. Move apenas o cursor, espera 850 ms e captura a superfície e
um único `LiteSic.TooltipWnd` owned, se visível. `-Label` cria arquivos
`<label>-context.png` e `<label>-popup.png`. Nenhuma entrada é enviada ao
popup. A observação válida permanece associada à superfície.

`test-test_tooltip.exe --gui` executa testes de popup real em janelas próprias
no desktop interativo; sem esse argumento, a suíte usa consultas estruturais,
reprodução offline, recursos e geometria. `--matrix` restringe o adicional
gráfico à matriz. Os requisitos de autorização da plataforma continuam
aplicáveis; não se contorna o sandbox ou as guardas do driver.

# Automação gráfica de desenvolvimento

## M8-T6 — amostras de idioma

Nesta tarefa o usuário autorizou verificações gráficas. A fixture
`scripts/gui.ps1 -Action launch -Architecture 64 -Configuration release
-Validation -Language ar-SA` seleciona um dos 18 catálogos somente no perfil
isolado. `-Language` sem `-Validation`, ou código fora da lista, é rejeitado
antes de acessar o desktop. O parâmetro não implementa preferência de produto.
`scripts/gui/capture_i18n.ps1` reproduz a sequência launch/capture/close,
sem entrada; requer playlist e EQ visíveis no perfil isolado. Executar com
as guardas e revisão de aprovação existentes. [54 capturas e hashes](evidence/m8-i18n/README.md).

## M7 — sessão isolada e autorização de capturas

Na rodada M7 o usuário autorizou explicitamente capturas **somente das
janelas da sessão isolada**. Essa autorização é específica e posterior à
restrição da auditoria M6 abaixo. A suíte automatizada continua usando
`LITESIC_NO_CAPTURE=1`; a inspeção interativa representativa usa o helper
existente com `-Validation`, sem abrir o perfil pessoal.
Resultados, tentativas interrompidas e limites:
[M7-INTEGRATED-VALIDATION.md](M7-INTEGRATED-VALIDATION.md).

## Auditoria integrada sem capturas — 11/10/2026

Esta rodada tem restrição explícita de screenshots, gravações e capturas
equivalentes. O driver histórico captura automaticamente após certas ações;
essas ações não foram usadas nesta rodada. Os testes Win32 enviam mensagens
às próprias janelas e verificam estado/posição, sem entrada no desktop pessoal.
Use `$env:LITESIC_NO_CAPTURE='1'` antes das suítes: imagens/pixels de janelas
ficam pendentes e são indicados no log; os demais testes continuam executando.
Desenho sintético em Canvas e pintura normal WM_PAINT não capturam janelas.
O probe de desempenho agora usa RedrawWindow, sem PrintWindow nem arquivos
de imagem. [Relatório e evidências atuais](M6-INTEGRATED-VALIDATION-2026-10-11.md).

## M6 — EQ real e sessão isolada

`-Action launch -Validation` passa `--validation` ao executável permitido e
usa exclusivamente `build/gui/m6-session/settings.ini` e `playlist.m3u8`.
Lançamentos normais conservam a sessão pessoal. As mesmas guardas de
PID/criação/hash/proprietário/observação continuam valendo. O menu de presets
agora reconhece `LiteSic.EqualizerWnd` como dono permitido no mesmo processo.

O EQ tem dez bandas e PRE reais, ON/OFF, presets, curva e teclado. Capturas
interativas de arraste, ON, Rock, restauração e temas; integridade das 36
paletas nas quatro variantes e medições estão no
[relatório M6](M6-EQUALIZER-2026-10-11.md). O painel antigo indisponível descrito
nas seções históricas abaixo foi substituído. O usuário confirmou o funcionamento da M6; a rodada posterior de encaixe
e alinhamento está em [WINDOW-SNAP](WINDOW-SNAP-2026-10-11.md).

## Temas — validação atual em 11/10/2026

Após os ajustes de playlist/menus, o seletor tem moldura, botões e rolagem
próprios. O launcher de categorias usa um menu com tema; o ComboBox interno
mantém o modelo de categorias. A ação `theme` usa mensagens escalares desse
controle para voltar a Todos e seleciona o tema no ListBox visível, mantendo
as guardas de processo/proprietário/observação. Menus do seletor também são
reconhecidos como pertencentes ao alvo. Nova rodada: 356 verificações, 27
guardas e 14 capturas. [Relatório](PLAYLIST-UI-2026-10-11.md).
O pacote atual é auditado por `scripts/diagnostics/package_adjustments.py`;
o pacote `themes` abaixo preserva a etapa anterior ao novo rodapé/seletor.

`-Target themes` captura apenas `LiteSic.ThemesWnd` pertencente ao player
lançado pelo auxiliar. `-Action theme -Target themes -Query Claro` seleciona
um nome da whitelist de 36 temas usando os controles nativos, após captura
recente inspecionada. Não aceita nomes arbitrários nem atua no player errado.
O auxiliar agora tem **27 guardas PASS**, incluindo proprietário do seletor,
rejeição de tema/alvo inválidos e conversão de coordenadas da área cliente
com `ClientToScreen` nativo. Isso corrige o deslocamento da barra de título
em janelas com moldura; as janelas sem moldura mantêm a mesma origem.

`scripts/diagnostics/package_themes.py` audita e empacota 25 capturas,
catálogo/paletas, quatro logs de build/teste e quatro cargas de memória.
[Galeria](evidence/themes/index.html), [manifesto](evidence/themes/manifest.json)
e [limites/resultados](THEMES.md). As seções abaixo preservam as rodadas anteriores.

## Modernização Direct2D — 11/10/2026

O backend novo usa `ID2D1DCRenderTarget`, compatível com o caminho de captura
existente: PrintWindow foi demonstrado nas quatro variantes, com imagens reais
iguais do player vazio. Isso não certifica captura genérica de qualquer DirectX.
`-Target eq` seleciona somente `LiteSic.EqualizerWnd`, no mesmo processo lançado
e com o player como proprietário, usando as mesmas guardas da playlist.
O painel EQ atual é indisponível (DSP pendente), não um teste de equalização.

`scripts/gui/verify_ui.py` audita hashes, proveniência, pausa, repetição e geometria
do docking, sem enviar entrada. Manifesto: `build/gui/ui-evidence.json`.
Resultado desta etapa: 142/142 verificações; guardas do driver: 20/20.
Evidências e limites: [relatório UI/UX](design/UI-VALIDATION-2026-10-11.md).

## Infraestrutura e prova anterior

O auxiliar `scripts/gui.ps1` permite ao agente lançar o LiteSic, identificar sua
janela, capturar a interface GDI e enviar mouse/teclado ao processo de teste.
Foi demonstrado no Windows 11 nas quatro variantes x86/x64 Debug/Release.
Não depende da árvore de
acessibilidade dos controles personalizados. O relatório da prova está em
[GUI-VALIDATION-2026-10-11.md](GUI-VALIDATION-2026-10-11.md).

## Ferramentas e instalação

Use Python **3.11 x64** com o launcher `py`, já disponível nesta máquina:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action setup
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action test
```

As bibliotecas ficam exclusivamente em `.tools/gui-venv`, ignorado pelo Git.
O lock de wheels Windows/CPython 3.11 fixa versões e SHA-256: pywinauto 0.6.9,
Pillow 12.3.0, pywin32 312, comtypes 1.4.17 e six 1.17.0. Outros intérpretes
exigem um lock adequado; não retire `--require-hashes` para contornar isso.
`truststore` usa os certificados confiáveis do Windows; a validação HTTPS
continua ativa. `ExecutionPolicy Bypass` vale somente para aquele processo.

Nada disso é vinculado ao LiteSic.exe. Nenhum MCP comunitário, serviço Windows,
endpoint de rede, tarefa agendada ou dependência Python global foi instalado.
Um MCP adicional não foi necessário: o script local fornece operações curtas,
auditáveis e limitadas ao aplicativo, sem manter acesso remoto ao desktop.

## Desktop interativo e sandbox

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action doctor
```

`interactive: true` requer `WinSta0`, desktop atual `Default` e desktop de entrada
`Default`. Nesta sessão, comandos comuns do agente usam `CodexSandboxDesktop`,
embora o número da sessão seja o mesmo do Explorer. O driver recusa captura,
lançamento e entrada nesse desktop isolado. `doctor`, `fixture` e `test` podem
rodar isolados.

Para operar a GUI, execute os comandos em um terminal local do usuário, com
o Windows desbloqueado. O agente pode executar cada comando específico fora
do sandbox mediante a revisão de aprovação da plataforma, como nesta prova.
Isso não muda a configuração global do sandbox nem cria uma exceção permanente.
Se essa revisão não permitir a operação, o usuário deve executar o mesmo comando
no terminal interativo e disponibilizar a captura gerada. Não troque o desktop
da thread, não tente desbloquear o Windows e não execute como serviço/SYSTEM.

## Ciclo de observar, agir e verificar

Compile a variante desejada com `scripts/dev.ps1` antes de iniciar. O driver
aceita somente os quatro caminhos `build/{32,64}/{debug,release}/LiteSic.exe`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action launch -Architecture 64 -Configuration debug -Label initial
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action inspect
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action capture -Label before-action
```

Abra `build/gui/before-action.png` com a ferramenta de visualização de imagem
do agente e inspecione o controle. Escolha coordenadas em pixels físicos do
cliente, a partir dessa imagem. Após os ajustes dos controles, um exemplo
observado com cliente 413 × 270 e escala 150% foi Play em `(74,150)`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action click -X 74 -Y 150 -Label after-play
```

As coordenadas são exemplos desta captura; não as reutilize automaticamente
com outra escala ou versão. O comando captura novamente após a ação. Inspecione
o resultado antes da próxima entrada. A observação é consumida por uma ação,
expira em 120 segundos e perde validade se a janela mudar de posição/tamanho.
Repetições de captura para observar timers podem usar nomes diferentes.

Mouse: `click`, com `-Button left` ou `right`. Teclado: `key`, com `-Key escape`,
`enter`, `tab`, `space`, `left`, `right`, `home` ou `end`. Não há entrada de comandos
de terminal, texto arbitrário, teclas Windows, clipboard ou anexação por PID
arbitrário. Para um diálogo, acrescente `-Dialog`; o driver exige que pertença
à janela principal do processo lançado.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action capture -Dialog -Label dialog-before
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action key -Dialog -Key escape -Label escape-return
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action close
```

`close` fecha somente a instância iniciada pelo auxiliar. Encerre-a antes de
recompilar. A sessão registra PID, instante de criação, caminho canônico e hash
do executável; mudança de identidade ou de binário causa recusa. O driver não
fecha outras instâncias do LiteSic que o usuário possa estar usando.

## Reproduzir a prova com áudio silencioso

`fixture` gera um WAV PCM estéreo 48 kHz de 120 segundos, totalmente silencioso,
em `build/gui/gui-silence.wav`. Abra **Adicionar → Arquivos...** na playlist
e inspecione a captura do diálogo retornada. O botão Abrir foi removido do
player principal. Após carregar pela playlist, clique em Play no player.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action fixture
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action open-fixture -Dialog -Label loaded
```

`open-fixture` com a fixture padrão aceita somente Abrir/Open e digita o
caminho da fixture gerada, confirma o foco/texto do campo e envia Enter. O
player deve mostrar `gui-silence`, 48 kHz estéreo e duração `02:00`.

## Operações da M5

A janela nova exigiu ampliar o alvo antes limitado à janela principal.
`-Target playlist` seleciona somente `LiteSic.PlaylistWnd`, no PID lançado e
com a janela principal como proprietária. `-Target menu` seleciona um popup
`#32768` do mesmo processo; menus aninhados exigem `-MenuIndex 0..3` explícito
quando houver ambiguidade. O HWND/retângulo observado continuam sendo
verificados antes de cada ação. Nenhum alvo aceita PID ou caminho arbitrário.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action m5-fixtures
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action capture -Target playlist -Label playlist-before
# Inspecione playlist-before.png e escolha coordenadas para a escala atual.
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action click -Target playlist -X 80 -Y 96 -Double -Label double-click
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action click -Target playlist -X 80 -Y 150 -Modifier ctrl -Label select-another
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action key -Target playlist -Key delete -Label removed
```

`-Modifier shift` testa seleção por intervalo. As teclas permitidas também
incluem `up`, `down`, `delete` e `ctrl-a`. `drag` exige `-X/-Y/-EndX/-EndY`;
seus pontos ficam na janela observada e a identidade sob o cursor é verificada
durante o movimento. Pode testar reordenação, barra de rolagem e redução de
tamanho por arraste de borda. `resize -Target playlist -Width ... -Height ...`
usa apenas a janela própria, limitado a 275–1200 × 180–1000 pixels; é uma
operação de geometria, diferente da prova de arraste real da borda.

`m5-fixtures` gera três WAV silenciosos de 120 s, cópias dos arquivos de tags,
um WAV propositalmente inválido e 2.000 WAV pequenos, todos em `build/gui`.
Nos diálogos próprios, `open-fixture -Dialog -Fixture ...` aceita uma lista
fixa: `silence`, `multi`, `tags`, `invalid`, `folder`, `large-folder`,
`m3u-save` e `m3u-open`. Cada opção exige o título esperado do diálogo.
O salvamento fica restrito a `build/gui/m5-export-validated.m3u`. Se esse
arquivo já existir, a confirmação de sobrescrita exige decisão manual;
o helper não confirma automaticamente nem apaga o arquivo.

As capturas novas registram também PID, criação, caminho e SHA-256 do
executável efetivamente conectado, além do hash do PNG. Os parâmetros de
arquitetura/configuração são usados no lançamento; operações posteriores
usam a identidade registrada em `session.json`.

Python x64 emite um aviso do pywinauto ao conectar ao executável x86. As
operações Win32 utilizadas foram demonstradas com sucesso nas duas variantes
x86; o aviso não foi ocultado e não foi instalado outro Python.

O drop físico pelo Explorer não faz parte deste helper restrito. A M5 tem
testes Win32 que entregam `HDROP` válido dentro do processo da janela. Uma
tentativa de enviar um HGLOBAL de outro processo não acrescentou faixas e foi
retirada do helper: isso não é evidência de drop funcional. O gesto com o
Explorer permanece no checklist manual, sem automatizar outras janelas.

Resultados e capturas: [validação da M5](M5-VALIDATION-2026-10-11.md).

Após reproduzir os cenários, `scripts/gui/verify_m5.py` audita os registros e
imagens, sem enviar entrada. Ele verifica hashes, identidade das quatro
variantes, seleção, pausa/retomada, fim da faixa, geometria e o resultado WMP.
Seu manifesto fica em `build/gui/m5-evidence.json`. São verificações de pixels
e proveniência que complementam a inspeção visual, sem OCR ou avaliação audível.
`verify_poc.py` preserva a prova histórica; capturas antigas sem metadados do
processo recebem hash de executável desconhecido, nunca o da sessão atual.

Capture `play-click` ao clicar em Play. Aguarde alguns segundos e capture
`playing-a`: o LCD e seek devem avançar. Clique Pause (no layout atual, `(121,150)`)
com label `paused-a`, aguarde pelo menos três segundos e capture `paused-b`:
o relógio deve permanecer igual. Clique Play com label `resumed`, aguarde e
capture `resumed-later`. Clique Stop (no layout atual, `(170,150)`) com label `stopped`:
relógio e seek devem voltar ao início. Reabra/carregue a fixture com label
`reloaded`; reabra o diálogo pela playlist e cancele com Escape, label `escape-return`.

Com esses nomes e a captura `initial`, verifique a evidência:

```powershell
.tools/gui-venv/Scripts/python.exe scripts/gui/verify_poc.py
```

O verificador associa os PNGs aos hashes/eventos registrados, compara a área
do LCD e a estabilidade da pausa. Ele complementa a inspeção visual; não faz
OCR, não verifica qualidade audível e não substitui testes funcionais completos.

## Evidências e limites

### Encerramento da M5

A rodada atual está em [M5-CLOSEOUT-2026-10-11.md](M5-CLOSEOUT-2026-10-11.md),
com 33 PNGs selecionados e manifesto em `docs/evidence/m5-closeout/`.
Esse pacote versionável complementa os artefatos temporários ignorados abaixo.
Não inclui a playlist pessoal usada como backup.

O auxiliar aceita `-Action search -Target playlist -Query Teste-020` depois
de uma captura recente e clique inspecionado no campo. As únicas consultas
permitidas são `Teste-020`, `Alpha` e `inexistente-validacao`; o Edit visível
deve ter foco antes da digitação. As fixtures `m3u-save-closeout` e
`m3u-open-closeout` usam exclusivamente `build/gui/m5-closeout-export.m3u8`.
As guardas foram ampliadas para 24 testes. Diagnósticos passivos, comparação
de renderers e auditoria do pacote estão em `scripts/diagnostics/`; comandos
e limites metodológicos constam no relatório de encerramento.

`build/gui/` contém PNGs reais, `events.jsonl`, `session.json`, fixture e
`poc-verification.json`. Capturas usam `PrintWindow`/GDI na janela alvo, inclusive
quando parcialmente fora da tela, sem fotografar o desktop inteiro. São imagens
renderizadas pela janela real; a prova de timers e entradas demonstrou mudanças
reais. Não são mockups nem reconstruções do código. O teste rejeita imagem
uniforme/vazia. Isso não certifica captura de aplicativos DirectX ou de vídeo.

Os arquivos de evidência e o venv são ignorados pelo Git. Imagens de diálogos
podem conter nomes de arquivos locais; revise antes de compartilhar. Logs
registram operações, resultados e hashes. Tentativas malsucedidas permanecem
registradas e não contam como aprovação.

O foco e a janela sob o ponto são verificados antes de clicar. Apesar dessas
proteções, o mouse/teclado usam o desktop ativo: evite entrada simultânea do
usuário durante os poucos instantes de uma ação. As restrições do driver limitam
suas operações; não são uma fronteira de segurança contra alguém que possa
editar os próprios scripts/arquivos do projeto.

## Seleção de idioma e informações (M8-T7)

`-Action language -Query <código|automatic>` executa o comando real do submenu
Idioma, exclusivamente em sessão lançada com `-Validation`; exige observação
recente do player principal. Diferentemente de `-Language` na inicialização
(fixture T6), aplica e persiste a preferência no perfil isolado. Nunca usa INI
pessoal nem altera preferências Windows. Exemplos:

```powershell
powershell -NoProfile -File scripts/gui.ps1 -Action capture -Target main -Label language-before
powershell -NoProfile -File scripts/gui.ps1 -Action language -Query ar-SA -Label language-ar
powershell -NoProfile -File scripts/gui.ps1 -Action capture -Target info -Label language-info
```

`-Target info` aceita apenas `LiteSic.TrackInfoWnd` do mesmo PID, com owner
principal ou playlist pertencente ao principal. Janelas alheias são recusadas.
`scripts/gui/capture_language_selection.ps1` captura os 18 idiomas nas cinco
janelas já abertas em uma sessão isolada. Não abre diálogos por comandos
arbitrários. Inspecionar resultados, não apenas retorno de execução.
32 guardas atuais; [evidências e limites T7](M8-T7-LANGUAGE-SELECTION.md).

## Computer Use nativo — disponibilidade

Atualização da consolidação T6–T8: `@oai/sky` listou janelas e capturou/operou
a sessão isolada x64 Release nesta execução. Play/Pause/Stop foram inspecionados
e capturas preservadas em [evidências novas](evidence/m8-consolidation/README.md).
O diagnóstico de indisponibilidade abaixo é histórico, não o resultado atual.
O auxiliar restrito continua disponível para operações que a API nativa não expõe.

Prefira a integração nativa quando estiver disponível. Nesta instalação, os
plugins Computer Use estão habilitados e `@oai/sky` importa, mas `list_windows`
falhou duas vezes, inclusive após reset do REPL, com `native pipe is unavailable`
e `os error 2`. O canal nativo não estava disponível. Isso é separado do desktop
isolado usado pelos comandos de shell. A causa anterior à criação/conexão desse
canal não foi comprovada; não há evidência suficiente para atribuí-la a uma
permissão específica ou versão do Codex.

Nenhuma configuração global do Codex foi alterada. Para retestar a integração,
salve o trabalho e reinicie o aplicativo hospedeiro; em Plugins > Computer Use,
confira servidor/skill habilitados e, em Settings > Computer use, o acesso ao
LiteSic. Um agente deve reinicializar `@oai/sky` e conseguir listar janelas antes
de declarar o recurso nativo recuperado. Nesta entrega, a capacidade demonstrada
é o auxiliar local, com execução pontual no desktop interativo.

Referências primárias: [Computer Use — OpenAI](https://learn.chatgpt.com/docs/computer-use),
[pywinauto: execução em desktop](https://pywinauto.readthedocs.io/en/latest/remote_execution.html),
[pywinauto: click_input](https://pywinauto.readthedocs.io/en/latest/code/pywinauto.base_wrapper.html),
[PrintWindow — Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-printwindow),
[SendInput e UIPI — Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput).
