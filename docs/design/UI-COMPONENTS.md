# Tokens e componentes

`src/ui_theme.h` centraliza RGB, espaçamento 8 DIP, barras 32 DIP e controles
28 DIP. Segoe UI 12/10/14 DIP e Consolas 27 DIP, fontes do sistema com fallback
DirectWrite. Contraste: texto creme sobre superfície escura, verde para ativo,
verde-amarelo para foco. Sem bitmaps de referência ou ícones de rede.

`Ui::Canvas` possui recursos D2D/DirectWrite por janela, cache de formatos,
brush reutilizado, botões normal/hover/pressionado/ativo/desabilitado/foco,
slider, círculo, texto truncado e clipping. Coordenadas em DIP; uma transformação
no render target recebe escala física. DC target permite capturar WM_PRINTCLIENT
através do helper PrintWindow; não se confunde com captura acelerada de desktop.
BindDC acompanha resize. Falha/EndDraw descarta target e brush; a próxima pintura
recria os recursos; o chamador pode usar GDI como fallback. Factories e formatos
permanecem em cache. Toda pintura ocorre na thread de UI.

Dimensão do player: 460 × 290 DIP, 30 DIP acima da meta conceitual para comportar
cinco comandos de transporte distintos com área adequada e rodapé técnico.
Playlist 460 × 360 DIP e EQ 460 × 210 DIP. Não mostrar maximizar sem ação.

Aleatório e repetição usam ícones vetoriais em botões circulares na linha de
transporte; repetição de uma faixa acrescenta o indicador `1`. LISTA e EQ ficam
à direita do rodapé técnico em controles de 52 × 18 DIP. A área do visualizador
mede 436 × 68 DIP, começando em Y=196, aproveitando a antiga linha de toggles.

Internacionalização M8-T6: textos próprios usam IDs de `I18n`; dados de mídia
e identificadores persistidos são preservados. Canvas invalida formatos pela
geração de idioma e tema, usa fontes nativas por script, shaping/reticências
DirectWrite e direção RTL para prosa árabe. Números técnicos mantêm LTR;
transportes e controles não são espelhados. Geometria e 36 paletas preservadas.
Atualização global de janelas/menus/tooltips existentes entregue na
[M8-T7](../M8-T7-LANGUAGE-SELECTION.md), com default Windows, seleção no contexto
e preferência no INI existente. [T8](../M8-T8-TOOLTIPS.md) entregue e aceita:
tooltips explicativos, hover, tema e ciclo de vida. Nomes localizados separados
não constituem uma árvore UIA/MSAA dos controles Canvas; essa limitação permanece.
Ver [contratos e evidências](../M8-T6-I18N.md).
