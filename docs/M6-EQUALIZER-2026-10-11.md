# M6 — Equalizador real — 11/10/2026

Atualização posterior: o usuário confirmou “Funcionou”. M6 aceita; os ajustes
de magnetismo/alinhamento estão em [WINDOW-SNAP](WINDOW-SNAP-2026-10-11.md).
O relatório e as evidências abaixo preservam a rodada original de validação.

**Situação na rodada original: IN_PROGRESS.** Implementação, quatro builds, regressão automatizada,
DSP real, persistência, interface e medições entregues. O checkpoint de alteração
audível/aceite manual do roadmap ainda depende do usuário; não foi declarado PASS.
Windows 7 continua na M9. Não houve commit nem push.

## A. Arquitetura e investigação do pipeline

Estado inicial confirmado: `master`, commit `b27da1e`, upstream `origin/master`,
árvore limpa. Consultados ROADMAP, M6, DEVELOPMENT, encerramento M5, persistência
da aparência, GUI-AUTOMATION e a skill local `litesic-task`. A instrução desta
rodada para não commitar prevalece sobre a preferência permanente dessa skill.

O engine permanece miniaudio **0.11.25**, com stb_vorbis **1.22**. Arquivos são
abertos por `ma_sound_init_from_file_w`, usando o resource manager. MP3/WAV/FLAC
usam seus decoders incorporados; OGG usa stb_vorbis. A sondagem `ma_decoder`
independente só obtém a taxa nativa para exibição; não alimenta o DSP.

O resource manager do engine configura **float32**, preserva os canais do
arquivo (`decodedChannels=0`) e converte/resampleia para a taxa do engine
(`decodedSampleRate=ma_engine_get_sample_rate`). O sound faz a conversão de
canais/spatialization e aplica o pan; o processamento geral também possui
resampler para as conversões necessárias. As posições/seek continuam usando
a taxa da data source, que pode diferir da taxa nativa do arquivo.

Fluxo efetivo após a integração:

```text
arquivo → decoder/resource manager → sound (canais/pan/resampling)
        → nó EQ (PRE + cascata de dez biquads, ou bypass)
        → tap existente (mix mono → ring buffer → FFT/OSC)
        → endpoint (volume master) → dispositivo/conversão final
```

