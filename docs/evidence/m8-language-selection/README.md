# Evidências M8-T7 — seleção de idioma

Execução local 11/10/2026, eventos UTC em 12/10. Relatório A–N:
[M8-T7-LANGUAGE-SELECTION](../../M8-T7-LANGUAGE-SELECTION.md).

- `manifest.json`: quatro executáveis finais (tamanho/SHA-256), 684 casos,
  métricas, origem e hash individual das capturas. 32 guardas separadas.
- `build-*.txt`, `test-*.txt`: logs finais UTF-8, quatro variantes PASS.
  `full-build-*.txt`: primeira compilação completa, incluindo warnings existentes.
- `catalog-validation.txt`: 18 × 191 IDs e auditoria dos literais/IDs PASS.
- `gui-guards.txt`: 32 testes do driver PASS; não são 32 ensaios visuais.
- `events.jsonl`: ações T7, incluindo recusas do próprio driver (não aprovações).
  Capturas de diálogos nativos com diretórios pessoais foram excluídas.
- `t7-<idioma>-<janela>.png`: 90 capturas reais, 18 × player/playlist/EQ/temas/
  informações, mesma sessão/PID 56216. `contact-<idioma>.png` compõe originais,
  sem reconstruir a UI. Nem toda captura é alegada como inspecionada.

Inspeção representativa: 50 capturas nas dez folhas pt-BR, en-US, de-DE, ru-RU,
ja-JP, zh-CN, zh-TW, ko-KR, ar-SA e hi-IN. Também inspecionados o submenu com
marca automática, a marca manual restaurada, o player sueco após restart, as
informações árabe/hindi em tamanho original e o menu/playlist do smoke final.
Os PNGs restantes permitem revisão adicional, sem afirmar aceite linguístico.

As 90 capturas usam x64 Release SHA-256
`075dcd19086ef4ed91292221fc8ca7d239e2f4c876b872f6b8c89718e63213b3`.
Um ajuste posterior afeta a identificação do idioma Windows no rótulo
Automático enquanto outro idioma está selecionado manualmente. `t7-final-*.png`
identificam essa build candidata x64 Release
`0b263754bc63274bfcb718295d7e659a427957cb59f58aa979195a1dd7f3718b`, sessões
PID 38708 (menu) e 40884 (reprodução).
`t7-final-manual-check.png` mostra interface sueca, idioma Windows português e
marca Svenska; clique real em Automático atualiza a playlist para português.
Cada evento/manifest guarda seu próprio hash, sem atribuir imagens antigas à
build final. Depois disso foi corrigida a direção inicial da pesquisa ao criar
uma janela já em árabe, com assert específico e repetição das quatro suites.
`t7-startup-rtl-*.png` registram esse smoke no executável final; seu SHA-256 está
no manifest. Screenshots de restart manual/automático pertencem a sessões distintas.

Desktop Default, escala observada 150%, Windows do ambiente; testes não mudam
preferências Windows. WAV de silêncio do próprio auxiliar foi reproduzido no
smoke inicial, mas terminou antes da matriz. O smoke adicional PID 40884 com
dispositivo real mostra 00:01 em português, 00:23 em árabe/FFT, 00:52 em hindi/OSC,
mesma faixa e volume, em `t7-final-audio-*.png`. As provas precisas de continuidade de áudio,
posição, estado, FFT e OSC estão nos testes Win32 com leitura determinística de WAV.
Não inferir continuidade audível ou ausência absoluta de leaks por screenshots.

Evidências T6 permanecem em `../m8-i18n`, com seus números/hashes históricos.
Nenhum commit/push ou pacote M10 nesta entrega.
