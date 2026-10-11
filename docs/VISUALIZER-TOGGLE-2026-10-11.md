# Mostrar e ocultar o visualizador — 11/10/2026

O botão **VIZ**, à esquerda de LISTA e EQ no rodapé do player, alterna a
visibilidade do osciloscópio/espectro FFT. Usa o tema ativo, tooltip e
navegação por teclado. O modo FFT/OSC escolhido é preservado ao restaurar.

Ocultar reduz a altura da janela de 290 para 214 DIP, sem área vazia;
mostrar restaura os 290 DIP. O rodapé, seus botões e tooltips acompanham
o tamanho atual, inclusive ao mudar DPI ou escala da interface.
O desenho periódico fica suspenso enquanto oculto; a reprodução e os
controles de áudio continuam disponíveis. A visibilidade agora é persistida
entre sessões, juntamente com playlist, painel EQ, modo FFT/OSC e escala.
Veja [persistência da aparência](APPEARANCE-PERSISTENCE-2026-10-11.md).
O caminho GDI também oferece o botão e respeita a visibilidade.

## Ajuste posterior: player compacto

As quatro variantes x86/x64 Debug/Release foram compiladas novamente.
A suíte x64 Debug passou (89 testes); o teste de janela verifica a altura
compacta, largura e posição preservadas, clique no VIZ reposicionado,
restauração exata do retângulo original e estado pausado do áudio preservado.
No Windows 11, escala 150%, as capturas confirmam 690 × 435 → 690 × 321
→ 690 × 435 pixels. O encaixe da playlist usa a notificação existente de
mudança de geometria da janela principal.

| Expandido | Compacto | Restaurado |
|---|---|---|
| ![Expandido](evidence/visualizer-compact/viz-resize-expanded.png) | ![Compacto](evidence/visualizer-compact/viz-resize-compact.png) | ![Restaurado](evidence/visualizer-compact/viz-resize-restored.png) |

Logs e registros desta rodada: `evidence/visualizer-compact/`.

## Verificação inicial (antes do ajuste de altura)

- Compilação x86/x64 Debug/Release: PASS, sem novos avisos.
- Suíte x64 Debug: 89 testes aprovados. O teste da janela real verifica
  pixels ao ocultar/restaurar e confirma que o áudio permanece pausado.
  Os demais testes de transporte e seek continuam aprovados.
- Validação gráfica no Windows 11, x64 Debug, escala 150%: PASS.
  Cliques reais em VIZ ocultaram e restauraram a área e os botões FFT/OSC;
  a geometria permaneceu 690 × 435 pixels.

Capturas da janela real, produzidas pelo auxiliar restrito `scripts/gui.ps1`:

| Antes | Oculto | Restaurado |
|---|---|---|
| ![Visível](evidence/visualizer-toggle/viz-shown.png) | ![Oculto](evidence/visualizer-toggle/viz-hidden.png) | ![Restaurado](evidence/visualizer-toggle/viz-restored.png) |

Logs e registros de captura ficam em `evidence/visualizer-toggle/`.
A validação gráfica foi feita no caminho Direct2D; o fallback GDI foi
compilado, mas não foi forçado nesta rodada. Não foi realizada validação
em Windows 7 nesta tarefa.
