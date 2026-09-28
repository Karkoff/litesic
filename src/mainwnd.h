/*
 * mainwnd.h — Janela principal sem borda (estilo Winamp)
 *
 * M1-T1: esqueleto da janela — abre, fecha limpo, arrastável pela barra de
 * título, botões fechar/minimizar desenhados.
 * M3: display LCD, título rolando, seek bar, volume/pan, navegação.
 */

#ifndef MAINWND_H
#define MAINWND_H

#include <windows.h>

/* Tamanho fixo da janela na escala 1x (Winamp clássico).
 * Expandido de 116 para acomodar todos os controles do player (M3). */
#define MAINWND_WIDTH   275
#define MAINWND_HEIGHT  200

/* Altura da barra de título (área sensível para arrastar e botões). */
#define TITLEBAR_HEIGHT 24

/* Tamanho dos botões na barra de título. */
#define BTN_SIZE        16

/* Posição X dos botões (alinhados à direita, com margem). */
#define BTN_CLOSE_X     (MAINWND_WIDTH - BTN_SIZE - 2)
#define BTN_MINIMIZE_X  (BTN_CLOSE_X - BTN_SIZE - 2)

/* Estado dos botões da barra de título. */
typedef enum
{
    BTN_NONE = 0,
    BTN_MINIMIZE,
    BTN_CLOSE
} TitleBarButton;

/* ID do ícone definido em res/app.rc (M0-T5). */
#define IDI_APP_ICON 101

/* ------------------------------------------------------------------ */
/* Display LCD de tempo — M3-T1                                         */
/* Dimensões fixas: 80x26 px, canto superior esquerdo abaixo da titlebar. */
/* ------------------------------------------------------------------ */
#define LCD_X           4
#define LCD_Y           (TITLEBAR_HEIGHT + 2)
#define LCD_W           80
#define LCD_H           26

/* ------------------------------------------------------------------ */
/* Título rolando — M3-T2                                               */
/* Fica à direita do LCD, mesma altura.                                 */
/* ------------------------------------------------------------------ */
#define TITLE_X         (LCD_X + LCD_W + 4)
#define TITLE_Y         LCD_Y
#define TITLE_W         (MAINWND_WIDTH - TITLE_X - 4)
#define TITLE_H          LCD_H

/* ------------------------------------------------------------------ */
/* Barra de busca (seek) — M3-T4                                        */
/* Abaixo do LCD/título.                                                */
/* ------------------------------------------------------------------ */
#define SEEK_Y          (LCD_Y + LCD_H + 2)
#define SEEK_H          16
#define SEEK_X          4
#define SEEK_W          (MAINWND_WIDTH - 8)

/* ------------------------------------------------------------------ */
/* Volume e balanço — M3-T5                                             */
/* Dois sliders horizontais lado a lado, abaixo do seek.                */
/* ------------------------------------------------------------------ */
#define VOLPAN_Y        (SEEK_Y + SEEK_H + 2)
#define VOLPAN_H        16

#define VOLUME_SLIDER_X 4
#define VOLUME_SLIDER_W 80

#define PAN_SLIDER_X    (VOLUME_SLIDER_X + VOLUME_SLIDER_W + 10)
#define PAN_SLIDER_W    80

/* ------------------------------------------------------------------ */
/* Botões de navegação — M3-T6                                          */
/* Prev/Next na linha inferior.                                         */
/* ------------------------------------------------------------------ */
#define NAV_Y           (VOLPAN_Y + VOLPAN_H + 4)
#define NAV_BTN_W       28
#define NAV_BTN_H       18
#define NAV_GAP         4

#define BTN_PREV_X      4
#define BTN_NEXT_X      (BTN_PREV_X + NAV_BTN_W + NAV_GAP)

/* ------------------------------------------------------------------ */
/* Botões toggle aleatório/repetir — M3-T6                              */
/* Ao lado dos botões de navegação.                                     */
/* ------------------------------------------------------------------ */
#define TOGGLE_Y        NAV_Y
#define TOGGLE_W        40
#define TOGGLE_H        NAV_BTN_H
#define TOGGLE_GAP      4

#define BTN_SHUFFLE_X   (BTN_NEXT_X + NAV_BTN_W + NAV_GAP)
#define BTN_REPEAT_X    (BTN_SHUFFLE_X + TOGGLE_W + TOGGLE_GAP)

/* ------------------------------------------------------------------ */
/* Botões de controle "Abrir/Play/Pause/Stop" — M2-T4                   */
/* Reposicionados para a parte inferior da janela, abaixo dos controles */
/* do player principal.                                                 */
#define BOTTOM_CONTROLS_Y   (NAV_Y + NAV_BTN_H + 6)

#define BTN_OPEN_X      4
#define BTN_OPEN_Y      BOTTOM_CONTROLS_Y
#define BTN_OPEN_W      (MAINWND_WIDTH - 4*2 - 3*30 - 3*4)

#define BTN_PLAY_X      (MAINWND_WIDTH - 30 - 4)
#define BTN_PAUSE_X     (BTN_PLAY_X)
#define BTN_STOP_X      (BTN_PLAY_X)

#define BTN_PLAY_Y      BOTTOM_CONTROLS_Y
#define BTN_PAUSE_Y     (BOTTOM_CONTROLS_Y + 16 + 4)
#define BTN_STOP_Y      (BTN_PAUSE_Y + 16 + 4)

/* ------------------------------------------------------------------ */
/* Timer IDs                                                           */
/* ------------------------------------------------------------------ */
#define POSITION_TIMER_ID   1  /* Atualiza LCD, seek bar, título rolando. */
#define SCROLL_TIMER_ID     2  /* Contador de ms para scroll do título.    */

/* Velocidade do scroll do título (pixels/segundo). */
#define TITLE_SCROLL_SPEED  40.0f

/* ------------------------------------------------------------------ */
/* Slider dragging state                                               */
/* ------------------------------------------------------------------ */
typedef enum
{
    SLIDER_NONE = 0,
    SLIDER_SEEK,
    SLIDER_VOLUME,
    SLIDER_PAN
} SliderType;

/* Registra a classe da janela principal (chamar uma vez antes de criar).
 * Retorna TRUE em caso de sucesso, FALSE caso contrário. */
BOOL MainWnd_RegisterClass(HINSTANCE hInstance);

/* Cria e exibe a janela principal. Retorna o HWND ou NULL em erro. */
HWND MainWnd_Create(HINSTANCE hInstance, int nCmdShow);

#endif /* MAINWND_H */
