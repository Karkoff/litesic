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

/* Registra a classe da janela principal (chamar uma vez antes de criar).
 * Retorna TRUE em caso de sucesso, FALSE caso contrário. */
BOOL MainWnd_RegisterClass(HINSTANCE hInstance);

/* Cria e exibe a janela principal. Retorna o HWND ou NULL em erro. */
HWND MainWnd_Create(HINSTANCE hInstance, int nCmdShow);

#endif /* MAINWND_H */
