# PROMPT PARA O CODEX — LITESIC: IMPLEMENTAÇÃO DA NOVA UI/UX

**Projeto:** LiteSic — player desktop nativo para Windows  
**Decisão de arquitetura:** Win32 para janelas e eventos; Direct2D + DirectWrite para desenho e tipografia; C/C++; motor de áudio existente preservado.  
**Referência visual obrigatória:** `docs/design/LITESIC-UI-UX-REFERENCIA.png` (anexo fornecido junto deste prompt).  
**Autoridade sobre o estado funcional:** código local, testes, `docs/ROADMAP.md` e documentação atual. Não infira que o screenshot representa funcionalidades já implementadas.  
**Missão:** IMPLEMENTAR, em incrementos testados, uma interface moderna e visualmente fiel à proposta, sem descaracterizar o player compacto nem introduzir regressões. Não se limite a uma proposta teórica.

## 0. INSTRUÇÃO CRÍTICA SOBRE A REFERÊNCIA VISUAL

Abra de fato o arquivo `docs/design/LITESIC-UI-UX-REFERENCIA.png` na ferramenta de inspeção de imagem disponível na sua sessão. A imagem não é apenas inspiração: é o **contrato de direção visual** para paleta, hierarquia, disposição, estilo de controles, três janelas e variantes compacto/integrado/independente. Registre que a visualizou antes de desenhar componentes. O agente também deve inspecionar o estado REAL da interface atual com a automação gráfica. Não use a imagem como bitmap de fundo, nem copie o conteúdo para uma tela estática: todos os controles devem ser objetos reais e interativos.

A imagem é um **mockup conceitual**, não um gabarito de pixels. Nela há dimensões indicativas em DIPs, mas a própria composição ilustrada foi ampliada para apresentação. **Não replique a posição dos pixels de uma imagem de 1536 × 1024 diretamente no executável.** Preserve o desenho por hierarquia, proporção e espaçamento; ajuste dimensões mínimas quando necessário para legibilidade e DPI. Se houver conflito entre o desenho e os comportamentos já aprovados pelo usuário, preserve o comportamento e adapte apenas a representação visual, documentando a divergência.

## 1. ESTADO EXISTENTE A PRESERVAR — NÃO REGREDIR

O projeto já possui player Win32/GDI, miniaudio 0.11.25, stb_vorbis 1.22, visualizador FFT/osciloscópio, seek, volume, balanço e playlist em desenvolvimento. O ambiente usa MSYS2 em `.tools/msys64`, GCC/G++ 16.2.0, Make 4.4.1, GDB 18.1, quatro variantes x64/x86 Debug/Release, tarefas do VS Code e automação gráfica via `.tools/gui-venv`/`scripts/gui.ps1`.

O último agente declarou **292 execuções de testes aprovadas** após ajustes do player. Trate esse valor como **baseline informada**, não como resultado desta execução; confirme os testes e sua contagem na máquina atual.

**Três decisões recentes são obrigatórias:**

1. **Repetição:** DESLIGADO → `REP` (repetir lista) → `REP 1` (repetir faixa) → DESLIGADO; sem alteração silenciosa na semântica.
2. **Playlist magnética:** quando encaixada, acompanha a janela principal; arrastar a barra da própria playlist a desprende; aproximar da borda a reencaixa. Preservar posição relativa, estados e detecção de arraste.
3. **Controles agrupados:** Anterior, **Play, Pause, Stop** e Próximo na **mesma linha**, com ações Play e Pause disponíveis distintamente. O comando Abrir separado do player principal foi removido: adicionar arquivos se faz pela playlist. A referência possui um botão visual central Play/Pause; **não use isso para eliminar ou fundir os comandos recém-aprovados**. Adapte a disposição dos 5 controles mantendo uma estética semelhante e foco no transporte.

Nenhum commit/push. Não efetue `reset --hard`, `clean -fd`, checkout destrutivo ou sobrescrita de trabalho local.

