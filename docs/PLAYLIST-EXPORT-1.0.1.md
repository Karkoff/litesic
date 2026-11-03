# Correção de exportação de playlists — 1.0.1

Solicitação de 03/11/2026: corrigir o salvamento M3U das coleções Geral e
Favoritas e distribuir a atualização no GitHub. Correção de patch sobre 1.0.0;
VERSIONINFO 1.0.1.0. A publicação anterior e a preparação da Microsoft Store
conservam seus registros históricos.

## Problemas e correções

- A janela sempre copiava a playlist inteira para exportação, inclusive na aba
  Favoritas. Agora usa a coleção ativa: Geral completa ou somente as favoritas,
  na ordem canônica, independentemente da busca e das linhas selecionadas.
- A exportação compartilhava o cancelamento dos trabalhos de metadados:
  Limpar descartava salvamentos pendentes e fechar encerrava o worker antes
  de gravá-los. Exportações aceitas agora sobrevivem à troca de geração e
  são concluídas antes da liberação da fila no encerramento normal.
- Resultados de exportação continuam chegando à interface depois de Limpar,
  inclusive erros. Metadados e importações antigos continuam canceláveis.
- O diálogo oferece tipos separados M3U e M3U8, M3U como padrão e sugestão de
  nome/título conforme a aba. Falhas do diálogo deixam de ser tratadas como
  cancelamento silencioso.

Os dois formatos continuam usando Extended M3U em UTF-8 sem BOM, caminhos
absolutos, títulos, durações e comentários `#LITESIC-FAVORITE:1`. Outros players
podem ignorar esse comentário; ao reabrir no LiteSic as estrelas são preservadas.
A sessão automática continua guardando a lista completa com suas marcações.
Não há exportação automática de um segundo arquivo ao salvar uma aba.

## Validação

O novo caso `accepted_exports_survive_clear_and_shutdown` falhou antes da
correção em `LoadM3U(path,loaded,error)`: o arquivo aceito não havia sido gravado.
Depois passou, cobrindo encerramento imediato com metadados pendentes e
Limpar seguido da entrega do resultado de salvamento.

`export_general_and_favorites_m3u` usa a janela real e a mesma rotina de
exportação chamada após a escolha do destino, o worker e o gravador atômico.
Confere Geral, Favoritas, M3U8, favoritos vazios e busca sem resultados;
reabre os arquivos, verifica ordem/estrelas e que a lista original não foi
modificada. A rotina também rejeita chamadas sem janela ativa.

As suítes são executadas com `LITESIC_NO_CAPTURE=1`, sem imagens de diálogos,
desktop ou pastas pessoais. A tentativa de automatizar o diálogo comum do
Windows não concluiu neste ambiente e não é contada como aprovação visual
ou teste completo do seletor de arquivos. A integração validada começa na
rotina invocada pelo diálogo após a escolha do destino.

Release x64 e x86: **32 executáveis de teste e 186 casos PASS por arquitetura**,
372 casos no total. Quatro ZIPs extraídos e **16 inicializações reais PASS**,
incluindo reprodução, persistência, modo portátil sem flags, mudança para
caminho Unicode, 18 catálogos e fallback inglês. O perfil pessoal e o diretório
de trabalho alheio permaneceram inalterados. Arquiteturas PE, VERSIONINFO
1.0.1.0, conteúdo e hashes foram conferidos. Permanecem somente os avisos já
existentes no miniaudio vendorizado, sem novos avisos nos arquivos alterados.

[Inventário, hashes e resultados verificáveis](evidence/playlist-export-1.0.1/summary.json).
Logs integrais: `build/desktop/v1.0.1-20261103-165023-310/`.
Os quatro ZIPs e SHA256SUMS.txt estão preparados para a release
[`v1.0.1`](https://github.com/Karkoff/litesic/releases/tag/v1.0.1).
A confirmação da publicação será registrada após o envio.

## Reprodução da distribuição

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/package-desktop.ps1 -Version 1.0.1
./.tools/gui-venv/Scripts/python.exe scripts/diagnostics/validate_desktop_packages.py build/desktop/v1.0.1-<timestamp>
```

O procedimento gera quatro ZIPs, x64/x86 padrão e portátil, e SHA256SUMS.txt.
Executáveis validados desta rodada:

- x64: `build/desktop/v1.0.1-20261103-165023-310/native/64/release/LiteSic.exe`
- x86: `build/desktop/v1.0.1-20261103-165023-310/native/32/release/LiteSic.exe`
