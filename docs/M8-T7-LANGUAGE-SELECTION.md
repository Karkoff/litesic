# M8-T7 — idioma do Windows e seleção em tempo real

Entrega em 11/10/2026 (eventos UTC em 12/10). **M8-T7 COMPLETE; M8 IN_PROGRESS.**
Não houve commit/push. T8, M10 e auditoria geral de desempenho não foram iniciadas.
[Evidências, logs e hashes](evidence/m8-language-selection/README.md).

## A. Estado inicial

Branch `master`, HEAD `266270aa9caa2469144f9af30d9609877941f7b4`, alinhada ao
`origin/master` local. A T6 já estava implementada, sem commit: 18 catálogos ×
188 IDs, serviço central, fallback incorporado en-US, Unicode, DirectWrite e
fontes por escrita. Baseline confirmado: 161 casos por variante, 644 no total,
29 guardas e 54 capturas inspecionadas. Código e evidências anteriores preservados;
esta entrega amplia a mesma infraestrutura.

O inventário e a varredura de literais foram revistos. Não restaram textos próprios
visíveis fora do catálogo no escopo atual; símbolos, nomes técnicos, classes Win32,
seções INI e dados de músicas têm exceções documentadas. A T7 acrescentou somente
`appearance.language`, `appearance.auto_language` e `errors.language_change`:
**191 IDs em cada um dos 18 catálogos**, sem IDs ausentes/duplicados, UTF-8 inválido
ou contratos de placeholders divergentes. Isso não equivale a revisão linguística.

## B. Arquitetura utilizada

`src/language_selection.h/.cpp` coordena preferência, Windows, persistência,
comandos e notificação. Reutiliza `I18n::Resolve/Select/Text/Format`, catálogos e
metadados da T6, sem segundo gerenciador de traduções. `Ui::SettingsPath()` expõe
o caminho já configurado pelo mecanismo existente.

`InitializeLanguage` executa após configurar o INI e antes de criar qualquer
janela. A fachada legada da T6 ainda pode inicializar internamente pt-BR, mas a
resolução efetiva ocorre antes da primeira UI: não há janela em idioma transitório.
O argumento de catálogo do auxiliar gráfico é fixture exclusiva de validação;
não substitui o comportamento de produção nem grava preferência manual.

`ChangeLanguage` é a operação central: valida comando, impede reentrância e
execução fora da thread UI, persiste a seção, aplica catálogo, atualiza preferência
e notifica sincronicamente as janelas LiteSic da mesma thread. O serviço da T6
continua protegendo acesso concorrente com `CRITICAL_SECTION`; a notificação não
ocorre com seu lock retido. Textos são valores `std::wstring`, não referências a
catálogos descarregados. Nenhuma operação de localização foi adicionada ao callback
de áudio, DSP ou gerenciamento de janelas.

## C. Detecção automática do Windows

Consulta ordenada de preferências de **exibição**, usando duas chamadas a
[GetUserPreferredUILanguages](https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getuserpreferreduilanguages)
com `MUI_LANGUAGE_NAME`: tamanho, depois MULTI_SZ UTF-16. Valida limites e duplo
NUL; repete até três vezes se o tamanho mudar. Em falha/lista vazia, consulta
[GetUserDefaultUILanguage](https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getuserdefaultuilanguage),
`MAKELCID` e
[LCIDToLocaleName](https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-lcidtolocalename).
Sem resultado utilizável, usa en-US. Não consulta região geográfica ou fuso,
nem modifica preferências Windows.

Requirements oficiais conferidos: GetUserPreferredUILanguages e LCIDToLocaleName
estão disponíveis desde Vista; GetUserDefaultUILanguage desde Windows 2000.
Todas atendem `_WIN32_WINNT=0x0601`; isso não certifica ensaio físico no Windows 7.

Reutiliza integralmente a resolução T6: percorre a lista na ordem, buscando exato
e depois variante compatível em cada entrada. Normaliza caixa e `_`/`-`, rejeita
tags inválidas e escrita explicitamente incompatível. `en-GB, pt-BR` resulta
en-US; `xx-YY, es-MX, ja-JP` resulta es-ES.

