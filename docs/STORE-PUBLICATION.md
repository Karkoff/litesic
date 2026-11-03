# Publicação do LiteSic na Microsoft Store

Preparação de 03/11/2026. **Submission 1 criada no Partner Center, em rascunho;
não enviada para certificação e não publicada.** O pacote foi preparado e
validado localmente; a conclusão dos campos e do upload no portal permanece
pendente. Estado verificável: [STORE-STATUS.json](STORE-STATUS.json).

## Identidade e versão

- Nome: LiteSic; publicador: Brasiliense; Store ID: `9NJ52QFN0LMX`.
- Identity Name: `Brasiliense.LiteSic`.
- Publisher: `CN=90CC5C0C-B20C-4457-BDE7-D0126388DE3C`.
- PFN oficial: `Brasiliense.LiteSic_t2m7qap71167r`.
- Identidade fornecida pelo proprietário e conferida na página real do produto.
- Versão candidata do MSIX: **1.0.0.0**, x86 e x64. Não foi enviada.
- O código-fonte de VERSIONINFO tradicional conserva `0.1.0.0`. A versão do
  pacote é independente: a Store exige o primeiro componente maior que zero e
  reserva o quarto componente. Conferir o histórico do produto antes de atualizar.
- O portal confirmou que este é o primeiro rascunho, sem publicação existente.

## Auditoria e preservação da distribuição tradicional

Player C++17/Win32, Direct2D/DirectWrite, miniaudio e stb_vorbis vendorizados.
Build: Makefile com MSYS2 MINGW64/MINGW32 (MSVCRT), ligação estática; sem .NET,
WinUI, Windows App SDK ou runtime adicional no aplicativo. As importações PE
das duas arquiteturas contêm somente DLLs do Windows; nenhuma DLL do sistema é
copiada. Backends de áudio continuam WASAPI, DirectSound e WinMM.

O MSIX inclui o executável, os 18 catálogos TSV, recursos visuais, `resources.pri`
e licenças. Não inclui fixtures, músicas, testes, ferramentas, logs ou perfis.
Licenças vendorizadas e notices dos runtimes GCC/MinGW acompanham o pacote.
O arquivo LICENSE materializa a licença MIT já declarada no README.

A única adaptação no player é a seleção de um perfil separado quando há
identidade de pacote: `%LOCALAPPDATA%\Packages\<PFN>\LocalState\LiteSic`.
Ela evita compartilhar sessão, mutex e preferências com a instalação tradicional.
`GetCurrentPackageFamilyName` é resolvida dinamicamente; no Windows 7 a API
ausente mantém o comportamento anterior. Não há nova importação estática
da API. Preferências tradicionais continuam em `%LOCALAPPDATA%\LiteSic`, e
`--portable` continua usando `data` junto ao executável não empacotado.
Em MSIX, o perfil normal permanece em LocalState mesmo com `--portable`.

O script gera as releases da Store em `build/store-native/{64,32}/release` e
os intermediários em `build/internal/store-native`. Isso preserva os executáveis
tradicionais homologados, inclusive quando um deles está em uso. Seus hashes
anteriores foram conferidos e permanecem inalterados.

MSIX declara Windows Desktop 10.0.17763.0 ou posterior (Windows 10 1809), com
teste nesta máquina Windows 11 build 26200. O objetivo Windows 7–11 da
distribuição tradicional e as ressalvas anteriores de compatibilidade continuam
válidos; não houve novo teste em Windows 7–10 nesta preparação.

## Empacotar uma atualização

