# AGENT.md — LiteSic

Este arquivo é o ponto de entrada para qualquer agente (humano ou IA) que for
implementar o LiteSic. Leia isto antes de tocar em qualquer arquivo, e
consulte `docs/ROADMAP.md` para saber qual tarefa executar.

## O que é o projeto

Player de áudio estilo Winamp, em C++ nativo com Win32 + Direct2D/DirectWrite, rodando sem
runtime extra do Windows 7 ao Windows 11. Três telas: player principal,
equalizador de 10 bandas e lista de reprodução. Sem streaming por enquanto.

O plano de produto/arquitetura completo está em `PLANO-VSCODE.md` — este
documento (`AGENT.md`) não repete decisões de lá, apenas resume o que é
obrigatório respeitar durante a implementação.

## Decisões técnicas obrigatórias

- **Linguagem/UI**: C++17 + Win32 API + Direct2D 1.0/DirectWrite; GDI como fallback durante a migração. Nada de Qt, WinUI, .NET,
  MFC ou qualquer coisa que exija runtime extra.
- **Áudio**: [miniaudio](https://miniaud.io) (single header) + `stb_vorbis`
  para OGG, ambos em `third_party/`.
- **Compilador**: MinGW-w64 via MSYS2, ambiente **MINGW64** (ou MINGW32 para
  32 bits). Nunca usar o ambiente UCRT64 — quebra compatibilidade com
  Windows 7.
- **Alvo mínimo**: `_WIN32_WINNT=0x0601` (Windows 7). Qualquer API que exija
  versão maior do Windows é proibida, a menos que tenha fallback.
- **Build**: `Makefile` + tasks do VS Code (`Ctrl+Shift+B`). Comandos:
  `make`, `make DEBUG=1`, `make ARCH=32`, `make clean`.

## Regras de código (não negociáveis)

1. Usar sempre as variantes **`...W` (Unicode)** das funções Win32 para
   nomes de arquivo — nunca a versão ANSI.
2. **Proibido `std::thread`/`std::mutex`/`std::condition_variable`** — usar
   `CreateThread` e `CRITICAL_SECTION`, que não dependem de `winpthread` e
   funcionam em qualquer Windows.
3. O **callback de áudio** (rodando na thread do miniaudio) nunca pode
   travar, alocar memória (`new`/`malloc`) ou tocar em objetos de janela
   diretamente — apenas `PostMessage` para a UI.
4. Antes de fechar uma tarefa que toca em APIs novas do Windows, checar se
   elas existem no Windows 7 (MSDN "Requirements: Minimum supported client").
5. Não introduzir dependências novas sem necessidade — o objetivo é um
   `.exe` único e estático.

## Estrutura de pastas esperada

Ver seção 3 do `PLANO-VSCODE.md`. Resumo:

```
src/        código-fonte (app.cpp, mainwnd.cpp, eqwnd.cpp, plwnd.cpp,
            skin.*, audio.*, playlist.*, config.*, util.cpp, ...)
third_party/  miniaudio.h, stb_vorbis.c
res/        ícone, manifest (DPI + compatibilidade Win7-11), app.rc
.vscode/    tasks.json, launch.json, c_cpp_properties.json, extensions.json
```

## Testes

Para validação gráfica, leia `docs/GUI-AUTOMATION.md`. Prefira Computer Use
nativo quando funcionar. O auxiliar local `scripts/gui.ps1` permite captura
real e entrada restrita ao LiteSic; exige o desktop interativo e respeita a
revisão de aprovação da plataforma para execuções fora do sandbox. Sempre
inspecione a captura antes de escolher coordenadas e confirme o resultado
visualmente. Não confunda uma execução sem erro com validação da interface.

Nem tudo em Win32/GDI é testável automaticamente. A regra prática:

- **Lógica pura sem dependência de janela** (parsing de M3U, parsing de INI,
  matemática do EQ, modelo da playlist, parsing de tags) **deve** ter testes
  unitários. Colocar em `tests/`, rodados via `make test`.
- **Código de UI/janela** não tem teste automatizado — a validação é
  compilar sem erros/warnings novos e, ao final de cada milestone, **abrir o
  `.exe` para o usuário validar manualmente** (ver skill de execução de
  tarefas).
- Nunca marcar uma tarefa como concluída só porque compilou: rodar
  `make test` (quando a tarefa tiver testes) antes de commitar.

## Quando travar: use os servidores MCP

Este ambiente já tem os servidores MCP **context7** e **github** disponíveis
na extensão local do agente. Sempre que ficar travado em algo técnico
(comportamento de uma API do miniaudio, assinatura de uma função Win32,
exemplo de uso de `stb_vorbis`, como outro projeto resolveu um problema
parecido), a ordem é:

1. Consultar **context7** para puxar documentação atualizada da biblioteca
   em questão (miniaudio, stb_vorbis, Win32 API).
2. Consultar **github** para buscar exemplos de código, issues ou projetos
   de referência (ex.: outros players Winamp-like em C++/Win32).
3. Só perguntar ao usuário depois de tentar os dois acima e ainda assim
   estar bloqueado.

Não adivinhar assinaturas de API ou comportamento de biblioteca — checar.

## Fluxo de execução de tarefas

O fluxo de "implementar → testar → commitar → perguntar próxima tarefa" está
definido como uma skill em `.agent/skills/litesic-task/SKILL.md`. Use-a
sempre que for executar uma tarefa do roadmap — não improvise um fluxo
diferente.

## Roadmap

`docs/ROADMAP.md` é só o índice das milestones. Cada milestone (M0–M10)
tem seu próprio arquivo em `docs/milestones/M<n>.md`, com a lista
detalhada de tarefas — cada tarefa é um checkbox (`- [ ]`, marcado
`- [x]` quando concluída), com arquivos envolvidos, o que fazer, testes
esperados e critério de "pronto".
