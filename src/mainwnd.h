/*
 * mainwnd.h — Janela principal sem borda (estilo Winamp)
 *
 * M1-T1: esqueleto da janela — abre, fecha limpo, arrastável pela barra de
 * título, botões fechar/minimizar desenhados.
 */

#ifndef MAINWND_H
#define MAINWND_H

#include <windows.h>

/* Tamanho fixo da janela na escala 1x (Winamp clássico). */
#define MAINWND_WIDTH   275
#define MAINWND_HEIGHT  116

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
/* Botões de controle (M2-T4)                                           */
/* Posicionados abaixo da barra de título.                              */
/* ------------------------------------------------------------------ */

/* Área dos botões de controle — logo abaixo da titlebar. */
#define CONTROLS_Y      TITLEBAR_HEIGHT
#define CONTROLS_BTN_W  30
#define CONTROLS_BTN_H  16
#define CONTROLS_GAP    4

/* Botão "Abrir" ocupa o espaço restante à esquerda. */
#define BTN_OPEN_X      4
#define BTN_OPEN_Y      CONTROLS_Y
#define BTN_OPEN_W      (MAINWND_WIDTH - 4*2 - 3*CONTROLS_BTN_W - 3*CONTROLS_GAP)

/* Botões de controle empilhados verticalmente no canto direito. */
#define BTN_PLAY_X      (MAINWND_WIDTH - CONTROLS_BTN_W - 4)
#define BTN_PAUSE_X     (BTN_PLAY_X)
#define BTN_STOP_X      (BTN_PLAY_X)

#define BTN_PLAY_Y      (CONTROLS_Y + 2)
#define BTN_PAUSE_Y     (BTN_PLAY_Y + CONTROLS_BTN_H + CONTROLS_GAP)
#define BTN_STOP_Y      (BTN_PAUSE_Y + CONTROLS_BTN_H + CONTROLS_GAP)

/* ID do timer para atualização de posição (M2-T4). */
#define POSITION_TIMER_ID 1

/* Registra a classe da janela principal (chamar uma vez antes de criar).
 * Retorna TRUE em caso de sucesso, FALSE caso contrário. */
BOOL MainWnd_RegisterClass(HINSTANCE hInstance);

/* Cria e exibe a janela principal. Retorna o HWND ou NULL em erro. */
HWND MainWnd_Create(HINSTANCE hInstance, int nCmdShow);

#endif /* MAINWND_H */
