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
 * Expandido de 116 para acomodar todos os controles do player (M3).
 * Altura reduzida de 200 para 140 depois de unificar Abrir/Play/Pause/Stop
 * em uma única linha (antes ocupavam 3 linhas empilhadas). */
#define MAINWND_WIDTH   275
#define MAINWND_HEIGHT  140

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
/* Menu de contexto — Aparência > Escala da interface                   */
/* Clique direito em qualquer ponto da janela abre o menu.              */
/* ------------------------------------------------------------------ */
#define IDM_SCALE_AUTO   4000
#define IDM_SCALE_80     4001
#define IDM_SCALE_90     4002
#define IDM_SCALE_100    4003
#define IDM_SCALE_110    4004
#define IDM_SCALE_125    4005
#define IDM_SCALE_150    4006

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
/* Uma única linha na parte inferior: Abrir à esquerda (compacto),      */
/* Play/Pause/Stop lado a lado à direita.                               */
#define BOTTOM_CONTROLS_Y   (NAV_Y + NAV_BTN_H + 6)

#define CTRL_BTN_W      30
#define CTRL_BTN_H      16
#define CTRL_GAP        4

#define BTN_STOP_X      (MAINWND_WIDTH - CTRL_BTN_W - 4)
#define BTN_PAUSE_X     (BTN_STOP_X - CTRL_BTN_W - CTRL_GAP)
#define BTN_PLAY_X      (BTN_PAUSE_X - CTRL_BTN_W - CTRL_GAP)

#define BTN_OPEN_X      4
#define BTN_OPEN_Y      BOTTOM_CONTROLS_Y
#define BTN_OPEN_W      (BTN_PLAY_X - CTRL_GAP - BTN_OPEN_X)

#define BTN_PLAY_Y      BOTTOM_CONTROLS_Y
#define BTN_PAUSE_Y     BOTTOM_CONTROLS_Y
#define BTN_STOP_Y      BOTTOM_CONTROLS_Y

/* ------------------------------------------------------------------ */
/* Visualizador — M4                                                   */
/* Área abaixo dos controles inferiores para osciloscópio/espectro.    */
/* ------------------------------------------------------------------ */
#define VIZ_X           4
#define VIZ_Y           (BOTTOM_CONTROLS_Y + 20)
#define VIZ_W           (MAINWND_WIDTH - 8)
#define VIZ_H           32

/* ------------------------------------------------------------------ */
/* Timer IDs                                                           */
/* ------------------------------------------------------------------ */
#define POSITION_TIMER_ID   1  /* Atualiza LCD, seek bar, título rolando. */
#define SCROLL_TIMER_ID     2  /* Contador de ms para scroll do título.    */

/* Velocidade do scroll do título (pixels/segundo). */
#define TITLE_SCROLL_SPEED  40.0f

/* ------------------------------------------------------------------ */
/* DPI scaling — M8-T3                                                 */
/* Retorna o fator de escala atual baseado no DPI da monitor da janela. */
/* 1.0 = 96 DPI (100%), 1.25 = 120 DPI, 1.5 = 144 DPI, 2.0 = 192 DPI.*/
/* ------------------------------------------------------------------ */

/* Escala um valor inteiro pelo DPI atual. */
int MainWnd_DPIScale(int px);

/* Escala um valor float pelo DPI atual. */
float MainWnd_DPIScaleFloat(float f);

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
