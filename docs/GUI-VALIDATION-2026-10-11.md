# Validação da automação gráfica — 10/11 de outubro de 2026

Prova realizada na transição de 10 para 11/10, horário de São Paulo. Eventos
usam UTC. Branch `master`, HEAD `0440a5d`; sem commit ou push.

## Diagnóstico comprovado

O processo Python do comando comum estava na sessão 1, `WinSta0`, desktop
`CodexSandboxDesktop-23bda26eb818ca36ca6572c5eaf2aa16`; o desktop de entrada era
`Default`. O Explorer e o host Codex também estavam na sessão 1. Mesmo número
de sessão não significa mesmo desktop. Uma execução pontual aprovada fora
do isolamento retornou `Default` tanto para o desktop atual quanto o de entrada.

Computer Use nativo: importação de `@oai/sky` bem-sucedida; `list_windows`
falhou com canal nativo indisponível, arquivo não encontrado, `os error 2`.
O reset do REPL seguido de nova tentativa produziu a mesma falha. Plugins
estavam habilitados. Não foi comprovada a causa da indisponibilidade do canal.

## Ambiente instalado

Python 3.11.9 x64 já existente. Venv `.tools/gui-venv`: pywinauto 0.6.9,
Pillow 12.3.0, pywin32 312, comtypes 1.4.17, six 1.17.0. `pip check` passou.
A instalação inicial encontrou bloqueio de rede no sandbox e depois erro de
certificado no pip; a execução pontual autorizada com certificados do Windows
(`truststore`) resolveu ambos, mantendo verificação HTTPS. Lock com hashes
adicionado e comando `setup` validado. Nenhuma dependência de runtime do player
foi acrescentada; nenhum MCP comunitário foi instalado.

## Prova prática

Executável existente `build/64/debug/LiteSic.exe`, SHA-256
`4e01fe2b3a6ab946bc5809338a199332f7659196b1ada0e4e2dba24536d7b630`.
Janela identificada por PID/criação/caminho/hash e classe `LiteSic.MainWnd`;
captura 413 × 270 pixels, escala observada 150%. A janela principal expõe zero
controles filhos; os botões GDI foram acionados por coordenadas observadas.

| Operação | Evidência real | Resultado |
|---|---|---|
| Lançar e capturar | `build/gui/initial.png` | Janela do player inspecionada pelo agente |
| Clicar Abrir | `open-dialog.png` | Diálogo real de arquivos capturado |
| Digitar caminho + Enter | `loaded.png`, depois `reloaded.png` | WAV silencioso carregado; título, 48 kHz estéreo, `02:00` |
| Clicar Play | `play-click.png`, `playing-a.png` | LCD avançou de `00:00` para `00:12`; seek avançou |
| Clicar Pause | `paused-a.png`, `paused-b.png` | LCD `00:20`, imagens idênticas após **46,599991 s** |
| Retomar Play | `resumed.png`, `resumed-later.png` | LCD voltou a avançar, chegando a `01:19` |
| Clicar Stop | `stopped.png` | LCD `00:00` e seek no início; imagem igual à tela carregada |
| Escape no diálogo | `escape-return.png` | Diálogo fechado e retorno ao player |
| Encerramento | `events.jsonl`, evento `close` | Instância do driver encerrada |

As imagens foram abertas e analisadas pelo agente com a ferramenta de
visualização local. São capturas `PrintWindow` da janela real, não mockups.
`verify_poc.py` passou em **8/8 verificações** de proveniência e alterações
visuais; detalhes/hashes/timestamps em `build/gui/poc-verification.json`.
O teste usou silêncio e não comprova qualidade audível humana.

## Falhas observadas e corrigidas no auxiliar

Na primeira tentativa, uma função de consulta de processo indisponível no
pywin32 causou falha; substituída pela API documentada `QueryFullProcessImageNameW`.
A instância órfã dessa tentativa foi identificada e encerrada especificamente.
Uma captura imediatamente após Abrir encontrou o diálogo ainda em criação;
adicionada espera limitada. A digitação inicialmente foi conferida antes de o
campo processar as mensagens; adicionada espera pelo texto esperado. O fluxo
Abrir → digitar → Enter foi repetido com sucesso. Esses eventos de falha ficam
no log; não foram contados como sucesso.

## Proteções e verificações

**14/14 testes** do código real do driver passaram: caminho externo/arbitrário,
desktop isolado, nome de captura com travessia de diretório, reutilização de PID,
executável trocado/recompilado, ausência/expiração/futuro da observação, outra
janela/geometria, consumo da observação e clique fora da janela. São testes de
guardas sem enviar entrada à GUI; a tabela acima registra a validação prática.

Hashes de todos os arquivos `src/*`, Makefile e configurações VS Code coincidiram
com a referência salva antes do trabalho funcional interrompido. As alterações
preexistentes de infraestrutura foram preservadas. Código preliminar da M5
ficou reservado em `build/m5-paused/`, ignorado e fora do build; nenhuma tarefa
M5 foi integrada ou marcada concluída nesta atividade.

## Limites e continuidade

Capacidade demonstrada: auxiliar local, x64 Debug, Windows 11, desktop interativo
desbloqueado. Não foi certificada a GUI em Windows 7, x86, todos os DPIs, desktop
bloqueado ou execução por serviço. Computer Use nativo permanece indisponível
nesta sessão; procedimento de reteste e uso do auxiliar em
[GUI-AUTOMATION.md](GUI-AUTOMATION.md). F5 visual e qualidade audível humana
continuam fora desta prova. Não há avanço para M5/M6 nem alteração de segurança
global. As evidências ficam localmente em `build/gui/`, ignoradas pelo Git.