## 2. INSPEÇÃO, BASELINE E PLANO DE ENTREGA

Antes de codificar:

- Leia `docs/ROADMAP.md`, `README.md`, `docs/DEVELOPMENT.md`, `docs/VALIDATION-2026-10-10.md`, `docs/GUI-AUTOMATION.md`, `docs/GUI-VALIDATION-2026-10-11.md` e `docs/PLAYER-ADJUSTMENTS-2026-10-11.md` se existirem.
- Inspecione `git status`, branch, commits e diffs não registrados; separe modificações de infraestrutura, playlist, reprodução e UI. Faça inventário, sem reverter nada.
- Inspecione `src/` e Makefile, identifique arquivos/contratos responsáveis por janela, mensagens, layout, pintura, entrada, playlist, visualizador e áudio.
- Execute build/testes de referência e capture o **antes** com o auxiliar gráfico. Registre dimensões reais, posicionamento, DPI e comportamentos existentes.
- Produza `docs/design/LITESIC-UI-IMPLEMENTATION-PLAN.md` com matriz requisito → arquivos → dependências → testes → evidência → status e mapeamento do trabalho para milestones M5–M10. Não invente novos statuses nem marque milestones completas sem evidência.

Execute as fases abaixo **incrementalmente**, sem aguardar nova autorização para cada alteração dentro do escopo. Para cada fase: implementar → compilar → testar → abrir o aplicativo → capturar/analisar → corrigir → registrar. Se um defeito bloqueante ou requisito ambíguo impossibilitar terminar tudo com segurança, preserve o estágio funcional e relate exatamente o impedimento e o próximo passo. Nunca afirme que concluiu tudo porque apenas gerou um layout.

## 3. SISTEMA VISUAL — TOKENS E COMPONENTES

Crie tokens de design centralizados, usados por TODAS as janelas, em vez de cores/valores mágicos dispersos. Paleta inicial da imagem (confirme por amostragem e ajuste apenas com justificativa documentada):

| Papel | Cor de referência |
|---|---|
| Fundo principal quente | `#3B2820` |
| Superfície escura | `#2B2018` |
| Molduras/bordas | `#5A4633` |
| Controles âmbar/bronze | `#887B5B` |
| Verde principal | `#00E676` |
| Verde/amarelo secundário | `#C4FF00` |
| Texto principal | `#E6E1D7` |
| Texto secundário | `#A8A096` |
| Erro/perigo | `#FF6668` |

Estilo: tema escuro grafite-marrom com verde digital e acentos âmbar; contraste legível; aparência de player clássico com acabamento vetorial contemporâneo. Não usar brilhos neon extensos, degradês pesados, animações incessantes ou consumo desnecessário de GPU. Use Segoe UI para texto e Consolas ou fonte monoespaçada disponível para visor digital e números; use fallback de fontes do sistema (não embutir arquivos de fonte).

Defina espaçamentos em DIPs (base 4/8), margens coerentes, estados normal/hover/pressed/checked/disabled/focus, áreas clicáveis adequadas, bordas arredondadas moderadas, separadores, rolagem, textos truncados com tooltip, ícones vetoriais desenhados/codificados sem dependência de ícones web. Ícones e rótulos devem indicar semanticamente a ação; mantenha português em UI, inclusive acessibilidade. Gere um pequeno guia de componentes/tokens no repositório.

## 4. ESTRUTURA DE JANELAS E DIMENSÕES

**Player principal:** referência compacta **460 × 260 DIP**. **Playlist:** referência **460 × 360 DIP**; redimensionável verticalmente e, se suportado, horizontalmente. **Equalizador:** referência **460 × 210 DIP**. São **metas de densidade**, não limites que justifiquem truncamento. Se o conteúdo realmente exigir tamanho mínimo maior, demonstre por captura em 100% e 200%, explique a escolha e preserve versão compacta utilizável. É preferível 480 × 290 bem resolvidos a 460 × 260 com controles espremidos.

