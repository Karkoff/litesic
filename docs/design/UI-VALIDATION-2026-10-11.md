# Migração visual do LiteSic — 11/10/2026

Implementação **parcial e funcional**, sem commit/push. A modernização completa
UI-00..08 ainda não está concluída. Aceite estético humano: **PENDENTE**.
Windows 7: **PENDENTE DE VALIDAÇÃO EM M9**.

## Implementado e integrado

- PNG aprovado efetivamente aberto antes das alterações; especificação lida
  em `LITESIC-UI-UX-PROMPT-CODEX.md`. O nome `IMPLEMENTACAO-UI-UX-CODEX.md`
  citado no pedido não existe neste checkout; a diferença está no plano.
- Tokens de cores, tipografia e dimensões; primitivas reutilizáveis em
  `ui_theme.h` e `ui_draw.*`, com Segoe UI/Consolas via DirectWrite.
- Direct2D 1.0 em DC render target, recursos persistentes por janela,
  brush/formats em cache, BindDC por pintura/tamanho, transformação DIP→pixel,
  descarte/recriação em falha de EndDraw. GDI anterior permanece como fallback.
  Não há PNG como fundo, framework externo nem segunda engine de áudio.
- Player 460 × 290 DIP: visor, título, estado, seek, cinco transportes distintos
  em linha, volume percentual, balanço L/R, SHUF/REP/REP 1, LISTA, FFT/OSC e
  informações reais de taxa/canais/bitrate. LISTA mostra/oculta a janela.
- Transporte atua no controlador existente; seek/volume/pan no motor existente.
  Mouse capturado durante sliders, soltura fora da janela e cancelamento de
  clique tratados; teclado Tab/Shift+Tab, Enter/Espaço, setas/Home/End nos sliders;
  foco visível, hover/pressionado e tooltips em português no player.
- Playlist 460 × 360 DIP: + Arquivos, + Pasta, Remover, menu Lista/M3U,
  pesquisa nativa, limpar filtro, tabela título/duração, índice canônico,
  seleção e faixa atual distintas, rodapé com duração conhecida e contagens.
  Reutilizados worker, diálogos, M3U, tags e drag-and-drop existentes.
- `PlaylistView` filtra sem alterar ordem/fila/identidade. Seleção filtrada usa
  índices canônicos; seleções ocultas são descartadas para Delete não remover
  itens invisíveis. Arraste de reordenação exige pesquisa limpa.
- Dock existente preservado, com tolerância escalada por DPI. Player e lista
  continuam independentes; movimento conjunto, desprendimento e reencaixe
  foram exercitados por gestos reais.
- FFT/OSC Direct2D usam amostras reais e os mesmos algoritmos FFT/Hann/bins
  de produção. Timer de visualização a 33 ms, condicionado a reprodução,
  visibilidade e janela não minimizada. Removido repaint contínuo do marquee
  em repouso; título longo usa ellipsis e tooltip.
- EQ 460 × 210 DIP separado: painel **indisponível**, com dez bandas mais PRE,
  Flat/reset desabilitados e aviso explícito de DSP pendente. EQ… abre esse
  painel informativo. Fechar oculta. **Não há equalização funcional**.

O arquivo `src/audio.cpp` é byte a byte igual ao início desta execução:
SHA-256 `5B7545FD649131ABBE29F6A35D6714B0D2C341BFCC8BCF9CEF2361584F0BA6E3`.
As alterações locais de áudio anteriores ao pedido foram preservadas.

## Fases e integração com roadmap

| Fase | Visual | Funcional/testes | Status |
|---|---|---|---|
| UI-00 | antes inspecionado | quatro builds, baseline 292 | COMPLETE |
| UI-01 | tokens/componentes no executável | primitivas em produção | COMPLETE |
| UI-02 | D2D/DirectWrite nas quatro variantes | DC resize/recriação testados | COMPLETE |
| UI-03 | novo player implementado | transporte/seek/estados reais; acabamento ainda parcial | IN_PROGRESS |
| UI-04 | nova playlist implementada | filtro/seleção/M3U/worker/dock preservados; matriz visual ainda parcial | IN_PROGRESS |
| UI-05 | painel desabilitado | DSP/presets/ganhos não implementados | IN_PROGRESS |
| UI-06 | três painéis coerentes | magnetismo da playlist provado; EQ ainda indisponível | IN_PROGRESS |
| UI-07 | foco/hover/tooltips do player | DPI real e performance precisam ampliar validação | IN_PROGRESS |
| UI-08 | comparação e auditoria executadas | regressões passaram; pendências impedem encerramento global | IN_PROGRESS |

M5 permanece IN_PROGRESS, com seu checkpoint manual anterior preservado.
M6 não foi marcada concluída. A implementação contribui para M7/M8 sem
substituir suas tarefas. M9/M10 não avançaram. Nenhum aceite humano inferido.

