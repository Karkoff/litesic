# M8 — matriz atual dos requisitos restantes

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

Os baselines e pendências abaixo são históricos; o encerramento acima prevalece
para o estado atual. [Relatório final M8](M8-COMPLETION.md).

[Implementação, contratos, testes e limites atuais](M8-COMPLETION.md).

Retomada final: quatro builds/suítes PASS (31 executáveis por variante), 11
cenários de processos, 24 amostras DPI simuladas, enumeração MSAA externa dos
controles em 18 idiomas, 36 guardas e ensaio gráfico de tooltip (3.092 frames)
PASS. Computer Use confirmou atalhos globais em segundo plano, foco, menus,
minimização/restauração e zoom real no único monitor físico a 150%.
Defeito encontrado de repaint parcial do EQ ao mudar zoom corrigido; teste
reproduz falha antes e PASS depois nas quatro variantes. Nenhum teste removido.
Evidências finais em [m8-completion](evidence/m8-completion/README.md).
Revisão linguística humana, leitor de tela, teclado de mídia e matriz física
multi-DPI/monitores não comprovados; aceites anteriores preservados.

| Requisito | Entrada | Estado atual | Entrega/evidência |
|---|---|---|---|
| M8-T1 | Parcial | IMPLEMENTED_AND_VERIFIED | ZXCVB/L/Ctrl+D em três painéis; EDIT protegido; seek preserva navegação/slider local; integração e regressões. |
| M8-T2 | Ausente | IMPLEMENTED_AND_VERIFIED | Quatro WM_APPCOMMAND, atalhos globais opt-in seguros, conflitos/rollback/liberação; testes com mensagens reais. |
| M8-T3 | Parcial | IMPLEMENTED_AND_VERIFIED | Menu topo persistente para conjunto; zoom existente e correções DPI/hit-test. |
| M8-T4 | Ausente | IMPLEMENTED_AND_VERIFIED | Perfil/mutex/IPC Unicode limitado, confirmação/timeout, dois processos, crash, perfis distintos e x86→x64. |
| M8-T5 | Parcial | IMPLEMENTED_AND_VERIFIED | Campos faltantes, geometria principal independente; instalado/portátil explícito; sem migração destrutiva; round-trip/reinício. |
| M8-T6 | Verificada | IMPLEMENTED_AND_VERIFIED | Infraestrutura reutilizada, 18 × 241 IDs, fallback e cobertura; linguística humana separada. |
| M8-T7 | Aceita | ALREADY_ACCEPTED | Aceite PASS preservado; seleção/troca/persistência atuais em regressão. |
| M8-T8 | Aceita | ALREADY_ACCEPTED | TUDO PASS preservado; tooltips reutilizados em MSAA e informações completas. |
| DPI negativo | Parcial | IMPLEMENTED_AND_VERIFIED | Clamp pela área útil do monitor, não pela origem global zero. |
| DPI independente EQ/PL | Parcial | IMPLEMENTED_AND_VERIFIED | DPI local × zoom compartilhado; mudar player não substitui o fator do painel. |
| Aparência/Informações abertas | Parcial | IMPLEMENTED_AND_VERIFIED | WM_DPICHANGED, zoom, layout/filhos/fontes e tooltips locais. |
| Área útil compacta | Parcial | IMPLEMENTED_AND_VERIFIED | Escala limitada pela área útil, sem alterar preferência de zoom ou DPI global. |
| Foco playlist | Parcial | IMPLEMENTED_AND_VERIFIED | Tab/Shift+Tab, pesquisa, abas, limpeza, linhas/favoritos, menus e ocultar; foco visível e edição preservada. |
| Nomes/papéis/estados/ações | Parcial | IMPLEMENTED_AND_VERIFIED | IAccessible/IEnumVARIANT, WinEvent, setters limitados, annotations dos EDITs, árvore nativa e regressão COM. |
| Rótulos/ajuda completos | Parcial | IMPLEMENTED_AND_VERIFIED | Nomes/descrições completos e ajuda/rodapé via tooltip/MSAA; ellipsis visual preservada. |
| Matriz física multi-DPI/monitores | Ensaio indisponível | IMPLEMENTED_NOT_VERIFIED | Simulações/produção não certificam transferência física, desligamento/reconexão ou menus em toda a matriz. |
| Leitor de tela/legibilidade ampla | Humana | IMPLEMENTED_NOT_VERIFIED | Provedor e bridge nativos verificados; ensaio humano assistivo/naturalidade não realizado. |
| Aceite final M8 | Ausente | PENDING_HUMAN_ACCEPTANCE | T1–T5 técnicas verificadas não concedem aprovação visual/humana da milestone. |

