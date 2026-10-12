# LiteSic — Release anterior à distribuição, 12/10/2026

## 1. Consolidação

Revisadas e publicadas as alterações locais da limpeza de runtime e da
investigação de intermitências. Nenhuma funcionalidade nova, alteração em
`src/`, `res/`, `third_party/` ou `locales/`, início da M10, instalador ou pacote.
A auditoria de desempenho permanece `COMPLETE_WITH_CAVEATS`; não foi repetida.

- Makefile: objetos/dependências, testes e diagnósticos separados do runtime;
  catálogos próprios para testes/probes e limpeza conservadora.
- Helpers: reconstrução integral, logs/identidades internos e caminhos atualizados.
- Cinco cenários de falha preservados em quatro testes: desktop privado para
  topmost; isolamento de desktop/IME em tooltip; heap vivo em i18n e seleção
  de idioma com controle positivo; INI exclusivo por PID na seleção de idioma.
- Verificador Release: perfis novos, idiomas, persistência, módulos carregados,
  ícone, D2D/DirectWrite e avanço de reprodução pelo MSAA existente.
- Documentação e evidências relevantes preservadas byte a byte, com atributos
  Git próprios. Não foram incluídos objetos, `.d`, executáveis, logs temporários,
  perfis pessoais ou ferramentas de `.tools/`.

O “commit privado” citado no diagnóstico é **memória comprometida do processo**.
A origem interna exata de todo aumento observado continua indeterminada; não é
um commit Git de origem desconhecida nem foi atribuída a um subsistema específico.
Não foi comprovado defeito funcional do LiteSic nesses cinco cenários.

Referências históricas:
[limpeza](CLEAN-RUNTIME-2026-10-12.md),
[intermitências](INTERMITTENT-FAILURES-2026-10-12.md),
[auditoria anterior](PERFORMANCE-AUDIT.md).

## 2. Commit de origem

Branch: `master`. HEAD inicial: `609cb38a2f6fa73cde22e56354b51ead08ee5da0`.

Commit das fontes consolidadas:
**`23310e32b5ddfa99e2014374d02e9e75f0df9348`**.

Mensagem: `fix(litesic): stabilize tests and clean release build layout`.
Remoto: `https://github.com/Karkoff/litesic.git`.

## 3. Publicação

`git push origin HEAD:master` concluído sem force push ou reescrita do histórico.
Antes das builds, `git ls-remote origin refs/heads/master`, HEAD e a referência
local `origin/master` confirmaram o commit acima às **15:38:48 -03:00**.
[Registro](evidence/pre-distribution-release/published-source.json).

O relatório e as evidências desta etapa são publicados em um commit documental
posterior. O commit de origem dos executáveis continua sendo o registrado acima;
as fontes, scripts de build, testes e seus 81 inputs não mudaram nessa etapa documental.

## 4. Reconstrução limpa

Executados, após confirmar a publicação:

```powershell
./scripts/dev.ps1 -Architecture 64 -Rebuild
./scripts/dev.ps1 -Architecture 32 -Rebuild
```

Árvore limpa antes, durante e ao término das duas reconstruções e da validação
de runtime. `-Rebuild` passou `make -B`: **26 unidades de código + recurso RC
recompilados por arquitetura**. Conferidos comandos de compilação, conjunto de
fontes e timestamps de todos os 27 objetos posteriores ao início da build.
Nenhum objeto antigo foi reutilizado na linkedição do aplicativo.

MSYS2 MINGW64/MINGW32, GCC/G++ 16.2.0 Rev5, MSVCRT, Release `-O2`, runtime
GCC/C++ estático. Sem `DEBUG` ou `LITESIC_TEST` no aplicativo. Permanecem os
avisos anteriores de conversão de GetProcAddress, funções GDI sem uso e
miniaudio em otimização; nenhuma alteração funcional para escondê-los.

[Verificações dos inputs e recompilação](evidence/pre-distribution-release/clean-rebuild-checks.json),
[build x64](evidence/pre-distribution-release/build-64-release.txt),
[build x86](evidence/pre-distribution-release/build-32-release.txt).

## 5. Validações

| Verificação desta entrega | x64 Release | x86 Release |
| --- | --- | --- |
| Reconstrução integral | PASS, exit 0 | PASS, exit 0 |
| Suíte completa do commit publicado, sequencial e sem retries | 31/31, exit 0 | 31/31, exit 0 |
| Catálogos idênticos às fontes | 18/18 | 18/18 |
| Inicialização e encerramento normal | 19/19, exit 0 | 19/19, exit 0 |
| Avanço real de reprodução pelo seek MSAA | 2/2 | 2/2 |
| Títulos traduzidos, persistência, ícone e D2D/DirectWrite | PASS | PASS |
| Cópia contendo somente runtime + fixture local | PASS | PASS |
| Arquitetura PE e imports do Windows | x64, PASS | x86, PASS |
| Estrutura Release e hash inalterados após testes | PASS | PASS |

As 19 inicializações por arquitetura incluem os 18 idiomas e uma cópia isolada.
As quatro checagens de reprodução usam perfil novo, volume zero e observam o
percentual da faixa avançar. Não constituem avaliação auditiva. Os filhos têm
PATH restrito ao Windows, cwd da própria Release/cópia e `--profile=` explícito.
Nenhum módulo de `.tools/` ou DLL de desenvolvimento foi carregado pelo player.
Nenhuma dependência adicional precisou ser adicionada à Release.

