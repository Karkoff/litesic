# M8-T8 — inventário anterior à implementação

Inspeção das regiões e despachos reais de mainwnd, ui_player, plwnd, eqwnd e theme_window. Prefixo das chaves: `tooltip.`; título da faixa é dado.

## player

| Controle/ID real | Chave(s) |
|---|---|
| 1 | previous |
| 2 | next |
| 3 | shuffle_on / shuffle_off |
| 4 | repeat_off / repeat_all / repeat_one |
| 5 | play |
| 6 | pause |
| 7 | stop |
| 8 | seek |
| 9 | volume |
| 10 | balance |
| 11 | show_playlist / hide_playlist |
| 12 | show_eq / hide_eq |
| 13 | fft |
| 14 | osc |
| 15 | track title (data) |
| 16 | show_viz / hide_viz |
| 17 | minimize |
| 18 | close |
| 19 | time |

## playlist

| Controle/ID real | Chave(s) |
|---|---|
| AddMenu | add |
| Remove | remove |
| ListMenu | list_menu |
| GeneralTab | all |
| FavoritesTab | favorites |
| ClearSearch | clear_search |
| SearchId / search region | search |
| row | row |
| row star | favorite_on / favorite_off |
| scroll rail | scroll |
| -1 | hide_playlist |

## equalizer

| Controle/ID real | Chave(s) |
|---|---|
| 0 | eq_on / eq_off |
| 1 | presets |
| 2 | reset |
| 3 | preamp |
| 4…13 | band / band_unavailable |
| 14 | hide_eq |

## appearance

| Controle/ID real | Chave(s) |
|---|---|
| 702 | theme_list |
| 703 | theme_reset |
| 704 / 705 | close |
| 706 | category |
| scroll rail | scroll |

## information

| Controle/ID real | Chave(s) |
|---|---|
| 801 | info |
| 802 | close |

Menus nativos já possuem rótulos localizados e navegação de teclado: adicionar arquivos/pasta, importar/exportar, ordenar nome/duração, misturar, limpar e informações; não recebem popups sobre menus modais. Colunas não ordenam por clique. LCD alterna tempo decorrido/restante. Título mostra metadado completo. Combo 701 é modelo oculto, sem região elegível. Sem novos botões.
