# Reprodução e rolagem da playlist — 11/10/2026

Ao tocar uma faixa que já está na área visível da playlist, a rolagem agora
permanece onde estava. Isso inclui a última linha parcialmente visível.
O destaque da faixa em reprodução continua sendo atualizado.

A faixa é centralizada apenas quando está fora da área visível, acima ou
abaixo. Se uma pesquisa oculta a faixa a revelar, o comportamento existente
de limpar o filtro e revelar a faixa é preservado.

A alteração fica em `PlWnd_Refresh`: a posição de rolagem só muda quando a
linha está fora da área de desenho. Não há alterações no layout, motor de
áudio, persistência ou comandos.

Validação: quatro builds x86/x64 Debug/Release PASS; suíte completa x64 Debug
com 89 casos PASS. O teste de integração foi ampliado para conferir primeira
linha, última linha completa, linha parcialmente visível e centralização de
faixas fora da visão em ambas as direções. Os sete casos da playlist passaram
novamente após completar essas verificações. Não foi realizada nova captura
gráfica nesta correção; a comprovação é por mensagens reais de janela no
processo de teste, verificando qual faixa permanece nas mesmas coordenadas.
