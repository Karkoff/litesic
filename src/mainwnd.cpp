/*
 * mainwnd.cpp — Janela principal sem borda (estilo Winamp)
 *
 * M1-T1: WndProc tratando WM_CREATE, WM_DESTROY e WM_PAINT com cor sólida.
 * M1-T2: barra de título arrastável via ReleaseCapture + HTCAPTION.
 * M1-T3: botões fechar/minimizar desenhados com hover/pressed visual.
 * M1-T4: ícone carregado do recurso e enviado via WM_SETICON.
 * M2-T4: botões abrir/play/pause/stop + timer de posição.
 */

#include "mainwnd.h"
#include "audio.h"

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

/* Cor do ícone "X" (fechar) — vermelho. */
static const COLORREF CLOSE_ICON_COLOR = RGB(0xff, 0x40, 0x40);

/* Cores dos botões de controle (play/pause/stop). */
static const COLORREF CTRL_BG_NORMAL   = RGB(0x6a, 0x5a, 0x3a);
static const COLORREF CTRL_BG_HOVER    = RGB(0x8a, 0x7a, 0x5a);

/* ------------------------------------------------------------------ */
/* Estado dos botões (módulo-local)                                     */
/* ------------------------------------------------------------------ */

/* Botão da titlebar que o mouse está sobre. */
static TitleBarButton g_hoverBtn = BTN_NONE;

/* Botão da titlebar pressionado (mouse down). */
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

