# M8 — internacionalização integral e tooltips

Planejamento aprovado em 11/10/2026. **PENDING; nenhuma implementação neste ciclo.**
Complementa M8-T1…T5, preservando atalhos, mídia, sempre no topo, instância
única, persistência, DPI, escalonamento e refinamento visual/acessibilidade.

## Entregas e proposta inicial de idiomas

M8-T6: catálogo, inventário integral de strings, Unicode, layout e 18 traduções.
M8-T7: resolução automática/manual, menu existente, atualização e persistência.
M8-T8: tooltips explicativos localizados, temáticos e acessíveis.
Executar nessa ordem, integrando testes e revisão visual/linguística em cada entrega.
Adicionar idiomas futuramente deve exigir apenas catálogo e metadados, sem
mudança estrutural. A quantidade 18 usa Winamp como referência de abrangência;
esta proposta não afirma reproduzir sua lista histórica.

| BCP 47 | Nome no seletor |
|---|---|
| pt-BR | Português (Brasil) |
| en-US | English |
| es-ES | Español |
| fr-FR | Français |
| de-DE | Deutsch |
| it-IT | Italiano |
| pt-PT | Português (Portugal) |
| nl-NL | Nederlands |
| pl-PL | Polski |
| ru-RU | Русский |
| ja-JP | 日本語 |
| zh-Hans | 简体中文 |
| zh-Hant | 繁體中文 |
| ko-KR | 한국어 |
| tr-TR | Türkçe |
| ar | العربية |
| hi-IN | हिन्दी |
| sv-SE | Svenska |

## Arquitetura e resolução

Projetar um serviço leve central de localização adequado ao C++17/Win32,
com identificadores estáveis como `player.play`, `playlist.add_files` e
`equalizer.reset`. Catálogos externos UTF-8 são a opção a avaliar para
`locales/`; validar codificação e converter corretamente para UTF-16 nas APIs W.
Separar catálogo, idioma ativo, resolução, persistência e notificação das
janelas. Reutilizar configuração existente; não criar INIs concorrentes nem
introduzir framework grande ou framework adicional de UI.