As três janelas devem compartilhar linguagem visual, barras de título coerentes, bordas e tipografia, preservando a janela sem moldura nativa onde isso já existir. Os botões da barra de título só devem aparecer se o comportamento correspondente existir (não mostrar Maximizar falso). Fechar a playlist/EQ normalmente oculta o painel, conforme modelo atual e roadmap, sem desligar o player. Evite mudanças arbitrárias de close/minimize do player.

**Modos previstos:** player compacto isolado; conjunto player + playlist encaixados; playlist/equalizador independentes. O mecanismo atual de acoplamento magnético da playlist é requisito funcional imediato. A expansão de docking para EQ só entra após verificar roadmap e projetar separação robusta; não introduza acoplamento frágil para cumprir uma imagem ilustrativa. Persistência de visibilidade, tamanho e posição pode ser implementada quando houver suporte de configuração coerente; nunca deslocar janelas para fora das telas disponíveis no restore.

## 5. PLAYER PRINCIPAL — DISTRIBUIÇÃO E INTERAÇÕES

Da barra superior ao rodapé, preserve a sequência de hierarquia exibida na imagem, adequada a DIPs:

**5.1 Título:** marca/ícone LiteSic, nome do produto, arraste da janela e ações nativas efetivas à direita. Hit-test correto da barra: arrastar apenas na região vazia; clique em controle não deve mover janela. Sem zona perdida.

**5.2 Identificação de faixa:** cronômetro verde de alto contraste no painel escuro, título em destaque, artista/álbum como metadados secundários. Não invente artista/álbum quando indisponíveis; fallback ao nome do arquivo, texto como “Artista desconhecido” apenas se útil, com truncamento/tooltip para nomes extensos. Estados sem faixa, carregando, carregada, tocando, pausada, fim e erro devem ter mensagens visuais consistentes. Se o tempo estiver em dois locais, evite redundância enganosa.

**5.3 Seek:** barra horizontal longa com indicador e tempo total. Arrastar, clicar e navegação por teclado (quando focado), sem saltos aleatórios; desabilitada sem faixa ou sem seek. Calcular tempo/posição com a taxa de amostragem correta da fonte e do domínio relevante (há achado anterior sobre frames); criar teste de regressão com sample rates diferentes antes de declarar posição/seek confiáveis. Não mascarar valores de duração ou bitrate incorretos.

**5.4 Transporte:** cinco botões visíveis **Anterior, Play, Pause, Stop, Próximo na mesma linha**, bem centralizados. Destaque primário para Play/Pause (sem ocultar o outro). Ícones consistentes, tooltip, hover/pressed/disabled/focus, comandos conectados a ações reais e feedback de estado. Não recrie botão Abrir no player. Próximo/Anterior devem navegar pela playlist quando houver faixas, não apenas reiniciar a música; documentar estados sem playlist. Os botões só são habilitados conforme a elegibilidade real da ação.

**5.5 Volume e balanço:** controles com legenda, indicação percentual e L/R; clamp de valores, arraste e teclado, sem cliques acidentais. Estado atual corresponde ao áudio realmente produzido; não redesenhar um slider que pareça modificar volume se a engine não receber o comando.

**5.6 Opções:** Shuffle/`SHUF`, Repeat/`REP`/`REP 1`, `LISTA` e `EQ` em linha própria, com estado ativo visual. Preserve o ciclo de repetição aprovado, inclusive ao mudar de faixa, fechar/abrir playlist e reiniciar o programa conforme persistência existente. `LISTA` abre/oculta playlist; `EQ` abre/oculta EQ se funcional. Se EQ ainda não existir, sinalize indisponibilidade real ou implemente-o por completo conforme fase 7; não apresente controles vazios como entregues.

**5.7 Visualizador:** zona inferior com FFT/OSC selecionáveis, alimentados por áudio real, sem dados sintéticos durante a reprodução normal. Atualização proporcional à visibilidade e estado (não fazer repaint contínuo desnecessário), sem flicker, sem acesso bloqueante a dados de áudio no callback. O seletor precisa funcionar mesmo que se mantenha a alternância por clique já existente.

