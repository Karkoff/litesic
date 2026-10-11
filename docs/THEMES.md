# Temas visuais do LiteSic

**Implementado e validado tecnicamente em 11/10/2026. Aceite visual humano
dos novos temas: pendente.** São 36 temas selecionáveis, com aplicação imediata
e preferência persistente. A M5 funcional já tinha seu aceite; esta ampliação
visual não inicia M6 nem implementa DSP do equalizador.

Atualização posterior: seletor e menus próprios agora usam a interface da
aplicação; comandos da playlist foram para o rodapé e a seleção primária
passou a persistir. [Relatório atual, nova rodada de testes e capturas](PLAYLIST-UI-2026-10-11.md).
A galeria e a tabela de memória desta página preservam a primeira etapa.

## Uso

Clique com o botão direito no player → **Aparência > Temas...**. Escolha uma
categoria ou mantenha **Todos**. Cada linha mostra nome, categoria e cinco
amostras da paleta. Um ✓ e a borda de destaque identificam o tema ativo.
Clique numa linha ou navegue por teclado para aplicar imediatamente.
**Restaurar Original** retorna ao LiteSic Original; **Fechar** ou Escape fecha
o seletor. Tab navega pelos controles. O painel não ocupa espaço permanente
no player e permite continuar usando player e playlist enquanto está aberto.

## Arquitetura

- `ui_theme.h/.cpp`: `ThemePalette` com 19 papéis de cor; `Typography` com
  famílias, quatro tamanhos/pesos e estilo do título; `Theme` com código,
  nome e categoria. Catálogo imutável, paleta ativa e geração de aparência
  pertencem à thread da UI. Não há trabalho de temas no callback de áudio.
- `ThemeCount`, `ThemeAt`, `ActiveTheme`, `SelectTheme`, `RestoreTheme` e
  `ThemeConfigure` expõem consulta, seleção, restauração e configuração.
  Código desconhecido escolhe Original. O retorno de `SelectTheme` informa
  se a gravação solicitada funcionou; uma falha não desfaz a aparência aplicada
  e o seletor informa o problema ao usuário.
- `SelectTheme` altera somente a aparência e envia `WM_THEME_CHANGED` às
  janelas `LiteSic.*` da mesma thread. Player, playlist e painel EQ invalidam
  a pintura. O Edit da pesquisa substitui brush/fonte preservando seu texto.
  Nenhuma troca recria janelas, motor, decoder, playlist ou worker.
- `Canvas` conserva fábrica Direct2D, fábrica DirectWrite compartilhada,
  render target e brush. As cores usam `SetColor` no brush existente.
  Somente ao mudar a geração são liberados e reconstruídos os quatro formatos
  de texto e seus sinais de reticências. Não há cache de recursos para 36
  temas nem recriação de formatos em cada frame. Falha de target continua
  usando o descarte/recriação e fallback GDI existentes.
- `theme_window.cpp`: seletor Win32 não modal; launcher com menu de categorias,
  modelo ComboBox interno e ListBox com desenho de prévias. Barra de título,
  botões e rolagem próprios usam a paleta; navegação dos controles continua
  nativa, sem novo framework. Brushes temporários dessas prévias são liberados.
  `TrackThemedMenu` em `ui_draw.cpp` compartilha desenho/fonte/recursos dos
  menus próprios e preserva a navegação Win32 e submenus.
- Pintura principal, seleção, Edit, indicadores, sliders, estados de hover,
  foco e pressionado usam papéis compartilhados. FFT/osciloscópio mudam
  somente as cores; cálculo, suavização, dados e timers foram preservados.
  O painel EQ indisponível já lê o mesmo tema, sem ganhar processamento DSP.

Novas janelas que usam `Canvas` leem o tema ativo automaticamente. Para
repintura imediata, sua classe deve manter o prefixo `LiteSic.` e tratar
`WM_THEME_CHANGED`; recursos GDI próprios devem ser substituídos nesse evento.
Não alterar a paleta diretamente: usar `SelectTheme` para manter geração e
notificações coerentes.

## Catálogo, paletas e fontes

[Catálogo completo com códigos, cores e fontes](evidence/themes/catalog.md)
e [paletas efetivas em JSON](evidence/themes/palettes.json).

| Categoria | Temas |
|---|---|
| Essenciais | LiteSic Original, Escuro, Claro, Inverso, Alto Contraste, Monocromático |
| Sazonais | Verão, Inverno, Primavera, Outono, Tropical, Aurora Boreal |
| Comemorativos | Natal, Ano Novo, Carnaval, Páscoa, Halloween, Festa Junina, Dia dos Namorados, Dia das Crianças |
| Tecnológicos | Cyberpunk, Matrix, Neon Blue, Neon Purple, Terminal Retro, Synthwave, Midnight, Carbon |
| Naturais | Oceano, Floresta, Deserto, Montanha, Pôr do Sol, Sakura |
| Clássicos | Winamp Classic, Vintage Audio |