## Quinze critérios de internacionalização

I01–I10 e I13–I15: infraestrutura/seleção/fallback/plurais/cobertura/recursos
reutilizados e regressão atual. I11: lacunas técnicas de DPI/adaptação/identificação
corrigidas, com inspeção representativa; matriz completa humana continua pendente.
I12: Unicode, fontes, shaping/RTL preservados e testados tecnicamente; revisão
linguística de scripts/RTL complexos permanece humana. **Revisão linguística
integral não concedida**. Consultar os 15 critérios históricos abaixo e o
[contrato oficial](M8-I18N-REQUIREMENTS.md); não há alteração indiscriminada dos
catálogos anteriores nem nova aprovação T7/T8.

## Persistência e reutilização

Volume/EQ/tema/idioma/zoom/visibilidade/lista/favoritos/seleção/docking já existiam
com testes; preservados. Novos PanPermille/Shuffle/Repeat/Topmost/GlobalHotkeys
usam a mesma seção Appearance. PlayerWindow mantém posição independente do sucesso
da sessão M3U. Instalação continua LOCALAPPDATA; `--portable` usa data junto ao exe.
Sem migração automática: cópia manual voluntária é a política explicitada.

M0–M7 e M9 permanecem COMPLETE. M10 e auditoria geral de desempenho não iniciadas.
Aceite final M8 e revisão linguística continuam pendentes; logs/hash/capturas
atuais e Git ficam no [relatório](M8-COMPLETION.md).

---

## Anexo histórico — auditoria anterior sobre 6ae73fd

Todas as classificações, contagens, falhas reproduzidas e referências a commit/push
abaixo pertencem à execução anterior, preservadas como evidência da entrada.
Os probes foram atualizados nesta rodada para verificar as correções; os logs
antigos em m8-consolidation continuam intactos.

### Auditoria integral anterior dos requisitos restantes

Auditoria em 11/10/2026 America/Sao_Paulo, 12/10 UTC. **M8 OPEN/IN_PROGRESS;
não está pronta para aceite final. M8-T6, T7 e T8 COMPLETE.**
M0–M7 e M9 permanecem COMPLETE; M10 permanece aberta.

## Escopo, fontes e método

Fontes oficiais: [ROADMAP](ROADMAP.md), [M8 e seu checkpoint](milestones/M8.md),
[15 critérios de internacionalização](M8-I18N-REQUIREMENTS.md), requisitos
transversais DPI/acessibilidade/refinamento do
[plano visual](design/LITESIC-UI-UX-PROMPT-CODEX.md), AGENT.md e arquitetura atual.
As matrizes abaixo desdobram os oito requisitos oficiais e os critérios
adicionais; não criam tarefas novas nem somam subcritérios como milestones.

Inspecionados diff completo, arquivos novos, arquitetura de entrada/renderização,
catálogos, persistência, skin/window_snap, código e testes relacionados.
Existência de código, teste automatizado, inspeção do agente e aceite humano
são evidências distintas. Relatórios antigos são históricos.

Classificações: **IMPLEMENTED_AND_VERIFIED** (comportamento coberto),
**IMPLEMENTED_NOT_VERIFIED** (código sem prova suficiente),
**PARTIALLY_IMPLEMENTED** (falta parte concreta do contrato),
**NOT_IMPLEMENTED**, **BLOCKED** (ensaio indisponível) e
**ALREADY_ACCEPTED** (aceite humano documentado, sem inventar nova validação).

## T6, T7 e T8 consolidadas

| Entrega | Estado | Evidência e limite |
|---|---|---|
| T6 — catálogo, Unicode, fontes | COMPLETE / IMPLEMENTED_AND_VERIFIED | 18 catálogos × **237 IDs atuais**; testes/validador novos PASS. Os 188 IDs do relatório T6 são históricos. Revisão linguística integral pendente. |
| T7 — seleção Windows/manual, persistência, cinco janelas | COMPLETE / ALREADY_ACCEPTED | PASS humano anterior preservado; nova regressão de núcleo/integração PASS. Os 191 IDs do relatório T7 são históricos. |
| T8 — tooltips | COMPLETE / ALREADY_ACCEPTED | Usuário declarou **TUDO PASS** na avaliação final. Nova regressão funcional/gráfica PASS; 46 chaves `tooltip.*` em cada idioma. Aceite visual não implica revisão linguística profissional dos 18 idiomas. |