**5.8 Rodapé técnico:** formato, sample rate, bitrate (somente quando significativo e corretamente calculado), canais, duração quando pertinente. Não inventar metadados; mostrar “—”/ocultar campo indisponível. Corrigir truncamentos e colisões com outros campos.

## 6. PLAYLIST — ESTRUTURA, CONTROLE E DOCKING

A referência exibe barra de título, linha de ações, pesquisa, tabela/lista e rodapé. Implementar visualmente e integrar à playlist real:

- Barra de ações: `+ Arquivos`, `+ Pasta`, `Remover`, seletor de lista/menu de opções se existirem funcionalidades de lista; não criar botões sem ação. **Entrada de arquivos permanece na playlist**, não no player.
- Campo `Pesquisar músicas...` filtrando a apresentação sem destruir a ordem canônica, índice original ou fila de reprodução. Limpar pesquisa retorna a lista completa.
- Linhas com índice, indicação de tocando/seleção, título, duração e — quando houver metadados confiáveis e largura — artista/álbum. Em 460 DIP, **priorize Título e Duração**; colunas opcionais podem ocultar/reorganizar em larguras estreitas. Não permitir sobreposição, títulos cortados sem indicação, scroll horizontal desnecessário.
- Double click/Enter para tocar faixa; seleção simples/múltipla com padrões Windows, remoção, rolagem, estados vazios úteis, importação via diálogo e drag-and-drop de arquivos/pastas (com o tratamento funcional previsto no roadmap).
- Operações longas de enumerar pastas, metadados e processamento de listas não devem bloquear a UI. Não descartar itens silenciosamente; mostrar falhas tratáveis sem paralisar.
- Rodapé com total de faixas, selecionadas e duração total se calculável; respeitar filtro vs total com rótulos corretos.
- **Dock magnético existente obrigatório:** ao encaixar, player e playlist se movem juntos; arrastar a barra de título da playlist desencaixa; ao aproximar, reencaixa, com tolerâncias razoáveis independentes do DPI e sem oscilação/falsos encaixes. Validar com duas janelas reais, inclusive ao mover o player após encaixe.
- Shuffle, repeat e navegação do player devem operar sobre o modelo de playlist, não sobre linhas filtradas sem decisão explícita; dar prioridade à consistência do estado real.

## 7. EQUALIZADOR — UI VISUAL E DSP REAL

A imagem prevê janela de equalizador gráfico com ativar/desativar, presets, reset, pré-amplificador e **10 bandas** em aproximadamente 31, 62, 125, 250, 500, 1k, 2k, 4k, 8k e 16k Hz, escala de referência −12 a +12 dB. O canal de pré-amplificação é separado das 10 bandas. Corrija qualquer interpretação equivocada de quantidade na figura.

- Primeiro inspecione no roadmap se EQ e DSP já possuem etapa definida. Alinhe as entregas visuais/funcionais aos marcos existentes e registre os subciclos necessários; não marque como concluído só por desenhar sliders.
- Ações `Ativado`, preset `Flat`/outros presets realmente definidos, `Redefinir` e os sliders devem corresponder a parâmetros DSP efetivos e verificáveis.
- Arquitetura: UI controla estado/parâmetros; processador de áudio aplica ganhos com limites e suavização, sem acesso a UI nem ao disco e **sem locks, waits, alocações ou I/O dentro do callback de tempo real**. Parâmetros publicados à thread de áudio com transferência segura e de custo previsível. Não faça refatoração indiscriminada do motor.
- Projeto de filtros apropriados e numericamente estáveis; trate sample rate de cada stream, faixas próximas/superiores ao Nyquist e arquivos mono/estéreo. Testar bypass = sinal original dentro da precisão esperada, flat próximo de unidade, ganho por banda em frequências-alvo, reset, toggle e troca de arquivo/sample rate.
- Proteja contra saturação/clipping e variações abruptas de ganho; não aplique normalização/compressor ou alterações sonoras não solicitadas silenciosamente. Defina comportamento claro e teste transições.
- Se DSP completo não puder ser entregue com segurança nesta execução, registre EQ como **IN_PROGRESS**; implemente uma UI coerente porém claramente não comissionada/indisponível, e apresente plano verificável de integração. **Nunca disponibilize um equalizador visual que indique atuação mas não modifique o som.**

