# Auditoria geral de desempenho — pré-distribuição (12/10/2026)

**Estado: COMPLETE_WITH_CAVEATS** — encerramento formal por decisão expressa
do usuário (**PERF-DEC-2026-10-12**, "LiteSic — Encerramento formal da
Auditoria Geral de Desempenho"), que aprovou as três correções e os resultados
apresentados. O status vale exclusivamente para o escopo efetivamente
validado; os itens da seção 16 não executados são observações e validações
futuras, **não** aprovadas. Neste encerramento não foram repetidos
benchmarks, testes prolongados, auditorias nem capturas de tela.
M10 — Distribuição **não iniciada**; será autorizada separadamente.

Referência auditada: `9b2689b310130df80cb2b268c19241b5b6d7c810`. As builds de
referência foram reconstruídas a partir de `git archive 9b2689b` e são
**idênticas byte a byte** aos executáveis preservados (SHA-256 x64
`74a38ebd…1082`, x86 `cb441aad…c651`), o que torna as comparações antes/depois
reprodutíveis.

## 1. Resumo

| # | Achado | Classe | Situação | Antes → depois (medido) |
|---|---|---|---|---|
| 1 | Cada janela criava sua própria fábrica Direct2D e, com ela, um dispositivo Direct3D próprio | **Alto** (memória/threads anormais) | Corrigido | 3 janelas: privados* 92,1 → 59,8 MiB; WS 63,2 → 49,5 MiB; threads 115 → 45; handles 823 → 524 |
| 2 | Arquivo de áudio codificado inteiro lido para a memória, na thread da UI | **Crítico** no x86 (WAV de 1,31 GB não carrega); **Alto** no x64 (+502 MiB num FLAC de 60 min) | Corrigido para arquivos > 64 MiB | FLAC 60 min: +501,9 → +0,1 MiB, carga 130–143 → 2,7 ms; WAV 1,31 GB no x86: falha → reproduz (+2 MiB) |
| 3 | Título longo: temporizador de rolagem (30 ms) e do visualizador (33 ms) disparavam repinturas completas independentes | Médio (renderização desnecessária) | Corrigido | CPU tocando com osciloscópio e título longo: 9,23% → 4,67% de um núcleo |
| — | DSP do equalizador | — | Sem defeito | 10 bandas: 23 µs por bloco de 10 ms (0,23% do orçamento) |
| — | Independência áudio × UI | — | Sem defeito | 0 quedas em 10 rodadas reais de 180 s, inclusive com CPU 100% saturada |
| — | Vazamentos progressivos | — | Não observados | 34,8 min de reprodução contínua x64/x86 (seção 11) |

\* "Privados*" = memória privada (commit) do processo **menos** o commit de
imagens de DLLs de terceiros injetadas (seção 2.3). Working set (WS) não é
afetado por essa correção.

## 2. Metodologia

### 2.1 Ambiente

| Item | Valor |
|---|---|
| CPU | AMD Ryzen 9 5950X, 16 núcleos / 32 threads, plano "Alto desempenho" |
| RAM | 127,9 GiB |
| GPU | NVIDIA RTX 2000 Ada (vídeo) + RTX 4000 Ada + RTX 2000 Ada; driver 32.0.15.9686 |
| Tela | 3840×2160 a 60 Hz, 150% (144 DPI) |
| SO | Windows 11 Pro 10.0.26200, sessão de console |
| Disco do repositório e fixtures | NVMe Kingston SNV3S1000G |
| Toolchain | MSYS2 MINGW64/MINGW32, GCC 16.2.0 (Rev5), MSVCRT |
| Áudio | Dispositivo padrão do sistema via WASAPI (miniaudio 0.11.25) |

Condições concorrentes registradas durante a sessão (não controladas pela
auditoria): o computador estava em uso pelo usuário, havia um `llama-server`
(LM Studio) usando as GPUs a 43–47% e o RivaTuner Statistics Server injetava
`RTSSHooks64.dll` nos processos com interface. Isso aumenta o ruído das
medições de CPU (±1 ponto percentual entre rodadas) e afetou testes por captura
de janela (seção 15).

### 2.2 Instrumentos (todos reproduzíveis, em `scripts/diagnostics/`)

| Ferramenta | O que mede | Tipo |
|---|---|---|
| `perf_session.py` | Executável real: inicialização, CPU/memória por cenário, sessão prolongada, continuidade de áudio | **Teste real**, processo externo |
| `loopback_probe.cpp` | Saída real do dispositivo via loopback WASAPI; detecta janelas de 2 ms com RMS < 50% da mediana | **Teste real** |
| `perf_probe.cpp` (`make perf-probe`) | Objetos de produção vinculados: DSP, grafo offline, pintura, recursos D2D, playlist, temas, idiomas, janelas, tooltips, estresse, estágios de memória | Real em processo; o "dispositivo" de áudio é **simulado** quando indicado |
| `perf_fixtures.py` | Gera ~1 GB de áudio sintético isolado em `build/perf/fixtures` | Dados sintéticos |
| `perf_summary.py` | Tabelas a partir de `build/perf/results` | Análise |

Isolamento: todas as execuções do aplicativo usam `--profile=build/perf/profiles/<cenário>`
(ou `ThemeConfigure(nullptr)` nas sondas). Nenhum perfil, playlist,
configuração ou arquivo pessoal foi lido ou alterado. Controle externo apenas
por mensagens do próprio player (`WM_APP+97`, `WM_CLOSE`, `WM_COPYDATA`).

Fixtures: faixas "musicais" sintéticas de 4 min (acorde com trêmolo + ruído
rosa) em WAV, MP3 192k, FLAC 44,1/48/96 kHz-24 bits e OGG q5; MP3 e FLAC de
60 min (82 e 501 MiB); WAV de 130 min (1,31 GB); senoides de 1 kHz de 15 e
30 min para continuidade; 10.000 MP3 de 1 s com tags ID3v2.4 únicas.

Repetições: inicialização 10 processos por build (mediana); recursos D2D 3
processos (mediana); cenários 60 s cada; DSP 2.800 blocos por configuração;
continuidade 180 s por rodada; sessão prolongada 34,8 min (interrompida a pedido).

### 2.3 Correção de memória privada (RTSS)

`CoInitializeEx` faz o RivaTuner carregar `RTSSHooks64.dll`, cuja seção
copy-on-write de **51,7 MiB** é cobrada como memória privada, embora quase não
seja residente (+2 MiB de WS). Isso é específico deste computador e aparece em
qualquer processo com interface. As ferramentas medem esse commit
(`foreign_image_commit`) e as tabelas apresentam "privados*" sem ele. A sonda
`canvas` não inicializa COM e, por isso, mede o Direct2D sem a injeção.

## 3. Baseline (referência `9b2689b`)

| Métrica (mediana) | x64 | x86 |
|---|---:|---:|
| Janela visível | 0,126 s | 0,224 s |
| Interface responde (`WM_NULL`) | 0,428 s | 0,606 s |
| WS em repouso (player + playlist) | 58,9 MiB | 50,2 MiB |
| Privados* em repouso | 75,5 MiB | 58,9 MiB |
| Threads em repouso | 81 | 78 |
| Handles / GDI / USER | 674 / 16 / 20 | 546 / 16 / 20 |
| CPU em repouso | 0,26–0,39% de um núcleo | 0,23–0,34% |
| CPU tocando, osciloscópio | 4,96% | 6,77% |
| CPU tocando, visualizador oculto | 1,05% | 2,20% |
| Com janela do EQ aberta | 113 threads, 98,3 MiB privados* | 109 threads, 73,1 MiB |
| Fechamento | 0,08 s | 0,12 s |

Composição do tempo de inicialização (x64, sonda `startup-parts`, 5 rodadas):
`CoInitializeEx` 14 ms; tema/idioma 6 ms; **primeiro quadro Direct2D 184–384 ms**
(criação do dispositivo D3D pelo driver); quadros seguintes ~2 ms; dispositivo
de áudio WASAPI 53–62 ms.

## 4. Investigação da memória: Direct2D × GDI

A medição anterior (M5) registrou WS de 58,67 MiB com Direct2D e 15,69 MiB com
GDI. Decomposição atual por estágio da inicialização (x64, mediana de 3
processos, privados* em MiB / WS em MiB / threads):

| Estágio | Referência | Após correções |
|---|---|---|
| Processo | 1,8 / 8,9 / 4 | 1,8 / 8,9 / 4 |
| `CoInitializeEx` (STA) | 2,2 / 10,4 / 4 | 2,2 / 10,4 / 4 |
| Tema + idioma | 2,3 / 10,9 / 4 | 2,3 / 10,7 / 4 |
| Janela do player + áudio + 1º dispositivo D2D | 56,9 / 48,1 / 44 | 56,7 / 48,1 / 44 |
| + janela da playlist | 75,5 / 56,1 / 80 | 59,3 / 49,1 / 45 |
| + janela do EQ | 92,1 / 63,2 / 115 | 59,8 / 49,5 / 45 |
| Tocando há 1 s | 97,8 / 69,1 / 115 | 65,5 / 55,5 / 45 |
| Janelas destruídas | 45,9 / 36,7 / 7 | 24,7 / 32,1 / 7 |

Custo por `Canvas`, isolado (sonda `canvas`, sem COM e sem RTSS, mediana de 3):

| Canvases | Referência: privados / WS / threads / handles | Fábrica compartilhada | Render target software |
|---:|---|---|---|
| 1 | 52,6 / 42,1 / 40 / 420 | 53,1 / 42,2 / 40 / 420 | 4,7 / 21,2 / 10 / 215 |
| 3 | 85,3 / 55,8 / 110 / 724 | 53,2 / 42,3 / 40 / 422 | 8,1 / 25,4 / 10 / 267 |
| 6 | 134,1 / 75,4 / 215 / 1180 | 53,7 / 42,6 / 40 / 425 | 13,0 / 31,6 / 10 / 346 |

Conclusões:

1. **Custo legítimo de inicialização:** o primeiro render target por hardware
   cria um dispositivo Direct3D com o driver NVIDIA: cerca de +51 MiB privados,
   +33 MiB WS e ~36 threads do driver.
   O áudio e as janelas somam ~3 MiB. É o preço da aceleração por GPU, não um
   vazamento; permanece estável em todos os ciclos medidos.
2. **Alocação permanente indevida (corrigida):** cada `Canvas` criava sua
   fábrica, e o Direct2D 1.0 não compartilha dispositivos entre fábricas. Cada
   janela adicional custava +16–18 MiB, +150 handles e +35 threads. Tooltips
   também possuem `Canvas` próprio e repetiam o custo ao serem exibidos.
3. **Software não é alternativa melhor:** o render target por software reduz
   a memória, mas custa 3–5× mais CPU por quadro (seção 5) e pode alterar o
   antialiasing aceito visualmente. Mantido hardware.
4. **GDI (15,69 MiB) não é meta.** A diferença restante para GDI é o
   dispositivo D3D do driver. O valor de 58,67 MiB da M5 não pode ser
   reatribuído retroativamente: não se sabe se a injeção do RTSS estava ativa
   naquela sessão (o WS, entretanto, não é afetado por ela).
5. **Caches:** fontes DirectWrite por canvas e layout do título rolando são
   reutilizados entre quadros; trocas de tema/idioma recriam apenas formatos.
   Crescimento observado: +1,9 MiB em 1.800 trocas de idioma, estável após o
   ciclo 60 (cache de fontes de 18 scripts), sem tendência posterior.

## 5. Recursos gráficos e frequência de redesenho

Pintura das janelas reais (sonda `paint`, x64, 300 quadros por caso, 150%):

| Caso | Referência: ms/quadro · CPU ms/quadro | Fábrica compartilhada | Software |
|---|---|---|---|
| Player sem faixa | 1,69 · 0,99 | 1,40 · 0,89 | 2,69 · 3,59 |
| Playlist vazia | 1,44 · 0,73 | 1,50 · 0,68 | 2,51 · 2,92 |
| Equalizador | 1,49 · 0,99 | 1,48 · 1,04 | 2,44 · 3,70 |
| Osciloscópio (quadro do visualizador) | 2,13 · 1,20 | 1,75 · 1,20 | 3,82 · 6,41 |
| FFT (quadro do visualizador) | 1,82 · 0,83 | 1,41 · 0,99 | 2,57 · 4,27 |

Frequência: posição a cada 250 ms durante reprodução; visualizador a 30 Hz
(33 ms) somente tocando, visível e não minimizado; rolagem do título a 33 Hz
quando o título excede o LCD. A pintura redesenha a janela inteira e o
`EndPaint` copia só a região inválida; o custo dominante é a montagem do
quadro (~1 ms de CPU), não o tamanho da região.

Recursos GDI/USER ficaram constantes (16–19 / 19–24) em todos os cenários e
ciclos. Não houve falha de criação de recurso em nenhuma execução.

## 6. Áudio

### 6.1 Custo do equalizador (sonda `dsp`, x64, estéreo, ruído branco)

| Configuração | 48 kHz, 480 quadros: média / p99 / máx (µs) | ns/quadro | % do orçamento |
|---|---|---:|---:|
| EQ desligado | 1,35 / 2,6 / 6,5 | 2,8 | 0,013% |
| Ligado, plano | 1,57 / 1,7 / 6,2 | 3,3 | 0,016% |
| Uma banda +6 dB | 10,2 / 18,6 / 23,5 | 21,3 | 0,10% |
| Preset Rock (10 bandas) | 23,3 / 38,8 / 54,7 | 48,5 | 0,23% |
| ±12 dB alternados | 23,4 / 32,9 / 123,5 | 48,8 | 0,23% |
| Novo conjunto de coeficientes a cada bloco (crossfade) | 43,0 / 64,1 / 86,2 | 89,7 | 0,43% |

44,1 e 96 kHz e blocos de 1.024 quadros têm o mesmo custo por quadro
(máximo observado 0,87% do orçamento a 96 kHz). O algoritmo não foi alterado:
não há defeito, nem custo relevante.

### 6.2 Grafo completo (decodificação + EQ + tap do visualizador)

Sonda `graph`, x64, motor offline a 48 kHz, blocos de 10 ms. Antes das correções:

| Arquivo | Carga (ms) | Δ privados ao carregar | Bloco, EQ desligado / Rock (µs) |
|---|---:|---:|---|
| WAV 4 min | 10,4 | +40,5 MiB | 9,7 / 32,8 |
| MP3 192k 4 min | 1,7 | +5,5 MiB | 16,3 / 37,6 |
| FLAC 4 min | 8,4 | +33,5 MiB | 15,3 / 37,3 |
| OGG q5 4 min | 11,5 | +3,3 MiB | 18,8 / 40,4 |
| FLAC 96 kHz/24 4 min | 16,4 | +56,3 MiB | 23,3 / 46,7 |
| MP3 60 min | 21,4 | +82,6 MiB | 15,8 / 38,2 |
| FLAC 60 min | 129,6–143,0 | **+501,9 MiB** | 16,4 / 37,5 |
| WAV 130 min (1,31 GB) | 336–365 | **+1.315 MiB** | 9,7 |

O decodificador mantinha o arquivo **codificado** inteiro em memória e a leitura
acontecia na thread da UI dentro de `Audio_Load`. No executável x86 real (não
*large address aware*, 2 GB de espaço), o WAV de 1,31 GB **não carregou**.

Após a correção (seção 13.2), arquivos ≤ 64 MiB seguem idênticos; acima disso,
com leitura no ritmo do dispositivo (`pace` 1×), x64: MP3 60 min 2,7 ms /
+0,8 MiB; FLAC 60 min 2,7 ms / +0,1 MiB; WAV 1,31 GB 1,9 ms / +0,0 MiB; x86:
5–8 ms / ≤ +0,8 MiB. **Zero blocos silenciosos** a 1×, 4× e 8× o tempo real
em MP3, OGG, FLAC 96/24 e WAV de 1,31 GB (folga de decodificação ≥ 8×).

### 6.3 Independência entre áudio e interface

Simulação (sonda `stress`, referência x64): uma thread de prioridade
*time-critical* consome blocos de 480 quadros a cada 10 ms pelo grafo real
(EQ Rock, tap do visualizador), enquanto a UI executa 20 s de cada carga.

| Fase da UI | Iterações | Despacho mais longo da UI | Bloco de áudio média / p99 / máx | Blocos > 10 ms |
|---|---:|---:|---|---:|
| Ociosa | 1.721 | 24,9 ms | 47,6 / 80,7 / 327,5 µs | 0 |
| Visualizador 30 Hz | 506 | 25,6 ms | 47,7 / 82,0 / 368,8 µs | 0 |
| Trocas de tema | 3.303 | 10,4 ms | 52,9 / 156,1 / 287,2 µs | 0 |
| Trocas de idioma | 2.523 | 20,9 ms | 50,5 / 94,3 / 146,0 µs | 0 |
| Importação de 5.000 faixas | 6.119 | 25,2 ms | 48,6 / 83,2 / 126,3 µs | 0 |
| Mover/alternar janelas | 4.191 | 35,3 ms | 51,2 / 92,8 / 281,3 µs | 0 |
| Presets/liga-desliga do EQ | 1.969 | 21,0 ms | 50,9 / 114,7 / 809,6 µs | 0 |
| Busca e ordenação | 1.001 | 25,6 ms | 27,4 / 55,3 / 82,1 µs | 0 |

Análise de código coerente com os números: o callback não aloca, não usa
janelas, usa `TryEnterCriticalSection` no buffer do visualizador (nunca espera
a UI) e recebe coeficientes do EQ por troca atômica de três slots.

### 6.4 Continuidade no dispositivo real

`perf_session.py continuity`: o aplicativo real toca uma senoide de 1 kHz
(EQ Rock, FFT, todas as janelas) enquanto o loopback WASAPI captura a saída por
180 s. A UI recebe 344–346 ações (FFT/OSC, EQ, playlist, visualizador,
minimizar/restaurar, mover) e a importação encaminhada de 10.000 arquivos.
"Saturação" = 32 processos ocupando todos os núcleos durante toda a rodada.

| Rodada | Arquivo | Saturação | Janelas de 2 ms < 50% | Eventos |
|---|---|---:|---:|---:|
| Referência x64 | 38 MB (memória) | 0 / 32 | 0 / 0 | 0 / 0 |
| Referência x86 | 38 MB | 32 | 0 | 0 |
| Corrigido x64 | 38 MB | 0 / 32 | 0 / 0 | 0 / 0 |
| Corrigido x86 | 38 MB | 0 / 32 | 0 / 0 | 0 / 0 |
| Final x64 | 77 MB (**streaming**) | 32 | 0 | 0 |
| Final x86 | 77 MB (**streaming**) | 32 | 0 | 0 |

RMS mínimo igual à mediana em todas as rodadas (nenhuma perda parcial).
Controle positivo: pausas de 300 ms e 50 ms e um seek foram detectados como 3
eventos, comprovando a sensibilidade do detector. Medição de seek (10 seeks de
±5 s, ciclos inteiros da senoide): arquivo em memória 0 ms de silêncio;
arquivo em streaming ~10 ms por seek (um período do dispositivo); referência
com o mesmo arquivo de 77 MB: 0 ms.

## 7. FFT e osciloscópio

FFT radix-2 de 512 pontos, janela Hann e 32 barras log, na thread da UI:
custo incluído no quadro do visualizador (0,83–1,20 ms de CPU por quadro com
hardware). Osciloscópio desenha 511 segmentos por quadro. Ambos a 30 Hz:
CPU do processo tocando 4,2–6,8% de um núcleo, contra 0,7–2,2% com o
visualizador oculto. Fluidez: despacho da UI ≤ 25,6 ms (simulação) e nenhum
quadro bloqueante observado. Sem otimização: o custo é proporcional e
previsível, e reduções exigiriam pintura parcial (recomendação 18.4).

## 8. Playlist

Sonda `playlist`, x64 (referência), importação real de pasta com leitura de
metadados no worker:

| Faixas | Importação até metadados (s) | Varredura (ms) | CPU (s) | Despacho máx. UI (ms) | Δ privados |
|---:|---:|---:|---:|---:|---:|
| 100 | 0,24 | 89 | 0,06 | 4,6 | +0,7 MiB |
| 1.000 | 1,16 | 71 | 0,25 | 11,4 | +2,0 MiB |
| 5.000 | 5,63 | 250 | 2,03 | 12,4 | +7,1 MiB |
| 10.000 | 11,29 | 361 | 2,39 | 18,8 | +13,0 MiB |

Operações com 10.000 faixas (ms): refresh 0,03; pintura 2,6; 10 roladas 2,4;
End 8,3; busca 9,6–9,7; limpar busca 6,1; ordenar por nome 47,0; por duração
17,7; misturar 6,9; tocar item 12,7; salvar M3U 17,9; ler M3U 40,7; Ctrl+A
13,5; excluir tudo 15,8. Sessão de 10.000 faixas: restauração 84–149 ms na
criação da playlist, salvamento no fechamento 68–159 ms. Nenhuma operação
bloqueou a interface de forma perceptível; nada alterado. Custo por faixa
~1,3 KiB.

## 9. Temas, idiomas, tooltips e janelas

Mediana com três janelas abertas (x64; referência → corrigido):

| Carga | Tempo por operação (média / p95 / máx) | Memória/recursos |
|---|---|---|
| 720 trocas de tema + pintura das 3 janelas | 6,59 / 12,75 / 42,4 → 6,05 / 11,95 / 49,7 ms | privados* +1,3 → +0,7 MiB, platô; GDI 15–16, USER 11–13 constantes |
| 1.800 trocas de idioma (18 idiomas × 100) | 7,2 → 7,0 ms | +2,7 → +1,9 MiB, platô após o ciclo 60 |
| 200 ciclos: 20 movimentos, 6 alternâncias, minimizar/restaurar | mover 1,53 → 1,56 ms; alternar 2,33 → 2,37 ms; min/restaurar 72 → 71 ms (animação do sistema) | handles/GDI/USER estáveis |
| 5.000 ciclos de descrição/atualização de 16 tooltips | < 0,01 ms | +0,2 MiB, GDI/USER inalterados |

A suíte automatizada acrescenta 27.000 descrições de tooltip sem crescimento
de memória e 1.980 trocas de idioma (`I18N_METRICS`). Notificação de idioma às
3 janelas: 1,62 → 0,96 ms.

## 10. Disco

* Carga de faixa (cache quente, NVMe): 1,6–16 ms até 56 MiB; arquivos maiores
  eram lidos integralmente na UI (até 365 ms para 1,31 GB) — corrigido.
* Metadados (`ReadTrackMetadata`, worker): 1,3–1,6 ms por arquivo real em
  cache quente; 0,15 ms para arquivos pequenos. Lê até 4 MiB do início de
  cada arquivo para tags embutidas.
* Sessão: a playlist restaurada é relida integralmente pelo worker a cada
  inicialização (10.000 arquivos: ~11 s em segundo plano, ~2,4 s de CPU).
* Persistência: INI por seção e M3U8 (17,9 ms para 10.000 faixas).
* **Não medido:** cache frio e HDD. Os fixtures estão em NVMe e limpar o
  cache do sistema exige privilégios administrativos.

## 11. Sessão prolongada (34,8 min)

Planejada para 65 min; **interrompida aos 34,8 min a pedido do usuário** para
encerrar a auditoria. Ambas as builds finais em paralelo, EQ Rock, FFT, player,
playlist e EQ abertos, volume baixo, amostragem a cada 10 s (207 amostras cada),
verificando resposta da janela (`WM_NULL`, limite 2 s).

| Build / faixa | WS início → fim (MiB) | Privados* (MiB) | Handles | GDI / USER | Threads | CPU média / máx 10 s | Sem resposta |
|---|---|---|---|---|---|---|---:|
| x64, FLAC 60 min (streaming) | 53,8 → 53,9 | 60,8 → 60,8 | 525 → 522 | 18/22 → 18/22 | 43 → 41 | 4,95% / 8,44% | 0 |
| x86, FLAC 4 min (memória, repetição: ~8 recargas) | 79,0 → 79,0 | 84,4 → 84,2 | 475 → 471 | 18/22 → 18/22 | 42 → 40 | 6,32% / 9,22% | 0 |

Valores "início" após o aquecimento (1/6 inicial). Não houve tendência de
crescimento de memória, handles, GDI/USER ou threads, inclusive com a
recarga repetida da faixa no x86 (o WS do x86 inclui os 33,5 MiB do FLAC em
memória). Resposta máxima da janela: 21,5 ms. A meta de 60 min não foi
cumprida; os 35 min não mostraram tendência que justifique estender.

## 12. Gargalos identificados

| Classe | Gargalo | Evidência | Decisão |
|---|---|---|---|
| Crítico | Arquivo inteiro em memória: x86 falha com WAV de 1,31 GB | Seção 6.2, executável real | Corrigido (13.2) |
| Alto | Um dispositivo D3D por janela (+16–18 MiB, +35 threads cada) | Seção 4 | Corrigido (13.1) |
| Alto | +502 MiB e 130–365 ms na UI para arquivos longos | Seção 6.2 | Corrigido (13.2) |
| Médio | Dois temporizadores repintando a janela com título longo | 9,23% vs 5,13% de CPU | Corrigido (13.3) |
| Médio | Primeira pintura de cada janela/tooltip criava um dispositivo (~35–180 ms) | Criação da playlist 48,7 → 15,3 ms | Resolvido por 13.1 |
| Médio | Releitura completa de metadados da sessão a cada inicialização | Seção 10 | Recomendação (exigiria cache persistente = nova funcionalidade) |
| Médio | Título longo rolando a ~33 Hz mesmo pausado (~5% de CPU) | Seção 13.3 | Comportamento especificado; recomendação |
| Baixo | Leitura de até 4 MiB por arquivo para tags | 1,3 ms em cache quente | Recomendação (impacto só em disco frio) |
| Baixo | Buscas lineares (`Find`, `View::Row`) | ≤ 13,5 ms com 10.000 faixas | Mantido |
| Baixo | Pintura completa a cada quadro do visualizador | ~1 ms de CPU por quadro | Mantido |

Não foram encontrados travamentos, vazamentos progressivos, falhas de
recurso GDI/USER ou interrupções de áudio nas condições medidas.

## 13. Otimizações realizadas

### 13.1 Fábrica Direct2D compartilhada (`src/ui_draw.cpp`)

* **Problema/causa:** `Canvas::Initialize` criava um `ID2D1Factory` por
  instância. Render targets de fábricas diferentes não compartilham o
  dispositivo Direct3D; cada janela (player, playlist, EQ, temas, cada tooltip
  exibido) criava o seu.
* **Correção:** fábrica única de processo, com contagem de usuários e
  liberação quando o último `Canvas` é reiniciado. Mesma configuração
  (hardware, single-threaded, mesma thread de UI): renderização idêntica.
* **Antes → depois:** ver seções 3, 4 e 14. Ex.: inicialização x64 81 → 45
  threads, 674 → 521 handles, WS 58,9 → 51,9 MiB; com EQ aberto 113 → 43
  threads e 98,3 → 66,7 MiB privados*. CPU por quadro inalterada (seção 5).

### 13.2 Streaming acima de 64 MiB (`src/audio.cpp`)

* **Problema/causa:** `ma_sound_init_from_file_w(..., 0, ...)` usa um buffer
  de dados do resource manager que lê o arquivo codificado completo para a
  memória, de forma síncrona, na thread que chama `Audio_Load` (UI).
* **Correção:** arquivos maiores que 64 MiB usam `MA_SOUND_FLAG_STREAM`: a
  thread de jobs do miniaudio decodifica páginas à frente do dispositivo.
  Arquivos menores permanecem no modo atual.
* **Justificativa do limiar:** 64 MiB cobre todas as faixas de 4 min medidas,
  inclusive FLAC 96 kHz/24 bits (56 MiB), preservando seek instantâneo e o
  áudio durante o arraste da barra de posição; limita a memória retida por
  faixa à ordem de grandeza do próprio aplicativo; elimina a falha de espaço de
  endereçamento do x86. Streaming total foi testado e **rejeitado**: cada seek
  produz ~10 ms de silêncio e o arraste da barra (um seek por movimento do
  mouse) ficaria praticamente mudo.
* **Testes:** o dispositivo offline dos testes (`Audio_TestRead`, apenas em
  `LITESIC_TEST`) passou a aguardar a thread de jobs como um dispositivo real,
  e gasta em um quadro descartável o período silencioso que o dispositivo real
  renderiza após um seek em stream. Nenhuma asserção existente foi alterada.
* **Antes → depois:** seção 6.2; continuidade real sem quedas (6.4); x86
  reproduz o WAV de 1,31 GB com +2 MiB.

### 13.3 Um único relógio de repintura com título longo (`src/mainwnd.cpp`)

* **Problema/causa:** com título excedente, `SCROLL_TIMER` (30 ms) e
  `VIZ_TIMER` (33 ms) invalidavam regiões diferentes em momentos diferentes,
  produzindo até ~63 repinturas completas por segundo.
* **Correção:** enquanto o visualizador anima (visível e tocando), o tique do
  visualizador também avança o deslocamento do título (pelo tempo decorrido
  real, como antes) e invalida o LCD; o temporizador de rolagem fica parado.
  Pausado, parado ou com visualizador oculto, o comportamento anterior volta.
* **Antes → depois (x64, 2 rodadas de 45 s):** osciloscópio + título longo
  9,23% → 4,67%; título curto 5,13% → 3,98%; visualizador oculto e pausado
  mantêm o comportamento especificado (4,4–6,2%).
* **Teste novo:** `visualizer_frame_drives_title_while_playing` verifica
  ausência do temporizador de rolagem tocando com visualizador, movimento
  real do título (captura) e retorno do temporizador ao ocultar/pausar.

## 14. Comparação antes/depois (executável real, mediana/60 s)

| Cenário | x64 referência | x64 final | x86 referência | x86 final |
|---|---|---|---|---|
| Inicialização: responde | 0,428 s | 0,389–0,580 s¹ | 0,606 s | 0,497 s |
| Repouso: WS / privados* / threads | 58,9 / 75,5 / 81 | 51,9 / 59,4 / 45 | 50,2 / 58,9 / 78 | 44,5 / 50,4 / 44 |
| Tocando OSC: CPU / WS / threads | 4,96% / 65,8 / 76 | 5,30% / 58,4 / 41 | 6,77% / 56,6 / 74 | 6,41% / 50,5 / 40 |
| Tocando FFT: CPU | 4,21% | 4,56% | 5,97% | 5,97% |
| Visualizador oculto: CPU | 1,05% | 0,70% | 2,20% | 1,42% |
| FFT + EQ aberto: CPU / WS / privados* / threads | 3,86% / 73,6 / 98,3 / 113 | 3,72% / 58,9 / 66,7 / 43 | 5,32% / 62,2 / 73,1 / 109 | 5,52% / 50,9 / 56,3 / 41 |
| Pausado: CPU | 0,23% | 0,44% | 1,08% | 0,41% |
| Minimizado tocando: CPU | 1,15% | 1,99% | 1,83% | 3,26% |
| EQ ativo (Rock) tocando FFT: CPU | — | 5,64% | — | — |

¹ Duas séries de 10 inicializações em horários diferentes; a variação acompanha
a criação do primeiro dispositivo D3D (184–384 ms) sob carga concorrente da GPU.
Diferenças de CPU abaixo de ~1 p.p. estão dentro do ruído entre rodadas
(mesmo cenário repetido: 4,96–5,59%).

## 15. Testes e builds

| Build | Compilação | Suíte | Resultado |
|---|---|---|---|
| x64 Release | `make ARCH=64 DEBUG=0` sem erros | 31 binários, `LITESIC_NO_CAPTURE=1` via `dev.ps1` | **168 PASS** |
| x86 Release | `make ARCH=32 DEBUG=0` sem erros | idem | **168 PASS** |
| x64 Debug | `make ARCH=64 DEBUG=1` sem erros | 31 binários executados individualmente | **168 PASS** |
| x86 Debug | `make ARCH=32 DEBUG=1` sem erros | idem | **168 PASS** |

Antes da correção 13.3, as suítes Release x64/x86 completas também passaram
**com captura** (167/167 cada, mesma contagem por suíte da referência). 168 = 167 da referência + 1 caso novo. Avisos de compilação
existentes na referência (funções GDI não usadas, cast em `app.cpp`, avisos
do miniaudio no x86) não mudaram.

Instabilidade ambiental (não regressão): com o computador em uso e as GPUs
ocupadas pelo `llama-server`, as execuções encadeadas por `make test` com
captura falharam em `test_themes.cpp:192` (pixel via `PrintWindow` durante
1.980 trocas rápidas) e, no encadeamento Debug, em `test_m8_remaining.cpp:43`
(estilo TOPMOST). O **código de referência `9b2689b` falhou exatamente nos
mesmos pontos** nas mesmas condições; os binários passam isoladamente. As
asserções por captura foram executadas com sucesso antes dessa carga e ficam
como verificação a repetir em desktop ocioso.

## 16. Medições reais, simulações e verificações não realizadas

* **Reais:** executável completo (inicialização, cenários, sessão de 34,8 min,
  continuidade por loopback, arquivos grandes), sondas com objetos de produção
  e janelas reais, suítes automatizadas.
* **Simuladas:** dispositivo de áudio offline das sondas `graph`/`stress`
  (ritmo por relógio, não WASAPI); dados de áudio e bibliotecas sintéticos.
* **Simplificadas a pedido do usuário:** sessão prolongada encerrada aos
  34,8 min (meta 60 min); playlist e estresse áudio×UI não foram repetidos na
  build final (medidos na referência; as correções não alteram esses
  caminhos para arquivos < 64 MiB); suítes com captura não foram repetidas
  após a correção 13.3 em desktop ocioso.
* **Não realizadas:** Windows 7/8/8.1/10 e hardware sem GPU dedicada (o
  dispositivo D3D e o custo de CPU variam por driver); HDD/cache frio e
  unidades de rede (relevante para o streaming acima de 64 MiB); outros DPIs
  além de 150%; dispositivos de áudio USB/Bluetooth; perfilamento por pilha de
  heap; medição isolada sem o RTSS e sem carga concorrente da GPU.

### 16.1 Validações futuras (observações não bloqueantes, não aprovadas)

1. Sessão prolongada de 60 min ou mais (executados 34,8 min).
2. Playlist (100–10.000 faixas) e estresse áudio × UI na build final.
3. Suítes com asserções por captura de janela em desktop ocioso, após a
   correção 13.3 — somente com autorização prévia explícita do usuário.
4. Windows 7/8/8.1/10, GPU integrada ou sem driver dedicado, outros DPIs.
5. HDD/cache frio e unidades de rede, especialmente para o streaming > 64 MiB.
6. Dispositivos de áudio USB/Bluetooth; medição sem RTSS e sem carga de GPU.

## 17. Problemas remanescentes

1. Arquivos > 64 MiB: ~10 ms de silêncio por seek e áudio quase ausente
   durante o arraste da barra de posição; em disco muito lento (HDD em
   repouso, rede) o streaming pode sofrer lacunas se a leitura de uma página
   de ~1 s atrasar. Comportamento das faixas comuns inalterado.
2. Primeira pintura (dispositivo D3D) domina a inicialização: 184–384 ms.
3. Releitura dos metadados de toda a sessão a cada inicialização.
4. Título longo rolando a ~33 Hz mesmo pausado/parado: ~5% de um núcleo.
5. Medidas absolutas de memória dependem do driver de vídeo e de software
   injetado (RTSS: +51,7 MiB de commit neste computador).

## 18. Recomendações para a M10

1. Publicar as metas de referência medidas, não limites arbitrários: neste
   hardware, repouso ≈ 52 MiB WS / 45 threads (x64) e 44,5 MiB / 44 (x86);
   reprodução 4–7% de um núcleo com visualizador, 1–2% sem. Usar a sessão
   prolongada (`perf_session.py soak`) como teste de regressão de vazamento
   antes de cada versão: critério técnico = sem tendência de crescimento de
   handles/GDI/USER e WS estável após o aquecimento.
2. Validar em Windows 7–10 e em GPU integrada/sem driver dedicado — o custo
   do dispositivo D3D e o fallback do Direct2D variam por driver.
3. Considerar marcar o x86 como *large address aware* somente após validação
   (não necessário após 13.2).
4. Itens opcionais para depois da distribuição, só com nova evidência:
   pintura parcial do visualizador, cache persistente de metadados, leitura de
   tags proporcional ao tamanho declarado do tag e pausa da rolagem do título
   quando a reprodução não está ativa — os dois últimos alteram comportamento
   e dependem de decisão do usuário.

## 19. Reprodução

```powershell
python scripts/diagnostics/perf_fixtures.py          # ~1 GB em build/perf/fixtures
./scripts/dev.ps1 -Action build -Architecture 64; ./scripts/dev.ps1 -Action build -Architecture 32
./scripts/dev.ps1 -Action perf-probe -Architecture 64
build/64/release/perf-probe.exe dsp | canvas | paint <faixa> | playlist <pasta> <n> | themes 20 | languages 100 | windows 200 | stress <faixa> <pasta> 20 | memory-stages <faixa>
python scripts/diagnostics/perf_session.py startup <exe> <rótulo> 10
python scripts/diagnostics/perf_session.py scenarios <exe> <rótulo> 60 [1]
python scripts/diagnostics/perf_session.py continuity <exe> <rótulo> 180 <senoide.flac> [1]
python scripts/diagnostics/perf_session.py soak <exe> <rótulo> 65 <faixa>
python scripts/diagnostics/perf_summary.py
```

`loopback_probe.cpp` compila com `miniaudio_impl.o` (ver cabeçalho do arquivo).
Resultados brutos desta auditoria: `docs/evidence/performance-audit/`.

## 20. Arquivos alterados

* Produção: `src/ui_draw.cpp`, `src/audio.cpp`, `src/mainwnd.cpp`.
* Testes: `tests/test_player_window.cpp` (1 caso novo).
* Ferramentas: `scripts/diagnostics/perf_probe.cpp`, `perf_session.py`,
  `perf_fixtures.py`, `perf_summary.py`, `loopback_probe.cpp`; alvo
  `perf-probe` no `Makefile` e em `scripts/dev.ps1`.
* Documentação: este relatório, `PROJECT_STATE.md`, evidências.
