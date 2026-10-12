# M8-T6–T8 — consolidação e regressão final

Execução em 11/10/2026 America/Sao_Paulo (12/10 UTC).
Baseline real: `master`, HEAD `266270aa9caa2469144f9af30d9609877941f7b4`,
origin `https://github.com/Karkoff/litesic.git`. Alterações locais T6–T8
inspecionadas e preservadas, incluindo arquivos novos. Nenhuma funcionalidade
nova foi implementada neste ciclo de consolidação.

## Aceites e estado formal

- **M8-T6 COMPLETE:** infraestrutura, catálogos e tipografia entregues.
- **M8-T7 COMPLETE:** aceite humano PASS anterior preservado, sem nova solicitação.
- **M8-T8 COMPLETE:** usuário declarou **TUDO PASS** na avaliação final;
  este é o aceite visual humano dos tooltips.
- O aceite visual T8 não comprova revisão linguística profissional dos 18 idiomas.
- M8 permanece aberta. M0–M7 e M9 permanecem COMPLETE; M10 aberta.
- Ordem aprovada: conclusão M8 → auditoria geral de desempenho → M10.
  Não houve auditoria geral, otimização preventiva ou empacotamento nesta execução.

## Verificações executadas nesta sessão

Os números abaixo foram obtidos novamente; resultados históricos T6–T8
permanecem nos respectivos pacotes de evidências, sem substituição de logs.

| Variante | Build | Suíte funcional | Casos |
|---|---|---|---:|
| x64 Release | PASS | PASS | 179 |
| x86 Release | PASS | PASS | 179 |
| x64 Debug | PASS | PASS | 179 |
| x86 Debug | PASS | PASS | 179 |
| Total | Quatro PASS | Quatro PASS | **716** |

Contagem por variante: 152 casos C++, 16 FFT/mapeamento/ring buffer,
10 FormatTime e um smoke real de dependências. Testes executados sequencialmente
porque fixtures existentes de geometria da playlist compartilham um arquivo.
Builds incrementais atuais passaram, sem alterações de código após a validação;
não são prova de compilação limpa reproduzível da M10.

`scripts/i18n_catalogs.py --source`: **18 × 237 IDs**, **46 chaves tooltip.***
por catálogo. UTF-8, cobertura, duplicatas, vazios, placeholders, plurais,
fallback inglês incorporado, inventário e varredura de literais PASS.
36 testes de proteção do driver gráfico PASS.

`test-test_tooltip.exe --gui`, x64 Release no desktop interativo: dez casos PASS,
dos quais oito já pertencem à suíte funcional e **dois são adicionais**.
18 idiomas × 36 temas × quatro escalas = 2.592 apresentações;
500 ciclos adicionais = **3.092 frames**. Frames não são casos independentes.
Atraso, ocultação, expiração, idioma/tema ao vivo, estado DSP publicado,
EDIT nativo, reutilização e encerramento PASS. Private Bytes após aquecimento:
delta 0 B; USER/GDI estáveis no ensaio limitado. Não é auditoria geral de desempenho.

As suítes exercitam áudio offline real, EQ, playlist/favoritos/pesquisa,
temas, seleção Windows/manual/persistida, cinco janelas, idioma ao vivo,
tooltips, docking, posições, zoom e persistência dos campos existentes.
Preservação do áudio/DSP/tema/modelo/geometria durante mudanças de idioma
tem asserções em `test_language_window.cpp` e `test_i18n_window.cpp`.
Isso não equivale a avaliação audível prolongada nem valida campos ainda ausentes.

## Inspeção gráfica nova

Computer Use nativo funcionou nesta sessão. Build x64 Release iniciada com
`--validation --locale=pt-BR`, PID 6940, perfil exclusivo `build/gui/m6-session`.
Player, playlist e EQ inspecionados. Fixture `gui-silence`: Play avançou
00:00 → 00:13; Pause manteve 00:22 em duas observações separadas; Stop voltou
a 00:00. Instância encerrada normalmente. Perfil pessoal e preferências do
Windows não foram alterados. As imagens são capturas do aplicativo real.

A árvore nativa retornou somente painéis player/EQ/playlist, sem elementos
individuais dos controles Canvas. Nomes `PlayerControlName` existentes
não são equivalentes a providers UIA/MSAA. Não se afirma acessibilidade completa.

## Git e publicação

Diff, arquivos novos e evidências revisados. Somente código, catálogos,
testes, scripts e documentação pertinente são incluídos. Toolchains, objetos,
dependências geradas, executáveis, perfis pessoais e temporários ficam ignorados.
Capturas/manifestos/logs oficialmente necessários em `docs/evidence` são incluídos.
Varredura de padrões de credenciais executada antes do commit, sem achados.
`.gitattributes` preserva os bytes/hashes dos pacotes M8 e permite espaços
finais do output bruto de console. `git diff --check` continua verificando
código, catálogos e documentação fora desses artefatos brutos.

A consolidação usa a mensagem
`feat(litesic): complete M8 internationalization and multilingual tooltips`.
Commit/push autorizados pelo usuário para `origin/master`, sem force push,
reescrita de histórico ou descarte. O resultado e o hash publicado são
registrados na auditoria residual posterior e no fechamento da execução.

Evidências novas: [manifesto](evidence/m8-consolidation/manifest.json) e
[guia dos logs/capturas](evidence/m8-consolidation/README.md).
