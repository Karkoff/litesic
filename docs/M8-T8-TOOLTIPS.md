# M8-T8 — Sistema de Tooltips Multilíngues

**M8-T8 COMPLETE; M8 IN_PROGRESS.** Entrega técnica em 11/10/2026 (logs UTC em 12/10). Aceite visual humano da T8 **TUDO PASS**, concedido pelo usuário na solicitação de consolidação T6–T8. A aprovação humana **PASS da T7** é preservada. O aceite dos tooltips não representa revisão linguística profissional dos 18 idiomas. M8 permanece aberta; M10 e auditoria geral de desempenho fora do escopo.

O corpo abaixo preserva as medições e o estado Git da entrega técnica original.
A nova regressão e publicação estão em [consolidação](M8-CONSOLIDATION.md).

## A. Arquitetura

`src/ui_tooltip.h/.cpp` centraliza associação, hit-test, temporização, conteúdo, desenho, posicionamento e ciclo de vida. Cada superfície registra três callbacks: região/identificador atual, descrição atual e escala atual. HWNDs nativos usam `AttachControl`; controles Canvas reutilizam o hit-test ou a geometria funcional existente. Nenhuma janela foi recriada para idioma ou tema. Os antigos 16 tooltips nativos do player foram substituídos; não há dois sistemas concorrentes.

Foram avaliados os tooltips Win32 existentes (TTF_SUBCLASS/TTM_*): são úteis para regiões, mas o desenho nativo legado não garante tokens de tema, medição e DirectWrite uniformes. O popup leve reutiliza `Ui::Canvas`, Direct2D 1.0 e DirectWrite. `Canvas::TextLayout` permite desenhar o mesmo layout medido, com wrapping, sem alterar TextAt ou a renderização dos controles existentes. O fallback GDI mantém fundo, borda, fonte e Unicode.

`SetWindowSubclass` preserva o WndProc funcional. O popup é owned, `WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT`, retorna `HTTRANSPARENT` e `MA_NOACTIVATE`; não captura mouse/foco. `WS_EX_TRANSPARENT` sozinho não é tratado como garantia de click-through. Criação preguiçosa, um popup reutilizado por superfície; destruição em `WM_NCDESTROY`, remoção do subclass, cancelamento do timer e liberação COM/Canvas.

Atraso 650 ms, atualização apenas enquanto visível a cada 150 ms, expiração em oito segundos. Sair, clicar, arrastar/capturar, teclado, menu, mover/redimensionar, perder contexto, ocultar ou destruir cancela a apresentação. Depois da expiração, exige sair e entrar novamente. O timer é eliminado quando oculto, exceto durante o atraso de um hover elegível. Não há polling de disco, renderização ou tradução no callback de áudio.

