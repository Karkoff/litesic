/*
 * app.cpp — Entry point (wWinMain) + loop de mensagens
 *
 * M1-T1: registra a classe, cria a janela principal, executa o loop
 * GetMessageW/TranslateMessage/DispatchMessageW.
 */

#include <windows.h>

#include "mainwnd.h"

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                    LPWSTR /*lpCmdLine*/, int nCmdShow)
{
    /* Registrar a classe da janela (uma vez). */
    if (!MainWnd_RegisterClass(hInstance))
    {
        return 1;
    }

    /* Criar e exibir a janela principal. */
    HWND hwnd = MainWnd_Create(hInstance, nCmdShow);
    if (hwnd == NULL)
    {
        return 1;
    }

    /* Loop de mensagens padrão. */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
