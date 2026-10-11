# Temas, favoritos e informações das músicas — 11/10/2026

## Comportamento

- A janela Temas mantém a posição da lista e a seleção ao aplicar um tema.
  O título é apenas TEMAS. A rolagem invalida somente a barra lateral;
  os controles filhos ficam fora da pintura do fundo da janela.
- Os três botões do rodapé do equalizador têm largura de 120 DIP.
- A faixa em reprodução fica em negrito na playlist, inclusive durante pausa.
- O menu de contexto de uma faixa oferece **Informações da música...**.
  A janela apresenta tags disponíveis, formato, duração, frequência, canais,
  tamanho, taxa média aproximada do arquivo, favorito e caminho.
  Escape ou Fechar encerra a janela sem interromper a reprodução.
- Cada linha tem uma estrela. **Geral** mostra todas as entradas;
  **Favoritas** mostra somente as marcadas. A pesquisa funciona nas duas abas.

## Estratégia de favoritos

Favoritos são uma propriedade da entrada da playlist, sem cópias da música ou
uma segunda playlist. As duas abas usam os mesmos identificadores e índices
canônicos. Desmarcar uma estrela em Favoritas remove a linha dessa aba e mantém
a entrada em Geral. Entradas repetidas podem ser marcadas separadamente.

A sessão salva o atributo no M3U8 com o comentário opcional
`#LITESIC-FAVORITE:1` antes da entrada. Exportação e importação M3U preservam
essa marcação; leitores que desconhecem o comentário podem ignorá-lo.
O fechamento normal salva a sessão existente. Reordenação por arraste fica
disponível em Geral sem pesquisa ativa.

Play iniciado em Favoritas reproduz somente entradas marcadas, seguindo sua
ordem na playlist. Sem repetição, para na última favorita; repetir todas e
shuffle ficam restritos às favoritas, e repetir uma mantém a faixa atual.
Play iniciado em Geral usa a lista completa. O mesmo vale para duplo clique
e Enter. Apenas trocar a aba durante a reprodução não altera a sequência;
um novo Play na outra aba muda sua origem. A pesquisa continua sendo um
filtro de exibição dentro da aba, sem criar uma terceira sequência.
Próxima e Anterior respeitam a origem. Novas marcações passam a participar
da navegação e entradas desmarcadas são ignoradas nas próximas transições.

A alteração da sequência passou em build e suíte completos x86/x64,
Debug/Release: 136 casos por variante, 544 aprovações. As regressões novas
cobrem fim sem repetição, repetir todas/uma, shuffle e histórico restritos,
arquivos inválidos, remoção/desmarcação e origem do Play pela interface,
incluindo duplo clique, Enter e a troca de aba durante a reprodução.
Logs locais em `build/favorites-scope-*-*.log`, resumo em
`build/favorites-scope-summary.json`. Sem capturas; os grupos de imagem
omitidos e o aceite visual humano permanecem pendentes.

A aba ativa também persiste: a troca entre Geral e Favoritas salva
`Appearance/PlaylistFavoritesOnly` no `settings.ini` configurado. A próxima
criação da playlist restaura essa escolha, inclusive se o painel estava
oculto. Configurações antigas ou valores inválidos abrem em Geral.
O teste `active_tab_survives_restart` exercita ocultar/mostrar e recriar
o painel nas duas abas, verificando qual entrada aparece no filtro.

Após o ajuste de persistência da aba, build e suíte completos passaram em
x86/x64, Debug/Release: 132 casos por variante, 528 aprovações. Logs locais
em `build/playlist-tab-*-*.log` e resumo em `build/playlist-tab-summary.json`.
As verificações continuam sem capturas; os 12 grupos de imagem omitidos
permanecem pendentes.

As informações usam os metadados lidos pelo worker existente. O menu não abre
o arquivo na thread da interface nem interfere no decodificador em reprodução.
A leitura limitada de tags foi ampliada para álbum, gênero, data e número da
faixa em ID3 e comentários Vorbis/FLAC. Campos ausentes indicam Não informado.

## Validação

Build e suíte completos passaram em x64 Release, x64 Debug, x86 Release e
x86 Debug: 23 executáveis e 126 casos funcionais por variante, totalizando
504 aprovações. Resumo local em `build/favorites-summary.json` e logs em
`build/favorites-{64,32}-{release,debug}.log`.

Os testes exercitam filtros e índices canônicos, persistência M3U e reinício,
tags adicionais, estrelas, seleção e reprodução pela aba Favoritas, abertura
e fechamento da janela de informações, estabilidade da posição na lista de
temas, arraste da barra lateral e dimensões dos botões do EQ.

As verificações de janela usam mensagens Win32 em instâncias de teste,
com `LITESIC_NO_CAPTURE=1`. Não foram feitas capturas de tela. As 12 verificações
de grupos de imagem omitidas permanecem pendentes. A inspeção visual humana
do resultado permanece necessária, especialmente para confirmar a
percepção de cintilação no desktop do usuário.

Uma execução intermediária ficou presa no encerramento, com a pilha na
liberação de Direct2D/D3D e no driver gráfico NVIDIA. Os recursos do Canvas
agora são liberados no WM_DESTROY, antes da destruição estática do processo;
os testes direcionados seguintes encerraram normalmente. Isso não constitui
diagnóstico geral do driver.

Os relatórios anteriores de M6 e seus hashes representam os respectivos
estados anteriores do código; seus artefatos não foram regenerados para
estas alterações. Nenhum aceite visual humano ou nova compatibilidade de
sistema operacional é declarado aqui.