APIs compatíveis com Win7: [SetWindowSubclass](https://learn.microsoft.com/en-us/windows/win32/api/commctrl/nf-commctrl-setwindowsubclass), [CreateTextLayout](https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritefactory-createtextlayout), [estilos estendidos](https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles), [hit-test](https://learn.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest). Compatibilidade física com outros Windows continua nos limites da decisão M9.

## B. Controles atendidos

[Inventário anterior ao código](M8-T8-CONTROL-INVENTORY.md): player, playlist, EQ, Aparência e Informações. Inclui anterior/próximo, play/pause/stop, shuffle/repeat, seek/volume/balanço, janelas auxiliares, FFT/OSC, título completo, LCD, minimizar/fechar; adicionar/remover/lista, pesquisa/limpar, abas/favorito por faixa, linhas/rolagem; ON/OFF, presets/reset, pré e dez bandas; temas/categorias/restaurar/fechar; texto selecionável de informações/fechar.

Ordenar por nome/duração, importar/exportar, misturar, limpar lista, adicionar arquivo/pasta e informações continuam disponíveis pelos menus nativos já localizados, com navegação de teclado. Não se criaram botões ou hit-tests falsos para esses comandos. Combo 701 oculto mantém seu papel de modelo. A dica das linhas descreve somente seleção e duplo clique, gestos válidos também com filtros; reordenação continua restrita à lista sem filtros.

## C/D. Catálogos

**46 novas chaves `tooltip.*`, 237 IDs × 18 idiomas**. Nomes/chaves no inventário de controles e no inventário central `I18N-STRING-INVENTORY.json`. Famílias: transporte (previous/next/play/pause/stop/seek/time), shuffle_on/off, repeat_off/all/one, volume/balance, show/hide playlist/eq/viz, fft/osc, minimize/close; add/remove/list_menu/all/favorites/clear_search/search/row/favorite_on/off/scroll; eq_on/off/presets/reset/preamp/band/band_unavailable; theme_list/theme_reset/category/info.

Reutiliza exclusivamente I18n::Text/Format, o catálogo ativo e o fallback en-US incorporado. Não há descrições fixas no código dos controles. O título completo é metadado, não texto traduzível. Os IDs antigos do player permanecem como nomes localizados separados, por `PlayerControlName`, conservando os contratos T6/T7. O teste T7 passou a consultar a associação real do novo sistema em vez de TTM_GETTEXTW.

18 catálogos: pt-BR, en-US, es-ES, fr-FR, de-DE, it-IT, pt-PT, nl-NL, pl-PL, ru-RU, ja-JP, zh-CN, zh-TW, ko-KR, tr-TR, ar-SA, hi-IN, sv-SE. UTF-8, não vazios, cobertura, placeholders, fallback incorporado, inventário e literais verificados. Traduções iniciais não equivalem a revisão linguística humana.

## E. Temas, Unicode e DPI

Fundo Surface, texto Text, borda Bronze e tipografia body do tema; sem paletas próprias. Sem animação/sombra que consuma CPU. Idioma e tema atualizam o popup visível pela notificação existente; a próxima apresentação também lê os tokens atuais. A escala vem da própria janela, incluindo DPI e zoom. As regiões são resolvidas novamente no layout atual; não há cópia permanente de retângulos após resize.

DirectWrite mede o texto com largura máxima de 360 DIP, margem de 10 × 7 DIP e quebra de linha; a largura também respeita a área útil. O layout medido é o desenhado. `Place` tenta abaixo do controle, depois acima e limita ao monitor, inclusive coordenadas negativas. Textos excepcionalmente maiores que toda a altura útil ficam limitados ao monitor.

UTF-16, famílias por script existentes, fallback nativo DirectWrite, bidi/shaping e direção RTL do idioma. Placeholders numéricos seguem isolamento bidi de Format. Controles de áudio não são espelhados. Os ensaios de escalas são sintéticos; não equivalem a uma matriz física de monitores.

## F. Estados reais

Shuffle e repeat consultam os campos funcionais existentes. Visibilidade consulta as próprias janelas. Volume/balanço usam o mesmo valor enviado ao motor pelos comandos da UI; não há cache independente de estado. Favoritos consultam o Track atual. EQ consulta `Eq::State()`, parâmetros sanitizados e publicados ao DSP, com frequência de `Eq::Frequencies` e disponibilidade por sample rate. Não se infere ganho pela posição visual. O mecanismo não calcula filtros nem participa da transição suavizada interna do DSP: mostra o parâmetro publicado, não uma estimativa instantânea das amostras.

## G/H/I. Validação

Resultados finais, métricas, hashes, capturas e comandos: [evidências](evidence/m8-tooltips/README.md). Suítes funcionais preservam os casos anteriores e acrescentam oito: regiões/nomes, estados/visibilidade, valores DSP/indisponibilidade, catálogos/temas, posicionamento/DPI, controles nativos/reabertura/destruição, favoritos e reprodução/custo incremental.

Modo gráfico adicional: `build/64/release/test-test_tooltip.exe --gui`. Requer desktop interativo e autorização de execução da plataforma. Testa atraso real, clique, saída, perda de foco, menu/movimento, captura, teclado, expiração, atualização de idioma/tema/ganho e EDIT nativo. Matriz do popup real com 18 idiomas × 36 temas × quatro escalas e 500 ciclos de reutilização, leitura do fundo renderizado, limites do monitor e recursos USER/GDI/memória. Essas contagens de frames não são casos funcionais independentes.

Para captura dentro da mesma thread, `WM_PRINTCLIENT/WM_PRINT` desenham no HDC fornecido usando o mesmo Canvas; as primeiras chamadas PrintWindow intra-thread produziam bitmap vazio e foram substituídas por WM_PRINTCLIENT no teste, com asserção de pixels. Capturas do driver entre processos continuam usando PrintWindow.

A primeira execução concorrente das variantes teve uma falha no teste preexistente de geometria da playlist: ele usa `build/playlist-geometry-test.m3u8` compartilhado. A execução final é sequencial, sem alterar a funcionalidade ou esse teste. As primeiras fixtures gráficas foram corrigidas para drenar eventos reais de mouse, declarar DPI awareness e usar janela própria (WindowFromPoint ignora STATIC). Não se corrigiu produto com base nesses falsos negativos.

Quatro configurações aprovadas: x64/x86 Release/Debug, MinGW MSVCRT estático, C++17 e `_WIN32_WINNT=0x0601`. Nenhum warning novo no módulo entregue. Warnings preexistentes de helpers GDI/DPI/miniaudio constam nos logs das compilações; não foram corrigidos fora do escopo.

## J. Desempenho incremental

Medições finais de 27.000 consultas (incluem renderização offline de áudio no laço):

| Variante | Tempo do laço (ms) | Private Bytes final − aquecido | GDI/USER finais |
|---|---:|---:|---:|
| x64 Release | 16.678 | 0 B | 12 / 16 |
| x86 Release | 25.461 | 0 B | 12 / 16 |
| x64 Debug | 139.833 | 0 B | 12 / 16 |
| x86 Debug | 166.084 | 0 B | 12 / 16 |

O ensaio gráfico final registra 0 B de delta após aquecimento, GDI 24 e USER 30 estáveis
por 500 ciclos; logs e hash do teste no manifesto. Esses números variam por máquina/carga.
As contagens GDI/USER são do processo de teste, não uma estimativa de overhead por popup.

Métricas limitadas à tarefa: 27.000 consultas a descrições com renderização offline de áudio, memória privada aquecida, USER/GDI; adicional gráfico mede reutilização do popup após matriz de fontes/temas. Layouts/fábricas/Canvas são reutilizados; layouts textuais se renovam quando muda texto, idioma, tema ou escala. Descrições dinâmicas são consultadas a 150 ms somente enquanto visíveis, sem redesenho quando texto não mudou. Não se afirma auditoria geral de desempenho ou prova absoluta de ausência de leaks.

## K. Acessibilidade e limitações

Tooltips complementam os nomes. Botões/EDIT/ListBox nativos conservam nomes, foco, seleção e navegação Windows; pesquisa conserva cue banner. Os controles Canvas já focáveis por Tab e teclas continuam assim. `PlayerControlName` separa nome curto da descrição; os popups expõem seu texto por WM_GETTEXT.

Não foi criada árvore UI Automation/MSAA para cada região Canvas. Leitores de tela podem não identificar individualmente essas regiões; isso exige trabalho futuro de acessibilidade e validação humana. A classe do popup também não declara um provider UIA dedicado. Revisão linguística dos 18 idiomas, RTL complexo, leitor de tela, monitor físico multi-DPI e ensaios práticos Windows 7/8/8.1/10 permanecem explícitos. O aceite visual humano T8 foi posteriormente concedido: TUDO PASS. Ensaio offline de áudio não é avaliação audível prolongada.

## L/M. Situação e Git na entrega técnica original

**Conclusão técnica PASS:** quatro builds, 179 casos por variante (**716 total**), 36 guardas e dois casos gráficos adicionais x64 Release aprovados; 3.092 frames do popup real. M8 geral permanece aberta por requisitos residuais T1…T5 e checkpoint. T6/T7 locais preservadas; aprovação humana T7 PASS não é estendida à T8. Sem commit/push, M10 ou auditoria geral de desempenho. HEAD inicial `266270aa9caa2469144f9af30d9609877941f7b4`, branch master. Patch/status do baseline guardados em build/m8-t8; evidências históricas T6/T7 preservadas.
