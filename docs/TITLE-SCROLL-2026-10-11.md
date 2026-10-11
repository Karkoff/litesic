# Título longo no LCD — 11/10/2026

A renderização DirectWrite agora mede o título completo, sem reticências,
e move o texto dentro de um recorte exclusivo do LCD. O movimento vai e
volta com aceleração/desaceleração suave e pausa de 1,5 segundo nas duas
extremidades. A distância depende da largura real do texto no tema ativo.
Títulos que cabem permanecem parados; mudar o título reinicia a animação.

O layout do texto é reutilizado entre quadros e recriado ao mudar texto,
fonte, tema ou tamanho. O timer usa tempo decorrido real e só solicita
redesenho para título excedente em janela visível e não minimizada.

Quatro builds passaram: debug/release, 32/64 bits. Suítes completas release
32/64 passaram, incluindo teste das pausas, sentidos, continuidade e limites
do deslocamento; teste de renderização verifica recorte e títulos curtos em
quatro escalas. A validação interativa usa faixa de teste e sessão isolada.
Capturas e logs estão em `docs/evidence/title-scroll/`.

As interfaces utilizadas permanecem disponíveis no Windows 7, conforme
[IDWriteFactory::CreateTextLayout](https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritefactory-createtextlayout)
e [IDWriteTextLayout](https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nn-dwrite-idwritetextlayout).
Validação em VM permanece no escopo da M9. Sem commit ou push.
