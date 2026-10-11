# Auditoria integrada M6 — 11/10/2026

> Registro histórico anterior ao encerramento definitivo. O estado vigente é
> **M6 COMPLETE**, por aceite final informado pelo usuário em 11/10/2026.
> Ver [encerramento e nova validação](M6-CLOSEOUT-2026-10-11.md).
> Os resultados, hashes e limitações abaixo preservam a auditoria original.

**M6: READY_FOR_HUMAN_ACCEPTANCE.** O equalizador real está matematicamente
validado nas condições abaixo e integrado ao grafo de reprodução, aos painéis,
à persistência e aos visualizadores. As quatro variantes passam com as
correções locais. Não se declara aprovação humana nova, qualidade audível
universal, saída física sem underruns ou ausência universal de races/vazamentos.

O COMPLETE e o “Funcionou” da entrega anterior permanecem como histórico.
Esta auditoria encontrou e corrigiu problemas de encaixe e de timer do LCD;
o aceite humano desses ajustes e as verificações explicitamente pendentes
abaixo ainda não foram realizados. Não houve commit, push ou captura de tela.

## Repositório e baseline

- Branch `master`; HEAD **a35c993ed89fc021cf8730f13bddaf7a1273a6eb**.
- Working tree inicial limpa; referência local `origin/master` igual ao HEAD.
  `git ls-remote --heads origin master` confirmou o mesmo hash no GitHub.
  A primeira tentativa no sandbox falhou por credenciais Schannel; a leitura
  autorizada fora do sandbox passou. Nenhuma operação de escrita remota.
- `0497b82` é o commit anterior. O HEAD acrescenta cache de duração MP3 e
  regressões para evitar consulta concorrente ao decoder em reprodução.
  A auditoria testa o HEAD efetivamente publicado, sem reverter essa correção.
- Executados `git status`, `git log -5 --oneline`, `git rev-parse HEAD`,
  `git branch -vv` e `git fsck --no-reflogs --connectivity-only`.
  Sem erro de conectividade; objetos dangling existentes não foram removidos.
- Lidos ROADMAP, DELIVERY, DEVELOPMENT, GUI-AUTOMATION, THEMES, M6, relatório
  anterior M6, PROJECT_STATE, M7, Makefile, DSP/estado/áudio/EQ, encaixe/LCD,
  persistência/temas e testes relevantes. Os links locais dos seis documentos
  principais foram conferidos; não faltam os arquivos referenciados.
  A conferência ampliada ao README encontrou o link preexistente para
  `LICENSE`, cujo arquivo não existe no HEAD. Pendência documental fora da
  M6; não foi criada nem escolhida uma licença nesta auditoria.
- A M6 estava formalmente COMPLETE com aceite humano anterior. M0–M5
  concluídas e M9 encerrada administrativamente foram preservadas.
- Antes de alterar fontes/testes versionados: compilação x64 Release e
  **19 executáveis, 97 casos aprovados**. Três executáveis com PrintWindow
  foram adiados, totalizando 14 casos; não foram executados capturando imagens.
- Após adicionar somente o opt-out de captura, antes das correções de produção:
  **22 executáveis, 111 casos funcionais aprovados** em x64 Release.
  As verificações de imagens permaneceram pendentes.

Ambiente: Windows build **10.0.26200.0**, AMD Ryzen 9 5950X, 32 processadores
lógicos, GCC/G++ **16.2.0 Rev5 MSYS2**, MINGW64/MINGW32 MSVCRT.
Área útil observada: **2560 × 1392**, monitor 2560 × 1440; testes no desktop
isolado do executor. CIM/WMI recusou leitura; versão obtida do runtime e
identificação da CPU do registro. Não certifica outros sistemas/monitores.

## DSP: arquitetura, matemática e taxas

Fluxo inspecionado na unidade de produção:

`decoder/resource manager → sound (conversão/pan) → EQ → tap FFT/OSC → endpoint/volume master → dispositivo`.

