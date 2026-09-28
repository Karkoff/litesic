/*
 * mainwnd.cpp — Janela principal sem borda (estilo Winamp)
 *
 * M1-T1: WndProc tratando WM_CREATE, WM_DESTROY e WM_PAINT com cor sólida.
 * M1-T2: barra de título arrastável via ReleaseCapture + HTCAPTION.
 * M1-T3: botões fechar/minimizar desenhados com hover/pressed visual.
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

/* Cores dos botões. */
static const COLORREF BTN_BG_NORMAL   = RGB(0xc0, 0xa0, 0x60);
static const COLORREF BTN_BG_HOVER    = RGB(0xe0, 0xc0, 0x80);
static const COLORREF BTN_BG_PRESSED  = RGB(0xa0, 0x80, 0x40);

/* Cor do ícone "X" (fechar) — branco. */
static const COLORREF CLOSE_ICON_COLOR = RGB(0xff, 0x40, 0x40);

/* ------------------------------------------------------------------ */
/* Estado dos botões (módulo-local)                                     */
/* ------------------------------------------------------------------ */

/* Botão que o mouse está sobre no momento. */
static TitleBarButton g_hoverBtn = BTN_NONE;

/* Botão que foi pressionado (mouse down). */
static TitleBarButton g_pressedBtn = BTN_NONE;

/* ------------------------------------------------------------------ */
/* Helpers — hit-test e desenho de botões                               */
/* ------------------------------------------------------------------ */

/* Verifica se (x, y) está dentro do botão especificado. */
static BOOL PtInButton(int x, int y, TitleBarButton btn)
{
    RECT r{};
    switch (btn)
    {
    case BTN_MINIMIZE:
        r = { BTN_MINIMIZE_X, 2, BTN_MINIMIZE_X + BTN_SIZE, 2 + BTN_SIZE };
        break;
    case BTN_CLOSE:
        r = { BTN_CLOSE_X, 2, BTN_CLOSE_X + BTN_SIZE, 2 + BTN_SIZE };
        break;
    default:
        return FALSE;
    }
    return PtInRect(&r, (POINT){ x, y });
}

/* Obtém a cor do botão com base no estado. */
static COLORREF ButtonColor(TitleBarButton btn)
{
    if (btn == g_pressedBtn)
        return BTN_BG_PRESSED;
    if (btn == g_hoverBtn)
        return BTN_BG_HOVER;
    return BTN_BG_NORMAL;
}

/* Desenha um botão na DC de memória. */
static void DrawButton(HDC hdc, TitleBarButton btn)
{
    RECT r{};
    switch (btn)
    {
    case BTN_MINIMIZE:
        r = { BTN_MINIMIZE_X, 2, BTN_MINIMIZE_X + BTN_SIZE, 2 + BTN_SIZE };
        break;
    case BTN_CLOSE:
        r = { BTN_CLOSE_X, 2, BTN_CLOSE_X + BTN_SIZE, 2 + BTN_SIZE };
        break;
    default:
        return; /* Não desenhar botão inexistente. */
    }

    HBRUSH brush = CreateSolidBrush(ButtonColor(btn));
    FillRect(hdc, &r, brush);
    DeleteObject(brush);

    /* Desenhar ícone "—" (minimizar) ou "X" (fechar). */
    if (btn == BTN_MINIMIZE)
    {
        /* Linha horizontal — ícone de minimizar. */
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x00, 0x00, 0x00));
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, r.left + 4, r.top + r.bottom / 2, NULL);
        LineTo(hdc, r.right - 4, r.top + r.bottom / 2);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
    else if (btn == BTN_CLOSE)
    {
        /* "X" — duas linhas diagonais. */
        HPEN pen = CreatePen(PS_SOLID, 1, CLOSE_ICON_COLOR);
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, r.left + 4, r.top + 4, NULL);
        LineTo(hdc, r.right - 4, r.top + r.bottom - 4);
        MoveToEx(hdc, r.right - 4, r.top + 4, NULL);
        LineTo(hdc, r.left + 4, r.top + r.bottom - 4);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
}