T6–T8 publicadas no commit **`1aa560fc0978ba8b53b07bd24746e5d62eb654fd`**,
mensagem `feat(litesic): complete M8 internationalization and multilingual tooltips`.
`git ls-remote origin refs/heads/master` confirmou o mesmo hash após o push.
[Consolidação e evidências novas](M8-CONSOLIDATION.md).

## Matriz principal: todos os requisitos oficiais M8

| ID / especificação | Classificação | Código funcional e testes | Aceite / comportamento faltante |
|---|---|---|---|
| T1 — Z anterior, X play, C pause, V stop, B próxima, seek por setas, L abrir, Ctrl+D aleatório; foco nos três painéis | PARTIALLY_IMPLEMENTED | mainwnd `WM_KEYDOWN`: Tab/Space/Enter e setas de slider focado; EQ navega/ajusta bandas; playlist seleciona/toca/remove. `test_player_window`, `test_eq_window`, `test_playlist_window` PASS. | Não há tabela/despacho global ZXCVB/L/Ctrl+D; setas dependem do controle focado. Atalhos de transporte não funcionam a partir de EQ/playlist. Preservar edição de pesquisa e navegação local. |
| T2 — quatro teclas de mídia | NOT_IMPLEMENTED | Não há `WM_APPCOMMAND`/APPCOMMAND_MEDIA_* em src. | Play/Pause, Stop, Next, Previous não têm despacho multimídia. Falta teste de mensagens e confirmação com teclado físico disponível. |
| T3 — sempre no topo e zoom | PARTIALLY_IMPLEMENTED | app InitDpiAwareness; main ApplyInterfaceScale/DPIUnscale; Canvas; persistência ScalePercent; `test_appearance`, `test_player_window`, `test_window_integration` e probe novo. | Não há toggle HWND_TOPMOST/HWND_NOTOPMOST. Zoom existente é reutilizável; há lacunas específicas de posição negativa, DPI independente/auxiliar e telas pequenas descritas abaixo. Menu existente funciona; decidir sua forma definitiva sem substituição desnecessária. |
| T4 — instância única e abertura de arquivos | NOT_IMPLEMENTED | app wWinMain: sem mutex nomeado/WM_COPYDATA, sem parsing dos arquivos normais de linha de comando. | Falta localizar instância, encaminhar arquivos, acrescentar à lista e trazê-la à frente. `--validation` é fixture de desenvolvimento, não contrato de abertura do Explorer. |
| T5 — INI, instalado/portátil e sessão | PARTIALLY_IMPLEMENTED | ui_theme, eq_state, plwnd Geometry/Selection + M3U; testes round-trip existentes PASS. Não existe config.cpp separado e não é necessário criá-lo só para cumprir um nome de arquivo. | Faltam balanço, shuffle/repeat, estado futuro topo, modo portátil explícito `data/`, migração compatível e garantia de posição principal independente de salvar a sessão PL. Reutilizar INI existente. |
| T6 — internacionalização integral e tipografia | IMPLEMENTED_AND_VERIFIED | i18n, fallback incorporado, 18 messages.tsv, Canvas, inventário/literais; `test_i18n`, `test_i18n_window`, validador PASS. | Entrega técnica COMPLETE. Revisão linguística/RTL complexo e critérios de legibilidade global continuam no checkpoint geral, sem reabrir a tarefa. |
| T7 — Windows automático/manual, menu e atualização ao vivo | ALREADY_ACCEPTED | language_selection, notificação cinco janelas, Language no INI; `test_language_selection`, `test_language_window` PASS. | PASS humano preservado. Reavalia Windows ao iniciar/selecionar Automático; não há listener de alteração externa ao vivo, que não foi exigido como novo bloqueador. |
| T8 — tooltips explicativos, estado real, tema, DPI e ciclo de vida | ALREADY_ACCEPTED | ui_tooltip, hit-tests e AttachControl; oito casos funcionais/variante e dois gráficos adicionais PASS. | TUDO PASS humano. Descrições leem estado funcional/publicado ao DSP. Limites gerais DPI e de acessibilidade não são certificados por esse aceite. |

## T5: campos existentes e lacunas precisas

