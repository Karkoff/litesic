# Ajustes de botões e painéis — 11/10/2026

Atualização posterior: [volume persistido, EQ compacto e formação preservada](VOLUME-DOCKING-2026-10-11.md).
As dimensões e os resultados abaixo registram a primeira rodada visual.

Atendendo ao retorno visual posterior à auditoria M6:

- Botões ativos têm fundo claro derivado da cor de destaque do tema e
  texto/ícones escuros. O estado permanece claro durante hover/pressionamento.
  Shuffle/repeat desligados mantêm o fundo normal; repeat de uma faixa
  preserva o indicador «1».
- O botão EQ do player reflete painel aberto/fechado, independentemente do
  bypass do DSP. Fechar o painel também invalida a pintura do player.
- EQ ON/OFF, preset e Redefinir ficam à direita da barra inferior, com
  altura de 18 DIP, fonte pequena e borda no padrão da playlist/player.
  Sliders, hit testing, foco por teclado e popup de presets acompanham o
  novo layout; a janela mantém 460 × 244 DIP.
- Os cabeçalhos visíveis dos painéis mostram PLAYLIST e EQUALIZADOR.

Validação: quatro builds x86/x64 Debug/Release e suítes funcionais com
`LITESIC_NO_CAPTURE=1`. Os testes existentes verificam fechamento/reabertura
do EQ, controles no novo rodapé, encaixe, escalas e preservação do DSP.
O teste de temas exige contraste ≥4,5:1 entre fundo/texto dos botões ativos
nos 36 temas e verifica as dimensões tipográficas dos controles inferiores.
Logs: `build/ui-adjustments-{64,32}-{release,debug}.log`.
Resultado: quatro builds aprovados, 122 casos funcionais por variante
(488 execuções aprovadas, nenhuma falha final). Doze grupos de assertivas
de captura/pixels ficaram pendentes, três por variante.

Nenhuma captura de janela foi feita. As assertivas de pixels de janelas
permanecem pendentes; aceite visual humano ainda necessário. A M6 continua
READY_FOR_HUMAN_ACCEPTANCE e a M9 permanece encerrada. Sem commit/push ou
alteração do perfil pessoal.

O [relatório integrado](M6-INTEGRATED-VALIDATION-2026-10-11.md) e seu manifesto
registram o snapshot anterior a estes ajustes; seus hashes não representam
as fontes/binários modificados nesta rodada visual.
