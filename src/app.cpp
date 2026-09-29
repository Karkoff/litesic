/*
 * app.cpp — Entry point (wWinMain) + loop de mensagens
 *
 * M1-T1: registra a classe, cria a janela principal, executa o loop
 * GetMessageW/TranslateMessage/DispatchMessageW.
 * M8-T3: Per-Monitor DPI Awareness V2 para telas 4K/high-DPI.
 */

#include <windows.h>

#include "mainwnd.h"

/* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 requer Windows 10 1703+.
 * Verificar existência da função antes de chamar (fallback para o manifest). */
typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);

static void InitDpiAwareness(void)
{
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32 != NULL)
    {
        PFN_SetProcessDpiAwarenessContext pSetDpi =
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(
                hUser32, "SetProcessDpiAwarenessContext");
        if (pSetDpi != NULL)
        {
            /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4 */
            pSetDpi((HANDLE)-4);
        }
        FreeLibrary(hUser32);
    }
}

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                    LPWSTR /*lpCmdLine*/, int nCmdShow)
{
    /* Configurar Per-Monitor DPI Awareness V2 antes de criar janelas. */
    InitDpiAwareness();

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