Cada composição define fundo, superfície, visor, tonalidade das bordas e dois
destaques próprios. Estados complementares são derivados centralmente, com
ajuste de luminância para atender aos pares de contraste testados. Inverso
possui paleta clara quente própria; não inverte pixels. Alto Contraste usa
fundos quase pretos e destaques amarelo/ciano. Nenhum tema exige imagens,
texturas, fontes externas ou animações.

Original conserva os valores aprovados e as fontes Segoe UI/Consolas. Matrix
e Terminal Retro usam Consolas na interface; Deserto, Winamp Classic e Vintage
Audio usam Tahoma. Sakura usa título em itálico. Os tamanhos permanecem
12/10/14/27 DIP (corpo/pequeno/título/digital), com pesos 400/400/600/600.
DirectWrite consulta a coleção de fontes e usa Segoe UI se a família estiver
ausente; o fallback de glifos do Windows continua disponível. Textos longos
mantêm recorte/reticências e cronômetros conservam seu espaço fixo.

Textos comuns são testados a **4,5:1**, inclusive textos maiores. Foco e cores
dos visualizadores têm mínimo **3:1** nos pares pertinentes. Bordas de
separação e grades decorativas permanecem suaves; esses mínimos não são uma
afirmação de conformidade WCAG completa da aplicação desktop. No Original,
os números do cabeçalho usam texto secundário legível, e linhas destacadas
usam texto principal para evitar o contraste menor do texto de metadados.
Botões desabilitados não adotam o fundo de pressionado.

## Persistência

Sem configuração geral adequada existente, foi adotado um INI simples na
mesma pasta de dados usada para a sessão:

```ini
; %LOCALAPPDATA%\LiteSic\settings.ini
[Appearance]
Theme=original
```

Somente a chave `Appearance/Theme` é gravada, usando identificadores ASCII
estáveis. Nomes traduzidos não participam da persistência. O tema é carregado
antes da criação das janelas. Arquivo ausente, código desconhecido ou removido
restauram Original; **Restaurar Original** também grava `original`.
Não existe banco de dados nem imagem por tema. Não há gravação durante frames.
O INI pode ser copiado para outra instalação na pasta equivalente; não foi
adicionado um modo portátil de sessão nem alterada sua política de localização.

APIs Unicode usadas são compatíveis com Windows 7, conforme
[GetPrivateProfileStringW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprivateprofilestringw),
[WritePrivateProfileStringW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-writeprivateprofilestringw)
e [CreateTextFormat](https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritefactory-createtextformat).
Não foram adicionadas dependências ao executável.

## Como adicionar um tema

Adicionar uma entrada no catálogo de `ui_theme.cpp` com código único e estável,
nome, categoria, cores de composição e tipografia. A função `Make` deriva
estados e ajusta contraste; paletas especiais também podem declarar todos os
papéis diretamente. Manter tamanhos aprovados. Verificar todos os pares de
texto/fundo e indicadores, incluindo seleção e controles pressionados.

O seletor lista o catálogo automaticamente. Uma categoria nova deve ser
adicionada também ao filtro `groups` de `theme_window.cpp`. Atualizar o teste
de disponibilidade, a whitelist do auxiliar gráfico e a documentação quando
o catálogo mudar. Não renomear códigos já persistidos sem migração explícita.

## Testes e evidências

**Quatro builds PASS**, x64/x86 Debug/Release. **356 verificações**, 89 por
variante em 16 executáveis, sem falhas: as 82 anteriores e sete novos casos
de temas. **27 guardas** do auxiliar gráfico também passaram.

Os sete casos novos verificam: 36 códigos únicos, integridade e contraste;
métricas DirectWrite nos espaços fixos de controles/cronômetro/título;
seleção, INI, recarga, restauração e fallback; aplicação real nas duas janelas
com comparação de pixels; preservação de faixa, estado/posição de áudio,
ordem/identidades, seleção, pesquisa e geometria; descarte/recriação de target
e alternância repetida; categorias/restauração via botão nativo/Escape; carga
sustentada de memória. Várias verificações pertencem ao mesmo caso.

