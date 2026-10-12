# Evidências da consolidação T6–T8

Nova execução em 11/10/2026 America/Sao_Paulo, 12/10 UTC.
[Relatório](../../M8-CONSOLIDATION.md), [manifesto](manifest.json).

- `build-{32,64}-{debug,release}.txt`: quatro builds incrementais atuais.
- `test-{32,64}-{debug,release}.txt`: quatro suítes completas, 179 casos cada.
- `catalog-validation.txt`: 18 catálogos, 237 IDs, inventário/fallback/literais.
- `gui-guards.txt`: 36 testes de proteção do driver.
- `tooltip-gui-64-release.txt`: dez casos, oito repetidos e dois adicionais;
  3.092 frames, 500 ciclos, recursos estáveis no intervalo medido.
- `startup.json`: identidade da sessão gráfica isolada.
- `playing-start-{0,1,2}.jpg` e `playing-later-{0,1,2}.jpg`: player,
  playlist e EQ; avanço 00:00 → 00:13 da fixture silenciosa.
- `paused-start.jpg`, `paused-later.jpg`: relógio mantido em 00:22.
- `stopped.jpg`: retorno a 00:00.

Capturas via Computer Use nativo, visualmente inspecionadas pelo agente.
O aceite visual humano T8 **TUDO PASS** veio do usuário; a inspeção desta
sessão não substitui nem amplia esse aceite para revisão linguística integral.
Áudio offline e fixture silenciosa não avaliam qualidade sonora subjetiva.
Não foram feitos testes físicos de troca/desconexão de monitores ou outros Windows.