| Família | Variante compatível |
|---|---|
| pt | pt-PT exato; demais variantes/pt → pt-BR |
| en, es, fr, de, it | en-US, es-ES, fr-FR, de-DE, it-IT |
| nl, pl, ru, tr, sv | nl-NL, pl-PL, ru-RU, tr-TR, sv-SE |
| ja, ko, ar, hi | ja-JP, ko-KR, ar-SA, hi-IN |
| zh-Hant; zh-TW/HK/MO | zh-TW |
| zh-Hans; zh/zh-CN/SG | zh-CN |
| Chinês com escrita/variante desconhecida | Próxima preferência; não trocar escrita silenciosamente |
| Nenhuma preferência compatível | en-US incorporado |

Escritas esperadas: Latn para famílias latinas, Cyrl/ru, Arab/ar, Deva/hi,
Jpan/ja, Kore/ko. Hans/Hant explícito tem precedência no chinês.
Automático é reavaliado na inicialização e ao selecionar o modo; não há listener
para mudança externa das preferências Windows enquanto o aplicativo está aberto.

## D. Menu e seleção manual

O menu existente do player ganhou o submenu **Idioma**, sem janela nova.
Contém Automático (Windows), separador e os 18 nomes nativos definidos na T6.
Marca somente Automático nesse modo, ou somente a escolha manual. No automático,
o rótulo inclui o idioma efetivamente ativo. No manual, o rótulo Automático indica
o idioma resolvido do Windows, permitindo prever o retorno ao modo automático.
IDs 920 e 921…938 são despachados pela mesma operação central usada nos testes.

Menus são reconstruídos ao abrir, com textos atuais; não se mantêm HMENUs antigos
ou ponteiros de tradução. Comandos desconhecidos são rejeitados sem mudar estado.

## E. Atualização global

`WM_LANGUAGE_CHANGED = WM_APP + 89`, independente da mensagem de tema.

| Componente | Atualização e estado preservado |
|---|---|
| Player | Estados, informações técnicas, rótulos, menus e tooltips; invalidação de desenho sem mudar transporte/posição/volume/balanço/visualizador |
| Playlist | Título nativo, colunas, rodapé, comandos, contagens, estados, cue da pesquisa, direção e fonte; query, caret, seleção e modelo canônico preservados |
| EQ | Título nativo e desenho dos rótulos/preset/estado; bandas, pré, enable e preset preservados |
| Aparência | Título, botões, combo nativo e prévias; categoria, seleção e primeira linha visível preservadas; sem aplicar outro tema |
| Informações | Título, campos, fechar, fonte e direção do EDIT; faixa, caret, rolagem e geometria preservados |
| Erros/resultados assíncronos | Status mantém ID semântico; worker M3U transporta `errorId`; tradução acontece no idioma atual na UI |

O parâmetro opcional `errorId` de Load/SaveM3U conserva as APIs e comportamentos
existentes; não altera parsing, arquivos ou execução da playlist. Nomes reais de
faixas, artistas, álbuns e caminhos continuam sendo dados, sem tradução.

DirectWrite recria os formatos dependentes da geração de idioma conforme T6.
EDITs atualizam direção/fontes e layout; janelas são invalidadas sem recriação.
Mensagens e diálogos nativos recebem textos localizados ao serem abertos; controles
fornecidos pelo Windows podem manter o idioma do SO. Um MessageBox modal já aberto
não é reconstruído durante sua exibição.

Os 16 tooltips existentes mantêm sua instância e regiões. Quinze rótulos são
atualizados com TTM_UPDATETIPTEXTW; o título da faixa permanece callback.
Foi necessário corrigir `TOOLINFO.cbSize` para `TTTOOLINFOW_V2_SIZE`, pois o manifest
atual usa controles comuns legados. O teste lê os textos reais registrados.
Não foram criadas descrições, estados explicativos, comportamento de hover, tema
ou acessibilidade da futura T8.

## F. Persistência

Mesmo `settings.ini` existente (`LOCALAPPDATA/LiteSic` no uso normal), seção:

```ini
[Language]
Mode=manual
Selected=de-DE
```

Instalação nova/valores inválidos usam automático. Não se escreve escolha fictícia
ao iniciar. Retornar ao automático grava `Mode=automatic`; um `Selected` manual
anterior pode permanecer auxiliar e nunca prevalece. Manual válido prevalece
sobre Windows ao reiniciar, antes da criação da UI.

