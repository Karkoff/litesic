# Reprodução inoperante após inatividade prolongada (15/10/2026)

**Estado: PASS por decisão expressa do usuário (15/10/2026).** Resultado
técnico entregue: COMPLETE_WITH_CAVEATS — causa raiz identificada, defeito
reproduzido antes da correção, corrigido e validado com testes determinísticos
e com o dispositivo WASAPI real. A validação humana com o LiteSic aberto por
várias horas e desconexão real do fone Bluetooth (seção 6) será feita pelo
usuário; não está registrada como executada.

## 1. Incidente

O LiteSic ficou aberto por horas, sem tocar. Ao voltar, selecionar qualquer
faixa não produzia som nem erro visível. O usuário informou depois que usava
um **fone Bluetooth e o desconectou** nesse intervalo.

## 2. Causa raiz

O dispositivo de saída do `ma_engine` (miniaudio 0.11.25, WASAPI) é aberto na
inicialização e fica rodando até o encerramento, inclusive em silêncio.
Quando o endpoint desaparece, a chamada de renderização
(`IAudioRenderClient::GetBuffer`) retorna `AUDCLNT_E_DEVICE_INVALIDATED` e o
loop de dados do miniaudio termina (`ma_device_write__wasapi` →
`ma_device_audio_thread__default_read_write` → `ma_worker_thread`): o
dispositivo vai para `ma_device_state_stopped`. A parada do cliente
invalidado também falha, então nem a notificação `stopped` é emitida.

O miniaudio só reinicia o dispositivo sozinho se as notificações do Windows
chegarem **antes** desse erro: `OnDeviceStateChanged` marca
`isDetachedPlayback` apenas se o dispositivo ainda estiver `started`, e
`OnDefaultDeviceChanged` reencaminha para o novo padrão, mas só religa se o
dispositivo estava `started` ou marcado como destacado. Se o erro de
renderização vence essa corrida (comum, porque as notificações MMDevice são
assíncronas), o dispositivo é reencaminhado ou não, mas **permanece parado
para sempre**.

O LiteSic não verificava o estado do dispositivo: `ma_sound_start` apenas
marca o som como ativo e retorna sucesso, então `Audio_Play` registrava
`PLAYING`, `PlaybackController` retornava sucesso e a interface não mostrava
erro, sem renderizar nada e com a posição parada. É exatamente o sintoma
relatado. Reiniciar o aplicativo “resolvia” porque criava um dispositivo novo.

A inatividade em si não é a causa: uma hora simulada de renderização em
silêncio não afeta a reprodução (teste `idle_hour_then_select_plays`).

### Evidências

* Código do miniaudio citado acima (`third_party/miniaudio.h`, linhas
  ~22367–22592, 24922–25010, 42762–42870).
* Teste antes da correção (`docs/evidence/playback-idle-recovery/prefix-repro-x64.txt`):
  `idle_hour_then_select_plays` PASS; `endpoint_lost_while_idle_then_select_plays`
  **FAIL** em `Advances()`: estado `PLAYING`, nada renderizado.
* Logs do Windows (`windows-events.txt`): instância x64 iniciada às 00:55:34;
  o Kernel-PnP removeu os endpoints do **fone Bluetooth Fuxi-H4** (A2DP
  “Fones de ouvido” e Hands-Free) às **01:50:04** e às **02:28:56**
  (“ausente no barramento”). Nenhuma suspensão/retomada no período.
* Observação não explicada: antes de ser encerrada (com autorização do
  usuário), a instância do incidente tinha **1 thread** (ociosa normal ≈ 45)
  e não atendeu ao fechamento normal em 5 s. Sem despejo de memória, a causa
  não pôde ser determinada; ver seção 6.

## 3. Correções

| Arquivo | Alteração |
|---|---|
| `src/audio.cpp`, `src/audio.h` | Recuperação da saída (abaixo); `Audio_CheckOutput`; log de diagnóstico; volume/pan guardados mesmo sem engine; nó do visualizador separado do buffer para ser recriado. |
| `src/playback.cpp`, `src/playback.h` | `LoadIndex` distingue arquivo inválido de saída indisponível (`outputAvailable` no backend). Sem saída, nenhuma faixa é marcada como defeituosa e Próxima/Anterior param na primeira tentativa, em vez de percorrer a playlist. |
| `src/mainwnd.cpp` | O temporizador de posição (250 ms) chama `Audio_CheckOutput`; o clique do mouse nos botões usa `ExecutePlayerControl` (antes ignorava a falha do Play, ao contrário do teclado); Próxima/Anterior mostram erro quando não há saída. |
| `src/plwnd.cpp` | Enter, menu de contexto e acessibilidade mostram `errors.play_selected` em caso de falha (antes silenciosos). |
| `src/app.cpp` | Caminho do log: `<perfil>\audio.log`. |
| `src/ma_config.h`, `Makefile` | Somente em `LITESIC_TEST`: backend “custom” do miniaudio (`miniaudio_impl-test.o`). O executável de produção continua com `miniaudio_impl.o` inalterado. Alvo `output-recovery-probe`. |