Settings, playlist e arquivo associado do perfil pessoal foram comparados
com os hashes preservados na investigação: **3/3 idênticos**. Nenhum processo
do usuário foi encerrado. Ao concluir os testes não restou processo LiteSic
iniciado pela automação, servidor auxiliar ou instrumentação no executável.

As suítes usam `LITESIC_NO_CAPTURE=1`: assertions de pixels permanecem PENDING.
Os resultados anteriores de **92/92 execuções dirigidas**, quatro builds e
quatro suítes 31/31 foram preservados; hashes dos 124 binários dessas suítes
foram reconferidos antes da consolidação. Não foram somados como novas execuções.

[Suíte x64](evidence/pre-distribution-release/test-64-release.txt),
[suíte x86](evidence/pre-distribution-release/test-32-release.txt),
[smoke e módulos](evidence/pre-distribution-release/runtime-checks.json),
[validação final e hashes](evidence/pre-distribution-release/final-validation.json),
[revisão anterior ao commit](evidence/pre-distribution-release/precommit-review.json).

## 6. Estrutura final

```text
build/
├── 64/release/
│   ├── LiteSic.exe
│   └── locales/<18 idiomas>/messages.tsv
├── 32/release/
│   ├── LiteSic.exe
│   └── locales/<18 idiomas>/messages.tsv
└── internal/<64|32>/release/
    ├── obj/
    ├── tests/
    ├── diagnostics/
    ├── logs/
    └── identity/
```

Cada Release tem exatamente 19 arquivos: executável + 18 catálogos. Perfis,
fixtures e cópias de validação permanecem em `build/validation/`; não fazem
parte do runtime. Debug e evidências históricas foram preservados.

## 7. Identidade inequívoca

Commit de origem de **ambos**: `23310e32b5ddfa99e2014374d02e9e75f0df9348`.
Horários abaixo em 12/10/2026, America/Sao_Paulo, UTC−03:00.

| Arquitetura | Início | Fim | Bytes |
| --- | --- | --- | ---: |
| x64 | 15:39:07.1035460 | 15:39:38.0770239 | 4.767.936 |
| x86 | 15:39:50.8704658 | 15:40:20.7952402 | 4.694.637 |

**x64**

- Caminho: `D:\Projetos\IA\litesic\build\64\release\LiteSic.exe`
- SHA-256: `E9C3963B0CC2D294E8AB9512E5A53AA1166B617D3147809707FD4A5C508FC4B1`
- [Identidade completa, compilador e 81 inputs](evidence/pre-distribution-release/identity-64-release.json).

**x86**

- Caminho: `D:\Projetos\IA\litesic\build\32\release\LiteSic.exe`
- SHA-256: `93908A174F9B0F9FA5AA35A9F06F9A7DA2D04C07BA460EC44993065EDD82C899`
- [Identidade completa, compilador e 81 inputs](evidence/pre-distribution-release/identity-32-release.json).

Os hashes coincidem com os executáveis preservados da preparação anterior.
Essa é uma comparação efetiva de bytes, mas **não é uma certificação de build
reproduzível** sob condições equivalentes: não se executou nesta etapa uma
segunda reconstrução independente do mesmo commit/toolchain.

## 8. Versão para avaliação humana

Caminho definitivo x64:
**`D:\Projetos\IA\litesic\build\64\release\LiteSic.exe`**.

Executáveis validados e preservados desde **15:45:00 -03:00**. Não foram
substituídos depois desse ponto. O SHA-256 acima identifica a versão durante
os próximos 1–2 dias de teste. Abrir normalmente, sem flags de automação,
utiliza o perfil pessoal habitual. Não há instrumentação pesada, servidor
auxiliar ou processo de teste necessário à execução. Não foram habilitados
updates automáticos ou alterações silenciosas no executável.

Evitar comandos de build/clean direcionados às Releases durante essa avaliação;
novas investigações devem usar saídas isoladas.

## 9. Pendências conhecidas

- Avaliação humana prolongada, visual e auditiva ainda a realizar.
- Origem interna exata do aumento histórico de commit privado permanece indeterminada.
- Compatibilidade prática com outros Windows e observações anteriores de M8/M9
  continuam conforme os relatórios históricos; milestones não foram reabertas.
- Modos opcionais de hover/captura e entrada IME real não foram certificados aqui.

Não restou falha nas validações executadas; não se declara estabilidade
integral em ambientes ou durações ainda não testados.

## 10. Estado final do Git

`master` publicada em `origin/master`, sem force push, com HEAD local igual
à referência remota e árvore limpa após o commit documental deste relatório.
As alterações posteriores às builds são somente documentação/evidências e
atributos Git desses arquivos; não comprometem a correspondência fonte/binário.
Os 81 inputs e os dois hashes finais são reconferidos após essa publicação.
O hash fonte nas identidades é o commit compilado, não o commit documental.

Nenhuma alteração local anterior foi descartada. `build/` e `.tools/` continuam
ignorados e não fazem parte dos commits. M10 permanece não iniciada.