| Contrato | Classificação | Evidência / falta |
|---|---|---|
| Volume | IMPLEMENTED_AND_VERIFIED | VolumePermille, restauração no player; round-trip/bordas em test_appearance. |
| Balanço | NOT_IMPLEMENTED | g_panValue inicia no centro; não há campo persistido/restore. Não confundir atuação sonora testada com persistência. |
| EQ enabled, pré, dez bandas, preset | IMPLEMENTED_AND_VERIFIED | Eq::Configure/Save via seção Equalizer; testes EQ de persistência, bordas/ganhos negativos. Não modificar DSP. |
| Shuffle e Repeat | NOT_IMPLEMENTED | Estado funcional existe, mas g_shuffleOn/g_repeatMode não têm save/restore no INI. |
| Sempre no topo | NOT_IMPLEMENTED | Toggle e campo não existem; incluir quando T3 entregar comportamento. |
| Zoom, tema, FFT/OSC e visibilidade dos painéis | IMPLEMENTED_AND_VERIFIED | Appearance, ScalePercent e testes round-trip/estado preservado. |
| Playlist, ordem, favoritos e seleção | IMPLEMENTED_AND_VERIFIED | M3U UTF-8, sidecar Selection/Geometry, sessões testadas; preservar infraestrutura da M5/M7. |
| Geometria/dock de EQ/playlist e origem principal | PARTIALLY_IMPLEMENTED | SaveEqualizerPosition + SaveGeometry/RestoreGeometry; origem principal fica no sidecar da playlist. Restore depende de Geometry válida e salvar sessão com sucesso. Falta garantia independente desse acoplamento. |
| Pasta instalada do usuário | IMPLEMENTED_AND_VERIFIED | LOCALAPPDATA/LiteSic/settings.ini e playlist.m3u8. Decisão atual documentada; APPDATA do plano antigo não justifica migrar novamente sem necessidade. |
| Portátil, sinal explícito e migração | NOT_IMPLEMENTED | Não há decisão de runtime para usar data/ ao lado do exe nem migração do perfil. `--validation` não é modo portátil. Definir contrato sem criar INI concorrente. |
| Round-trip integral de todos os campos | PARTIALLY_IMPLEMENTED | Tests de Appearance/EQ/Language/playlist cobrem campos existentes. Campos ausentes ainda precisam de testes de borda/Unicode e reinício real. |

## DPI, DIPs, fontes, dimensões e resoluções

Novo diagnóstico [m8_remaining_probe.cpp](../scripts/diagnostics/m8_remaining_probe.cpp)
usa as janelas e handlers de produção, com persistência desativada. Executado
em x64 e x86 Release: **12 combinações por arquitetura**, 96/120/144/192 DPI ×
zoom 100/125/150%; verifica dimensões físicas e centros de hit-test do player
e das onze regiões de sliders do EQ. São **24 amostras complementares**,
não novos casos da suíte de 716. PASS significa que o probe reproduziu os
resultados registrados, inclusive lacunas esperadas; não aprovação dessas lacunas.