Ferramentas apenas na máquina de desenvolvimento: MSYS2 MINGW64/MINGW32,
Windows SDK com MakeAppx/MakePri, PowerShell e o Python/Pillow já usados pelo
auxiliar de GUI (`scripts/gui.ps1 -Action setup`). Nenhuma delas acompanha o app.
Também é possível indicar `-Msys2Root`, `-SdkBin` e `-AssetPython`.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/package-store.ps1 -Version 1.0.0.0
```

O script usa `packaging/store/identity.json`, recompila as duas releases,
audita PE/dependências, copia somente arquivos permitidos, gera imagens e PRI,
valida com MakeAppx sem `/nv`, empacota e desempacota ambas as arquiteturas,
confere todos os hashes e gera o bundle. Cada execução tem diretório novo em
`build/store`; nenhuma saída anterior é removida. O manifesto usa
`Windows.FullTrustApplication`, `asInvoker` no EXE e apenas `runFullTrust`.

Não comprar certificado. O bundle de produção fica **sem assinatura local**:
a Microsoft aplica sua assinatura durante a publicação. Não adicionar o OID
especial de testes à identidade oficial.

O script também prepara `listing/` com descrições PT-BR/EN-US, ícones, capturas,
política de privacidade e notas de certificação. Os materiais são revisáveis;
o script não faz upload nem declara publicação. `package-evidence.json` registra
identidade, versões, arquivos, hashes, arquiteturas e status inicial pendente.

## Testar o pacote

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-store-wack.ps1 -PackagePath <pacote-x64.msix> -ReportDirectory <pasta-nova-x64>
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/test-store-wack.ps1 -PackagePath <pacote-x86.msix> -ReportDirectory <pasta-nova-x86>
```

O WACK deve executar na sessão interativa. O script aguarda o processo e exige
relatório completo, saída zero e **todos** os testes PASS. Isso inclui testes
opcionais: o kit inicialmente informou OVERALL_RESULT=PASS, mas falhou no ícone
de 600 px acima de 200 KiB. O ícone foi otimizado e ambas as arquiteturas passaram
nos 24 testes após a correção. Resumos e hashes dos XML originais estão em
[evidence/store-2026-11-03](evidence/store-2026-11-03/README.md).

Para instalar sem modificar configurações globais nem confiar em certificado,
foi usado o recurso oficial de teste sem assinatura do Windows 11, com identidade
**separada** `LiteSic.LocalValidation`. Gerar com `-ValidationOnly`; o Publisher
inclui o OID exigido pela Microsoft. Instalar em PowerShell elevado:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/install-store-validation.ps1 -PackagePath <pacote-validation.msix> -ResultPath <caminho-em-build/store/resultado.json>
```

`-ReplaceInstalledValidation` executa o ensaio de desinstalação/reinstalação,
limitado à identidade descartável. Ela nunca é enviada à Store. O EXE de teste
é idêntico ao de produção por SHA-256; a identidade e o PRI de pacote diferem.

Reprodução WAV/MP3/FLAC/OGG, pausa, seek, volume, EQ, favoritos, exportação de
playlist e restauração de configurações/sessão foram exercitados na x64.
A x86 também abriu instalada, reproduziu os quatro formatos e preservou playlist,
volume e EQ ao reiniciar. A troca x64 → x86 removeu o perfil de teste e voltou
aos padrões. A desinstalação final verifica a preservação dos arquivos externos.
As suítes nativas Release passaram nas duas arquiteturas. Não confundir captura
do relógio/visualizador com uma avaliação humana da qualidade sonora.

O auxiliar de GUI restrito é `scripts/gui/store_gui.py`: verifica WindowsApps,
família de pacote, PID, criação e SHA-256. Sua sessão fica em `build/store/gui`.
Ele reutiliza o driver existente; o primeiro attach exige um PID observado.
Para as capturas foram usados somente sons originais gerados por
`prepare_store_demo.py`, também convertidos com o FFmpeg existente para exercitar
os quatro codecs. FFmpeg não é dependência de build nem de execução do app.

## Capturas e privacidade

As quatro imagens em `packaging/store/screenshots` têm 1920×1080. São capturas
PrintWindow/GDI das janelas reais instaladas, em sua resolução original e nas
posições relativas reais, sobre fundo liso; não há UI reconstruída nem sobreposição
de marketing. O osciloscópio e o FFT estão em reprodução real. Os títulos são
genéricos e o áudio é original. A composição é declarada no manifesto de evidência.

**Não capturar diálogos de arquivos, pastas pessoais ou o desktop.** A regra foi
solicitada expressamente pelo proprietário. As duas imagens iniciais de diálogos
foram excluídas de `build/store/gui`; nunca foram adicionadas ao Git ou à Store.
O wrapper agora retorna somente geometria para `#32770`, sem ler seus pixels ou
textos internos. Há teste de regressão desta proteção. Rever todas as imagens
antes de publicar e refazê-las se a interface mudar.