/* Redesenha apenas a região dos botões (para atualizar hover/pressed). */
static void InvalidateButtons(HWND hwnd)
{
    RECT r = {
        BTN_MINIMIZE_X, 2,
        /* BTN_CLOSE_X > BTN_MINIMIZE_X sempre — usar o valor maior. */
        (int)(BTN_CLOSE_X + BTN_SIZE),
        2 + BTN_SIZE
    };
    InvalidateRect(hwnd, &r, FALSE);
}

/* ------------------------------------------------------------------ */
/* WndProc                                                              */
/* ------------------------------------------------------------------ */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        break;

    /* ------------------------------------------------------------------
     * Arraste da janela — clicar na barra de título e arrastar.
     * M1-T2: ReleaseCapture + WM_NCLBUTTONDOWN HTCAPTION permite que o
     * Windows mova a janela nativamente (suporta Aero snap do Win7+).
     * ------------------------------------------------------------------ */
    case WM_LBUTTONDOWN:
    {
        int const mx = (int)(short)LOWORD(lParam);
        int const my = (int)(short)HIWORD(lParam);

        /* Verificar se clicou num botão primeiro. */
        if (my < (int)TITLEBAR_HEIGHT)
        {
            if (PtInButton(mx, my, BTN_CLOSE))
            {
                g_pressedBtn = BTN_CLOSE;
                InvalidateButtons(hwnd);
                return 0;
            }
            if (PtInButton(mx, my, BTN_MINIMIZE))
            {
                g_pressedBtn = BTN_MINIMIZE;
                InvalidateButtons(hwnd);
                return 0;
            }

            /* Clique na barra de título — iniciar arraste nativo. */
            ReleaseCapture();
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
        }
        else
        {
            SetForegroundWindow(hwnd);
        }
        break;
    }

    /* ------------------------------------------------------------------
     * Soltar botão — executar ação se o clique foi dentro do mesmo botão.
     * ------------------------------------------------------------------ */
    case WM_LBUTTONUP:
    {
        int const mx = (int)(short)LOWORD(lParam);
        int const my = (int)(short)HIWORD(lParam);

        if (g_pressedBtn != BTN_NONE)
        {
            /* Verificar se soltou dentro do mesmo botão. */
            if (my < (int)TITLEBAR_HEIGHT && PtInButton(mx, my, g_pressedBtn))
            {
                switch (g_pressedBtn)
                {
                case BTN_CLOSE:
                    DestroyWindow(hwnd);
                    break;

                case BTN_MINIMIZE:
                    ShowWindow(hwnd, SW_MINIMIZE);
                    break;

                default:
                    break;
                }
            }
            g_pressedBtn = BTN_NONE;
            InvalidateButtons(hwnd);
        }
        break;
    }

    /* ------------------------------------------------------------------
     * Movimento do mouse — detectar hover sobre botões.
     * M1-T3: invalida a região dos botões para redesenhar estado.
     * ------------------------------------------------------------------ */
    case WM_MOUSEMOVE:
    {
        int const mx = (int)(short)LOWORD(lParam);
        int const my = (int)(short)HIWORD(lParam);

        TitleBarButton newHover = BTN_NONE;
        if (my < (int)TITLEBAR_HEIGHT && g_pressedBtn == BTN_NONE)
        {
            if (PtInButton(mx, my, BTN_CLOSE))
                newHover = BTN_CLOSE;
            else if (PtInButton(mx, my, BTN_MINIMIZE))
                newHover = BTN_MINIMIZE;
        }

        if (newHover != g_hoverBtn)
        {
            g_hoverBtn = newHover;
            InvalidateButtons(hwnd);
        }
        break;
    }

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

        /* Botões da barra de título. */
        DrawButton(memDC, BTN_MINIMIZE);
        DrawButton(memDC, BTN_CLOSE);

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