`ma_engine_read_pcm_frames` puxa o grafo. `EqProcess`, vtable de um **ma_node**
com uma entrada/saída, é chamado nesse grafo. Não substituímos o callback do
dispositivo nem o engine. Foram examinados `ma_peak2`, `ma_peak_node` e seu
`reinit`: oferecem o filtro, mas não a publicação de snapshots nem as transições
duplas necessárias. O nó customizado permite prealocar estados e manter
coeficientes fixos durante cada transição. O contrato de nós e exemplos estão
na [documentação oficial do miniaudio](https://miniaud.io/docs/manual/index.html).

O nó opera em float intercalado, com taxa/canais do engine, sem presumir a taxa
do arquivo. Aceita tamanhos variáveis de bloco e buffers in-place; o teste
offline usa também leituras maiores, subdivididas pelo grafo. Não impõe tamanho
ou latência ao dispositivo. A medição controlada solicita 512 frames (10,667 ms
a 48 kHz); isso não identifica o buffer real negociado pelo driver.

O DSP suporta **1–32 canais** com estados independentes. O miniaudio admite até
254; se o engine negociar mais de 32, a inicialização falha explicitamente em
vez de processar parcialmente. A aplicação normalmente negocia estéreo; fontes
mono são convertidas pelo sound antes do nó. Mono/estéreo foram medidos; saída
multicanal física não foi certificada.

O EQ existe desde `Audio_Init`, mesmo sem visualizador ou janela EQ. O tap é
conectado depois dele quando inicializado. Trocar arquivo libera o sound antigo,
cria o novo e reconecta ao EQ; não recria filtros/engine. Shutdown para o engine,
libera sound, tap e EQ e encerra o engine. Pause preserva o estado; unload,
troca de faixa, stop e seek publicam um pedido de reset consumido na thread de
áudio. Recriar o engine prepara coeficientes na nova taxa antes de tocar.

## B. Processamento

Bandas do roadmap: **60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000 Hz**.
O desenho anterior tinha rótulos incompatíveis; foram corrigidos. O anexo não
continha uma lista após “frequências nominais”; prevaleceu a lista explícita da M6.

Cada banda é peaking IIR de segunda ordem, em cascata, com ganhos ±12 dB.
Coeficientes e estados DF-II transpostos são double; buffers de entrada/saída
são float. Usamos **BW de uma oitava**, entre os pontos de metade do ganho,
com compensação da transformação bilinear (`alpha=sin(w)*sinh(ln(2)/2*w/sin(w))`).
Não é um Q arbitrário: a relação equivalente é `1/Q=2*sinh(ln(2)/2*w/sin(w))`.
Referência: [Audio EQ Cookbook, W3C/RBJ](https://www.w3.org/TR/audio-eq-cookbook/).

As frequências do roadmap não têm espaçamento uniforme; 12/14/16 kHz se
sobrepõem bastante. Os ganhos dos sliders são os ganhos de cada filtro,
**não pontos independentes da resposta total**. A curva do painel conecta
os controles, conforme M6-T3; não pretende mostrar a resposta acústica medida.

Uma banda só é aplicada quando `f < 0,475*Fs` (95% de Nyquist), com margem
para o cálculo próximo ao limite. Não há deslocamento silencioso do centro.
Em 32 kHz, 16 kHz fica N/A; seu ganho permanece salvo para taxas compatíveis.
As demais taxas testadas, 44,1/48/96 kHz, permitem todas as bandas.

Ganhos inválidos ficam em zero; finitos extremos são limitados a ±12 dB.
Coeficientes passam por verificação de finitude e condições de estabilidade
de segunda ordem. Coeficiente inválido vira identidade. Estados menores que
1e-24 são zerados contra denormais. Entrada NaN/Inf vira zero; estados/saídas
patológicas não finitas ou acima de 1e12 são descartados. Essa última guarda
é para corrupção numérica, **não um limitador de áudio**.

PRE é um ganho independente de ±12 dB, aplicado antes da cascata. OFF ignora
PRE e todos os filtros. Flat ON usa caminho de identidade; em regime estável,
os testes exigem **igualdade bit a bit** para sinal float finito normal.

Não há compressor, normalizador, ganho automático ou saturação oculta. O
domínio float preserva headroom interno acima de ±1; a conversão/dispositivo
final pode clipar. O painel avisa quando há ganho positivo e oferece PRE
explícito. Presets têm atenuação inicial visível, escolhida conservadoramente,
mas **não garantem ausência de clipping** para qualquer sinal/cascata. Para
boosts extremos simultâneos, até −12 dB pode ser insuficiente: reduza os boosts
ou o volume master. Não se promete proteção absoluta.

## C. Segurança de tempo real

`eq_dsp.*` contém processamento; `eq_state.*` contém estado/presets/INI;
`eqwnd.cpp` contém apresentação. O producer único é a UI e o consumer único é
a thread que processa o grafo. Um mailbox de **três slots com propriedade
exclusiva** publica snapshots completos via atomic exchange acquire/release.
O produtor nunca escreve o slot que o consumidor está lendo. Um static_assert
exige atomics unsigned de 32 bits lock-free em ambas as arquiteturas.

Trigonometria, pow e validação de coeficientes ocorrem em `Publish`/Initialize,
fora do áudio. O consumer consome no início do bloco quando terminou a transição
anterior. Alterações rápidas coalescem para o snapshot mais recente; não cresce
uma fila. Reset usa um contador atômico, sem escrever estados desde a UI.

Cada alteração inicia um crossfade linear de **20 ms** entre dois bancos com
coeficientes constantes e estáveis. Não interpolamos coeficientes IIR. Os estados
do banco anterior são copiados; stages recém-ativados após bypass e stages
identidade são limpos. Um banco é promovido ao fim da transição. ON/OFF e PRE
seguem o mesmo mecanismo; não mudam cursor, engine ou reprodução.

O EQ não usa mutex, espera, alocação, disco, objetos gráficos ou Win32 no callback.
O tap herdado conserva `TryEnterCriticalSection`: se a UI mantém o lock,
descarta a captura, sem esperar. O teste existente dessa condição continua PASS.
As janelas e persistência nunca são acessadas pelo DSP.

O stress concorrente publica 20.000 configurações enquanto outro thread
processa 20.000 blocos, incluindo resets. Não houve não-finitos, corrupção ou
deadlock observado. Isso complementa o contrato de propriedade; não é uma
prova universal de concorrência nem uma execução sob ThreadSanitizer.

## D. Interface

Painel Win32 sem moldura, Direct2D/DirectWrite, 460 × 244 DIP. Cabeçalho e X,
ON/OFF, menu de presets com tema, Redefinir para Flat, PRE, dez sliders verticais,
marcas ±12/0, ganhos numéricos, curva de segmentos e indicação N/A. Tokens,
fontes, desenho e menus são os mesmos do player/playlist.

Arraste usa o ganho e posição iniciais para evitar salto no início. Hover,
pressionado, foco, ativo e indisponível são desenhados. Tab/Shift+Tab navegam;
setas alteram 0,5 dB, Home/End vão a ±12, Espaço zera um slider ou ativa um botão;
Escape oculta. Alterações manuais mudam para Custom. O botão do player deixou
de anunciar “DSP pendente”. Ocultar/destruir o painel não desliga EQ.

Seis escalas 80/90/100/110/125/150 foram verificadas por geometria; a interface
real foi capturada em 150%. Movimento físico entre monitores/DPI não foi
certificado. O docking magnético da playlist não foi alterado; docking adicional
do EQ permanece na M7.

## E. Presets

Valores em dB; colunas correspondem às frequências da M6, nessa ordem.

| Preset | PRE | 60 | 170 | 310 | 600 | 1k | 3k | 6k | 12k | 14k | 16k |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Flat | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| Rock | -4 | 4 | 3 | 1 | -2 | -1 | 2 | 3 | 2 | 1 | 1 |
| Pop | -3 | -1 | 1 | 3 | 3 | 1 | -1 | -1 | 1 | 2 | 2 |
| Jazz | -3 | 3 | 2 | 0 | 1 | -1 | -1 | 1 | 2 | 2 | 1 |
| Clássica | -3 | 2 | 1 | 0 | 0 | -1 | -1 | 0 | 2 | 2 | 1 |
| Graves | -5 | 5 | 4 | 2 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| Voz | -3 | -3 | -2 | -1 | 1 | 3 | 3 | 1 | -1 | -2 | -2 |

São escolhas iniciais de produto, não perfis calibrados. Seleção publica todos
os parâmetros juntos e preserva ON/OFF. A tabela permite novos presets sem
um gerenciador de arquivos. Custom, inclusive valores coincidentes com Flat,
é preservado pela persistência.

## F. Persistência

Mesma `%LOCALAPPDATA%\LiteSic\settings.ini`, seção `Equalizer`: Enabled, Preamp,
Preset e Band0..Band9. A infraestrutura de caminho da aparência configura o
estado antes de criar o engine. Save grava só a seção EQ com
`WritePrivateProfileSectionW`; não substitui Appearance nem playlist.
Gravações ocorrem ao concluir interação, escolher preset/toggle e encerrar,
nunca no movimento de cada amostra ou callback. Leitura parcial/corrompida usa
padrões seguros; nome de preset incompatível com os ganhos é tratado como Custom.
A [API de gravação suporta Windows 2000 ou posterior](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilesectionw);
isso não certifica o aplicativo no Windows 7.

`--validation` é um modo de desenvolvimento com caminho fixo derivado do exe:
`build/gui/m6-session`. Isola INI e playlist para o auxiliar gráfico; o modo
normal conserva o caminho pessoal. A playlist/configuração pessoal não foi
alterada na validação. A preferência existente EqualizerVisible foi reutilizada.

## G. Temas e visualizadores

Os 36 temas passaram por pintura real PrintWindow, verificação do pixel de
fundo e preservação dos parâmetros nas quatro variantes. Foram inspecionadas
as capturas de Original, Escuro, Claro, Inverso, Alto Contraste, Verão, Natal
e Cyberpunk. Claro/Cyberpunk também foram aplicados no seletor do executável
interativo, preservando Rock/ON/ganhos. [Galeria completa](evidence/m6/index.html)
e [prancha dos oito temas](evidence/m6/representative-themes.png).

FFT/OSC continuam com o mesmo ring buffer e algoritmos, agora após o EQ.
Permanecem antes do master volume, como no tap anterior: mostram a forma
equalizada e balanceada, não a conversão final do dispositivo. Essa decisão
preserva o comportamento de visualização ao mudar volume e evita outro tap.
Não foi alterado silenciosamente o desenho dos visualizadores.

## H. Testes e limites

**20 executáveis, 105 verificações por variante, 420 aprovações nas quatro**.
Mais 27 guardas do auxiliar gráfico. [Logs e manifesto](evidence/m6/manifest.json).
95 rótulos PASS aparecem nos logs; dez testes de formatação só imprimem o
total final, por isso o total de verificações é 105.

- DSP real: cada banda disponível em ±6/±12 dB, 32/44,1/48/96 kHz, tolerância
  de **0,08 dB** no centro após meio segundo de estabilização; largura de uma
  oitava a 1 kHz com tolerância **0,04 dB** nos pontos de metade do ganho.
- Flat/bypass bit-exatos em regime; PRE −6 dB com erro absoluto <1e-8;
  impulsos, sweep, extremos simultâneos, canais independentes e parâmetros
  NaN/Inf/fora de faixa. Guarda para entrada finita extrema com PRE.
- Transição ON/PRE medida com entrada DC, incremento <0,00004 por amostra;
  mudança para OFF termina com bytes iguais à entrada. Stress concorrente.
- Fixtures originais de 1 kHz, aproximadamente −34 dBFS, em WAV mono/estéreo,
  MP3/FLAC/OGG. O grafo real apresenta ±6 dB com tolerância **0,1 dB**;
  Flat varia <1e-6 na razão RMS. Volume ×0,25 e pan esquerda mantidos.
- Presets rápidos, PRE, toggle, troca de faixa, seek, pause/resume e stop/play
  exercitados no engine offline. EQ oculto continua produzindo ganho medido.
- Win32 entrega mouse/teclado ao próprio painel e verifica parâmetros/RMS;
  36 temas, geometria em seis escalas, regressão completa da playlist/player.
- Auxiliar interativo: abertura, arraste, ON, seleção Rock, fechamento,
  reabertura/restauração e mudanças de tema; capturas inspecionadas.

Falhas intermediárias corrigidas: novas fixtures inicialmente afetavam a
contagem de um teste de importação de pasta (foram isoladas em `tests/eq_fixtures`);
um include faltava no teste de janela; o auxiliar não reconhecia EQ como dono
de menu; comparação x87 com literal float falhava apesar dos bytes preservados.
O teste novo usa memcmp e o teste antigo de escala usa tolerância 1e-6.
Nenhum teste foi removido. Falhas de observação/foco não foram contadas como PASS.

Pendente: julgamento auditivo humano e aceite estético; VM Windows 7; soak
prolongado e saída multicanal física; medição de underruns e qualidade analógica
no dispositivo. Não há garantia de ausência de estalos em todo conteúdo a partir
de sinais sintéticos, nem de ausência universal de race por um stress curto.

## I. Desempenho

Três processos por cenário/arquitetura Release. Mesmo tom WAV 44,1 kHz,
engine offline 48 kHz estéreo, 100 blocos de aquecimento e 2.000 blocos medidos,
512 frames. QPC mede só a leitura do grafo; GetProcessTimes mede CPU do loop
inteiro; GetProcessMemoryInfo e GetGuiResources são amostras finais do processo.
Os cenários FFT/OSC desenham os componentes reais a cada quatro blocos; o
cenário completo pinta também player, playlist vazia e EQ. Não usam áudio físico.

O baseline compila a unidade `audio.cpp` de **b27da1e** contra o harness/objetos
atuais, com dois shims de controle sem DSP. Serve para comparação do caminho
de áudio anterior, não para medir um executável histórico inteiro. O cenário
“sem nó atual” também desvia o sound do EQ em API exclusiva de teste.

Medianas das três rodadas, x64 Release; µs por bloco, MiB por processo:

| Cenário | Média | p99 | Máximo observado por rodada (mediana) | Residentes | Privados |
|---|---:|---:|---:|---:|---:|
| Áudio anterior b27da1e | 9,630 | 16,7 | 23,8 | 14,38 | 3,80 |
| Atual sem nó EQ | 9,390 | 16,1 | 24,0 | 14,36 | 3,83 |
| EQ OFF | 11,192 | 19,6 | 49,2 | 14,35 | 3,83 |
| EQ ON Flat | 12,806 | 19,0 | 28,7 | 14,33 | 3,82 |
| EQ ON Rock | 37,940 | 57,9 | 134,9 | 14,43 | 3,83 |
| Rock + FFT | 36,390 | 59,1 | 88,7 | 49,63 | 55,80 |
| Rock + OSC | 36,152 | 57,5 | 88,8 | 50,22 | 56,82 |
| Rock + OSC + player/PL/EQ | 37,530 | 58,0 | 115,8 | 73,46 | 106,25 |

Rock acrescenta cerca de **28,3 µs/bloco**, aproximadamente **0,27% do tempo de
um núcleo por duração de áudio**, ao baseline. O custo total da leitura Rock
é ~0,36% desse orçamento. Em x86: baseline 32,843 µs; OFF 33,494; Flat 36,029;
Rock 125,847; FFT 127,646; OSC 125,738; painéis 134,854 µs. Rock ocupa ~1,18%
do orçamento de um núcleo. Não foi necessário SIMD novo obrigatório.

CPU acumulada mediana no loop completo x64: baseline/OFF 15,625 ms, Flat
31,250, Rock 78,125, FFT 765,625, OSC 875,000, painéis 2.890,625 ms. Os counters
têm granularidade e a execução é acelerada; não equivalem a CPU amostrada durante
reprodução física. O pico medido é o tempo de bloco; pico de CPU do dispositivo
e underruns não foram medidos. Dados individuais, CPU, tempo de parede, GDI/USER
e métricas x86 estão nos [JSONs de desempenho](evidence/m6/performance-64-release.json)
e no [JSON x86](evidence/m6/performance-32-release.json).

O incremento privado no caminho sem renderização ficou na ordem de centésimos
de MiB, dentro da variação de heap. A carga D2D aumenta memória em dezenas de
MiB, coerente com a investigação M5; não se atribui esse total aos biquads.
Não são medições de pico de memória nem prova de ausência de vazamento.

Reprodução:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Action test
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/diagnostics/run_eq_probe.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/diagnostics/run_eq_probe.ps1 -Architecture 32
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action launch -Validation
.tools/gui-venv/Scripts/python.exe scripts/diagnostics/package_m6.py
```

O empacotador exige logs/capturas/medições reais existentes; não substitui rodar
os gestos. Acrescente Architecture/DebugBuild para as outras suítes. Fixtures
podem ser regeneradas com `tests/fixtures/generate_eq.py` e FFmpeg, que não é
dependência do player ou da execução dos testes.

## J. Evidências gráficas

`docs/evidence/m6/` contém 36 PNGs dos temas, prancha, galeria, capturas da GUI,
oito logs de build/teste, guardas, JSONs de desempenho e manifesto SHA-256.
O manifesto distingue captures interativas (PID, criação, hash do executável,
geometria, evento) de imagens PrintWindow do harness (hash do binário de teste).
Capturas interativas anteriores à última revisão numérica mantêm seus hashes
originais; não recebem o hash do executável final.

Exemplos: [Custom após arraste](evidence/m6/m6-eq-custom.png),
[Rock](evidence/m6/m6-rock-eq.png), [restauração](evidence/m6/m6-reopened-eq.png),
[Claro](evidence/m6/m6-claro-eq.png), [Cyberpunk](evidence/m6/m6-cyberpunk-eq.png).
Inspeção automática/por agente não constitui aceite estético humano.

## K. Git e arquivos

Branch inicial/final **master**, HEAD **b27da1e**, sem commit/push por instrução.
Árvore final contém apenas alterações desta entrega e evidências; não houve
reset, limpeza destrutiva ou modificação de histórico.

Criados: `src/eq_dsp.h/.cpp`, `src/eq_state.h/.cpp`, três testes EQ,
`tests/eq_fixtures/*`, gerador de fixtures, `eq_probe.cpp`, `run_eq_probe.ps1`,
`package_m6.py`, relatório e pacote de evidências.
Modificados: `audio.h/.cpp`, `eqwnd.cpp`, `app.cpp`, `ui_theme.h/.cpp`,
`ui_player.cpp`, `mainwnd.cpp`, Makefile, scripts dev/gui/driver, teste de escala,
README, ROADMAP, DEVELOPMENT, GUI-AUTOMATION e milestone M6.
Sem alteração funcional do modelo/controlador/docking da playlist, codecs ou
miniaudio incorporado. Os avisos existentes de cast FARPROC e helpers GDI
ociosos permanecem; não foram introduzidos avisos novos na produção.

## L. Roadmap e próximo passo

M6-T1/T3/T4 implementadas e verificadas. M6-T2 tem DSP completo e resposta
medida, mas seu critério explícito “audivelmente” ainda não tem checkpoint
humano; checkbox e milestone permanecem **IN_PROGRESS**. Demais itens técnicos
do checkpoint estão assinalados onde há evidência; aceite do usuário permanece
aberto. Não foi iniciada M7.

Próximo passo: ouvir uma faixa conhecida, alternar OFF/ON e presets e validar
visualmente o painel; registrar o resultado humano. Soak/dispositivo e M9
permanecem com seus limites próprios. O executável final disponível é
`build/64/release/LiteSic.exe` (também há Debug e ambas as variantes x86).
