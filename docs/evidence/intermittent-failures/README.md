# Evidências das cinco intermitências

Arquivos preservados intencionalmente para sustentar o diagnóstico de
[12/10/2026](../../INTERMITTENT-FAILURES-2026-10-12.md):

- `run-summary.json`: todas as amostras, incluindo reprovações e experimentos.
- `final-validation.json`, `final-*.json` e logs associados: validação final.
- `before-*`, `suite-*`, `trace-*`, `shared-*`, `foreground-*`, `private-*`,
  `isolated-*` e demais amostras diagnósticas: condições anteriores/intermediárias.
- Fontes diagnósticos e patches de instrumentação: reprodução do diagnóstico;
  não são compilados pelo Makefile nem incluídos na Release.
- Inventários, hashes e patches da árvore: preservação de alterações e dados.

Os `.txt` são resultados de testes/compilação e traces usados na investigação,
não logs temporários de ferramentas. Nenhum executável, objeto, `.d`, ambiente
local ou perfil pessoal está incluído. Os inventários pessoais contêm apenas
caminhos, tamanhos e hashes; nenhum conteúdo de configurações ou playlists.

O aumento de **commit privado de memória** ainda tem origem interna
indeterminada. Não é referência a um commit Git privado e não se atribui uma
causa específica sem evidência.