[Galeria dos oito temas solicitados](evidence/themes/index.html): Original,
Escuro, Claro, Inverso, Verão, Natal, Halloween e Cyberpunk, com player e
playlist do executável final. [Manifesto e hashes](evidence/themes/manifest.json):
25 capturas, incluindo seletor e evidência complementar de reprodução,
recarga do tema Claro e restauração. As seis capturas complementares são da
rodada anterior aos dois ajustes finais de contraste; sua identidade de
executável é registrada separadamente. As 16 imagens principais são finais.

O player vazio em Original tem **SHA-256 de imagem idêntico** ao baseline
anterior à implementação. Isso comprova esse estado/renderização, não todos
os estados possíveis. A gravação/recarga do Claro passou em GUI, e a troca
Cyberpunk → Claro manteve Teste-002 reproduzindo, com contador avançando e a
mesma seleção. Capturas usaram áudio sintético silencioso; não certificam
qualidade audível. Os outros 28 temas passaram pelas verificações automatizadas,
incluindo renderização das janelas, sem declaração de aprovação estética.

A sessão pessoal foi preservada, usada apenas como backup local e restaurada
byte a byte. A preferência inicial inexistente ficou em Original após os
testes. [Comprovante](evidence/themes/restoration.json). Nenhuma playlist
pessoal foi incluída no pacote. Na primeira etapa não houve commit ou push,
conforme a restrição então vigente. O pedido posterior de ajustes autorizou
a publicação conjunta; ver [relatório da entrega](PLAYLIST-UI-2026-10-11.md).

Houve tentativas descartadas: menu fechado antes da seleção; confirmação do
ListBox usando um método que pertence ao ComboBox; clique de restauração
acima do botão devido à conversão de coordenadas. O auxiliar foi corrigido,
as tentativas repetidas e apenas resultados conferidos foram usados como PASS.
Em builds completos x86 Release, GCC também registrou avisos de otimização
`__atomic_*` no miniaudio existente; não houve alteração dessa dependência.
Os avisos de cast DPI e helpers GDI não usados já existiam. As novas unidades
de temas compilaram sem avisos observados.

## Memória e recursos

Cada variante executou **1.980 trocas** entre os 36 temas, com pintura real
do player e playlist; 180 aquecem a carga e as 1.800 seguintes compõem dez
lotes amostrados. São 7.920 trocas sustentadas nas quatro variantes, além do
teste de recuperação de render target. Foram registrados private bytes,
working set, handles e GDI/USER. [Amostras e resumo](evidence/themes/memory-summary.json).

| Variante | Private bytes após 180 / 1.980 trocas (MiB) | GDI/USER inicial → final | Handles inicial → final |
|---|---:|---|---|
| x86 Debug | 62,99 / 60,73 | 21/18 → 21/18 | 503 → 503 |
| x86 Release | 62,91 / 60,65 | 21/18 → 21/18 | 503 → 503 |
| x64 Debug | 86,88 / 77,35 | 21/18 → 21/18 | 630 → 630 |
| x64 Release | 90,50 / 80,98 | 52/51 → 52/51 | 822 → 822 |

Não houve crescimento contínuo observado de memória privada nem de recursos
GDI/USER/handles na carga executada. O limite automático de regressão permite
até 8 MiB acima da primeira amostra aquecida e até dois objetos GDI/USER;
os dados efetivos terminaram abaixo do início, com contadores estáveis.
Working set pode variar independentemente de private bytes. Diferenças entre
processos, áudio disponível, shell e caches não permitem atribuir todo o
consumo ao sistema de temas. Esta carga não prova ausência de vazamentos em
execuções longas nem substitui perfilamento por stack de heap.

## Reprodução e limites

```powershell
# Repetir para Architecture 32; omitir DebugBuild para Release.
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Action build -Architecture 64 -DebugBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/dev.ps1 -Action test -Architecture 64 -DebugBuild
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/gui.ps1 -Action test
# Depois das capturas no desktop interativo:
.tools/gui-venv/Scripts/python.exe scripts/diagnostics/package_themes.py
```

O empacotador exige os artefatos da primeira etapa e seu binário correspondente: audita hashes de PNG/binários,
cores de fundo, dimensões, logs, catálogos iguais nas quatro variantes e dados
de memória. Não substitui abrir o app e conferir gestos/telas. O auxiliar
`-Target themes` exige janela do processo lançado com proprietário correto;
`-Action theme -Query Claro` só aceita nomes do catálogo. Cliques agora usam
`ClientToScreen` nativo e coordenadas absolutas, cobrindo molduras nativas.

Aceite visual humano, VM Windows 7, movimento físico entre monitores com DPIs
diferentes e medições prolongadas permanecem pendentes. Os oito temas foram
inspecionados tecnicamente a 150% no Windows 11; não se declara aprovação
visual humana dos 36. O EQ continua indisponível até a M6.