## 8. ARQUITETURA DE RENDERIZAÇÃO — WIN32 + DIRECT2D/DIRECTWRITE

- Preserve Win32 para janela, mensagens, foco, mouse, teclado, drag, DPI e timers; Direct2D para shapes, botões, sliders, ondas e FFT; DirectWrite para texto. Não trazer Qt, JUCE, Elements, Electron, GTK, webview ou dependências volumosas.
- Crie uma fronteira clara entre **estado funcional**, **layout geométrico**, **componentes**, **renderização** e **entrada**. Evite copiar regras de áudio dentro de WM_PAINT ou acoplar hit-tests a pixels de um screenshot.
- Reutilize objetos gráficos e cacheie recursos quando apropriado; não recrie DirectWrite formats/brushes todo frame. Trate mudança de tamanho/DPI, dispositivo perdido (`D2DERR_RECREATE_TARGET`), invalidation parcial ou taxa de repintura disciplinada. Sem cintilação; CPU/GPU ociosas baixas.
- Considere Direct2D 1.0/DirectWrite disponíveis no Windows 7, mas **não declare Windows 7 validado sem execução real no ambiente alvo**. Evite APIs exclusivas de Windows 10+; para `WM_DPICHANGED` e outras capacidades dependentes de versão, use rotas compatíveis e fallbacks devidamente testados. Não alterar todo o projeto para SDK/runtime moderno incompatível com a meta sem investigação.
- Compatibilidade de arquivos C/C++ e links com GCC MinGW32/64; suporte a compilação Debug/Release. Verifique e adicione `-ld2d1`, `-ldwrite` e demais bibliotecas somente se exigidas pelo código real.
- A camada GDI anterior pode permanecer temporariamente como fallback durante a migração; retirar apenas quando novo caminho estiver funcional, medido e regressões cobertas. Evitar fazer uma segunda engine de áudio.
- Controles customizados devem suportar teclado, foco visível, tooltip, contrastes e nomes/estados acessíveis sempre que tecnicamente viável. Documente limites do suporte à acessibilidade em vez de afirmar integração UI Automation não implementada.

## 9. DPI, LAYOUT ADAPTATIVO E PERFIL DE USO

Testar nominalmente 100%, 125%, 150% e 200% com evidências reais ou explicar quando a automação do sistema não permite alterar DPI. Coordenadas de layout em DIPs, separadas de pixels físicos; texto e regiões clicáveis dimensionados proporcionalmente; mouse e hit-tests na mesma transformação. Validar também:

- Monitor compacto, Full HD e alta resolução; sem controle fora da área da janela.
- Metadados longos e caracteres acentuados/Unicode; texto truncado sem destruir controles.
- Playlist vazia, curta e com muitas músicas; scroll e seleção.
- Player sozinho, playlist encaixada, playlist solta, equalizador separado.
- Alt-tab, minimizar/restaurar, arraste entre monitores com DPI diferente quando disponível, closing/reopening sem posição fora da tela.
- Redimensionamento em limites mínimos/máximos, sem imagem esticada nem coordenada absoluta copiada do PNG.

Não alterar a experiência compacta exigindo uma única janela gigantesca. Em condição de largura reduzida, reflow/ocultar metadados secundários e preservar controles fundamentais, em vez de sobrepor elementos.

## 10. MAPEAMENTO DE ENTRADA, ESTADO E ESTILOS