| Requisito | Classificação | Evidência / resultado preciso |
|---|---|---|
| Escala única no caminho de desenho | IMPLEMENTED_AND_VERIFIED | Canvas cria DC target a 96 DPI, usa uma transformação scale; layout/fontes em DIPs. Largura player/EQ = round(460 × DPI/96 × zoom) nas 24 amostras; não foi observada multiplicação duplicada nesse caminho. |
| Conversão pixel → DIP do mouse | IMPLEMENTED_AND_VERIFIED | Player DPIUnscale, EQ divisão por scale, PL regiões S(); hit-tests novos e testes de controles existentes PASS. Não afirmar que todas as coordenadas legadas foram provadas. |
| Fontes e controles proporcionais | IMPLEMENTED_AND_VERIFIED | DirectWrite usa métricas lógicas; EDIT/fontes GDI usam S(); test_ui_draw/test_i18n_window renderizam 100/125/150/200%. |
| Player/PL/EQ em 100/125/150/200% | IMPLEMENTED_AND_VERIFIED | Mensagens WM_DPICHANGED e testes de formação/controles PASS no cenário simulado de escala comum. Desenho, dimensões e entrada compartilham fator. |
| Monitores independentes com DPI diferentes | PARTIALLY_IMPLEMENTED | EQ/PL recebem DPI próprio, mas EqWnd_ScaleChanged/PlWnd_ScaleChanged usam fator do player. Probe: EQ a 192 DPI mede 920 px; mudança do player a 120 DPI reduz EQ a 575 px. Falta manter DPI local × zoom comum para painéis independentes. Não equivale a reprodução física entre dois monitores. |
| Aparência ao mudar DPI/zoom enquanto aberta | PARTIALLY_IMPLEMENTED | ShowThemes captura escala inicial; sem WM_DPICHANGED/relayout. Probe 96 → 192 mantém largura 520 px. Falta atualizar tamanho, regiões e fontes dos filhos. Reabrir calcula nova escala, mas não resolve janela aberta. |
| Informações ao mudar DPI/zoom | PARTIALLY_IMPLEMENTED | InformationProc tem WM_SIZE, mas não WM_DPICHANGED; usa variável scale compartilhada com playlist, com fonte criada em tema/idioma. Falta fator local e relayout/fontes consistente. Constatação de código, sem novo ensaio físico. |
| Zoom em coordenadas negativas | PARTIALLY_IMPLEMENTED | ApplyInterfaceScale limita x/y a zero. Probe posicionou (-900,-200) e zoom produziu (0,0). Falta limitar pela área útil do monitor, preservando origens negativas válidas. |
| Menus nativos temáticos | IMPLEMENTED_NOT_VERIFIED | MenuSkin mede texto/fontes GDI físicas com S(), desenho recebe rcItem físico, navegação Windows preservada. Inspeções históricas representativas existem; não houve novo ensaio de menus em todas as escalas/múltiplos monitores. Não há evidência suficiente para afirmar dupla escala nativa. |
| Tooltips | ALREADY_ACCEPTED | Medição/layout DirectWrite, escala da superfície, limites de monitor, hover e RTL; Place cobre negativos/arestas e --gui cobriu quatro escalas. Herdam fator da superfície; não corrigem DPI ausente da janela host. |
| Telas compactas e conjunto maior que área útil | PARTIALLY_IMPLEMENTED | Player fixo 460 DIP: 200% = 920×580 px. KeepVisible em área 800×600 conserva largura 920, excedendo a direita; não reduz/reflui controles. Zoom 150% com DPI 200% chega a 1380 px. Falta regra de adaptação que preserve controles essenciais sem mudar DPI global. |
| Full HD/alta resolução e matriz física | IMPLEMENTED_NOT_VERIFIED | Uso atual Windows 11 e capturas históricas; simulação 96–192 DPI. Não foi trocado DPI/resolução do Windows nesta execução; não há matriz física completa. |

Não implementar outro sistema de DPI. Corrigir futuramente os pontos concretos
na infraestrutura existente e acrescentar regressões de cada cenário.
O probe novo não alterou produto, monitores ou configurações pessoais.
Sua primeira compilação falhou por cwd do shell MSYS2; o runner recebeu
CHERE_INVOKING e as duas execuções finais passaram, sem correção no produto.

## Monitores e fronteira com M7

| Requisito / cenário | Classificação | Evidência e limite |
|---|---|---|
| Snap entre todos os pares, movimento transitivo, ocultar/restaurar | ALREADY_ACCEPTED | M7 TUDO PASS; regressões window_integration/window_snap PASS. Não reabrir M7. |
| Resoluções diferentes, negativos e lacunas entre áreas | IMPLEMENTED_AND_VERIFIED | WindowSnap::Covered/Graph/KeepVisible, test_window_snap com topologias simuladas. Sem inventar teste físico. |
| Limites da área útil e recuperação fora da tela | IMPLEMENTED_AND_VERIFIED | Skin::Recover/WorkAreas, WM_DISPLAYCHANGE; teste move EQ para 90000 e recupera. Janela maior que área útil é limitação M8 de adaptação, descrita acima. |
| Alteração da resolução e desconexão | IMPLEMENTED_NOT_VERIFIED | WM_DISPLAYCHANGE chama Recover nos três painéis, monitor mais próximo e área atual. Existem simuladores; sem alteração física de resolução/desconexão nesta sessão. |
| Reconexão de monitor | IMPLEMENTED_NOT_VERIFIED | Reenumeração de áreas ocorre em Recover. Não existe snapshot por identidade do monitor para restaurar automaticamente a antiga disposição ao reconectar. Essa restauração específica não está exigida no roadmap e não vira bloqueador novo. |
| Transferência com escala comum | IMPLEMENTED_AND_VERIFIED | Movimento/âncoras e reflow da formação sob WM_DPICHANGED testados. DPI independente é lacuna M8, não reabertura de docking M7. |
| Transferência física entre DPIs distintos | BLOCKED | Não há ambiente controlado de dois monitores com DPIs distintos para esse ensaio. Bloqueio do ensaio físico, não da análise/implementação autônoma nem do status M7/M9. |
| Restauração de posições/dock | IMPLEMENTED_AND_VERIFIED | EQ/PL/pais e recuperação testados; completar garantia principal na T5. |

## Refinamento visual, legibilidade e acessibilidade