Usa GetPrivateProfileStringW e
[WritePrivateProfileSectionW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilesectionw)
(Windows 2000+), substituindo apenas a seção Language numa operação; as demais
seções permanecem. Falha de escrita retorna erro e conserva catálogo/preferência/UI.
Catálogo manual indisponível/inválido conserva a preferência solicitada e aplica
en-US incorporado com diagnóstico; não deixa UI vazia. Chaves individuais usam
fallback T6 e o validador impede entregar catálogo incompleto.

## G. Unicode e RTL

Mantidos UTF-8 estrito, UTF-16 nas APIs W e fontes Meiryo/YaHei/JhengHei/Malgun
Gothic/Tahoma/Mangal com fallback nativo. Trocar para árabe atualiza direção,
alinhamento e métricas textuais, incluindo pesquisa e informações nativas.
DirectWrite realiza shaping/bidi; os controles Play/Pause/Stop, FFT/OSC, bandas
e indicadores de áudio não são espelhados. Voltar a LTR remove os estilos RTL
dos EDITs, conservando seu conteúdo e seleção. DPI e escala existentes preservados.

## H. Testes funcionais

**171 casos por variante × quatro = 684 PASS**, contra 644 do baseline.
Dez casos novos: oito em `test_language_selection.cpp`, dois em
`test_language_window.cpp`. A soma inclui os testes C, dez FormatTime e um smoke
de dependência, além dos summaries C++; não é contagem de linhas contendo PASS.
As 32 guardas do driver são contadas separadamente, contra 29 na T6.

Cobertura nova: MULTI_SZ/bounds, preferências ordenadas suportadas/regionais/
desconhecidas, fallback, provider real sem supor idioma do host, INI manual/
automático/restart, dados inválidos, comandos, falha de persistência, nomes e
marcas dos 18 itens, rótulo automático independente da escolha manual, 1.980
trocas e estabilidade de memória, geração sem recarga no mesmo idioma.

Testes Win32 criam janelas reais: alternam os 18 idiomas em OSC e FFT, com backend
de áudio determinístico lendo amostras WAV, EQ ligado e preset ativo. Conferem
cursor antes/depois, avanço posterior, estado playing, identidade/conteúdo da
playlist, seleção, query/caret, EQ, tema, posições e acoplamento. Abrem informações
e Aparência; conferem textos nativos/tooltip real, categoria/scroll/seleção,
idioma→tema e tema→idioma. Os testes T6 de catálogos, erros, fallback, renderização
dos 36 temas e escalas continuam passando. Isso não substitui ensaio audível
humano prolongado nem certificação de cada tradução.

## I. Validação gráfica

Sessão real x64 Release, perfil isolado `build/gui/m6-session`, desktop Default,
escala observada 150%. **90 capturas: 18 idiomas × cinco janelas**, mesmo PID
56216, mesmos HWNDs e retângulos, acionando o comando de produção. Nenhuma
preferência pessoal do Windows ou perfil normal do LiteSic foi alterada.

Inspeção representativa de 50 capturas em dez folhas: pt-BR, en-US, de-DE, ru-RU,
ja-JP, zh-CN, zh-TW, ko-KR, ar-SA e hi-IN; originais preservados. Conferidos títulos,
colunas, rodapé, controles, categorias, dados não traduzidos, fontes, RTL e
truncamento. Alemão/russo usam elipse nos espaços fixos, sem sobreposição impeditiva;
menu contém comandos completos. Árabe mostra letras formadas e texto misto;
hindi/CJK sem caixas de glyph ausente. Esta é revisão visual técnica, não linguística.
O teste também cria as janelas inicialmente em árabe e confere os estilos RTL da
pesquisa antes da primeira troca; o EDIT recebe a direção já na criação.

Menu automático e seleção manual foram operados por clique após inspeção;
restart manual em sueco e retorno/restart automático em português foram capturados.
O WAV silencioso foi reproduzido no smoke inicial; terminou antes da matriz das
90 capturas. Em smoke adicional com dispositivo real, o contador avançou de
00:01 para 00:23 com árabe/FFT e 00:52 com hindi/OSC, no mesmo PID 40884, mesma
faixa WAV de silêncio e volume 80%. Não houve avaliação audível dessa fixture.
As provas precisas de cursor/estado durante as 36 trocas OSC/FFT estão nos testes
de janelas/áudio, não são inferidas somente das imagens estáticas.

