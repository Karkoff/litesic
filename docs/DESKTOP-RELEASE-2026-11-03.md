# Release desktop 1.0.0 — 03/11/2026

Distribuição tradicional pelo [GitHub Releases](https://github.com/Karkoff/litesic/releases/tag/v1.0.0):
ZIPs x64 e x86, cada um em edição padrão e portátil. Os quatro pacotes contêm
o mesmo executável por arquitetura, 18 catálogos, ajuda PT-BR/EN-US e licenças
do projeto, miniaudio, stb_vorbis e runtimes estáticos GCC/MinGW-w64.

O recurso de versão nativo passa a 1.0.0.0 e usa o identificador numérico 1,
necessário para a leitura pelas APIs de versão do Windows. A identidade e o
bundle Microsoft Store anteriormente preparados não são alterados.

## Modo portátil

O arquivo `resources/portable.flag` seleciona `data/` junto ao executável,
inclusive com duplo clique e com um diretório de trabalho diferente. Um
diretório chamado `portable.flag` não ativa o modo. A opção `--portable`
continua disponível. Perfis explícitos de diagnóstico, modo de validação e
LocalState de MSIX mantêm sua precedência. A edição padrão não contém o marcador
e conserva `%LOCALAPPDATA%\LiteSic`.

Nenhum perfil é migrado automaticamente. Playlists ainda usam caminhos
absolutos dos arquivos de áudio; mover as músicas pode exigir atualizar a lista.
O programa solicita uma pasta gravável no modo portátil e não troca
silenciosamente para o perfil pessoal quando a escrita falha.

## Gerar os pacotes

No ambiente MSYS2 MINGW64/MINGW32 documentado em [DEVELOPMENT.md](DEVELOPMENT.md):

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/package-desktop.ps1 -Version 1.0.0
```

O script cria uma pasta nova em `build/desktop/v1.0.0-<timestamp>/`, compila
Release x64/x86 e executa as respectivas suítes com `LITESIC_NO_CAPTURE=1`.
`BUILD_DIR` e `INTERNAL_DIR` são isolados; o executável em uso pelo usuário
em `build/64/release` não é substituído nem encerrado. O Makefile continua
aceitando os comandos tradicionais `make ARCH=64 DEBUG=0` e
`make ARCH=32 DEBUG=0`.

O empacotamento audita arquitetura PE, versão e importações; admite somente
DLLs do Windows e não as redistribui. Uma lista permitida limita o conteúdo
dos ZIPs, que são extraídos novamente e conferidos por SHA-256. As entradas
têm ordenação e timestamp fixos. `manifest.json`, logs e intermediários ficam
fora dos downloads. Apenas os quatro ZIPs e `SHA256SUMS.txt` são anexos da release.

Estrutura após extração:

```text
LiteSic.exe
locales/<idioma>/messages.tsv
resources/README.pt-BR.txt
resources/README.en.txt
resources/portable.flag          # somente na edição portátil
licenses/*.txt
data/                           # criada em execução no modo portátil
```

## Verificar os ZIPs finais

Use o Python de diagnóstico já preparado no projeto:

```powershell
./.tools/gui-venv/Scripts/python.exe scripts/diagnostics/validate_desktop_packages.py build/desktop/v1.0.0-<timestamp>
```

O diagnóstico extrai os quatro arquivos em pastas novas, com espaços e Unicode,
inicia cada executável a partir de outro diretório e usa somente fixtures
sintéticas. As edições portáteis são iniciadas sem `--portable` ou `--profile`;
as edições padrão usam um perfil explícito descartável para preservar a sessão
pessoal. São verificados reprodução real via avanço MSAA, PT-BR, fallback inglês
sem catálogos, 18 catálogos íntegros, ícone, Direct2D/DirectWrite, DLLs do sistema,
persistência, reinício e mudança de pasta dos portáteis. Não usa capturas nem
entrada no desktop pessoal.

## Resultado desta entrega

Release x64 e x86: **32 executáveis de teste e 184 casos PASS por arquitetura**
(368 casos no total), com capturas desabilitadas. O caso novo cobre presença
do marcador, rejeição de diretório como marcador e precedência de perfil/MSIX.
As duas arquiteturas PE e o VERSIONINFO 1.0.0.0 passaram na auditoria.

Os quatro ZIPs passaram em **16 inicializações reais**, com quatro verificações
de avanço da reprodução. Portáteis abriram sem flags, persistiram volume,
idioma e sessão e reabriram após mudança para um caminho Unicode. O perfil
pessoal e o diretório de trabalho alheio permaneceram inalterados. Os 18
catálogos foram conferidos em cada ZIP; fallback inglês sem catálogos passou
em x64 e x86. Ícone e bibliotecas gráficas foram verificados.

As importações PE contêm apenas DLLs do Windows. Esta máquina também injeta
módulos de overlay RivaTuner e antivírus Kaspersky em processos gráficos;
eles foram observados separadamente, não acompanham os ZIPs e não são
dependências declaradas pelo player. Não houve mudança na configuração
desses programas. Os avisos do GCC no header vendorizado miniaudio permanecem;
nenhum aviso novo apareceu nos arquivos alterados.

[Resumo verificável, inventário e hashes](evidence/desktop-release-2026-11-03/summary.json).
`SHA256SUMS.txt` acompanha a release. Logs integrais e resultados locais ficam
em `build/desktop/v1.0.0-20261103-162108-091/`, fora dos artefatos públicos.

O teste formal de duas builds limpas de M10-T1 e a validação humana dos ZIPs
de M10-T2 continuam com seus critérios próprios. Esta entrega não declara
concluída a M10 nem publica na Microsoft Store. Windows 7–10, instalação por
Inno Setup e sessão prolongada não foram validados nesta rodada.