/* Desenha um botão da titlebar na DC de memória. */
static void DrawTitleButton(HDC hdc, TitleBarButton btn)
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
        return;
    }

    HBRUSH brush = CreateSolidBrush(ButtonColor(btn));
    FillRect(hdc, &r, brush);
    DeleteObject(brush);

    /* Desenhar ícone "—" (minimizar) ou "X" (fechar). */
    if (btn == BTN_MINIMIZE)
    {
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x00, 0x00, 0x00));
        HGDIOBJ oldPen = SelectObject(hdc, pen);
        MoveToEx(hdc, r.left + 4, r.top + r.bottom / 2, NULL);
        LineTo(hdc, r.right - 4, r.top + r.bottom / 2);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
    else if (btn == BTN_CLOSE)
    {
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

/* Redesenha apenas a região dos botões da titlebar. */
static void InvalidateButtons(HWND hwnd)
{
    RECT r = {
        BTN_MINIMIZE_X, 2,
        (int)(BTN_CLOSE_X + BTN_SIZE),
        2 + BTN_SIZE
    };
    InvalidateRect(hwnd, &r, FALSE);
}

/* Verifica se (x, y) está dentro de um botão de controle. */
static BOOL PtInControlBtn(int x, int y, int *pBtnId)
{
    /* Botão Abrir. */
    RECT r = { BTN_OPEN_X, BTN_OPEN_Y, BTN_OPEN_X + BTN_OPEN_W, BTN_OPEN_Y + CONTROLS_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 1; return TRUE; }

    /* Botão Play. */
    r = { BTN_PLAY_X, BTN_PLAY_Y, BTN_PLAY_X + CONTROLS_BTN_W, BTN_PLAY_Y + CONTROLS_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 2; return TRUE; }

    /* Botão Pause. */
    r = { BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_X + CONTROLS_BTN_W, BTN_PAUSE_Y + CONTROLS_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 3; return TRUE; }

    /* Botão Stop. */
    r = { BTN_STOP_X, BTN_STOP_Y, BTN_STOP_X + CONTROLS_BTN_W, BTN_STOP_Y + CONTROLS_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 4; return TRUE; }

    return FALSE;
}

/* Desenha um botão de controle na DC de memória. */
static void DrawControlButton(HDC hdc, int btnId)
{
    RECT r{};
    switch (btnId)
    {
    case 1: /* Abrir */
        r = { BTN_OPEN_X, BTN_OPEN_Y, BTN_OPEN_X + BTN_OPEN_W, BTN_OPEN_Y + CONTROLS_BTN_H };
        break;
    case 2: /* Play */
        r = { BTN_PLAY_X, BTN_PLAY_Y, BTN_PLAY_X + CONTROLS_BTN_W, BTN_PLAY_Y + CONTROLS_BTN_H };
        break;
    case 3: /* Pause */
        r = { BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_X + CONTROLS_BTN_W, BTN_PAUSE_Y + CONTROLS_BTN_H };
        break;
    case 4: /* Stop */
        r = { BTN_STOP_X, BTN_STOP_Y, BTN_STOP_X + CONTROLS_BTN_W, BTN_STOP_Y + CONTROLS_BTN_H };
        break;
    default:
        return;
    }

    HBRUSH brush = CreateSolidBrush(CTRL_BG_NORMAL);
    FillRect(hdc, &r, brush);
    DeleteObject(brush);

    /* Desenhar texto. */
    const wchar_t *text = NULL;
    switch (btnId)
    {
    case 1: text = L"Abrir"; break;
    case 2: text = L">";   break;
    case 3: text = L"||";  break;
    case 4: text = L"[]";  break;
    }

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0xff, 0xff, 0xcc));
    DrawTextW(hdc, text, (int)wcslen(text), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

/* ------------------------------------------------------------------ */
/* WndProc                                                              */
/* ------------------------------------------------------------------ */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        /* Garantir que o ícone aparece na taskbar e no Alt+Tab. */
        HICON hIcon = LoadIconW(
            GetModuleHandleW(NULL),
            MAKEINTRESOURCEW(IDI_APP_ICON)
        );
        if (hIcon != NULL)
        {
            SendMessageW(hwnd, WM_SETICON, ICON_BIG,  (LPARAM)hIcon);
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }

        /* Inicializar motor de áudio. */
        Audio_Init();

        /* Timer para atualização periódica da posição (250ms). */
        SetTimer(hwnd, POSITION_TIMER_ID, 250, NULL);
        break;
    }

    case WM_TIMER:
    {
        if (wParam == POSITION_TIMER_ID)
        {
            /* Se estiver tocando, invalidar a janela para redraw. */
            if (Audio_GetState() == AUDIO_STATE_PLAYING)
            {
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        break;
    }

    /* ------------------------------------------------------------------
     * Arraste da janela — clicar na barra de título e arrastar.
     * M1-T2: ReleaseCapture + WM_NCLBUTTONDOWN HTCAPTION permite que o
     * Windows mova a janela nativamente (suporta Aero snap do Win7+).
     * ------------------------------------------------------------------ */
    case WM_LBUTTONDOWN:
    {
        int const mx = (int)(short)LOWORD(lParam);
        int const my = (int)(short)HIWORD(lParam);

        /* Verificar se clicou num botão da titlebar primeiro. */
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
        else if (my >= CONTROLS_Y && my < CONTROLS_Y + CONTROLS_BTN_H * 3 + CONTROLS_GAP * 2)
        {
            /* Clique na área de botões de controle — apenas captura o foco. */
            SetForegroundWindow(hwnd);
        }
        else
        {
            SetForegroundWindow(hwnd);
        }
        break;
    }

    /* ------------------------------------------------------------------
     * Soltar botão — titlebar ou controles.
     * M2-T4: chama funções de audio.h para botões de controle.
     * ------------------------------------------------------------------ */
    case WM_LBUTTONUP:
    {
        int const mx = (int)(short)LOWORD(lParam);
        int const my = (int)(short)HIWORD(lParam);

        /* Verificar se soltou na titlebar. */
        if (g_pressedBtn != BTN_NONE && my < (int)TITLEBAR_HEIGHT)
        {
            if (PtInButton(mx, my, g_pressedBtn))
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

        /* Verificar se soltou num botão de controle. */
        if (my >= CONTROLS_Y)
        {
            int btnId = 0;
            if (PtInControlBtn(mx, my, &btnId))
            {
                switch (btnId)
                {
                case 1: /* Abrir arquivo */
                {
                    OPENFILENAMEW ofn{};
                    wchar_t szFile[MAX_PATH] = { 0 };

                    ofn.lStructSize  = sizeof(ofn);
                    ofn.hwndOwner    = hwnd;
                    ofn.lpstrFilter  = L"Audio Files\0*.mp3;*.wav;*.flac;*.ogg\0All Files\0*.*\0";
                    ofn.lpstrFile    = szFile;
                    ofn.nMaxFile     = MAX_PATH;
                    ofn.Flags        = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;

                    if (GetOpenFileNameW(&ofn))
                    {
                        Audio_Load(szFile);
                    }
                    break;
                }

                case 2: /* Play */
                    Audio_Play();
                    break;

                case 3: /* Pause */
                    Audio_Pause();
                    break;

                case 4: /* Stop */
                    Audio_Stop();
                    break;

                default:
                    break;
                }
            }
        }
        break;
    }

    /* ------------------------------------------------------------------
     * Movimento do mouse — detectar hover sobre botões.
     * M1-T3/M2-T4: invalida região para redesenhar estado.
     * ------------------------------------------------------------------ */
    case WM_MOUSEMOVE:
    {
        int const mx = (int)(short)LOWORD(lParam);
        int const my = (int)(short)HIWORD(lParam);

        /* Hover na titlebar. */
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
        DrawTitleButton(memDC, BTN_MINIMIZE);
        DrawTitleButton(memDC, BTN_CLOSE);

        /* Botões de controle (Abrir/Play/Pause/Stop). */
        DrawControlButton(memDC, 1); /* Abrir */
        DrawControlButton(memDC, 2); /* Play */
        DrawControlButton(memDC, 3); /* Pause */
        DrawControlButton(memDC, 4); /* Stop */

        BitBlt(hdc, 0, 0, MAINWND_WIDTH, MAINWND_HEIGHT, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldObj);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd, POSITION_TIMER_ID);
        Audio_Shutdown();
        PostQuitMessage(0);
        break;
    }

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
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
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
    /* Centralizar a janela na tela. */
    RECT desktop{};
    GetWindowRect(GetDesktopWindow(), &desktop);
    int const screenW = desktop.right - desktop.left;
    int const screenH = desktop.bottom - desktop.top;

    int x = (screenW - MAINWND_WIDTH) / 2;
    int y = (screenH - MAINWND_HEIGHT) / 2;
    /* Garantir que a janela não fique fora da tela. */
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    HWND hwnd = CreateWindowExW(
        0,                              /* dwExStyle */
        CLASS_NAME,                     /* lpClassName */
        L"LiteSic",                     /* lpWindowName — título da janela */
        WS_POPUP | WS_OVERLAPPED |     /* sem bordas padrão (sem WS_CAPTION/WS_THICKFRAME) */
        WS_MINIMIZEBOX,                 /* permite minimizar via ShowWindow(SW_MINIMIZE) */
        x, y,                           /* posição centralizada */
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