## Concluir a primeira publicação

No Partner Center, concluir **Submission 1**, já criada: verificar preço Free,
público e mercados; categoria Music; classificação etária; upload do bundle
oficial; descrições, recursos e imagens PT-BR/EN-US; política e suporte; justificativa
de `runFullTrust`; publicar assim que a certificação aprovar. O questionário deve
refletir o app local, sem conteúdo fornecido, comunicação, anúncios ou compras.
Qualquer contrato/aceite pessoal, CAPTCHA ou MFA é intervenção do proprietário.

O portal ainda não recebeu os pacotes ou materiais. Não houve envio oficial,
certificação da Microsoft ou confirmação de publicação. O status permanece
rascunho. Os links de suporte e privacidade passam a existir no GitHub após push;
conferir acesso público antes de registrá-los no portal.

## CLI/API: conclusão da pesquisa

Existe o **Microsoft Store Developer CLI (`msstore`)**, executável pelo terminal
do VS Code. A documentação atual exige **primeira publicação concluída pelo
Partner Center** e autenticação Microsoft Entra ID; a sessão de conta Microsoft
no Chrome não configura essa autenticação. O CLI automatiza as atualizações
posteriores, metadados, upload e acompanhamento. Não foi instalado nesta rodada,
pois não resolve o primeiro envio pendente.

A API REST também exige diretório Entra/Azure AD, aplicativo associado e uma
submissão inicial no portal com questionário etário preenchido. Não elimina esse
cadastro. Não gerar chaves, copiar cookies ou registrar credenciais no repositório.
Para futuras atualizações, configurar a autenticação diretamente no ambiente do
proprietário e usar os comandos oficiais `msstore submission get`, `publish` e
`poll`. Recuperar o JSON atual antes de modificá-lo; o CLI pode recriar rascunhos,
por isso não executar `msstore publish` sobre metadados preparados sem revisão.

## Referências oficiais consultadas em 03/11/2026

- [Empacotamento manual e manifesto](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-manual-conversion).
- [Requisitos, versões e assinatura pela Store](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/app-package-requirements?pivots=store-installer-msix).
- [Dados e virtualização de apps desktop](https://learn.microsoft.com/en-us/windows/msix/desktop/desktop-to-uwp-behind-the-scenes).
- [GetCurrentPackageFamilyName: Windows 8+, com resolução dinâmica para Win7](https://learn.microsoft.com/en-us/windows/win32/api/appmodel/nf-appmodel-getcurrentpackagefamilyname).
- [Teste de MSIX sem assinatura no Windows 11](https://learn.microsoft.com/en-us/windows/msix/package/unsigned-package).
- [Windows App Certification Kit](https://learn.microsoft.com/en-us/windows/uwp/debug-test-perf/windows-app-certification-kit).
- [Imagens da listagem MSIX](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/screenshots-and-images).
- [Criar submissão MSIX](https://learn.microsoft.com/en-us/windows/apps/publish/publish-your-app/msix/create-app-submission).
- [CLI e limitação da primeira publicação](https://learn.microsoft.com/en-us/windows/apps/publish/msstore-dev-cli/overview).
- [Comandos do CLI](https://learn.microsoft.com/en-us/windows/apps/publish/msstore-dev-cli/commands).
- [Pré-requisitos da API REST](https://learn.microsoft.com/en-us/windows/uwp/monetize/create-and-manage-submissions-using-windows-store-services).