| Requisito | Classificação | Código/teste/aceite e falta concreta |
|---|---|---|
| Tokens/coerência nos 36 temas | ALREADY_ACCEPTED | M6 final visual/funcional aceito; test_themes e tooltips novamente PASS. Não foi concedido aceite individual de cada combinação tema/idioma/DPI. |
| Contraste | IMPLEMENTED_AND_VERIFIED | Ui::Contrast; testes verificam 4,5 para texto/superfície e relações previstas de ativos/gráficos em 36 paletas. Não equivale a certificação integral de acessibilidade. |
| Tamanho de texto e fallback | IMPLEMENTED_AND_VERIFIED | Tipografia 12/10/14/27 DIP, cache por geração/idioma, fontes nativas por script e fallback DirectWrite; desenhos CJK/árabe/hindi executados. Fontes dependem do Windows instalado. |
| Longos títulos e metadados | IMPLEMENTED_AND_VERIFIED | ScrollingText/ellipsis, título completo no tooltip, informações em EDIT selecionável/rolável; tests longos/Unicode. Dados não são traduzidos. |
| Longos rótulos de controles/ajuda | PARTIALLY_IMPLEMENTED | Medição/ellipsis existe, tooltips explicam ações; algumas regiões fixas e help/rodapé podem truncar. Falta conferir textos completos e impossibilidade de perder identificação em todas as escalas mínimas. Não reduzir fonte arbitrariamente. |
| Foco/teclado player e EQ | IMPLEMENTED_AND_VERIFIED | Tab, foco desenhado, Space/Enter, setas/Home/End e ajustes EQ em testes existentes. Isso não substitui aceleradores globais T1. |
| Foco/teclado de toda a playlist | PARTIALLY_IMPLEMENTED | Linhas/Enter/Delete/Ctrl+A, pesquisa EDIT e menu contextual têm navegação. ADIC, abas Geral/Favoritas, limpar pesquisa e estrelas Canvas não integram uma ordem completa de Tab/ativação. Falta percurso e foco visível para essas ações, sem roubar texto digitado. |
| Nomes/estados acessíveis personalizados | PARTIALLY_IMPLEMENTED | PlayerControlName fornece nomes localizados e tooltips têm WM_GETTEXT; controles nativos expõem texto. Não há WM_GETOBJECT/provider por região Canvas. Árvore Computer Use atual expôs somente painéis. Falta exposição mínima de nomes, papéis, estado e ação, com ensaio assistivo; não propor framework extenso. |
| Unicode, árabe e hindi | IMPLEMENTED_AND_VERIFIED | UTF-8 estrito/UTF-16, DirectWrite shaping/bidi, script fonts, direção dos EDITs, parâmetros LRE/PDF. Capturas T6/T7/T8 representativas inspecionadas e renderização automatizada repetida. RTL complexo e qualidade de naturalidade permanecem revisão humana. |
| Hover/pressed/disabled/checked | IMPLEMENTED_AND_VERIFIED | Desenho deriva de PlayerSnapshot, Eq::State e Track; testes funcional/visual existentes. Não há novo mockup ou estado paralelo de tooltip. |
| Aceite visual final da M8 | NOT_IMPLEMENTED | Não foi concedido; obrigatório após os contratos faltantes. Não pedir novamente os aceites T7/T8/M6/M7. |

## Todos os 15 critérios oficiais de internacionalização

