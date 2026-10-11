# Ajustes de encaixe — 11/10/2026

Após confirmar o funcionamento da M6, o usuário pediu magnetismo do EQ com
o player e alinhamento automático dos dois painéis.

EQ e playlist compartilham `src/window_snap.h`: captura de 24 DIP, contato
sem espaço nas quatro bordas e alinhamento pelo início, centro ou fim da
borda, escolhendo a posição mais próxima. As âncoras são recalculadas após
movimento, redimensionamento ou mudança de escala. Arrastar para fora da
captura separa o painel; retornar reencaixa. O EQ abre encaixado à direita
quando há espaço, com alternativas nas outras bordas.

Quatro builds (32/64 bits, debug/release) e quatro suítes completas passaram:
432 verificações, 108 por variante.
Os testes cobrem geometria das 12 combinações lado/âncora, limite de captura,
coordenadas negativas, ausência de atração por cantos e mensagens reais de
encaixe, movimento, escala e separação dos dois painéis. Nenhum warning novo
nos arquivos de encaixe; avisos existentes de casts/rotinas não usadas foram
preservados.

Validação interativa em release 64 bits, escala 150%: player movido em
(80, 40) pixels; ambos os painéis acompanharam. Arraste do EQ em (20, 10)
voltou ao alinhamento exato; um arraste maior o soltou e o retorno reencaixou.
Arraste da playlist em (20, 20) voltou ao alinhamento exato. Capturas foram
inspecionadas. A sessão isolada foi deixada aberta para revisão.

Evidências, logs e hashes: [manifesto](evidence/window-snap/manifest.json).
O pacote anterior da M6 conserva os hashes da rodada original. Este ajuste
não altera DSP nem conclui o escopo de encaixe transitivo da M7.
Sem commit ou push, conforme a instrução vigente.
