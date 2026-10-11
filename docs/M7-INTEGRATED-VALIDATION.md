# M7 — integração e validação

> Atualização de 11/10/2026: **M7 COMPLETE**, aceite humano “TUDO PASS”.
> [Encerramento formal](M7-CLOSEOUT-2026-10-11.md) prevalece para o estado atual.
> O texto abaixo preserva os resultados e pendências da rodada histórica.

Data: 11/10/2026. Estado: **READY_FOR_HUMAN_ACCEPTANCE**.
Complemento posterior: [compactação ao ocultar painéis](M7-VISIBILITY-COMPACTION.md).
Os logs e hashes deste relatório registram a rodada anterior; os executáveis
atuais e a validação do ajuste estão no complemento.
Implementação e verificação técnica concluídas; aceite visual final do usuário
permanece aberto. M0–M6 e M9 continuam COMPLETE. Nenhuma implementação de
M8/M10, commit ou push foi realizada neste ciclo.

## A. Estado inicial

- Branch `master`, HEAD `82d28ce`, working tree limpa. O HEAD real já incluía
  a preferência permanente de produzir Release x64 e x86, posterior ao
  `3ca057a` citado no pedido.
- Baseline repetida x64 Release: 136 casos em 23 executáveis, PASS.
  As 544 aprovações da M6 nas quatro variantes são evidência histórica,
  não outra execução desta rodada.
- Inspecionados AGENT, PLANO-VSCODE, README, ROADMAP, M0–M10,
  documentação de UI/automações, código e testes. A
  [matriz completa](ROADMAP-IMPLEMENTATION-AUDIT.md) registra 48 requisitos.

## B/C. Auditoria M7 e implementação

| Requisito | Existia | Residual implementado |
|---|---|---|
| M7-T1 | Canvas comum, desenhos e controles funcionais | Skin comum para identidade, arraste e relações entre janelas |
| M7-T2 | Snap, quatro lados, âncoras e docking direto no player | Snap playlist↔EQ, grafo sem ciclos e integração com as posições existentes |
| M7-T3 | Painéis acompanhavam diretamente o player | Propagação transitiva nas duas ordens e movimento de componentes desencaixados |
| M7-T4 | Botões EQ/LISTA, visibilidade e DSP independentes | Preservação da formação no hide/show e reinício; painel oculto intermediário |

Reutilizados WindowSnap::Snap/Place/KeepVisible/PlaceVisible, Ui::Canvas,
conversões DPI, INI existente, sidecar da playlist, toggles e renderização.
A arquitetura de reprodução e o DSP permanecem os mesmos.

## D. Arquitetura e contratos

`src/skin.h/.cpp` mantém três identidades estáveis (player, playlist, EQ),
HWNDs registrados e um grafo com no máximo um pai lógico por painel.
O vínculo lógico não modifica o owner Win32 das janelas. A matemática pura
permanece em `window_snap.h`; a integração é executada apenas na thread UI.

WM_ENTERSIZEMOVE/WM_MOVING/WM_EXITSIZEMOVE usam o mesmo tratamento nas três
janelas. WM_WINDOWPOSCHANGING restringe a formação acessível;
WM_WINDOWPOSCHANGED propaga posições com guarda contra reentrada.
Arrastar um painel desfaz seu vínculo e conserva os seus descendentes.
Ao encaixar o player num componente independente, ele permanece raiz.
O limiar existente de 24 DIP é convertido para pixels pelo chamador.

WM_DISPLAYCHANGE e mudanças de escala reconciliam a formação. A união das
áreas úteis dos monitores aceita coordenadas negativas e travessia de áreas
contíguas, sem tratar uma lacuna entre monitores como área disponível.
Se uma formação não couber após mudança de topologia, apenas vínculos que
impedem a recuperação de acessibilidade podem ser desfeitos.

