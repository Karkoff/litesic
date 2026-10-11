# M7 — compactação ao ocultar painéis

> Atualização de 11/10/2026: **M7 COMPLETE**, aceite humano “TUDO PASS”.
> [Encerramento formal](M7-CLOSEOUT-2026-10-11.md) prevalece para o estado atual.
> O texto abaixo preserva os resultados e pendências da rodada histórica.

Implementação da estratégia aprovada pelo usuário em 11/10/2026.
Ocultar compacta; reabrir recompõe; arraste manual redefine a formação.

`WindowSnap::Graph::FollowVisible` calcula os retângulos exibidos a partir
dos vínculos lógicos existentes. Um descendente visível de painel oculto
encosta temporariamente no ancestral visível mais próximo, usando seu lado
e âncora de encaixe. O grafo, Parent e Dock não são alterados pela ocultação.
Não há novo formato de configuração ou dependência.

Funciona para cima, baixo, esquerda e direita e também para cadeias em L.
Uma projeção que sobreponha janela visível ou fique fora da união das áreas
úteis é recusada, conservando a posição lógica expandida. Painéis soltos
permanecem independentes. Mover o player ou alterar seu tamanho recalcula
a disposição compactada; reabrir o intermediário restaura o espaço.
Arrastar manualmente o descendente remove o vínculo anterior e permite um
novo encaixe; reabrir o intermediário respeita essa nova formação.

Skin usa o mesmo cálculo na movimentação, reconciliação, restrição da
formação e consulta de retângulos. Fechar EQ/playlist pelo X também
reconcilia imediatamente, além dos toggles existentes no player.
Reprodução, DSP e configurações pessoais não foram modificados.

## Validação

Acrescentados dois casos de matemática e um caso Win32:

- Todos os quatro lados e três âncoras: compactar, mover e restaurar.
- Formação em L, lacuna entre monitores e limite de área útil.
- Cadeias nas duas ordens: fechar pelo X, mover player, reabrir e comparar
  retângulos; arrastar o descendente ocultando o intermediário e verificar
  que reabrir não desfaz a escolha manual.

Quatro variantes `make -j2 ARCH=<64|32> DEBUG=<0|1> all test`,
com `LITESIC_NO_CAPTURE=1`: **148 casos por variante, 592 aprovações**,
24 executáveis por variante. Os três grupos de captura por variante
continuam omitidos, sem contá-los como aprovação visual. Nenhum teste
anterior removido, nenhum warning novo identificado.

A tentativa de iniciar sessão gráfica isolada retornou desktop não
interativo (input desktop inacessível). Não houve nova captura nem
validação visual interativa deste ajuste; os testes de integração usam
janelas Win32 e mensagens reais. O checkpoint humano continua pendente.
As imagens da rodada anterior não são evidência desta compactação.

Releases atuais: `build/64/release/LiteSic.exe` e
`build/32/release/LiteSic.exe`; Debug nas pastas correspondentes.
Os executáveis agora substituem as builds da validação M7 anterior.
[Manifesto atual](evidence/m7-integrated/compact-manifest.json) registra
hashes e logs `compact-*.log`; o manifesto anterior é histórico.
O hash de x64 Debug não pôde ser lido: Windows reportou o arquivo em uso
por outro processo. Compilação e suíte dessa variante passaram; o manifesto
registra a indisponibilidade, sem encerrar processos. As duas releases e
x86 Debug têm hashes registrados.

M7 permanece READY_FOR_HUMAN_ACCEPTANCE; nenhuma implementação M8/M10.
Mantida a instrução do ciclo M7 para não realizar commit ou push.