| Critério | Classificação | Evidência / limite |
|---|---|---|
| I01 — 18 disponíveis/selecionáveis | ALREADY_ACCEPTED | T7 PASS, Languages/menu e testes atuais. |
| I02 — lista Windows independente de região | IMPLEMENTED_AND_VERIFIED | WindowsLanguagePreferences, GetUserPreferredUILanguages e contingência; resolução/testes de ordem/variantes PASS. |
| I03 — menu Idioma integrado/ativo | ALREADY_ACCEPTED | T7; marca automático ou manual, idioma resolvido indicado. |
| I04 — troca imediata de todos os componentes | IMPLEMENTED_AND_VERIFIED | Cinco janelas/notificação, menus ao abrir e tooltip visível; testes atuais PASS. MessageBox já modal não é recriado. |
| I05 — preferência, precedência e retorno automático | IMPLEMENTED_AND_VERIFIED | Language section; persistência/bordas/falhas/retorno em test_language_selection. |
| I06 — textos próprios internacionalizados | IMPLEMENTED_AND_VERIFIED | Inventário, IDs/literais/exceções e validador --source PASS para UI existente. Dados, símbolos/unidades, nomes canônicos persistidos e controles do Windows são exceções documentadas. Não inventar Sobre inexistente. |
| I07 — tooltips traduzidos/tema/hover/DPI/monitor | ALREADY_ACCEPTED | T8 TUDO PASS, tests Place e popup --gui; limites do host explicitados na matriz DPI. |
| I08 — sem faltas/duplicatas/vazios | IMPLEMENTED_AND_VERIFIED | 18 arquivos contados diretamente: 237 IDs, 46 tooltip.*, validador PASS. Quantidade não é invariável futura. |
| I09 — placeholders/tipos/plurais | IMPLEMENTED_AND_VERIFIED | Validação dos conjuntos de parâmetros e seis formas; fronteiras de categorias testadas. Correção gramatical/naturalidade depende de revisão linguística. |
| I10 — fallback idioma/escrita/chave | IMPLEMENTED_AND_VERIFIED | en-US incorporado, Hans/Hant, ordem, inválidos e chave ausente testados; nunca fallback por país/fuso. |
| I11 — legível em todos os idiomas/escalas | PARTIALLY_IMPLEMENTED | Desenhos abrangentes e inspeções representativas; adaptação pequena área útil e janelas auxiliares/DPI faltam. Não certificar toda a matriz só porque DrawText retornou sucesso. |
| I12 — cobertura de glyphs e RTL/CJK/hindi | PARTIALLY_IMPLEMENTED | Caminhos técnicos e amostras verificados; fontes/sistemas físicos e bidi arbitrário não integralmente inspecionados. Não há perda Unicode comprovada nesta rodada. |
| I13 — quatro builds | IMPLEMENTED_AND_VERIFIED | Quatro novas execuções PASS; logs/hash no manifesto. |
| I14 — testes catálogo/resolução/round-trip/troca/erros/literais | IMPLEMENTED_AND_VERIFIED | Suítes novas executadas novamente e varredura fonte PASS. Cobertura de estados raros é técnica, não inspeção humana de todo diálogo. |
| I15 — áudio/DSP/playlist/posições preservados | IMPLEMENTED_AND_VERIFIED | test_language_window/test_i18n_window/test_tooltip: engine offline real, cursor/parâmetros/modelo/geometria preservados; inspeção real Play/Pause/Stop. Localização não é chamada pelo callback. |
| Revisão humana de naturalidade/terminologia/plurais dos 18 idiomas | IMPLEMENTED_NOT_VERIFIED | Traduções iniciais existentes; não há evidência de revisão integral. Contrato exige revisão humana, não contratação obrigatória de profissional. T7/T8 aceites visuais não a substituem. |

### Validação estrutural, visual e linguística

**Estrutural:** automática e aprovada nesta sessão, com contagem independente,
UTF-8/Unicode, parâmetros, categorias plurais, fallback/inventário/literais.
Nenhuma tradução foi substituída ou regenerada nesta auditoria.

**Visual:** inspeções históricas representativas em 18 idiomas e cinco janelas,
amostras específicas árabe/hindi/CJK e estados dinâmicos; nova sessão nativa
confirmou inicialização/reprodução/pausa/stop. Renderizações automatizadas abrangem
36 temas/quatro escalas; não equivalem a inspeção humana de todas as combinações.

**Linguística:** pendente. Não foi identificada tradução comprovadamente
incorreta nesta rodada; isso não afirma qualidade integral. Terminologia entre
ações/menus/tooltip, variantes pt-BR/pt-PT e plurais devem ser revisados por idioma.
[catalog-audit.json](evidence/m8-consolidation/catalog-audit.json) lista por catálogo
quantidade e cinco maiores razões de expansão em caracteres vs inglês, para
priorizar leitura. Razão de caracteres não mede glyphs/largura nem comprova erro.
Não alterar indiscriminadamente textos longos ou palavras iguais ao inglês.

## Plano objetivo para concluir M8

Não implementado nesta execução. Reutilizar componentes existentes e obter
autorização do próximo ciclo a partir desta auditoria.