Persistência usa as seções existentes, mantendo `Version=1` e acrescentando
`Parent`: `[EqualizerWindow]` no INI (0=player, 1=playlist) e `[Geometry]`
no sidecar da playlist (0=player, 2=EQ). Ausência do campo usa player,
compatível com arquivos anteriores. Valores inválidos/ciclos têm fallback
seguro. Coordenadas são físicas; dimensões seguem a escala existente.
EQ intermediário oculto é criado sem exibição para restaurar uma playlist
dependente dele. Ocultar EQ não desliga o filtro, nem ocultar playlist para
a reprodução. Não foi criado outro sistema de configuração.

A API acrescentada, EnumDisplayMonitors, está disponível desde Windows 2000,
conforme os [requisitos oficiais da Microsoft](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enumdisplaymonitors).
Isso verifica disponibilidade da API; não substitui ensaio físico no Windows 7.

## E/F. Testes e builds finais

Comandos: `make -j2 ARCH=<64|32> DEBUG=<0|1> all test`, ambiente MSYS2
MINGW64/MSVCRT, `LITESIC_NO_CAPTURE=1` nos testes integrados.

| Variante | Casos | Executáveis de teste | Resultado | Binário |
|---|---:|---:|---|---|
| x64 Debug | 145 | 24 | PASS | `build/64/debug/LiteSic.exe` |
| x64 Release | 145 | 24 | PASS | `build/64/release/LiteSic.exe` |
| x86 Debug | 145 | 24 | PASS | `build/32/debug/LiteSic.exe` |
| x86 Release | 145 | 24 | PASS | `build/32/release/LiteSic.exe` |

Total: **580 aprovações**, 96 execuções de binários de teste. São nove casos
novos por variante: três em test_window_snap e seis em
test_window_integration. Nenhum caso anterior foi removido. Teste existente
de aparência ampliado para roundtrip/legado/valor inválido de Parent.
Além disso, **27 testes das guardas do driver gráfico passaram**.

Três grupos de captura por variante foram omitidos intencionalmente pela
variável acima (12 omissões), sem contabilizá-los como aprovados. A sessão
gráfica autorizada abaixo produz evidência própria, não substitui esses grupos.

Cobertura acrescentada: cadeias nas duas ordens, desencaixe e reencaixe de
componentes, player encontrando painel independente, ponte oculta,
minimizar/restaurar, continuidade de PCM offline pela engine de produção,
resize da playlist, WM_DPICHANGED em 96/120/144/192 DPI,
WM_DISPLAYCHANGE, roundtrip de posições e rejeição de ciclo.
Topologias negativas, 800×600, 4K deslocado, áreas contíguas e lacunas
foram verificadas por matemática simulada. Mensagens DPI e engine offline
não comprovam troca física de monitores nem saída audível no dispositivo.

Inspeção de imports das duas releases: apenas DLLs do sistema
(COMCTL32, COMDLG32, D2D1, DWrite, GDI32, KERNEL32, MSVCRT, OLE32,
SHELL32, USER32), sem runtime externo acrescentado. Logs em evidence.
Nenhum warning novo identificado. Persistem avisos anteriores de cast de
GetProcAddress/DPI, funções GDI não usadas e miniaudio em Release.

## Automação gráfica real

O usuário autorizou capturas exclusivamente da sessão isolada de teste.
Executada x64 Release no desktop interativo, escala física de 150%, perfil
`build/gui/m6-session`, por scripts/gui.ps1. Capturas PrintWindow/GDI somente
das janelas dessa sessão, inspecionadas visualmente; nenhuma captura do
desktop ou da sessão pessoal. A lista vazia é intencional neste perfil.

| Cenário | Evidência/resultado |
|---|---|
| Primeira exibição de EQ | Encaixe à direita, indicador EQ claro |
| Desencaixar EQ | Arraste real separa o painel |
| Formação player→playlist→EQ | Playlist abaixo, EQ à direita da playlist |
| Mover player | Os três retângulos deslocaram exatamente (+72,+36) pixels |
| Desencaixar/reencaixar EQ horizontalmente | Separação de 90 pixels e retorno ao vínculo playlist→EQ |
| Ocultar/reabrir EQ e playlist | Indicadores mudaram; posições preservadas |
| Fechar/reiniciar a sessão isolada | Os três retângulos e a cadeia foram restaurados |