## Builds e testes

| Variante | Build final | Casos aprovados | GUI final inicializada/capturada |
|---|---|---:|---|
| x64 Debug | OK | 80 | sim |
| x64 Release | OK | 80 | sim |
| x86 Debug | OK | 80 | sim |
| x86 Release | OK | 80 | sim |

Baseline repetida: 73 × 4 = **292**. Após componentes/backend: 74 × 4 = 296.
Após filtro/playlist: 77 × 4 = 308. Final: **80 × 4 = 320**, quinze binários
por variante. Sete casos novos: renderização D2D real em DC (1), filtro (2),
seleção filtrada por mensagens Win32 (1), transporte/seek/capture/teclado (3).
Testes novos vinculam código de produção, sem duplicar algoritmos.
O teste de recriação descarta deliberadamente os recursos pelo mesmo caminho
de recuperação; não simula perda física de GPU/driver.

Logs em `build/ui-baseline/final-{build,test}-{32,64}-{debug,release}.log`.
`scripts/gui.ps1 -Action test`: **20/20** guardas do auxiliar passaram.
`scripts/gui/verify_ui.py`: **142/142** verificações de hashes, proveniência,
pixels e geometria passaram; manifesto `build/gui/ui-evidence.json`.
Isso não é julgamento estético nem teste audível humano.

Preservados os warnings anteriores: casts de funções DPI e duas funções
GDI não usadas; Release também avisa sobre atomics internos do miniaudio.
Nenhum warning novo dos componentes modernos. `git diff --check` passou.
Imports novos são DLLs de sistema: D2D1, DWrite e COMCTL32. Não há runtime
Qt/.NET/UCRT/winpthread. Inspeção estática não valida Windows 7.

## Capturas reais

Todos os PNGs estão em `build/gui/`, ignorados pelo Git. `events.jsonl` contém
PID, criação, caminho, SHA-256 do executável e do PNG. A auditoria conserva o
hash de cada etapa; capturas intermediárias não recebem o hash final.

| Cenário | Capturas |
|---|---|
| Antes | `ui-00-before.png` |
| Player sem faixa final | `ui-08-64-debug.png`, `ui-08-64-release.png`, `ui-08-32-debug.png`, `ui-08-32-release.png` |
| Reprodução/FFT | `ui-08-playing.png`, `ui-08-fft.png` |
| Pausa estável | `ui-08-paused.png`, `ui-08-paused-stable.png` |
| Stop | `ui-03-stopped.png` (etapa inicial do novo player) |
| REP/REP 1/desligado | `ui-08-repeat-all.png`, `ui-08-repeat-one.png`, `ui-08-repeat-off.png` |
| Playlist | `ui-07-search-empty.png`, `ui-04-playlist-populated.png` |
| EQ indisponível | `ui-07-eq-pending.png` |
| Dock/solto/reencaixado | `ui-06-*` (geometrias auditadas no manifesto) |

Pausa final permaneceu em 00:56 entre capturas separadas por mais de 40 s.
Ao mover o player por (60,30), a playlist encaixada moveu pelo mesmo vetor.
Solta, permaneceu em [1797,1382,2487,1922] enquanto o player se moveu.
Reencaixada na borda inferior, retomou o movimento conjunto por (60,30).
As capturas das quatro variantes finais do player vazio são pixel a pixel
iguais nesta máquina, embora seus executáveis tenham hashes distintos.

## Comparação visual e diferenças da referência

Preservados bronze/marrom escuro, verde digital, tipografia do sistema,
hierarquia título→visor→seek→transporte→volume→opções→visualizador→rodapé,
playlist própria e EQ próprio. Ícones de transporte desenhados em vetores.
Não há maximizar falso nem botão Abrir no player.

Diferenças intencionais: 460 × 290 DIP em vez de 460 × 260, para cinco comandos
separados; desenho mais plano, sem brilho neon/degradês pesados; visualizador
mais baixo (32 DIP); SHUF/REP na linha de opções; metadados indisponíveis não
inventados; tabela prioriza Título e Duração. Não há colunas independentes de
artista/álbum, pois o modelo atual guarda título combinado/duração, nem um
campo de formato detectado. Os campos técnicos ausentes são omitidos.
O mockup apresenta EQ verde ativo; o painel entregue é neutro e indisponível.
São diferenças materiais a revisar, não equivalência visual perfeita.

## DPI, desempenho e limitações