Defina contratos de ações e máquinas de estado para playback, seleção, filtro, shuffle/repeat, equalizador e docking. Desenho deriva do estado real, não de flags duplicadas que podem ficar desincronizadas. No mínimo:

- Transporte: desabilitado sem faixa; Play/Pause refletem engine; Stop limpa o estado necessário; fim de faixa atualiza UI e dispara próximo conforme Repeat/Shuffle.
- EQ: bypass e preset exibidos coincidem com parâmetros aplicados.
- Playlist: faixa atual ≠ faixa selecionada; filtro não remove música; linha tocando sempre identifica o item canônico.
- Barra de título/drag: controles não arrastam janelas; janelas não interceptam teclado do player indevidamente.
- Erros: falhas de carga/decodificação/DSP exibidas e tratadas, sem apresentar reprodução falsa.

Acrescente atalhos consistentes com convenções do aplicativo e sem conflitar com edição do campo de pesquisa. Manter rótulos e comportamento em português.

## 11. FASES DE IMPLEMENTAÇÃO — ENTREGÁVEIS VERTICAIS

**UI-00 — Inventário e baseline:** ler docs; confirmar estado do projeto e 292 testes (ou contagem atual real); capturar antes; identificar riscos; salvar plano.

**UI-01 — Design system:** tokens, tipografia, ícones básicos, primitives e documentação curta; visual isolado e compilação.

**UI-02 — Backend Direct2D/DirectWrite:** render target, resize/DPI, texto, input e device-loss; provar no player sem remover caminho antigo prematuramente.

**UI-03 — Player:** organizar campos, cinco botões de transporte, seek, volume, balanço, SHUF/REP, lista/EQ, visualizador e rodapé; testar reprodução, navegação e semântica aprovada; screenshot em reprodução/pausa/parado.

**UI-04 — Playlist:** barra de ações, pesquisa, linhas, colunas responsivas, seleção, drag-and-drop e estados; preservar docking magnético; testar com muitas faixas e deslocamento conjunto.

**UI-05 — Equalizador:** layout 10 bandas + PRE e fluxo de integração DSP real (ou subciclo explicitamente IN_PROGRESS com controle desabilitado até comissionamento); testes sonoros automatizados e estabilidade do callback.

**UI-06 — Coerência e docking:** rever modos compacto/encaixado/independente, regras de visibilidade/fechamento, posições, resize; só ampliar docking de EQ se houver base robusta e roadmap compatível.

**UI-07 — Acabamento:** DPI, foco, tooltips, hover, estados desabilitados, contrastes, performance e longos títulos.

**UI-08 — Auditoria visual e regressões:** comparar as capturas reais com o PNG de referência, corrigir diferenças relevantes, executar matriz de testes e atualizar docs.

Não misture toda a migração em um único patch. Se a M5 já estiver IN_PROGRESS, continue a implementação alinhada ao roadmap; registre quais fases UI são transversais a M5–M10, sem renomear arbitrariamente milestones.

## 12. TESTES MÍNIMOS E EVIDÊNCIAS

Em cada corte funcional, executar variantes x64/x86 Debug/Release existentes. Incluir teste automatizado de regressão para mudanças no domínio e testar caminhos reais quando possível; **nunca reduzir a contagem aprovando cópias simuladas do algoritmo em vez do código de produção**.

Cobertura esperada:

