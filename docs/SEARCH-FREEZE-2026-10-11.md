# Travamento ao pesquisar na playlist — 11/10/2026

O usuário relatou a interface travada durante a pesquisa, com o áudio tocando.
Foram obtidas duas amostras da instância x64 Debug aberta (PID 79648), sem
encerrá-la e sem capturas de tela. Ambas mostraram a thread da interface em
`PlaylistView::Update`, chamada por `PlWnd_Refresh` durante a recepção de
`WM_PLAYLIST_RESULTS`. A playlist tinha 10.453 entradas e uma pesquisa ativa.
Na primeira amostra a thread de áudio estava processando o EQ normalmente.
As pilhas ficam localmente em `build/live-freeze-79648*.log`; elas contêm
caminhos da biblioteca do usuário e não são artefatos públicos.

O worker enviava uma mensagem para cada resultado e outra ao terminar cada
job. A interface drenava vários resultados de uma vez, mas cada notificação
restante, mesmo vazia, reconstruía o filtro duas vezes. Com milhares de
entradas e pesquisa ativa, a fila acumulava varreduras e alocações de strings,
atrasando a entrada e a pintura. Isso explica o travamento observado; não foi
identificada uma espera de áudio bloqueando a interface nessas amostras.

A correção agrupa as notificações: apenas uma fica pendente até a drenagem.
A interface agenda o processamento em lotes por um timer de 50 ms, permitindo
entrada e pintura entre os lotes. Drenagens vazias não reconstroem o filtro.
Cada atualização necessária agora refaz o filtro uma vez. Os resultados
continuam limitados a 64, com cancelamento por geração e encerramento do worker
preservados. Não há alterações no callback ou no processamento de áudio.

Regressões novas:

- `notifications_coalesce_until_drain`: dois lotes reais de metadados geram
  uma notificação por lote, sem perder resultados, e voltam a notificar após
  a drenagem.
- `large_search_ignores_stale_metadata_notifications`: pesquisa sobre 10.453
  entradas sintéticas, mil notificações antigas e timers vazios, orçamento
  de dois segundos e remoção da entrada filtrada pelo índice canônico.

Os testes usam `LITESIC_NO_CAPTURE=1` e não acessam nem modificam o perfil
pessoal. Foi preservada uma cópia local dos arquivos de sessão existentes
em `build/live-freeze-backup-79648`. A instância original foi encerrada durante
a investigação, sem comando de encerramento deste agente; depois foram
observadas novas instâncias do executável Debug anterior à correção.

Build e suíte completos passaram nas quatro variantes x86/x64,
Debug/Release: 131 casos por variante, 524 aprovações. Logs em
`build/search-freeze-*-*.log` e resumo em `build/search-freeze-summary.json`.
Doze grupos de verificações de imagem continuam omitidos; não se declara
aceite visual humano.

O x64 Debug foi compilado primeiro como `LiteSic-fixed.exe`, pois havia uma
instância antiga em execução. Após sua validação, o executável antigo foi
renomeado para uma cópia de segurança no mesmo diretório e o novo foi copiado
para `build/64/debug/LiteSic.exe`, com hashes conferidos. A instância já aberta
continua executando o código antigo até o usuário reiniciá-la; ela não foi
encerrada pelo agente. O Release também está atualizado no caminho habitual.