Instalações novas usam **Automático (Windows)**. Consultar a lista ordenada
de preferências de exibição do usuário com
[GetUserPreferredUILanguages](https://learn.microsoft.com/en-us/windows/win32/api/winnls/nf-winnls-getuserpreferreduilanguages)
e `MUI_LANGUAGE_NAME`; considerar `GetUserDefaultUILanguage` com conversão
apropriada como contingência se a consulta falhar. Não inferir idioma por
país, região, fuso ou formato numérico. Windows em inglês no Brasil deve
produzir interface em inglês.

Política planejada: percorrer preferências na ordem fornecida; para cada uma,
procurar correspondência exata, depois variante compatível do mesmo idioma
e escrita; só após esgotar a lista usar **en-US**, fallback do projeto.
Mapeamento determinístico de variantes: espanhol → es-ES, inglês → en-US,
português sem variante disponível → pt-BR; preservar pt-PT quando exato.
Chinês exige distinguir Hans/Hant (por exemplo zh-CN → zh-Hans,
zh-TW → zh-Hant), sem trocar escrita silenciosamente. Documentar a tabela
completa na implementação e testá-la. Exemplos: pt-BR → pt-BR,
en-US → en-US, es-ES → es-ES, es-MX → es-ES; idioma sem catálogo → en-US.
Chave ausente usa texto en-US, nunca campo vazio ou identificador exposto;
o validador deve impedir entrega com chaves ausentes, mesmo havendo fallback.

Persistir separadamente modo (`automatic`/`manual`) e código manual.
Manual prevalece sobre Windows; retornar ao automático deve ser possível
a qualquer momento. Reavaliar preferências ao iniciar no modo automático.
Integrar **Idioma** ao menu de contexto real, sem reconstruí-lo: opção
Automático (Windows) e os 18 nomes próprios, com marca na seleção ativa
(no automático, marcar o modo e tornar identificável o idioma resolvido).

Troca em tempo real notifica player, playlist, EQ, menus, tooltips, títulos,
rótulos, diálogos próprios, mensagens/notificações e demais componentes;
recalcula textos/layouts e redesenha sem reiniciar. Não interromper reprodução,
recarregar/modificar playlist, reinicializar DSP ou alterar posições das janelas.

## Cobertura e tipografia

Inventariar todos os textos próprios, inclusive os de caminhos raros/erros:

- Player: botões, informações técnicas, estados, indicadores e controles auxiliares.
- Playlist: menus/botões, colunas, pesquisa, lista vazia, contagens/plurais,
  seleção, favoritos, importar/exportar, erros e informações das faixas.
- EQ: ativar/desativar, presets, redefinir, pré-amplificação, bandas e estados.
- Aparência/configuração: temas, categorias, descrições, idiomas, escala,
  visibilidade e preferências.
- Transversais: contexto, ajuda, tooltips, Sobre/informações do programa,
  diálogos próprios, erros/arquivos inválidos, avisos, confirmações,
  notificações, carregamento, operações concluídas e mensagens informativas.

Nomes de músicas, artistas, álbuns e arquivos são dados e não serão traduzidos.
Seletores e outros diálogos fornecidos pelo Windows podem manter o idioma
do sistema; documentar essa diferença e localizar os textos fornecidos pelo LiteSic.
Definir contratos de placeholders/tipos, contagens/plurais e parâmetros,
sem concatenações que imponham gramática de uma única língua.

Reutilizar DirectWrite e avaliar seu fallback tipográfico para CJK, árabe e
devanágari (Hindi). Validar glyphs, formação de letras e texto bidirecional,
incluindo números/caminhos dentro de árabe. Ajustar direção de leitura e
alinhamento nos componentes pertinentes; desenhar Unicode apenas em LTR
não atende RTL. Referências oficiais:
[DirectWrite](https://learn.microsoft.com/en-us/windows/win32/directwrite/introducing-directwrite) e
[direção de leitura](https://learn.microsoft.com/en-us/windows/win32/directwrite/how-to-ensure-text-is-displayed-with-the-correct-reading-direction).
Manter Windows 7–11: APIs posteriores exigem detecção e fallback; não tornar
interfaces recentes de fontes um import obrigatório. Testes físicos futuros
continuam sob a decisão M9, sem reabri-la.

Traduções longas, especialmente alemão/russo, exigem largura mínima,
medição, alinhamento, truncamento adequado com acesso ao texto completo e
layout responsivo. Não diminuir arbitrariamente fontes. Preservar zoom,
hit-test, docking e DPI por monitor; testar 96/120/144/192 DPI, escalas
manuais existentes, telas pequenas e os 36 temas.

## Tooltips (M8-T8)

Inventariar botões/controles reais; não criar controles a partir de exemplos.
Textos breves e claros, no mesmo catálogo dos 18 idiomas. Preferir mecanismo
nativo Win32 ou componente leve que atenda ao tema atual e ao desenho existente.
Hover com breve atraso; sair do controle oculta; não capturar interação nem
afetar áudio. Posicionar dentro da área útil do monitor, evitando cobrir
desnecessariamente o controle; acompanhar DPI/zoom e idioma em tempo real.
Identificação também por tecnologias de acessibilidade quando aplicável;
tooltip não deve ser o único nome acessível do controle.

Descrições podem refletir estado: shuffle desligado “Ativar reprodução
aleatória”; ligado “Desativar reprodução aleatória”; repeat “Repetição:
desligada/lista/faixa atual”. Localizar esses estados no catálogo comum.

## Critérios objetivos de conclusão da internacionalização

1. Os 18 idiomas disponíveis e selecionáveis.
2. Detecção automática da lista Windows correta, independente da região.
3. Menu Idioma integrado ao existente, com indicação ativa.
4. Troca manual sem reiniciar, atualizando todos os componentes.
5. Persistência de modo/preferência, precedência manual e retorno ao automático.
6. Todos os textos próprios da aplicação internacionalizados.
7. Tooltips traduzidos, temáticos, com hover/saída, DPI e limites de monitor corretos.
8. Nenhum identificador ausente/duplicado ou campo vazio nos catálogos entregues.
9. Placeholders, tipos e parâmetros compatíveis; plurais/contagens corretos.
10. Fallback de idioma, escrita e chave previsível e testado.
11. Interface legível em todos os idiomas e escalas, sem cortes impeditivos.
12. Fontes cobrem os sistemas de escrita; CJK, Hindi, shaping árabe/RTL verificados.
13. Quatro builds x64/x86 Debug/Release aprovados.
14. Testes relevantes aprovados: catálogos, UTF-8 inválido, resolução ordenada,
    variantes, fallback, roundtrip e troca ao vivo. Incluir inventário/varredura
    automatizada de textos fixos não localizados, com exceções justificadas
    para dados, símbolos técnicos e strings de sistema; validar também caminhos
    de erro e estados de UI, sem confundir igualdade com inglês com falta de tradução.
15. Sem regressão de reprodução: áudio contínuo, DSP preservado, playlist e
    posições inalteradas durante trocas; nenhuma atividade de localização no callback.

Cobertura técnica não comprova qualidade linguística. Exigir revisão humana
de naturalidade, terminologia, plurais e RTL dos 18 idiomas, além da validação
visual/acessível. M8 só encerra após também cumprir M8-T1…T5 e seu checkpoint.