O nó EQ existe desde a inicialização do engine e independe da criação ou da
visibilidade da janela EQ. `InitEq` usa canais e taxa **do engine**, e não a
taxa nativa mostrada no LCD. Resource manager/conversor ajusta a fonte antes
do DSP; cursor/seek usam a taxa da data source. A cobertura integrada inclui
fonte WAV 44,1 kHz em engines 32/44,1/48/96 kHz.

Dez filtros peaking de segunda ordem, em cascata na ordem:
**60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000 e 16000 Hz**.
Ganhos de cada filtro e PRE em ±12 dB; PRE precede a cascata e volume master
permanece independente depois dela. Coeficientes normalizados e estados
DF-II transpostos em double; entrada/saída float intercalada. Estados
separados por canal, suporte 1–32 canais; engine acima de 32 é recusado.

BW = uma oitava com compensação bilinear; o Q digital equivalente depende
da frequência/taxa, tendendo a aproximadamente 1,414 nas frequências baixas.
As equações de produção foram confrontadas com o
[Audio EQ Cookbook W3C/RBJ](https://www.w3.org/TR/audio-eq-cookbook/).
O contrato do grafo foi consultado no
[manual oficial miniaudio](https://miniaud.io/docs/manual/index.html);
a implementação local 0.11.25 é a referência executada.

Uma banda só é ativa abaixo de `0,475 × Fs`. Em 32 kHz, 16 kHz é N/A,
sem deslocar silenciosamente sua frequência e sem perder seu ganho salvo;
as outras nove bandas funcionam. Nas demais três taxas, as dez são ativas.
Coeficientes não finitos/instáveis são rejeitados. Parâmetros não finitos
viram zero e finitos extremos são limitados à faixa. Estados muito pequenos
são zerados; a guarda de valores patológicos >1e12 não é um limitador musical.

OFF ignora PRE e filtros. Flat ON e bypass OFF estabilizados preservam
bit a bit o sinal float finito normal. Durante uma alteração, dois bancos
estáveis são misturados por crossfade linear de **20 ms**; não se interpolam
coeficientes IIR. Alterações rápidas coalescem até o snapshot mais recente.
Pause preserva estado; stop/seek/unload/troca de faixa pedem reset consumido
na thread DSP. Inicialização em nova taxa prepara o banco nessa taxa.

### Oráculos e tolerâncias definidas nos testes

| Verificação | Oráculo / tolerância |
|---|---|
| Centros de todas as bandas disponíveis, ±6/±12 dB e quatro taxas | Seno real processado: erro <0,08 dB |
| Centros medidos por impulso de dois segundos | Transformada de Fourier das amostras de saída: erro <0,02 dB |
| BW a 1 kHz / 96 kHz | Meia amplitude de ganho nos pontos de meia oitava: erro <0,04 dB |
| Cascata com todas +12, todas −12 e combinação mista | Soma dos ganhos medidos dos filtros individuais: erro <0,05 dB |
| Cascata: impulso versus seno | Concordância <0,05 dB em 60/1000/8000/14000 Hz, quatro taxas |
| PRE −12/−6/0/+6/+12 dB | Medida senoidal: erro <0,00001 dB |
| Flat e bypass estabilizados | `memcmp`, identidade bit a bit |
| Grafo real por codec/taxa, ganho 1 kHz ±6 | Erro <0,1 dB; razão Flat/dry com erro <1e-6 |
| Transição PRE sobre DC | Incremento absoluto <0,00004 por amostra |
| Transições rápidas sobre seno 1 kHz, entrada 0,001 | Passo máximo <0,0006; referência de seno estável máximo ~0,000521 |

Os novos testes usam `Eq::Processor` real. Não contêm cópia das fórmulas dos
coeficientes: medem seno/impulso e compõem respostas de filtros individualmente
medidos. A varredura logarítmica existente verifica finitude/limites nos
extremos e quatro taxas; não é apresentada como resposta calibrada de sweep.
O teste de cascata verifica a interação entre bandas, sem exigir que cada
slider seja um ponto independente da resposta total.

Foram medidos impulsos em ±6/±12 dB, senoides, sweeps, combinações simultâneas,
PRE, partição de blocos/in-place, reset e isolamento do canal 31 em 32 canais.
Transições rápidas: passo máximo observado **0,000519634**, finito, com
retorno final ao bypass bit-exato. O teste parte de um banco com ganho real
para não aprovar uma sequência que apenas permaneça em bypass por coalescência.
Isso é evidência numérica, não aprovação auditiva de ausência de estalos.

### Clipping

Não há compressor, limitador ou ganho automático. Uma entrada 0,5 com PRE
+12 dB produz aproximadamente 1,991: o float mantém headroom acima de ±1.
A saída física/conversão final pode saturar. Em 96 kHz, todas as bandas
+12 dB dão **33,95786 dB em 14 kHz**, PRE zero; com PRE +12 seriam cerca de
45,96 dB. Todas −12 produzem −33,95786 dB nessa mesma frequência.
Portanto, até PRE −12 pode ser insuficiente para boosts extremos simultâneos.
O aviso da interface é pertinente; presets atenuados não garantem ausência
de clipping para qualquer conteúdo. Nenhum mecanismo automático foi adicionado.

## Callback e concorrência

`EqProcess → Processor::Process → Sample` não aloca, não faz trigonometria,
pow, disco, pintura, chamadas Win32, mutex ou espera. `Prepare` fica no produtor
UI ou na inicialização. Memória dos dois bancos e três snapshots é fixa.
Reset usa contador atômico; coeficientes/estado de áudio não são escritos pela UI.

Mailbox de três slots: um pertence ao produtor, outro ao consumidor e o
terceiro é trocado atomicamente com acquire/release. O produtor único escreve
seu slot antes de publicá-lo; o consumidor único troca a propriedade antes
de ler. Isso evita leitura parcialmente atualizada e reutilização simultânea
do slot. `static_assert` exige atomics unsigned lock-free nas duas arquiteturas.
O contrato é **um produtor/um consumidor**; não autoriza produtores múltiplos.
Inicialização/shutdown são coordenados pelo ciclo de vida do engine.

Stress existente: 20.000 publicações contra 20.000 blocos concorrentes, com
resets e parâmetros variados, sem deadlock/não-finitos observados nas quatro
variantes. Sem ThreadSanitizer: stress mais análise de propriedade não é prova
universal de ausência de races.

O tap herdado usa `TryEnterCriticalSection`, descartando apenas a captura de
amostras do visualizador quando ocupado. O teste real de contenção passou;
não espera pelo leitor UI. Locks/alocações/decodificação pertencentes ao engine
não são atribuídos ao EQ, nem se promete que toda a dependência seja lock-free.
A duração MP3 usa cache UI e sondagem independente anterior à reprodução;
os testes de consultas concorrentes de duração e EOF continuam passando.

## Integração e estados exercitados

| Área | Evidência atual |
|---|---|
| WAV mono/estéreo, MP3, FLAC, OGG Vorbis | Grafo real nas quatro taxas, OFF/Flat/+6/−6; tap com energia válida |
| Transporte | Play/pause/resume/stop/seek, EOF, repeat, troca de faixa/mono e arquivos inválidos |
| Parâmetros em reprodução | Presets/toggle/PRE rápidos, sliders por mensagens Win32, áudio efetivamente alterado |
| Volume/pan | Volume ×0,25 medido; canal direito silenciado com pan esquerdo; EQ preservado |
| Persistência EQ | ON/OFF, dez ganhos, PRE, preset/Custom, seção Appearance preservada, fallback inválido/parcial |
| Restauração sem EQ | INI recarregado antes do engine, engine 96 kHz processa sem criar janela EQ |
| Player sozinho / painéis ocultos | Fechar playlist/EQ mantém PLAYING, cursor avança, DSP ON; teste de painel oculto mantém ganho |
| Aparência | Tema, escala, FFT/OSC, VIZ, playlist/EQ e reabertura compacta por recriação real das janelas |
| Encaixe | Quatro bordas, três âncoras, tolerância/desprendimento, movimento/escala, borda de monitor e conflito de dois painéis |
| LCD | Curto, largura exata medida por DirectWrite, longo, acentos/Unicode, mudança de título, pausa, tema, escala, ocultação/minimização |
| Temas | 36 paletas/contrastes, seleção/INI/fallback, estado de áudio/playlist/DSP preservado; 1.980 alternâncias por variante |

O cenário EQ preset → bandas manuais → PRE → salvar/recarregar está coberto
por INI isolado e engine sem painel; fechar/reabrir/destruir o painel e
recriar as janelas têm testes complementares. Não se afirma que esse conjunto
seja uma execução humana ponta a ponta do executável no desktop pessoal.

Também foram feitas duas sessões do executável real Release em perfil
isolado, com encerramento normal (`WM_CLOSE`, código 0) e conservação dos
dez ganhos Custom, PRE −2 e EQ oculto. Duas sessões Debug sob GDB confirmaram
os valores restaurados, DSP ativo a 48 kHz sem painel EQ, visualizador oculto,
tema Original/escala 100 e saída normal. Os testes de estado exercitam a
sequência preset → ajustes manuais → PRE; as sessões do aplicativo verificam
a persistência desse perfil no ciclo de vida real, sem simular aceite humano.
A tentativa inicial com `Start-Process -WindowStyle Hidden` saiu com código 1,
sem alterar o INI. Corrigido o harness para criação direta do processo, as
duas execuções passaram; a tentativa foi descartada como validação, sem
atribuir sua causa ao produto. O breakpoint exploratório de inicialização
também não conta como sessão encerrada normalmente.

A rolagem é recortada no LCD e usa layout DirectWrite cacheado. Mudança de
título reinicia elapsed; troca de tema libera o layout/fontes, escala refaz
geometria. Pause mantém a animação visível, sem mexer no transporte. O teste
de largura exata e Unicode usa Canvas sintético, não captura de janela.
Suavidade percebida/glifos efetivamente visíveis ainda exigem inspeção humana.

O encaixe auditado continua **painéis → player**. Não foi implementado snap
EQ ↔ playlist nem propagação transitiva, ambos escopo aberto da M7.
Quando há espaço, uma borda alternativa evita sair da área útil/colidir com
o outro painel. Se nenhuma borda comporta o painel, ele fica acessível e
desacoplado; telas menores que o painel podem exigir escala menor e podem
ter sobreposição inevitável. Movimento físico entre monitores/DPI não testado.

## Defeitos reproduzidos e correções

| ID / teste | Esperado / observado antes | Causa / menor correção | Reexecução |
|---|---|---|---|
| M6-AUD-01 `docked_panel_remains_reachable_at_monitor_edge` | Painel dentro da área útil / painel além da borda direita, FAIL | Follow aplicava âncora sem considerar monitor; `PlaceVisible` procura borda que caiba, senão mantém acesso e desacopla | PASS, quatro variantes; regressão playlist e coordenadas negativas/oversize |
| M6-AUD-02 `simultaneous_panels_hide_without_stopping_audio` | EQ/playlist sem interseção / ambos na borda inferior se sobrepunham, FAIL | Cada painel ignorava o outro; consulta do painel visível do mesmo owner/thread e escolha de borda livre, inclusive ao reabrir | PASS, quatro variantes, inclusive reabertura e ocultação com cursor/áudio ativos |
| M6-AUD-03 `lcd_timer_only_runs_for_visible_overflow` | Sem timer 30 ms quando título cabe ou janela oculta/minimizada / timer sempre criado | `SetTimer` incondicional em WM_CREATE; ativação conforme overflow/visibilidade, desligamento ao ocultar/minimizar | PASS por contagem de mensagens, quatro variantes |

As duas primeiras falhas têm logs pré-correção conservados. A terceira foi
demonstrada pela chamada incondicional no código e recebeu regressão por
mensagens reais. O teste antigo de docking com painel esquerdo fora da tela
teve o owner movido de x=500 para x=800: suas assertivas de alinhamento foram
preservadas; a condição fora da tela agora pertence à regressão específica.
Não se aumentou tolerância para esconder falha nem se alterou DSP/engine/codecs.

## Testes, builds e pendências

Builds forçados das quatro variantes, seguidos de build/test incrementais
após os últimos ajustes, validam todas as fontes finais. Não se usa executável
histórico como validação do HEAD atual.

| Variante | Build | Executáveis de teste | Casos funcionais | Falhas finais | Grupos de captura pendentes |
|---|---|---:|---:|---:|---:|
| x64 Release | PASS | 23 | 122 PASS | 0 | 3 |
| x64 Debug | PASS | 23 | 122 PASS | 0 | 3 |
| x86 Release | PASS | 23 | 122 PASS | 0 | 3 |
| x86 Debug | PASS | 23 | 122 PASS | 0 | 3 |

Total **488 casos funcionais aprovados**, mais **27 guardas Python aprovadas**:
**515** execuções aprovadas na rodada final. Existem **12 grupos de assertivas
de captura/pixels de janelas não executados**, três por variante, identificados
por `PENDING` no log. Eles não contam como aprovados. Dois testes de defeito
falharam intencionalmente antes da correção; não são falhas finais.
Submedições/loops/CHECKs dentro de cada caso não inflacionam esse total.

Acrescentados 11 casos: seis DSP/estado em `test_eq_audit`, dois EQ Win32,
um playlist Win32, um player/LCD e um encaixe. A matriz de codecs/taxas e
largura exata/Unicode ampliam casos existentes. Preservadas as limitações dos
testes legados que copiam algoritmos de FFT/ring/amplitude; eles não provam
sozinhos o código de produção. As novas verificações DSP/áudio o exercitam.

Warnings relevantes preexistentes: casts FARPROC/DPI, helpers GDI ociosos e
avisos GCC `__atomic_*`/stringop-overflow em miniaudio sob otimização. Sem
dependência nova ou alteração da biblioteca para suprimir avisos. Os logs de
build conservam os avisos. O smoke e a compilação não certificam Windows 7.

## Desempenho

Medições atuais e amostras estão no pacote de evidências indicado abaixo.
O probe vincula o grafo e as janelas de produção com adaptador **offline**,
sem dispositivo. Ele usa pintura normal RedrawWindow/WM_PAINT, nunca PrintWindow.
Os números não são a medição de um executável histórico nem de saída física.
Não se compara o renderer com GDI nem se atribui toda a memória gráfica ao EQ.
As medições do probe precedem o último ajuste de reposicionamento ao reabrir
a playlist; os builds e as suítes finais incluem esse ajuste. Os hashes dos
probes medidos e dos executáveis finais são registrados separadamente.

Três rodadas por arquitetura Release, 100 blocos de aquecimento e 2.000
blocos medidos por cenário, 512 frames estéreo/48 kHz. Tabela: medianas das
três médias e dos três percentis 99, não percentil de amostras agregadas.
O orçamento por bloco é 10.666,7 µs. O tempo cronometrado é a leitura do
grafo completo, incluindo decoder/conversão/tap, e não somente a função EQ.

| Cenário (modo) | x64 média / p99 µs | x86 média / p99 µs | x64 WS / privada MiB |
|---|---:|---:|---:|
| Mesmo grafo sem nó EQ (6) | 11,005 / 20,2 | 33,631 / 48,7 | 63,75 / 61,46 |
| Repouso sem faixa (7) | 1,740 / 3,3 | 3,092 / 4,7 | 63,25 / 60,81 |
| EQ OFF (0) | 12,004 / 22,8 | 35,372 / 51,9 | 63,74 / 60,71 |
| EQ Flat (1) | 14,180 / 22,9 | 36,676 / 53,4 | 63,65 / 61,64 |
| EQ Rock (2) | 36,656 / 59,0 | 127,211 / 162,1 | 63,86 / 60,78 |
| FFT (3) | 36,288 / 58,5 | 128,614 / 164,5 | 70,14 / 76,41 |
| OSC (4) | 37,039 / 57,1 | 130,721 / 170,5 | 58,71 / 74,50 |
| Playlist aberta (8) | 37,607 / 78,8 | 128,226 / 166,6 | 59,62 / 74,88 |
| Playlist + EQ (5) | 37,814 / 72,2 | 129,766 / 170,3 | 73,49 / 106,96 |
| LCD longo (9) | 36,263 / 56,5 | 129,449 / 168,9 | 53,02 / 59,59 |
| Alternância de temas/presets, LCD/OSC/painéis (10) | 26,963 / 60,1 | 88,594 / 192,9 | 67,25 / 92,00 |

Rock usa aproximadamente **0,34% do orçamento x64 / 1,19% x86**. A diferença
descritiva Rock menos grafo sem EQ é ~25,65/~93,58 µs; as rodadas separadas
não constituem um benchmark pareado. O modo 10 alterna presets e bypass,
logo sua média menor não indica otimização de DSP. Modos 3–5 incluem também
desenho de visualizador em Canvas sintético a cada quatro blocos; modo 10
mantém OSC, sem alternar para FFT. Não são cargas gráficas idênticas.

Uma rodada x64 de cinco segundos por cenário, com bombeamento de mensagens
e Sleep(10), registra CPU total do processo, sem normalizar pelos 32
processadores lógicos. 100% significa um núcleo ocupado. Pico é o maior
intervalo observado de aproximadamente um segundo; startup/cauda podem
ficar fora desses intervalos, por isso pico e média não são comparadores
estritos em uma execução tão curta. A variação entre processos e inicialização
gráfica exige cautela; números não servem como limite de consumo do produto.

| Cenário | CPU média % / pico % de um núcleo | Threads finais |
|---|---:|---:|
| Repouso offline | 4,07 / 6,25 | 51 |
| OFF | 0,31 / 1,56 | 51 |
| Flat | 2,50 / 4,69 | 51 |
| Rock | 4,38 / 15,63 | 51 |
| FFT | 7,49 / 18,75 | 86 |
| OSC | 2,81 / 1,56 | 79 |
| Playlist | 4,06 / 4,69 | 80 |
| Playlist + EQ | 12,79 / 31,25 | 150 |
| LCD | 10,64 / 35,94 | 44 |
| Temas/presets | 11,52 / 18,75 | 115 |

O **executável real x64 Release**, em perfil isolado e sem música, foi
amostrado por 30 segundos. Entre segundos 5 e 30: CPU média **0,50%** de um
núcleo, pico de um segundo **3,125%**; memória privada 60,43 → 60,35 MiB,
WS 64,52 → 64,27 MiB; threads 52 → 50, GDI 50 → 50, USER 50 → 49.
Encerramento normal código 0. Isso mede repouso do aplicativo; os demais
cenários acima medem o harness com grafo/janelas reais, sem dispositivo.

Ensaio prolongado disponível: **300,009 segundos**, modo 10, x64 Release,
12.334 blocos após aquecimento. Média do grafo 57,389 µs, p99 107,9 µs,
máximo 291,3 µs; média 0,538% do orçamento. CPU total 43.234 ms, ou
14,41% de um núcleo, pico de um segundo 37,5%. GDI **12** e USER **14**
permaneceram constantes nas amostras. Threads 114 aos 30 s, 109 aos 270 s,
110 ao final. Memória privada 92,82 → 93,59 MiB entre 30 s e final
(+811.008 bytes, ~0,773 MiB); WS 68,34 → 69,35 MiB. A memória privada
teve platô nas amostras de 240/270 s, sem crescimento de GDI/USER.

O probe acumula tempos em um vetor que cresce durante o ensaio, além dos
caches gráficos. Isso explica parte possível do crescimento, mas não
atribui os bytes individualmente: não houve profiling de heap/stacks nem
prova de ausência de vazamento. Não se propõe otimização por esse resultado.
Recursos D2D/GPU não foram contabilizados individualmente. A duração foi
reduzida a cinco minutos neste desktop isolado; a sessão física preferida
de 60 minutos permanece pendente. Não houve medição de underruns do dispositivo.

## Verificações humanas ainda necessárias

1. Confirmar os novos ajustes de encaixe na área útil e com três janelas,
   inclusive arraste físico/desprendimento e monitores/DPI distintos.
2. Inspecionar LCD/Unicode, rolagem e sliders/indicadores/contraste visual
   dos temas na janela real. As assertivas de imagens desta rodada são pendentes;
   capturas só poderão ser feitas com autorização explícita posterior.
3. Ouvir transições, presets/PRE/bypass e reprodução física nas condições
   de uso; confirmar ausência de estalos percebidos/underruns. O aceite
   auditivo anterior não foi substituído por uma alegação de aprovação nova.
4. Completar sessão física de aproximadamente 60 minutos e, se necessário,
   investigar CPU/picos e multicanal físico. A carga offline reduzida desta
   auditoria não substitui esse ensaio. Windows 7–10 permanece no registro
   de pendências não bloqueantes da M9, sem reabri-la.

Não resta defeito reproduzido aberto nas condições automatizadas deste ciclo.
As limitações acima impedem declarar a nova rodada COMPLETE ou “plenamente
integrado em qualquer ambiente”; justificam READY_FOR_HUMAN_ACCEPTANCE.

## Arquivos, evidências e estado final

Produção alterada somente em `src/window_snap.h`, `src/eqwnd.cpp`,
`src/plwnd.cpp` e `src/mainwnd.cpp` (encaixe/timer). DSP, estado EQ, motor,
miniaudio e formatos não foram modificados. Diagnóstico: `eq_probe.cpp`.
Testes: `check.h`, `test_eq_audit.cpp` novo, `test_eq_audio.cpp`,
`test_eq_window.cpp`, `test_player_window.cpp`, `test_playlist_window.cpp`,
`test_themes.cpp`, `test_window_snap.cpp`, `test_ui_draw.cpp`.
Documentação atualizada: README, PROJECT_STATE, ROADMAP, M6, DEVELOPMENT,
GUI-AUTOMATION, THEMES e este relatório.

[Pacote textual de evidências](evidence/m6-integrated/manifest.json) contém
hashes, logs de build/test, falhas reproduzidas, guardas, medições/amostras e
estatísticas. Nenhuma imagem foi criada ou copiada para o pacote. Os artefatos
temporários completos e scripts de execução estão em `build/audit-m6/`.

Working tree final contém as alterações locais desta auditoria e evidências;
HEAD/branch permanecem `a35c993`/`master`. Nenhum commit/push, reset/clean Git,
instalação de dependências, escrita no perfil/playlist pessoal ou operação
em outra instância do usuário. INIs/playlist/fixtures dos testes usam
build/temporários isolados; não foi necessário copiar o perfil pessoal.

## Próxima milestone funcional

**M7 — Integração das janelas**, depende de M6, não implementada neste ciclo.
Requisitos ainda abertos: reutilizar base comum somente onde há duplicação
real; snap entre quaisquer pares; relações de docking/propagação transitiva;
testes do grafo de movimento; finalizar botões/posicionamento padrão e
checkpoint manual das três janelas. A integração pontual existente e as
correções de regressão acima não encerram esses requisitos.
M0–M5 seguem concluídas; M9 continua COMPLETE por decisão explícita do usuário.
