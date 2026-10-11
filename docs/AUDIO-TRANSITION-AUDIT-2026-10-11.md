# Auditoria: encerramento na transição de músicas — 11/10/2026

## Resultado e causa

Reproduzida a falha na instância debug 64 bits deixada aberta pelo usuário.
O depurador capturou SIGSEGV em `msvcrt!memmove`, chamado pelo reservatório
MP3 do miniaudio durante uma consulta de duração no redesenho do LCD:

`WM_PAINT → PlayerSnapshot → Audio_GetDurationSeconds → ma_sound_get_length_in_pcm_frames → ma_decoder_get_length_in_pcm_frames → ma_dr_mp3_get_pcm_frame_count → seek/decodificação → memmove`.

O sound usa dados MP3 codificados. Consultar o comprimento pode percorrer
os frames e restaurar a posição do decodificador. A UI fazia isso em cada
redesenho/consulta de duração enquanto o dispositivo de áudio decodificava
o mesmo sound. Decodificadores não são seguros para uso concorrente sem
sincronização, conforme a documentação embutida em `third_party/miniaudio.h`.
A consulta também explicava o bloqueio perceptível da UI. A rolagem do título
e o visualizador aumentam a frequência de desenhos, mas não são a origem
da manipulação concorrente do decodificador.

Os eventos Application Error do Windows registravam `0xc0000005` em
`msvcrt.dll`, compatível com a pilha capturada. Isso identifica o ponto da
falha, não um defeito comprovado no runtime do Windows.

## Auditoria dos arquivos e da navegação

Walk the Moon (198,870 s) e The Climb (231,024 s) foram carregadas e
decodificadas integralmente em processo separado, sem falha. A leitura de
metadados das 1.280 entradas da playlist também terminou sem erro fatal.
Sessenta carregamentos/transições com dispositivo real, sem a consulta
contínua de duração da UI, terminaram normalmente.

Não foi encontrada evidência de defeito nessas duas músicas que explique
o encerramento; a falha do aplicativo foi capturada diretamente. Isso não
certifica integridade de todos os arquivos da coleção.

`PlaybackController::Next` já pula entradas cujo carregamento retorna erro,
limitando tentativas ao tamanho da lista. Não foi necessário mudar essa
política. O crash ocorria na consulta de duração, antes que o fluxo normal
pudesse tratar uma falha de carregamento.

## Correção

`Audio_Load` calcula uma vez a duração com um decodificador independente,
antes da reprodução. Para Vorbis Unicode, a leitura independente de
metadados conserva o fallback pelo granule final. A duração fica armazenada
no estado de áudio e é zerada ao descarregar, encerrar ou iniciar novo load.
`Audio_GetDurationSeconds` só devolve esse valor; não consulta nem reposiciona
o decodificador ativo. Seek mantém a conversão pela taxa de saída do sound.
Não há bloqueio adicional no callback nem alteração do DSP do equalizador.

## Verificação

- Quatro builds: debug/release, x86/x64, sem erro de compilação.
- Quatro suítes completas: 111 verificações por variante, 444 no total.
- Novo teste: 10.000 consultas de duração durante leitura MP3 em outra thread,
  verificando duração estável, EOF e limpeza do cache.
- Novo teste: EOF de MP3, arquivo inválido intermediário e reprodução da
  próxima faixa com o backend real, marcando a entrada inválida como falha.
- Na interface: Walk the Moon → The Climb, permanência em reprodução com
  LCD/visualizador ativos e transição seguinte, usando sessão isolada.

[Manifesto e evidências](evidence/audio-transition/manifest.json).
O extrato da pilha publicado omite bytes do MP3 e caminhos pessoais.
Arquivos musicais não foram alterados nem incluídos no repositório.

Esta correção não constitui validação por sistema/arquitetura da M9.
Seu encerramento por decisão e os testes práticos futuros permanecem
conforme [PROJECT_STATE](../PROJECT_STATE.md).
