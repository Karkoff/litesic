# Volume, EQ compacto e formação dos painéis — 11/10/2026

O player salva o último volume ao encerrar normalmente e o restaura antes
de configurar o motor de áudio na próxima abertura. O INI usa
`Appearance/VolumePermille`, de 0 a 1000; 80% é somente o padrão quando não
há valor válido. A resolução de persistência é 0,1 ponto percentual,
incluindo volume zero. Nenhum perfil pessoal foi alterado pelos testes.

O equalizador tem **460 × 214 DIP**, o mesmo tamanho do player sem
visualizador. Track/sliders, valores, rótulos, foco, hit testing e rodapé
foram ajustados ao espaço. O texto de aviso de clipping foi removido da
janela. O comportamento de ganhos/headroom do DSP continua descrito no
relatório M6; não houve alteração no processamento de áudio.

Ao mover o player, os painéis encaixados **acompanham mantendo a formação**,
conforme esclarecimento do usuário. A rotina anterior movia a playlist
primeiro e comparava sua nova posição com a posição antiga do EQ, podendo
trocar o lado de encaixe por uma colisão temporária. A regressão
`docked_formation_survives_owner_moves` reproduziu a falha antes da correção
(`build/formation-before.log`) e passou depois (`build/formation-after.log`).

O movimento agora calcula o retângulo do conjunto a partir da posição
proposta do player e dos encaixes atuais. O conjunto é limitado à área útil
do monitor e cada painel segue seu encaixe sem escolher outra borda durante
esse movimento. Ao atingir uma borda, o conjunto para de avançar nessa
direção; a playlist não salta para outro lado. Se o conjunto for maior que
a área útil, não é possível mantê-lo inteiro visível: preserva-se a formação
com acesso ao canto superior esquerdo; uma escala menor pode ser necessária.
Painéis desprendidos continuam independentes. A escolha de borda livre ao
encaixar/reabrir um painel permanece disponível.

Testes: formação da imagem (playlist embaixo/EQ à direita), a disposição
inversa (playlist em cima/EQ à esquerda), playlist à esquerda/EQ embaixo e
playlist à direita/EQ em cima. Cada uma recebe movimentos nos dois sentidos
e tentativas de ultrapassar as quatro bordas; as posições relativas e a
visibilidade do conjunto são verificadas. Testes anteriores de arraste,
desprendimento, reencaixe, âncoras e escala permanecem na suíte.

Persistência: INI isolado verifica valores 0/1/310/999/1000 e fallback para
valores inválidos. O teste Win32 encerra com 30%, recria a janela, incrementa
o volume restaurado por teclado e verifica 32% no encerramento seguinte.
Dimensões do EQ são comparadas com `PlayerLayoutHeight(false)` nas seis
escalas disponíveis. O arraste de +6 dB considera até um pixel físico de
arredondamento na geometria compacta; os testes matemáticos DSP não mudaram.

Validação final: quatro builds x86/x64 Debug/Release, com
`LITESIC_NO_CAPTURE=1`; logs em `build/volume-docking-*-*.log`.
Os 23 executáveis de teste passaram em cada variante: **123 casos por
variante, 492 execuções aprovadas, nenhuma falha final**. Doze grupos de
assertivas de captura/pixels ficaram pendentes, três por variante.
Sem capturas, commit/push ou alterações de configurações pessoais.
Aceite visual humano continua pendente; M6 READY_FOR_HUMAN_ACCEPTANCE,
M9 encerrada e M7 não implementada.

Os relatórios anteriores e seus hashes registram seus respectivos snapshots,
anteriores a estes ajustes.

## Correção posterior: posição da playlist ao reabrir

A playlist agora preserva sua posição, dimensões e lado/âncora do encaixe
no arquivo de sessão auxiliar `playlist.m3u8.ini`, na seção Geometry.
A posição do player principal também é restaurada para manter a disposição.
Antes, a abertura escolhia novamente uma borda disponível, podendo colocar
a playlist acima do player mesmo após o usuário movê-la.

Painéis soltos mantêm coordenadas independentes. Painéis encaixados restauram
a relação com o player e continuam acompanhando seus movimentos. Dimensões
são salvas em DIP; posições fora dos monitores disponíveis são ajustadas para
manter as janelas acessíveis. Sessões antigas sem Geometry usam a disposição
inicial existente. Valores inválidos são descartados.

O teste `geometry_survives_restart` cobre quatro lados, painel solto,
fechar/mostrar, reinício da sessão, movimento após restauração e posição
fora da tela. Usa somente mensagens Win32 e arquivos isolados em build.

Após esta correção, build e suíte passaram nas quatro variantes x86/x64,
Debug/Release: 127 casos por variante, 508 aprovações. Logs locais em
`build/playlist-position-*-*.log` e resumo em
`build/playlist-position-summary.json`. Sem capturas; 12 grupos de
verificações de imagem continuam omitidos. O resultado não substitui
aceite visual humano.

A API GetWindowPlacement usada para consultar a posição normal de um player
minimizado está disponível desde Windows 2000, conforme
[documentação da Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getwindowplacement).

## Correção posterior: posição do equalizador ao reabrir

O EQ também salva coordenadas, lado e âncora do encaixe, na seção
`EqualizerWindow` do `settings.ini` configurado. A posição é salva ao terminar
um arraste, ocultar o painel ou encerrar o programa. Reabrir um EQ que estava
oculto utiliza a posição salva. Presets e ganhos continuam na seção Equalizer.

Na restauração, um EQ encaixado mantém a relação com o player. O ajuste para
a área útil considera também a playlist encaixada, preservando a disposição
do conjunto. Um EQ solto mantém suas coordenadas, ajustadas somente quando
ficam fora dos monitores disponíveis. A dimensão continua sendo a dimensão
fixa do EQ, calculada para a escala atual. Configurações antigas sem posição
válida usam a escolha inicial de borda existente.

Os testes `equalizer_position_roundtrip_and_validation` e
`position_survives_restart` cobrem persistência, valores inválidos, quatro
lados com âncora final, painel solto, ocultar/mostrar, recriar visível ou
oculto, acompanhamento do player e recuperação de coordenadas fora da tela.
Os testes usam arquivos próprios em build, sem alterar o perfil pessoal e
sem capturas de tela.

Build e suíte completos passaram em x86/x64, Debug/Release após esta correção:
129 casos por variante, 516 aprovações. Logs em `build/eq-position-*-*.log`
e resumo local em `build/eq-position-summary.json`. Continuam pendentes os
12 grupos de verificações de imagem omitidos e o aceite visual humano.
