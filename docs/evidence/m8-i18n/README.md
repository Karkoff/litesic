# Evidências M8-T6

Capturas realizadas em 11/10/2026 no horário America/Sao_Paulo (UTC já 12/10),
Windows do host atual, perfil isolado do auxiliar gráfico. 54 imagens reais
PrintWindow/GDI, três janelas por idioma, escala 150%, tema original.
As nove folhas `sheet-*` apenas agrupam essas imagens para inspeção.
Inspeção visual realizada nos 18 idiomas; não representa revisão linguística humana.
Combinações dos 36 temas/4 escalas verificadas por teste de renderização,
sem alegar inspeção visual de todas essas combinações.

| Variante | Build | Casos da suíte completa |
|---|---|---:|
| 64-release | PASS | 161 PASS |
| 64-debug | PASS | 161 PASS |
| 32-release | PASS | 161 PASS |
| 32-debug | PASS | 161 PASS |

**644 casos** nas quatro variantes, contra baseline 592: +13 por variante
(11 núcleo + 2 integração). 29 guardas gráficas PASS. `PASS` por linha não é
o total, pois testes C e algumas suítes imprimem somente um resumo.
Os logs são cópias normalizadas para UTF-8 dos comandos reais. Os builds finais
são incrementais; a cadeia de implementação também compilou as quatro variantes.
Uma interrupção deixou arquivos temporários .d com NUL; foram regenerados e
as variantes afetadas (64-debug/32-release) repetidas com sucesso. Os logs
preservados aqui são os finais após essa recuperação; nenhum .d corrompido restou.
Permanecem warnings preexistentes de casts Win32/third_party; nenhum warning
novo nas unidades i18n. Não houve teste prático em Windows 7 nesta execução.

Métricas incrementais do teste (ms e bytes):

```text
64-release arch=64 init_ms=0.093 mean_select_ms=0.162 fallback_private_delta=61440 warm_private=1830912 final_private=1900544 max_payload_bytes=17640 switches=1980
64-debug arch=64 init_ms=0.553 mean_select_ms=0.897 fallback_private_delta=204800 warm_private=1691648 final_private=1945600 max_payload_bytes=17640 switches=1980
32-release arch=32 init_ms=0.129 mean_select_ms=0.222 fallback_private_delta=24576 warm_private=2449408 final_private=2613248 max_payload_bytes=17640 switches=1980
32-debug arch=32 init_ms=0.595 mean_select_ms=1.015 fallback_private_delta=20480 warm_private=2445312 final_private=2473984 max_payload_bytes=17640 switches=1980
```

Máximo payload retido: 17.640 bytes, excluindo overhead do alocador/containers.
Private Bytes após aquecimento permanece dentro da tolerância de 512 KiB
nos 1.980 switches de cada variante; isso não prova ausência absoluta de leaks.
Hash do executável e de cada captura, horários e processo: `manifest.json`.
Eventos originais: `gui-events.jsonl`. Perfil pessoal não utilizado.

[Relatório e limitações](../../M8-T6-I18N.md).
