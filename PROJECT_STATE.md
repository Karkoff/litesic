# PROJECT_STATE — LiteSic

**Correção desktop 1.0.1 — validada em 03/11/2026; preparada para GitHub.**
Exportação M3U/M3U8 respeita Geral/Favoritas; exportações aceitas sobrevivem a
Limpar e ao encerramento normal. VERSIONINFO 1.0.1.0. Release x64/x86: 186 casos
por arquitetura PASS; quatro ZIPs padrão/portátil e 16 inicializações reais PASS,
com reprodução e persistência, sem alterar o perfil pessoal. Seletor nativo de
arquivos sem validação visual conclusiva nesta rodada. [Relatório e evidências](docs/PLAYLIST-EXPORT-1.0.1.md).
A publicação 1.0.0 e o candidato Store abaixo são registros anteriores.

**Distribuição desktop 1.0.0 — publicada em 03/11/2026, final estável/Latest.**
Quatro ZIPs: x64/x86 padrão
e portátil, com 18 idiomas, ajuda e licenças; portátil ativa `data/` por duplo
clique via `resources/portable.flag`. VERSIONINFO corrigido e atualizado para
1.0.0.0. Suítes Release 184 casos por arquitetura PASS; quatro ZIPs extraídos,
16 inicializações e reprodução real PASS, sem alterar o perfil pessoal.
[Release](https://github.com/Karkoff/litesic/releases/tag/v1.0.0) e
[procedimento/evidências](docs/DESKTOP-RELEASE-2026-11-03.md). Cinco anexos
públicos baixados e conferidos por SHA-256; [confirmação](docs/evidence/desktop-release-2026-11-03/publication.json).
M10 continua IN_PROGRESS; ensaio formal de reprodutibilidade T1 e validação
humana dos ZIPs T2 mantêm seus critérios. A preparação da Store abaixo
conserva sua identidade, fontes e pacotes anteriores.

**Microsoft Store — IN_PROGRESS (03/11/2026).** Identidade oficial
`Brasiliense.LiteSic`, publicador Brasiliense, Store ID `9NJ52QFN0LMX`.
MSIX candidato 1.0.0.0 x86/x64 gerado; MakeAppx e integridade PASS;
WACK 24/24 por arquitetura PASS; suítes Release PASS; instalação real,
reprodução dos quatro codecs, persistência e remoção do pacote de teste
validadas no Windows 11. Quatro capturas reais 1920×1080 PT-BR/EN-US,
com áudio original e proteção de diálogos privados. Releases tradicionais
homologadas preservadas. **Submission 1 está em rascunho no Partner Center;
pacotes não carregados, certificação não solicitada, publicação não confirmada.**
CLI oficial exige primeira publicação via portal; API exige Entra e preparação
inicial/classificação etária no portal. [Procedimento](docs/STORE-PUBLICATION.md)
e [status](docs/STORE-STATUS.json). Registros anteriores abaixo são históricos.

**Reprodução inoperante após inatividade — PASS por decisão expressa do usuário
(15/10/2026); resultado técnico COMPLETE_WITH_CAVEATS.**
Causa: perda do endpoint de áudio (fone Bluetooth desconectado) encerrava o
loop do dispositivo do miniaudio sem religá-lo; o LiteSic reportava PLAYING
sem renderizar. Correção: recuperação da saída motivada por estado observado
(no Play e enquanto toca), limitada, preservando faixa/posição/configurações,
com log `audio.log` só em eventos. Falha reproduzida antes da correção;
182 casos por arquitetura PASS (x64/x86) e probe no WASAPI real PASS.
Validação humana com desconexão física do fone e sessão de horas: a ser feita
pelo usuário, ainda não executada.
[Relatório](docs/PLAYBACK-IDLE-RECOVERY-2026-10-15.md).

**Auditoria Geral de Desempenho — COMPLETE_WITH_CAVEATS (12/10/2026).**
Decisão expressa do usuário: **PERF-DEC-2026-10-12**, que aprovou as três
correções e os resultados; status restrito ao escopo validado. Correções:
fábrica Direct2D compartilhada (um dispositivo D3D em vez de um por janela:
repouso x64 81 → 45 threads, WS 58,9 → 51,9 MiB), streaming de áudio acima de
64 MiB (FLAC de 60 min +502 → +0,1 MiB; WAV de 1,31 GB volta a tocar no x86;
~10 ms de silêncio por seek nesses arquivos) e um único relógio de repintura
com título longo (CPU 9,2% → 4,7%). Quatro builds, 168 casos por build PASS;
continuidade real sem quedas, inclusive com CPU saturada; sessão prolongada de
34,8 min sem crescimento. Validações não executadas (sessão ≥ 60 min, playlist
e estresse na build final, suítes com captura, Windows 7–10, HDD/rede) são
observações futuras, não aprovadas; capturas exigem autorização prévia.
[Relatório](docs/PERFORMANCE-AUDIT.md). Próxima atividade: **M10 — Distribuição**,
não iniciada; será autorizada separadamente.

**M8 — COMPLETE; aceite humano PASS (12/10/2026).**
Decisão expressa do usuário: **M8-DEC-2026-10-12**, solicitação
“LITESIC — ENCERRAMENTO DEFINITIVO DA M8”. Resultados técnicos, evidências
originais e aceites anteriores preservados. Validações físicas, assistivas e
linguísticas não realizadas são observações futuras não bloqueantes e não
reabrem automaticamente a milestone; não são convertidas em PASS técnico.
M0–M7 e M9 permanecem COMPLETE. A Auditoria Geral de Desempenho foi depois
encerrada (ver registro acima); **M10 — Distribuição** não iniciada.
Neste encerramento não foram repetidos testes ou auditorias nem implementadas
funcionalidades. Consolidação, commit e push para `origin/master` autorizados.

Os baselines e pendências abaixo são históricos; o encerramento acima prevalece
para o estado atual. [Relatório final M8](docs/M8-COMPLETION.md).

**M6 — COMPLETE, por decisão explícita do usuário em 11/10/2026.**
O usuário informou aprovação visual, audível, funcional, de uso prolongado
e do comportamento geral após os ajustes. Esse aceite final substitui
READY_FOR_HUMAN_ACCEPTANCE. [Encerramento e validação final](docs/M6-CLOSEOUT-2026-10-11.md).
M0–M5: COMPLETE. M9: COMPLETE, conforme decisão preservada abaixo.
**M7 — Integração das Janelas: COMPLETE (11/10/2026).**
Aceite humano final concedido pelo usuário: **“TUDO PASS”**.
[Encerramento formal](docs/M7-CLOSEOUT-2026-10-11.md). [Validação M7](docs/M7-INTEGRATED-VALIDATION.md) e
[auditoria do roadmap](docs/ROADMAP-IMPLEMENTATION-AUDIT.md).
Complemento aprovado: [compactação de painéis ocultos](docs/M7-VISIBILITY-COMPACTION.md),
com restauração da formação ao reabrir e prioridade ao arraste manual.
Histórico anterior ao encerramento: **M8 — Acabamento**, incluindo internacionalização
integral em 18 idiomas, seleção automática/manual e tooltips multilíngues.
**M8-T6 — COMPLETE (11/10/2026):** serviço central, 18 catálogos com 188 IDs,
fallback inglês, Unicode, fontes nativas e preparação RTL/troca programática.
Quatro builds, 644 casos e 29 guardas PASS; 54 capturas reais inspecionadas.
[Relatório e limites](docs/M8-T6-I18N.md). Revisão linguística humana pendente;
tooltips explicativos T8 entregues abaixo.
**M8-T7 — COMPLETE (11/10/2026):** padrão Automático (Windows), submenu Idioma
com 18 nomes nativos, preferência no INI existente e atualização das cinco janelas
sem reinício, preservando áudio/EQ/playlist/geometria. Baseline T7: 191 IDs.
Quatro builds, 684 casos funcionais e 32 guardas PASS; 90 capturas ao vivo,
50 inspecionadas em folhas representativas, além de menus/restarts.
[Relatório e limitações](docs/M8-T7-LANGUAGE-SELECTION.md). Aceite humano T7: **PASS**,
explicitamente informado pelo usuário.
**M8-T8 — COMPLETE (11/10/2026), aceite visual humano TUDO PASS:** mecanismo central de tooltips
Direct2D/DirectWrite nas cinco janelas, estados atuais, 46 novas chaves,
237 IDs × 18 idiomas e tokens dos 36 temas. Quatro builds, 716 casos,
36 guardas e dois casos gráficos adicionais PASS; 3.092 frames, 500 ciclos.
[Relatório](docs/M8-T8-TOOLTIPS.md) e [evidências](docs/evidence/m8-tooltips/README.md).
Aceite visual humano T8 concedido pelo usuário na solicitação de consolidação.
Não equivale a revisão linguística profissional dos 18 idiomas; limitações
de acessibilidade Canvas daquele baseline preservadas. M8-T1…T5 e checkpoint
posteriormente concluídos e aceitos em 12/10/2026.
[Consolidação, nova regressão e publicação](docs/M8-CONSOLIDATION.md).
M8 COMPLETE. Próxima atividade: **Auditoria Geral de Desempenho → M10 — Distribuição**.
M10 continua aberta, com distribuição limpa e reproduzível planejada.
A auditoria e suas evidências anteriores permanecem como histórico.

Correção de reprodução em 11/10/2026: duração MP3 armazenada antes de tocar,
evitando consulta concorrente ao decodificador ativo. Quatro builds e 444
verificações passaram; ver [auditoria](docs/AUDIO-TRANSITION-AUDIT-2026-10-11.md).
Esse resultado não altera a cobertura prática ou o encerramento da M9.

Registro canônico criado em 11/10/2026 para persistir a decisão de encerramento
da M9. As demais milestones continuam descritas no [ROADMAP](docs/ROADMAP.md);
esta decisão não altera o status delas.

## M9 — Compatibilidade: COMPLETE

**Decisão: M9-DEC-2026-10-11**, explícita do usuário em 11/10/2026.
Alvo de compatibilidade: **Windows 7–11, x86/x64**.

| Ambiente/escopo | Estado registrado |
|---|---|
| Windows 11 | Funcionamento confirmado pelo usuário; arquitetura não especificada |
| Windows 7 | Testes práticos pendentes |
| Windows 8 | Testes práticos pendentes |
| Windows 8.1 | Testes práticos pendentes |
| Windows 10 | Testes práticos pendentes |
| x86/x64 | Validação específica por arquitetura pendente |

O usuário realizará os testes restantes futuramente, sem bloquear o avanço
do projeto e sem reabrir automaticamente a M9. COMPLETE expressa encerramento
por decisão, não certificação dos ambientes ainda não testados. Build ou
análise estática não substituem confirmação prática por sistema/arquitetura.

Nenhuma alteração de código ou execução de testes nesta tarefa documental.
Origem, condições e escopo: [registro da decisão](docs/M9-CLOSEOUT-2026-10-11.md).
Plano técnico preservado: [M9](docs/milestones/M9.md). Relatórios históricos
mantêm o estado da época; este registro prevalece para o status atual da M9.

[Auditoria integral restante da M8 e plano de conclusão](docs/M8-REMAINING-REQUIREMENTS-AUDIT.md):
T1–T8 COMPLETE; aceite humano final PASS. Consolidação T6–T8 publicada em
`1aa560f`; resultados e baselines históricos preservados.
