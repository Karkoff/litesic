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
