/*
 * mainwnd.cpp — Janela principal sem borda (estilo Winamp)
 *
 * M1-T1: WndProc tratando WM_CREATE, WM_DESTROY e WM_PAINT com cor sólida.
 */

#include "mainwnd.h"

#include <windows.h>

/* ------------------------------------------------------------------ */
/* Classe da janela — nome único                                        */
/* ------------------------------------------------------------------ */

#define CLASS_NAME L"LiteSic.MainWnd"

/* Cor de fundo da janela (placeholder — arte real vem depois). */
static const COLORREF BG_COLOR = RGB(0x3c, 0x28, 0x1e);

/* Cor da barra de título (placeholder). */
static const COLORREF TITLEBAR_COLOR = RGB(0x5a, 0x42, 0x32);

/* ------------------------------------------------------------------ */
/* WndProc                                                              */
/* ------------------------------------------------------------------ */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        HDC memDC = CreateCompatibleDC(hdc);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, MAINWND_WIDTH, MAINWND_HEIGHT);
        HGDIOBJ oldObj = SelectObject(memDC, memBM);

        /* Fundo da janela. */
        HBRUSH bgBrush = CreateSolidBrush(BG_COLOR);
        FillRect(memDC, &ps.rcPaint, bgBrush);
        DeleteObject(bgBrush);

        /* Barra de título (faixa no topo). */
        RECT titleRect = { 0, 0, MAINWND_WIDTH, TITLEBAR_HEIGHT };
        HBRUSH titleBrush = CreateSolidBrush(TITLEBAR_COLOR);
        FillRect(memDC, &titleRect, titleBrush);
        DeleteObject(titleBrush);

        BitBlt(hdc, 0, 0, MAINWND_WIDTH, MAINWND_HEIGHT, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldObj);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Registro da classe                                                   */
/* ------------------------------------------------------------------ */

BOOL MainWnd_RegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;

    return RegisterClassExW(&wc) != 0;
}

/* ------------------------------------------------------------------ */
/* Criação da janela                                                    */
/* ------------------------------------------------------------------ */

HWND MainWnd_Create(HINSTANCE hInstance, int nCmdShow)
{
    HWND hwnd = CreateWindowExW(
        0,                              /* dwExStyle */
        CLASS_NAME,                     /* lpClassName */
        L"LiteSic",                     /* lpWindowName — título da janela */
        WS_POPUP | WS_OVERLAPPED |     /* sem bordas padrão (sem WS_CAPTION/WS_THICKFRAME) */
        WS_MINIMIZEBOX,                 /* permite minimizar via ShowWindow(SW_MINIMIZE) */
        CW_USEDEFAULT, CW_USEDEFAULT,  /* x, y — deixar o Windows posicionar */
        MAINWND_WIDTH,                  /* nWidth */
        MAINWND_HEIGHT,                 /* nHeight */
        NULL,                           /* hWndParent */
        NULL,                           /* hMenu */
        hInstance,                      /* hInstance */
        NULL                            /* lpCreateParams */
    );

    if (hwnd != NULL)
    {
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    return hwnd;
}