O manifest distingue o hash do executável das capturas e dos builds finais.
A matriz foi capturada antes dos ajustes finais do rótulo Automático no modo manual
e da direção inicial da pesquisa em árabe; ambos têm teste explícito e smoke
gráfico adicional. Não atribuir todas as imagens ao hash final.
Tentativas recusadas por observação vencida/alvo divergente ficam
nos eventos e não contam como aprovação. Capturas de seletor nativo com diretórios
pessoais não foram exportadas para o repositório.

## J. Desempenho incremental

Medições locais nos logs/manifest: inicialização e carga do catálogo via teste T6;
mudança central sem INI/janelas via 1.980 trocas (180 aquecimento + 1.800 medidas);
notificação de três janelas via 36 trocas. Valores variam com máquina/build e
não incluem custo de disco do INI ou latência até apresentar um frame completo.

<!-- METRICS_TABLE -->

| Variante | Inicialização (ms) | Carga média (ms) | Mudança central (ms) | Notificação UI (ms) | Private Bytes final − aquecido |
|---|---:|---:|---:|---:|---:|
| x64 release | 0.112 | 0.179 | 0.193 | 0.490 | +245,760 B |
| x64 debug | 0.579 | 1.014 | 0.950 | 1.286 | +32,768 B |
| x86 release | 0.132 | 0.235 | 0.249 | 0.566 | +163,840 B |
| x86 debug | 0.568 | 0.990 | 1.050 | 1.485 | -73,728 B |
<!-- END_METRICS_TABLE -->

São mantidos fallback inglês incorporado e somente catálogo ativo: dois conjuntos,
nunca os 18 carregados permanentemente; payload máximo ativo 18.356 bytes (não
inclui overhead de containers). Cache textual invalida por geração. Private Bytes
após aquecimento permanecem dentro do limite incremental de 512 KiB; GDI 19→19 e
USER 20→20 nos quatro testes de 36 trocas. Não houve crescimento descontrolado
nesses ensaios limitados. Sem alegação de prova absoluta de ausência de leaks ou
auditoria geral de desempenho, que continua futura.

## K. Builds

Quatro builds e suites completas aprovadas, MinGW MSVCRT estático, C++17,
`_WIN32_WINNT=0x0601`. Nenhuma dependência nova de runtime.
Comandos: `scripts/dev.ps1 -Action build|test -Architecture 64|32`, acrescentando
`-DebugBuild` no Debug. Logs UTF-8, tamanhos e SHA-256 no manifest de evidências.
Warnings preexistentes de casts DPI, helpers GDI não usados e miniaudio x86 constam
nos logs completos; nenhum warning novo do módulo T7.

| Variante | Executável | Casos |
|---|---|---:|
| x64 Release | `build/64/release/LiteSic.exe` | 171 PASS |
| x64 Debug | `build/64/debug/LiteSic.exe` | 171 PASS |
| x86 Release | `build/32/release/LiteSic.exe` | 171 PASS |
| x86 Debug | `build/32/debug/LiteSic.exe` | 171 PASS |

## L. Pendências

Revisão humana de naturalidade/terminologia/plurais nos 18 idiomas, acessibilidade
e bidi complexo; ensaios físicos Windows 7/8/8.1/10 e por arquitetura conforme
decisão M9 preservada. Não se modificaram idiomas Windows para simular outros
hosts: cenários de preferências usam provider injetável. Matriz física multi-DPI
não executada nesta T7; cobertura de escalas T6 retida. Tooltips explicativos T8,
residuais T1…T5, checkpoint M8, distribuição M10 e auditoria geral continuam abertos.

## M. Situação formal da M8-T7

**COMPLETE.** Detecção/default automático, menu, 18 escolhas marcadas, troca
imediata global, persistência, fallback, Unicode/tipografia, preservação de estado,
quatro builds e testes relevantes atendidos, sem regressão bloqueante observada.
Rastreabilidade atualizada no milestone, contrato e auditoria. **M8 segue aberta**;
conclusão técnica T7 não é aceite linguístico humano nem conclusão de T8/M10.

## N. Estado final do Git

HEAD preservado `266270aa9caa2469144f9af30d9609877941f7b4`, branch `master`, sem
commit/push. Alterações locais T6 conservadas e ampliadas pelos arquivos T7;
seu diretório de evidências permanece histórico. Working tree contém código,
catálogos, testes, automação e documentação T6/T7 ainda não commitados.
Builds/temporários permanecem ignorados; evidências selecionadas ficam em
`docs/evidence/m8-language-selection`. `git diff --check` executado ao finalizar.
