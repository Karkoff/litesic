# M8 — evidências da retomada final (12/10/2026)

Estado definitivo: **M8 — COMPLETE; aceite humano PASS (12/10/2026)**.
Decisão expressa do usuário: **M8-DEC-2026-10-12**.
[Relatório e observações futuras](../../M8-COMPLETION.md). Base técnica `6ae73fd`;
evidências originais preservadas para publicação em `origin/master`. Logs, hashes
e status abaixo são históricos da validação técnica anterior ao encerramento.
Nenhum teste ou auditoria repetido. Perfis exclusivamente em build.

## Resultados finais

| Verificação | Resultado | Evidência |
|---|---|---|
| Build/suíte x64 Debug | PASS | [build](final-build-64-debug.txt), [testes](final-test-64-debug.txt) |
| Build/suíte x64 Release | PASS | [build](final-build-64-release.txt), [testes](final-test-64-release.txt) |
| Build/suíte x86 Debug | PASS | [build](final-build-32-debug.txt), [testes](final-test-32-debug.txt) |
| Build/suíte x86 Release | PASS | [build](final-build-32-release.txt), [testes](final-test-32-release.txt) |
| Resumo de execução | 8 comandos exit 0 | [JSON](final-results.json) |
| Processos/perfis/portátil/IPC/hotkeys | 11 cenários PASS | [JSON](final-processes.json), [log](final-processes.txt) |
| DPI de produção simulado | 12 amostras por arquitetura PASS | [x64](final-dpi-64.txt), [x86](final-dpi-32.txt) |
| Catálogos/inventário/literais | 18 × 241 IDs PASS | [log](final-catalogs.txt) |
| Guardas de automação local | 36 PASS | [log](final-gui-guards.txt) |
| MSAA externo, fixture sandbox | 828 ocorrências PASS | [log](final-msaa-matrix.txt) |
| MSAA externo, GUI corrigida | 900 ocorrências PASS | [log](final-msaa-corrected.txt) |
| Foco real de Tab | child 1/Anterior, FOCUSED | [log](final-msaa-keyboard-focus.txt), [captura](final-focus-visible-0.jpg) |
| Tooltip GUI, mesmos testes | 10 casos PASS, 3.092 frames/500 ciclos | [log final](tooltip-retry.txt) |
| Regressão EQ antes da correção | FAIL esperado no interior não invalidado | [log](zoom-regression-before.txt) |
| Regressão EQ depois da correção | PASS nas quatro suítes | `zoom_repaints_old_equalizer_pixels` nos logs finais |
| Git e hashes | HEAD preservado, working tree local | [status](final-git-status.txt), [manifesto](final-manifest.json) |

31 executáveis por suíte; cada log contém 166 casos nomeados PASS, além de
verificações C com outro formato. Não confundir esses números com contagens
dos baselines anteriores. `pre-repaint-*` preserva a primeira regressão desta
retomada antes da correção do EQ; não é o resultado final do produto.

## Inspeção gráfica dirigida

Computer Use em perfil isolado, transporte de fixture silenciosa de 90 s:

- [Play X](final-keyboard-play-0.jpg), [Próxima B](final-keyboard-next-0.jpg),
  [Pause C](final-keyboard-pause-0.jpg).
- Ctrl+Alt+F9/F10/F11/F12 enviados à janela vazia de outro processo:
  [Play](final-global-play-background-0.jpg), [Pause](final-global-pause-background-0.jpg),
  [Stop](final-global-stop-background-0.jpg), [Próxima](final-global-next-background-0.jpg),
  [Anterior no início](final-global-previous-at-start-0.jpg).
- [Menu topo/atalhos ativos](final-topmost-hotkeys-checked-3.jpg),
  [atalho desativado não inicia Play](final-disabled-hotkey-no-play-0.jpg).
- [Minimização](final-minimized-state.json), [restauração](final-restored-state.json),
  [topo desativado nos três painéis](final-topmost-off-state.json).
- [Árvore assistiva corrigida](final-corrected-accessibility.txt),
  [nomes em inglês após percorrer os 18 idiomas](final-language-en-after-matrix-0.jpg).
- Defeito EQ: [antes, desenho duplicado](final-zoom125-settled-2.jpg);
  [reinício corrigido](final-fixed-restart-zoom125-2.jpg);
  [zoom 125→150% ao vivo corrigido](final-fixed-live-zoom150-2.jpg).

Capturas são por janela, algumas durante animação do menu. Não usar screenshot
por janela como prova isolada de z-order global: flags Win32 e observação das
operações complementam a evidência. O bridge resume foco como painel, enquanto
o cliente COM confirmou o controle focalizado.

## Infraestrutura e tentativas não aprovadas

Shell padrão estava no desktop isolado `CodexSandboxDesktop-…`; o auxiliar
detectou falta de desktop interativo e SetCursorPos falhou em
[final-tooltip-gui.txt](final-tooltip-gui.txt). A execução autorizada fora do
sandbox usa [desktop Default](interactive-doctor.json), sem contornar bloqueio
de segurança. Uma fixture topmost aberta coincidiu com falha inicial de hover
([log](final-tooltip-interactive.txt)); fechada a fixture, ocorreu uma falha
transitória da atualização do EQ ([log](final-tooltip-interactive-clean.txt)).
Reexecução dos mesmos testes passou integralmente em `tooltip-retry.txt`, sem
relaxar expectativas. Não foi demonstrado defeito persistente nesse tooltip.
O defeito de repaint do EQ foi reproduzido, corrigido e recebeu regressão própria.

Host físico: um monitor 3840×2160, DPI 144/150%, conforme
[estado final](final-corrected-state.json). Os probes simulam 96/120/144/192 DPI
e zoom 100/125/150%; não houve alteração de DPI global, desconexão de monitor,
teclado multimídia físico nem ensaio de leitor de tela. Revisão linguística
integral dos 18 idiomas permanece não realizada: observações futuras não
bloqueantes, sem atribuir PASS a esses ensaios. Aceite humano final M8: PASS.
36 temas preservados nos testes; isso não é revisão humana de cada combinação.

Capturas sem prefixo `final-` são da sessão anterior, preservadas como histórico.
Executáveis/objetos/perfis e fixtures temporários permanecem em build ignorado.
As instâncias e janelas auxiliares criadas para o teste foram encerradas.