| Prioridade / sequência | Trabalho concreto e pronto quando | Autonomia / validação humana |
|---|---|---|
| Alta 1 — T1/T2 | Despacho único de ações existentes, ZXCVB/L/Ctrl+D e seek; proteger EDIT/foco. WM_APPCOMMAND para quatro comandos. Testar foco nos três painéis, seleção/filtro e áudio preservado. | Implementação/testes autônomos; conferência de atalhos e teclado de mídia pelo usuário quando disponível. |
| Alta 2 — T3 topo | Toggle do conjunto e estado persistível, com menu localizado e sem alterar docking. Verificar normal/topmost, ocultar/reabrir e outros aplicativos. | Código/testes autônomos; aceite do comportamento visual. |
| Alta 3 — T4 | Mutex nomeado, parsing Unicode e encaminhamento limitado WM_COPYDATA; segunda instância termina, primeira recebe arquivos/foreground sem duplicação ou perda. | Autônomo em perfil isolado; teste Explorer com arquivos distintos e múltiplos. |
| Alta 4 — T5 | Persistir balanço/shuffle/repeat/topo; posição principal independente; sinal portátil/data/ e migração compatível. Round-trip/bordas/Unicode/reinício e ausência de efeito no perfil pessoal. | Autônomo; revisão do contrato de migração antes de ação real sobre perfis. Preparar runtime, sem empacotar M10. |
| Alta 5 — DPI/adaptação | Preservar monitor negativo no zoom, fator local por painel, relayout/fontes Aparência/Informações e regra para área útil insuficiente. Testes produção 96/120/144/192 DPI × zoom, menus/hit-tests/arestas. | Autônomo com testes determinísticos; físicos em ambientes disponíveis e aceite das disposições compactas. Não refazer sistema inteiro. |
| Média 6 — acesso/legibilidade | Completar Tab/ativação/foco da playlist, exposição mínima de nomes/estados Canvas prevista; inspecionar cortes reais de textos/ajuda, menus e scripts não latinos. | Implementação pontual autônoma; leitor de tela/teclado e legibilidade exigem inspeção humana. |
| Média 7 — linguística/checkpoint | Revisão por idioma: naturalidade/terminologia/plurais/RTL; corrigir somente achados demonstrados. Validar critérios residuais e pedir aceite final da M8, mantendo aceites anteriores. | Estrutural/automações autônomas; revisão linguística e aceite final humanos. Sem declarar revisão inexistente. |
| Baixa / opcional | Layout automático ao reconectar monitor por identidade, idiomas extras, novo seletor de zoom, espelhamento completo de transportes ou framework assistivo extenso. | Não são novos bloqueadores; considerar somente por solicitação/evidência. |

Pendências obrigatórias existem: **M8 não está pronta para encerramento**.
Os números de teste não compensam T1/T2/T3 topo/T4/T5 ausentes.
Manter checkboxes T1–T5 abertos até o contrato completo; T6–T8 permanecem [x].
Refinamento e revisões do checkpoint geral continuam separados.

## Evidências novas, limites e Git

Quatro builds incrementais e 716 casos funcionais novamente PASS;
36 guardas, dois casos gráficos adicionais, 3.092 frames/500 ciclos;
probe complementar x86/x64, 24 amostras e três lacunas/um limite reproduzidos.
[Logs/hash/capturas](evidence/m8-consolidation/README.md).
Testes de produção não foram removidos; não houve mudanças em áudio/DSP/docking.
Nova fonte/runner em scripts/diagnostics servem somente à auditoria e não entram
no exe do produto. Binários/objetos/perfis e temporários permanecem em build ignorado.

M9 COMPLETE por decisão expressa; testes práticos Windows 7/8/8.1/10 e
arquiteturas específicas permanecem com o usuário, sem reabertura.
Não houve alteração de DPI/resolução global, desconexão/reconexão física de
monitor, leitor de tela ou revisão linguística integral nesta sessão.

Baseline 266270a preservado, sem descarte/rebase/force. Consolidação 1aa560f
publicada e igualdade local/remoto confirmada. Primeira tentativa de stage
foi rejeitada pela revisão automática por considerar a proibição anterior;
a autorização explícita do pedido atual foi apresentada e a nova avaliação
aprovou a mesma ação. Não houve contorno do bloqueio. A primeira consulta
de rede no sandbox falhou com SEC_E_NO_CREDENTIALS; a execução aprovada fora
do sandbox confirmou remoto/push, sem troca de credenciais ou configurações.

Esta auditoria e sua rastreabilidade são publicadas em commit documental
posterior à consolidação, com os probes/evidências. O hash final é identificável
por `git log -1` e confirmado novamente em origin/master no fechamento;
não inserir o próprio hash no arquivo que pertence a esse mesmo commit.
M10 mantém distribuição limpa/reproduzível: somente LiteSic.exe na raiz,
recursos necessários em subpastas. Não houve pacote nem reorganização estética
de build. Próxima ordem: **concluir M8 → auditoria geral de desempenho → M10**.
