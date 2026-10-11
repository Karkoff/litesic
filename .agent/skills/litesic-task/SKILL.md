---
name: litesic-task
description: Executa uma tarefa do roadmap do LiteSic (docs/ROADMAP.md + docs/milestones/M<n>.md) do início ao fim — implementação, testes, commit, marcação do checkbox e proposta da próxima tarefa. Use quando o usuário pedir para "executar", "fazer", "implementar" ou "rodar" uma tarefa/milestone do LiteSic (ex.: "executa M2-T3", "faz a próxima tarefa", "implementa o M5-T2").
---

# Executar tarefa do LiteSic

Este skill implementa o fluxo padrão de trabalho no projeto LiteSic. Não
improvise um fluxo alternativo — siga os passos abaixo em ordem.

## Contexto obrigatório

Antes de qualquer coisa, leia (se ainda não estiverem no contexto):

1. `AGENT.md` — regras de código, decisões técnicas, convenções.
2. `docs/ROADMAP.md` — índice das milestones.
3. `docs/milestones/M<n>.md` correspondente à tarefa pedida (ID
   `M<n>-T<n>`) — contém o detalhamento, arquivos envolvidos, testes
   esperados e critério de "pronto" daquela tarefa específica.

Se o usuário pedir "a próxima tarefa" sem ID, descubra a última tarefa
commitada (`git log --oneline`, mensagens no padrão `M<n>-T<n>: ...`) e/ou
o primeiro checkbox `- [ ]` não marcado, em ordem, percorrendo
`docs/milestones/M0.md`, `M1.md`, etc.

## Passo 1 — Implementar

Implemente exatamente o escopo da tarefa, respeitando as regras de
`AGENT.md` (Unicode `...W`, sem `std::thread`/`std::mutex`, callback de
áudio nunca aloca/trava/mexe em janela, `_WIN32_WINNT=0x0601`).

Não adiante escopo de tarefas futuras nem deixe a tarefa atual pela
metade.

## Passo 2 — Testar

- Se a tarefa tem lógica pura testável (parsing, matemática de DSP, modelo
  de dados — ver `docs/ROADMAP.md`, cada tarefa indica quando há teste
  unitário esperado), escreva/atualize o teste em `tests/` e rode
  `make test`.
- Sempre rode `make` (ou `make DEBUG=1`) e confirme que compila sem erros
  nem warnings novos.
- Se a tarefa é a **última tarefa de uma milestone marcada com 🖥️** no
  roadmap, isso ainda não substitui a validação manual do passo 4 — mas
  já rode o `.exe` uma vez você mesmo para conferir que não crasha
  imediatamente.

**Se algo não for verde** (não compila, teste falha, comportamento
claramente errado): não commite. Volte ao passo 1. Se travar tecnicamente
tentando resolver (dúvida sobre uma API do miniaudio, Win32, stb_vorbis,
ou como resolver um bug específico), consulte nesta ordem antes de pedir
ajuda ao usuário:
1. MCP **context7** — documentação da biblioteca/API em questão.
2. MCP **github** — exemplos de código e issues de projetos parecidos.

## Passo 3 — Marcar o checkbox, commitar e fazer push

Só chegue aqui se compilação e testes estiverem 100% verdes.

1. Em `docs/milestones/M<n>.md`, marque a tarefa concluída trocando
   `- [ ] **M<n>-T<n> — ...`  por `- [x] **M<n>-T<n> — ...`. Se a tarefa
   era a última da milestone, marque também os itens do checklist em
   "Checkpoint da milestone" que já se confirmam (deixe os que exigem
   validação manual do usuário sem marcar até essa confirmação chegar).
2. Commit incluindo o código da tarefa e essa atualização do checkbox:

```
git add <arquivos da tarefa> docs/milestones/M<n>.md
git commit -m "M<n>-T<n>: <resumo curto do que foi feito>"
git push
```

Adicione apenas os arquivos relevantes à tarefa (nunca `git add -A` sem
checar `git status` antes). Nunca use `--no-verify`.

Por preferência permanente do usuário, registrada em `AGENT.md` em
11/10/2026, commit e push devem ocorrer logo após concluir cada tarefa e
suas verificações, sem nova confirmação, salvo instrução explícita posterior
em contrário. Usar o remote/upstream da branch atual; não forçar push.
Conferir o resultado do push e relatar falhas ou bloqueios com precisão.

## Passo 4 — Checkpoint visual (entre milestones)

Se a tarefa que acabou de ser commitada é a **última tarefa de uma
milestone** (conferir em `docs/milestones/M<n>.md` — todos os checkboxes
`M<n>-T*` daquela milestone já marcados):

1. Compile a versão mais recente (`make`).
2. Avise o usuário explicitamente: "Milestone M<n> concluída — abra
   `build/LiteSic.exe` e valide antes de eu seguir para a próxima
   milestone" (ou ofereça rodar você mesmo, se houver uma forma
   automatizada de abrir o app no ambiente).
3. **Não inicie a próxima milestone sozinho** — espere confirmação do
   usuário de que a validação manual passou. Testes verdes não substituem
   isso: é comum a aplicação não funcionar direito na prática mesmo com
   testes passando.
4. Quando o usuário confirmar, marque os checkboxes restantes do
   "Checkpoint da milestone" em `docs/milestones/M<n>.md` e faça um
   commit `M<n>: checkpoint validado`, seguido de push.

Se a tarefa NÃO é a última da milestone, pule este passo.

## Passo 5 — Perguntar sobre a próxima tarefa

Ao final (depois do commit, e do checkpoint visual quando aplicável),
pergunte ao usuário se deseja iniciar a próxima tarefa do roadmap,
identificando qual seria (ID + descrição curta). Não inicie a próxima
tarefa automaticamente.