Retângulos antes do movimento: player [1575,863,2265,1298], playlist
[1575,1298,2265,1838], EQ [2265,1298,2955,1619]. Após movimento e após
reinício: player [1647,899,2337,1334], playlist [1647,1334,2337,1874], EQ
[2337,1334,3027,1655]. Perfil final registra EQ Parent=1 e playlist Parent=0.

Uma tentativa intermediária de arraste foi interrompida pela guarda do
driver porque o ponto ficou coberto por outra janela após snap. Esse evento
`ok:false` foi preservado, sem declará-lo aprovado. Capturas subsequentes
confirmaram a formação; desencaixe/reencaixe horizontal foi então executado
com sucesso sem relaxar as guardas. As duas sessões isoladas foram fechadas
ao terminar; a aplicação pessoal não foi encerrada nem reconfigurada.

Não houve ensaio físico Windows 7/8/8.1/10, multi-monitor ou multi-DPI nesta
rodada. Esses limites não reabrem a M9 nem anulam os testes simulados.

## G. Regressões encontradas durante desenvolvimento

| Problema | Correção | Verificação final |
|---|---|---|
| Owner oculto usado pelos testes Win32 não era candidato ao snap | Player registrado pode ser candidato oculto; painéis ocultos não são alvos novos | Testes existentes e integração passaram |
| Reencaixe podia sobrepor EQ e playlist | Reutilizar PlaceVisible ao confirmar snap e reconciliar irmãos | Teste anterior de colisão e quatro suítes passaram |
| Wrapper legado não recuperava painel fora da área útil | Recuperação explícita no fluxo padrão; reconciliação quando preservar formação | Teste anterior de restauração e teste novo DISPLAYCHANGE passaram |

Todas as quatro suítes finais foram executadas depois dessas correções.

## H/I. Auditoria e roadmap

48 tarefas: 33 ALREADY_ACCEPTED (M0–M6), cinco
IMPLEMENTED_AND_VERIFIED (quatro M7 e imports M9), quatro
PARTIALLY_IMPLEMENTED (três M8 e uma M10), quatro NOT_IMPLEMENTED
(duas M8 e duas M10), duas BLOCKED por ambiente de ensaio (M9).
BLOCKED aqui qualifica testes físicos indisponíveis, não o estado
administrativo da milestone. A matriz detalha arquivos, testes, evidências,
lacunas, ação e impacto por requisito.

M7 está READY_FOR_HUMAN_ACCEPTANCE. O próximo trabalho funcional é M8,
após o aceite: completar atalhos globais/mídia, sempre no topo, instância
única e persistência/modo portátil. Zoom/DPI, volume, tema, EQ, favoritos e
sessão já existem e devem ser reaproveitados. M10 ainda requer prova formal
de reprodutibilidade, pacote portátil e decisão sobre instalador opcional.
M0–M6 e M9 permanecem COMPLETE; não se iniciou implementação M8/M10.

## J. Git e evidências

HEAD permanece `82d28ce`, branch `master`; mudanças locais sem staging,
commit ou push. Alterados Makefile, três painéis, ui_theme, window_snap,
testes existentes e documentação de estado/roadmap/milestones pertinentes.
Adicionados skin.h/.cpp, test_window_integration.cpp, este relatório,
matriz e evidências. A lista exata dos arquivos consta do inventário.

[Manifesto e hashes](evidence/m7-integrated/manifest.json),
[inventário](evidence/m7-integrated/changes.txt) e
[eventos gráficos](evidence/m7-integrated/gui-events.jsonl) preservam
os resultados da execução. Logs completos das quatro variantes, baseline,
guardas, imports, perfil isolado e capturas representativas estão no mesmo
diretório. O manifesto identifica o hash dos quatro executáveis, sem
versionar binários do produto. `build/LiteSic.exe` é a cópia anterior da M6;
para validar esta entrega, usar as releases nas pastas por arquitetura.

O checkpoint do usuário continua desmarcado em M7. Valide uma das releases
e confirme o comportamento das três janelas antes do avanço para M8.