Desktop real: Windows 11, 150%. Player final: 690 × 435 px; playlist 690 × 540;
EQ 690 × 315. Os testes de renderização usam 100/125/150/200% e os testes
Win32 exercitam DPI/layout/seek. Isso **não equivale** a alterar DPI do sistema
nem a capturas reais de monitores 100%/200%. O helper não oferece alteração
de DPI de monitor; essas capturas e arraste entre monitores ficam pendentes.
Não foi certificada acessibilidade completa MSAA/UI Automation dos custom
controls; teclado/foco e tooltips do player estão implementados. Tooltips
de títulos da playlist e navegação de foco entre busca/tabela precisam acabamento.

Medição curta comparável: x64 Debug, 150%, player + playlist vazios, EQ nunca
aberto, 3 s de amostra. Arquivos `idle.json` e `idle-final-comparable.json`:

| Medida | Antes | Depois |
|---|---:|---:|
| Working set | 19.771.392 B (18,9 MiB) | 64.335.872 B (61,4 MiB) |
| Private bytes | 4.419.584 B | 75.857.920 B |
| CPU acumulada por segundo | 26,04 ms/s | incremento abaixo da resolução da amostra |
| Respondendo | não registrado | sim |

O consumo de memória aumentou significativamente. Não foi comprovada a causa
isolada nem concluído o profiling de caches/runtime gráfico. Isso impede declarar
UI-07 concluída e equivalência de leveza com GDI. A amostra não prova CPU zero,
ganho sustentado, startup mais rápido nem ausência de picos.
Com EQ aberto, amostra intermediária chegou a 71.708.672 B de working set e
92.930.048 B privados; cenário diferente, não usado como comparação principal.

| Executável | Antes (B) | Final (B) |
|---|---:|---:|
| x86 Debug | 6.434.854 | 7.010.220 |
| x86 Release | 3.554.335 | 3.623.556 |
| x64 Debug | 7.158.069 | 7.768.703 |
| x64 Release | 3.699.629 | 3.758.400 |

Startup e latência/frame pacing não receberam benchmark comparável. O áudio
da prova é silencioso: FFT plano é esperado, sem animação sintética para imitar
o PNG. Fluidez com sinal não silencioso e profiling durante busca em milhares
de faixas precisam medição adicional. Os testes offline reais anteriores de
sample rates, EOF, repetição e callback sem espera continuam passando.

## Problemas tratados e próximos critérios

Corrigidos: rodapé “Estéreo” com formatação Unicode, placeholder da pesquisa,
seleção canônica sob filtro, remoção de seleção invisível, geometria do thumb
alinhada ao cálculo de entrada e captura/cancelamento de mouse.
Uma primeira execução do novo teste de transporte falhou porque o teste carregava
áudio sem registrar faixa no controlador; corrigida a preparação para usar a
playlist real. A falha permanece nos logs UI-07 e não é contada como aprovação.

Para fechar o restante:

1. M6/UI-05: implementar DSP real, publicação segura de parâmetros, suavização,
   estabilidade/Nyquist/mono-estéreo, bypass/Flat/ganho/presets/reset e troca de
   sample rate. Só depois habilitar os controles. Frequências do painel seguem
   a especificação visual 31..16k, diferente da tabela histórica da M6; alinhar
   o contrato antes de comissionar filtros, sem modificar o marco silenciosamente.
2. UI-03/04/07: finalizar estados elegíveis por ação, acessibilidade, tooltips
   da playlist, foco da pesquisa, metadados independentes quando disponíveis,
   matriz visual DPI/longos títulos/lista extensa e profiling de memória.
3. UI-06/08: repetir auditoria após essas alterações; testar EQ funcional,
   compatibilidade real em M9 e submeter o conjunto ao aceite visual humano.

## Git e reprodução

Branch master, HEAD 0440a5d preservado; nenhum commit/push/reset/clean destrutivo.
O working tree já estava sujo. Inventários antes/depois e diff anterior estão
em `build/ui-baseline/`; cópia dos fontes anteriores em `source/`.

Arquivos novos desta etapa: `src/ui_theme.h`, `ui_draw.*`, `ui_player.*`,
`playlist_view.*`, `eqwnd.*`, `tests/test_ui_draw.cpp`, `test_playlist_view.cpp`,
`test_player_window.cpp`, este relatório, plano, guia de componentes e
`scripts/gui/verify_ui.py`.
Alterados nesta etapa: `mainwnd.*`, `plwnd.*`, `visualizer.*`,
`tests/test_playlist_window.cpp`, Makefile, auxiliar GUI/guardas e documentação.
O restante do trabalho local foi mantido. Todas as instâncias do helper foram
fechadas ao terminar a prova; nenhuma outra instância foi encerrada.

Reprodução: `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1
-Action test -Architecture 64 -DebugBuild` (em uma linha). Omitir DebugBuild
para Release, usar Architecture 32 para x86. Auditoria sem entrada:
`.tools/gui-venv/Scripts/python.exe scripts/gui/verify_ui.py`.