### Recuperação (somente diante de estado observado)

1. **Quando:** em `Audio_Play` (comando do usuário), se o dispositivo não
   está `started`; e, durante `PLAYING`, pelo temporizador da interface,
   após **duas** observações consecutivas (uma só pode ser a janela do
   reencaminhamento do próprio miniaudio). Nada é verificado nem reiniciado
   enquanto o player está parado sem comando.
2. **Como:** primeiro `ma_device_start` no dispositivo existente (o miniaudio
   pode já tê-lo reencaminhado). Se falhar, ou se a saída falhar de novo em
   até 30 s, o engine é recriado no dispositivo padrão atual, a faixa é
   reaberta na mesma posição e volume, pan, EQ, visualizador, playlist e
   seleção são preservados.
3. **Limites:** no máximo 3 tentativas automáticas por comando do usuário;
   depois disso, ou se não houver dispositivo, o transporte passa a `PAUSED`
   e a interface mostra “Não foi possível reproduzir a faixa”. O próximo Play
   tenta de novo; quando o dispositivo volta, a reprodução funciona sem
   reiniciar o aplicativo. Sem laços, temporizadores novos ou reinícios
   periódicos.
4. **Diagnóstico:** `<perfil>\audio.log` (e `OutputDebugString`), criado
   somente no primeiro evento de falha/recuperação e limitado a 256 KiB:
   motivo, estado do dispositivo e do transporte, resultado do reinício,
   resultado da recriação e da reabertura da faixa. A reprodução normal não
   gera registros; o perfil continua com os mesmos arquivos em uso normal.

**Desempenho:** em operação saudável, o custo adicional é uma leitura
atômica de estado a cada 250 ms, sem alocação nem threads novas. Nenhuma
decisão de `docs/PERFORMANCE-AUDIT.md` foi alterada (streaming > 64 MiB,
fábrica D2D, relógio de repintura).

## 4. Validação

| Execução | Resultado |
|---|---|
| `test_audio_recovery` x64 e x86 Release (11 cenários) | PASS; 20 repetições por arquitetura sem falha |
| `test_playback` (novo `missing_output_neither_marks_nor_scans`) | 12/12 PASS |
| Suíte completa x64 e x86 Release, `LITESIC_NO_CAPTURE=1` | 32 binários, 182 casos PASS, 0 FAIL por arquitetura |
| `output-recovery-probe` no WASAPI real (fixture silenciosa) | 5/5 PASS: reinício, verificação do temporizador e recriação |
| Builds Release x64 e x86 | OK |

O teste `test_audio_recovery` usa um backend miniaudio programável que roda o
worker real do miniaudio; a falha injetada termina o loop de dados e faz a
parada falhar, como o WASAPI após `AUDCLNT_E_DEVICE_INVALIDATED`. O tempo é
simulado (o dispositivo só renderiza os quadros concedidos pelo teste).

Cenários: (1) tocar; (2) parar e retomar; (3) parado e iniciar outra faixa;
(4) pausado e retomar a mesma faixa na mesma posição; (5) troca de faixa após
1 h + 10 min simulados de inatividade; (6) perda do endpoint parado, pausado e
tocando, com reinício e com recriação; (7) arquivo inexistente e arquivo não
decodificável (marcados como falha; saída intacta); (8) 60 ciclos tocar/
pausar/retomar/parar com 20 perdas; (9) worker, visualizador e verificação do
temporizador operacionais após recriação; (10) handles 201 → 201 e threads
3 → 3 após 11 aberturas de dispositivo. Também: falha repetida escalona para
recriação; sem dispositivo, sem laço de tentativas e sem faixas marcadas;
orçamento automático limitado.

## 5. Não validado

* Desconexão física de um fone Bluetooth com a correção (a falha foi
  simulada no ponto exato do miniaudio; o probe real apenas para o
  dispositivo, sem invalidar o endpoint).
* Suspensão/hibernação e retomada do Windows; troca manual do dispositivo
  padrão; reinício do serviço de áudio; dispositivos USB.
* Sessão real de várias horas com o executável final.
* Windows 7–10.
* Asserções com captura de janela (não autorizadas; marcadas PENDING).

## 6. Próximos passos

1. **Validação humana necessária:** abrir o LiteSic (build Release),
   conectar o fone Bluetooth, tocar e parar, desconectar o fone, aguardar
   (de preferência horas) e selecionar uma faixa; repetir com a música
   tocando no momento da desconexão. Esperado: toca na saída padrão atual,
   ou mostra o erro se não houver nenhuma; nunca fica inerte. Enviar
   `%LOCALAPPDATA%\LiteSic\audio.log` (ou `data\audio.log` no modo portátil).
2. Se voltar a aparecer uma instância que não fecha, **não encerrá-la**:
   coletar um despejo (Gerenciador de Tarefas → Criar arquivo de despejo)
   para investigar a instância com 1 thread observada neste incidente.
