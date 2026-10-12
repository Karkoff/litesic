# Evidências M8-T8

Execução em 11/10/2026, America/Sao_Paulo; eventos em UTC 12/10.
[Manifesto com hashes e métricas](manifest.json), [eventos gráficos](events.json),
[relatório](../../M8-T8-TOOLTIPS.md), [inventário](../../M8-T8-CONTROL-INVENTORY.md).

- Quatro builds e suítes completas PASS, 179 casos por variante, 716 no total.
- Contagem: 152 casos C++ + 4 FFT + 6 mapeamento + 6 ring buffer + 10 FormatTime + 1 smoke.
- 36 guardas gráficas PASS, contadas separadamente; 46 IDs novos, 237 IDs × 18 catálogos.
- Teste gráfico adicional x64 Release: dois casos novos do popup real; frames/ciclos não são casos independentes.
- Capturas reais pelo driver restrito, PrintWindow entre processos, escala observada 150%.
- Dez imagens selecionadas e inspecionadas: pt-BR, árabe/RTL, alemão/slider, japonês e hindi.
  O ganho de 1 kHz mudou pelo arraste de +0,0 para +2,9 dB; texto do tooltip acompanha.
- Matriz gráfica automatizada: 18 × 36 × quatro escalas = 2.592 frames e 500 ciclos adicionais = 3.092 frames.
  Consulta/renderização WM_PRINTCLIENT dentro da mesma thread; não confundir com screenshot do desktop.
- As duas sessões interativas usaram hashes de executável anteriores ao build final; esses hashes estão separados
  no manifesto. A primeira tinha textos iniciais das linhas da playlist; a segunda antecede apenas a guarda de
  falha de layout. Nenhuma captura é atribuída retroativamente ao hash final.
- Tentativas sem tooltip após clique/expiração, painel oculto e desktop sandbox não interativo aparecem
  nos eventos; não contam como sucesso visual. O driver exige saída/reentrada após expiração.
- T7: aceite humano PASS informado pelo usuário. T8: aceite visual humano PENDING.

Comandos: `scripts/dev.ps1 -Action build|test -Architecture 64|32`,
usando PowerShell com `-NoProfile -ExecutionPolicy Bypass` e `-DebugBuild` nas variantes Debug. Executar sequencialmente:
algumas fixtures anteriores compartilham arquivos temporários entre variantes.
Validador: `python scripts/i18n_catalogs.py --source`. Guardas:
`.tools/gui-venv/Scripts/python.exe scripts/gui/test_driver.py`.
Gráfico: `build/64/release/test-test_tooltip.exe --gui` no desktop interativo, com a revisão
normal de execução da plataforma. Nenhuma alteração de preferência pessoal ou idioma Windows.

Logs completos estão em arquivos `.txt` para permanecerem versionáveis sem alterar .gitignore.
Medições são incrementais e limitadas; nenhuma auditoria geral de desempenho foi iniciada.
Revisão linguística/RTL complexo, acessibilidade Canvas e ensaios físicos multi-DPI/Windows
continuam nos limites do relatório. M8 geral permanece aberta; M10 não iniciada.