1. Paint/resize/device-loss e estabilidade dos componentes.
2. Controles e hit-tests, mouse/teclado, foco, disabled, sem overlaps.
3. Estados de Play, Pause, Stop e fim natural de faixa.
4. Seek com fontes de sample rates diferentes.
5. Playlist: adicionar/remover, pasta, seleção, pesquisa, next/previous, shuffle e `REP`/`REP 1`.
6. Dock: encaixar, mover player, soltar arrastando título da playlist, reencaixar.
7. EQ: bypass, Flat, ganhos, presets e troca de sample rate quando implementado.
8. Visualizador FFT/OSC alimentado por dados reais e sem regressões no callback.
9. DPI, textos longos, janela compacta, erro de abertura e lista vazia.
10. Windows 7 marcado como **PENDENTE DE TESTES PRÁTICOS**, não PASS por análise estática. M9 está COMPLETE por decisão explícita do usuário; testes Windows 7/8/8.1/10 e validação específica x86/x64 serão realizados futuramente pelo usuário, sem bloquear o avanço ou reabrir automaticamente M9. Windows 11 tem funcionamento confirmado. Alvo: Windows 7–11, x86/x64. Ver [decisão M9](../M9-CLOSEOUT-2026-10-11.md).

Automação gráfica: usar `docs/GUI-AUTOMATION.md` e `scripts/gui.ps1` conforme **sintaxe real do repositório**, sem inventar flags; operar só na instância do LiteSic iniciada pelo auxiliar; manter aprovações da plataforma, não desabilitar sandbox. Capturar `antes`, `player-reproduzindo`, `player-pausado`, `player-sem-faixa`, `playlist-vazia`, `playlist-populada`, `docking-encaixado`, `docking-solto`, `eq`, `dpi-100` e `dpi-200` (quando realizáveis). Salvar artefatos em diretório de evidências de build ignorado pelo Git, com paths no relatório; não versionar mídia de teste sem justificativa/licença. A inspeção visual pelo agente e o aceite estético humano são coisas diferentes; **não declarar PASS humano sem intervenção do usuário**.

Instrumentar medições comparáveis de CPU em repouso, memória do processo, tempo de inicialização, tamanho de executável e fluidez de renderização; comparar antes/depois sob condições próximas. Não afirmar ganho de desempenho sem medida.

## 13. CRITÉRIOS DE ACEITAÇÃO OBJETIVOS

- A interface real se parece claramente com a referência: paleta escura bronze, verde digital, organização de player/playlist/EQ, estilos e proporções consistentes.
- O player continua leve, compacto, independente e utilizável com playlist/EQ fechados.
- Os **cinco comandos** de transporte permanecem juntos; nenhum botão Abrir reaparece no player; repetir mantém ciclo DESLIGADO → REP → REP 1 → DESLIGADO.
- A playlist continua magnética e também funciona solta; mover/arrastar é validado em janela real.
- Nenhum elemento essencial cortado/sobreposto nas dimensões aprovadas ou adaptadas justificadamente; boa leitura em DPI configurável.
- Gráfico FFT/OSC, volume, balanço, seek, transporte, lista e controles ativos funcionam de verdade; EQ só é apresentado como funcional com DSP real integrado e testado.
- GDI → Direct2D não altera regras de áudio nem introduz travamento/flicker significativo.
- Compilação e testes passam nas quatro variantes; testes adicionados para novos componentes; nenhum defeito bloqueante conhecido ignorado.
- O Git preserva todas as alterações do usuário e não há commit/push.
- Roadmap/README mostram **implementado vs pendente** com honestidade.

## 14. SAÍDA E REPORTE

Entregar: (A) referência visual efetivamente aberta e comparação com UI anterior; (B) plano e status UI-00..08; (C) arquitetura e arquivos; (D) funcionalidades prontas vs mockup vs pendentes; (E) testes e builds com números reais; (F) evidências gráficas vinculadas a cenários; (G) desempenho, DPI e problemas; (H) cobertura do docking e repetição; (I) roadmap e limitações Windows 7; (J) Git status e lista de arquivos; (K) validações humanas requeridas. Informe separadamente fases COMPLETE / IN_PROGRESS / BLOCKED e não avance de forma enganosa.

**Você está autorizado a implementar e testar o escopo acima de forma incremental. Não precisa aguardar aprovação para cada fase técnica; aguarde apenas quando houver necessidade de alterar uma decisão funcional consolidada, um risco relevante não resolvido ou quando o aceite humano for indispensável. Não realize commit ou push.**
