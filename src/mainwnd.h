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

/* Tamanho fixo em DIP: transporte e modos em uma linha,
 * visualizador ampliado e atalhos de lista/EQ no rodapé técnico. */
#define MAINWND_WIDTH 460
#define MAINWND_HEIGHT 290

/* Altura da barra de título (área sensível para arrastar e botões). */
#define TITLEBAR_HEIGHT 32

/* Tamanho dos botões na barra de título. */
#define BTN_SIZE 28

/* Posição X dos botões (alinhados à direita, com margem). */
#define BTN_CLOSE_X (MAINWND_WIDTH - 32)
#define BTN_MINIMIZE_X (MAINWND_WIDTH - 64)

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
#define LCD_X 12
#define LCD_Y 42
#define LCD_W 108
#define LCD_H 42

/* ------------------------------------------------------------------ */
/* Título rolando — M3-T2                                               */
/* Fica à direita do LCD, mesma altura.                                 */
/* ------------------------------------------------------------------ */
#define TITLE_X 130
#define TITLE_Y         LCD_Y
#define TITLE_W 318
#define TITLE_H          LCD_H

/* ------------------------------------------------------------------ */
/* Barra de busca (seek) — M3-T4                                        */
/* Abaixo do LCD/título.                                                */
/* ------------------------------------------------------------------ */
#define SEEK_Y 92
#define SEEK_H 20
#define SEEK_X 48
#define SEEK_W 360

/* ------------------------------------------------------------------ */
/* Volume e balanço — M3-T5                                             */
/* Dois sliders horizontais lado a lado, abaixo do seek.                */
/* ------------------------------------------------------------------ */
#define VOLPAN_Y 164
#define VOLPAN_H 24

#define VOLUME_SLIDER_X 64
#define VOLUME_SLIDER_W 132

#define PAN_SLIDER_X 318
#define PAN_SLIDER_W 120

/* ------------------------------------------------------------------ */
/* Botões de navegação — M3-T6                                          */
/* Anterior, Play, Pause, Stop, Próximo em uma única linha.              */
/* ------------------------------------------------------------------ */
#define NAV_Y 120
#define NAV_BTN_W 48
#define NAV_BTN_H 36
#define NAV_GAP 12

#define BTN_PREV_X 86
#define BTN_PLAY_X      (BTN_PREV_X + NAV_BTN_W + NAV_GAP)
#define BTN_PAUSE_X     (BTN_PLAY_X + NAV_BTN_W + NAV_GAP)
#define BTN_STOP_X      (BTN_PAUSE_X + NAV_BTN_W + NAV_GAP)
#define BTN_NEXT_X      (BTN_STOP_X + NAV_BTN_W + NAV_GAP)

/* ------------------------------------------------------------------ */
/* Botões toggle aleatório/repetir — M3-T6                              */
/* Nas extremidades da linha de transporte.                            */
/* ------------------------------------------------------------------ */
#define TOGGLE_Y NAV_Y
#define TOGGLE_W 48
#define TOGGLE_H NAV_BTN_H
#define TOGGLE_GAP 8

#define BTN_LIST_X 336
#define BTN_EQ_X 396
#define FOOTER_BUTTON_Y 268
#define FOOTER_BUTTON_W 52
#define FOOTER_BUTTON_H 18
#define BTN_SHUFFLE_X 12
#define BTN_REPEAT_X 400

/* ------------------------------------------------------------------ */
/* Controles de transporte compartilham tamanho e alinhamento.          */
#define CTRL_BTN_W      NAV_BTN_W
#define CTRL_BTN_H      NAV_BTN_H
#define BTN_PLAY_Y      NAV_Y
#define BTN_PAUSE_Y     NAV_Y
#define BTN_STOP_Y      NAV_Y

/* ------------------------------------------------------------------ */
/* Visualizador — M4                                                   */
/* Área abaixo dos controles inferiores para osciloscópio/espectro.    */
/* ------------------------------------------------------------------ */
#define VIZ_X 12
#define VIZ_Y 196
#define VIZ_W 436
#define VIZ_H 68

/* ------------------------------------------------------------------ */
/* Timer IDs                                                           */
/* ------------------------------------------------------------------ */
#define POSITION_TIMER_ID   1  /* Atualiza LCD, seek bar, título rolando. */
#define VIZ_TIMER_ID        3  /* Redesenha o visualizador (~60fps).       */
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
float MainWnd_UserScaleMultiplier();

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

bool MainWnd_TranslateShortcut(MSG* message);
void MainWnd_ApplyTopmost(HWND hwnd);
bool MainWnd_SetGlobalHotkeys(HWND hwnd,bool enabled);
#endif /* MAINWND_H */
