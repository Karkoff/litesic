/*
 * mainwnd.cpp — Janela principal sem borda (estilo Winamp)
 *
 * M1-T1: WndProc tratando WM_CREATE, WM_DESTROY e WM_PAINT com cor sólida.
 * M1-T2: barra de título arrastável via ReleaseCapture + HTCAPTION.
 * M1-T3: botões fechar/minimizar desenhados com hover/pressed visual.
 * M1-T4: ícone carregado do recurso e enviado via WM_SETICON.
 * M2-T4: botões abrir/play/pause/stop + timer de posição.
 * M3-T1: display LCD de tempo (MM:SS), clique alterna decorrido/restante.
 * M3-T2: título rolando horizontalmente quando excede a área.
 * M3-T3: indicador kbps/kHz/estéreo ao lado do LCD.
 * M3-T4: barra de busca (seek slider) arrastável.
 * M3-T5: sliders de volume e balanço.
 * M3-T6: botões anterior/próxima + toggles aleatório/repetir.
 */

#include "mainwnd.h"
#include "audio.h"
#include "util.h"
#include "visualizer.h"

#include <windows.h>
#include <cmath>

/* DPI scale factor — inicializa em WM_CREATE. */
static float g_dpiScale = 1.0f;

/* Escala de interface definida pelo usuário (Configurações > Aparência).
 * 0 = Automática (segue só o DPI do monitor); caso contrário, percentual
 * (80/90/100/110/125/150) multiplicado pelo fator de DPI do monitor.
 * Só em memória por enquanto — persistência real entra no M8 (M8-T4). */
static int g_userScalePercent = 0;

/* ------------------------------------------------------------------ */
/* DPI scaling — M8-T3                                                 */
/* ------------------------------------------------------------------ */

static float GetDpiScaleFactor(HWND hwnd)
{
    /* GetDpiForWindow requer Windows 10 1607+. Fallback para 96 DPI. */
    typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND);
    static PFN_GetDpiForWindow pGetDpi = NULL;

    if (pGetDpi == NULL)
    {
        HMODULE hUser32 = LoadLibraryW(L"user32.dll");
        if (hUser32 != NULL)
        {
            pGetDpi = (PFN_GetDpiForWindow)GetProcAddress(
                hUser32, "GetDpiForWindow");
            FreeLibrary(hUser32);
        }
    }

    if (pGetDpi != NULL)
    {
        UINT dpi = pGetDpi(hwnd);
        return (float)dpi / 96.0f;
    }

    /* Fallback: usar GetDeviceCaps do DC da janela. */
    HDC hdc = GetDC(hwnd);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hwnd, hdc);
    return (float)dpiX / 96.0f;
}

int MainWnd_DPIScale(int px)
{
    return (int)round((float)px * g_dpiScale);
}

float MainWnd_DPIScaleFloat(float f)
{
    return f * g_dpiScale;
}

/* Converte uma coordenada física (recebida em WM_LBUTTONDOWN/MOUSEMOVE/UP,
 * em pixels reais da janela) de volta para a escala lógica 1x usada por
 * todas as constantes de hit-test em mainwnd.h (BTN_*, SEEK_*, LCD_* etc.).
 * Sem isso, os cliques passam a cair fora dos controles sempre que a janela
 * é redimensionada por DPI ou pela escala do usuário (M8-T3). */
static int MainWnd_DPIUnscale(int physicalPx)
{
    if (g_dpiScale <= 0.0f) return physicalPx;
    return (int)round((float)physicalPx / g_dpiScale);
}

/* Multiplicador de escala definido pelo usuário em cima do DPI do monitor
 * (Configurações > Aparência > Escala da interface). 1.0 quando "Automática". */
static float UserScaleMultiplier()
{
    return (g_userScalePercent == 0) ? 1.0f : (float)g_userScalePercent / 100.0f;
}

/* Recalcula g_dpiScale (DPI do monitor x escala do usuário) e redimensiona/
 * reposiciona a janela para o novo tamanho físico, mantendo-a centrada em
 * torno do centro atual. Usado tanto na criação quanto ao trocar a escala
 * pelo menu de contexto. */
static void ApplyInterfaceScale(HWND hwnd)
{
    g_dpiScale = GetDpiScaleFactor(hwnd) * UserScaleMultiplier();

    int scaledW = MainWnd_DPIScale(MAINWND_WIDTH);
    int scaledH = MainWnd_DPIScale(MAINWND_HEIGHT);

    RECT cur{};
    GetWindowRect(hwnd, &cur);
    int cx = cur.left + (cur.right - cur.left) / 2;
    int cy = cur.top + (cur.bottom - cur.top) / 2;
    int x = cx - scaledW / 2;
    int y = cy - scaledH / 2;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    SetWindowPos(hwnd, NULL, x, y, scaledW, scaledH, SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(hwnd, NULL, TRUE);
}

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

/* Cores do display LCD. */
static const COLORREF LCD_BG_COLOR     = RGB(0x0a, 0x0a, 0x0a);
static const COLORREF LCD_TEXT_COLOR   = RGB(0x00, 0xff, 0x40); /* verde LCD */
static const COLORREF LCD_DIM_COLOR    = RGB(0x00, 0x30, 0x10);

/* Cor do título rolando. */
static const COLORREF TITLE_TEXT_COLOR = RGB(0xf0, 0xd8, 0x90);

/* Cores dos indicadores de áudio (kbps/kHz). */
static const COLORREF AUDIO_INFO_COLOR = RGB(0xb8, 0xa0, 0x78);

/* Cor da barra de busca. */
static const COLORREF SEEK_BAR_BG      = RGB(0x2a, 0x2a, 0x2a);
static const COLORREF SEEK_BAR_FILL    = RGB(0x66, 0xcc, 0x44);
static const COLORREF SEEK_BAR_THUMB   = RGB(0xee, 0xee, 0xee);

/* Cores dos sliders de volume/pan. */
static const COLORREF SLIDER_BG        = RGB(0x2a, 0x2a, 0x2a);
static const COLORREF SLIDER_FILL      = RGB(0x66, 0xcc, 0x44);
static const COLORREF SLIDER_THUMB     = RGB(0xee, 0xee, 0xee);

/* Cores dos botões de navegação. */
static const COLORREF NAV_BG_NORMAL    = RGB(0x5a, 0x4a, 0x2a);
static const COLORREF NAV_BG_HOVER     = RGB(0x7a, 0x6a, 0x4a);

/* Cores dos toggles. */
static const COLORREF TOGGLE_ON_BG     = RGB(0x44, 0x88, 0x22);
static const COLORREF TOGGLE_OFF_BG    = RGB(0x3a, 0x3a, 0x3a);

/* ------------------------------------------------------------------ */
/* Estado da janela (módulo-local)                                      */
/* ------------------------------------------------------------------ */

/* Botão da titlebar que o mouse está sobre. */
static TitleBarButton g_hoverBtn = BTN_NONE;

/* Botão da titlebar pressionado (mouse down). */
static TitleBarButton g_pressedBtn = BTN_NONE;

/* M3-T1: mostrar tempo restante (TRUE) ou decorrido (FALSE). */
static BOOL g_showRemaining = FALSE;

/* M3-T2: contador de ms para scroll do título. */
static DWORD g_scrollElapsedMs = 0;

/* M3-T4/M3-T5: estado de arraste do slider. */
static SliderType g_draggingSlider = SLIDER_NONE;
static float    g_seekValue   = 0.0f; /* 0..1 normalizado para seek */
static float    g_volumeValue = 0.8f; /* 0..1 */
static float    g_panValue    = 0.5f; /* -1..1 (0.5 = centro, panReal = 0.0) */

/* M3-T6: estado dos toggles. */
static BOOL g_shuffleOn = FALSE;
static BOOL g_repeatOn  = FALSE;

/* M3-T6: índice atual da playlist (provisório, lista de 1 item). */
static int g_currentTrack = -1; /* -1 = nenhum arquivo carregado */

/* M4: Estado do visualizador (declarado em visualizer.h). */
static VizMode g_vizMode = VIZ_MODE_OSCILLOSCOPE;

/* ------------------------------------------------------------------ */
/* Helpers — hit-test e desenho de botões da titlebar                   */
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

/* ------------------------------------------------------------------ */
/* Helpers — desenho do display LCD (M3-T1)                             */
/* Fonte bitmap 5x7 por segmento, digits compostos de 3 chars.         */
/* ------------------------------------------------------------------ */

/* Mapa de bits para dígitos 0-9 (5 colunas x 7 linhas), cada linha é um
 * byte onde os 5 bits menos significáveis representam as colunas.
 * Bit 4 = esquerda, bit 0 = direita. */
static const unsigned char g_digitFont[10][7] = {
    /* '0' */ { 0x7C, 0xC6, 0xCE, 0xD6, 0xE6, 0xC6, 0x7C },
    /* '1' */ { 0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3C },
    /* '2' */ { 0x7C, 0xC6, 0x06, 0x7C, 0x60, 0x60, 0x7E },
    /* '3' */ { 0x7C, 0xC6, 0x06, 0x7C, 0x06, 0xC6, 0x7C },
    /* '4' */ { 0x1C, 0x3C, 0x6C, 0xCC, 0x7C, 0x06, 0x06 },
    /* '5' */ { 0x7E, 0xC0, 0xC0, 0x7C, 0x06, 0xC6, 0x7C },
    /* '6' */ { 0x7C, 0xC0, 0xC0, 0x7C, 0xCE, 0xC6, 0x7C },
    /* '7' */ { 0x7E, 0x06, 0x0C, 0x18, 0x30, 0x30, 0x30 },
    /* '8' */ { 0x7C, 0xC6, 0xC6, 0x7C, 0xC6, 0xC6, 0x7C },
    /* '9' */ { 0x7C, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0x78 },
};

/* Caractere ':' (dois pontos) para separar minutos de segundos. */
static const unsigned char g_colonChar[7] = {
    0x00, 0x18, 0x00, 0x00, 0x00, 0x18, 0x00
};

/* Caractere '-' (hífen) para tempo restante. */
static const unsigned char g_dashChar[7] = {
    0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00
};

/* Largura de um dígito em pixels (5 bits * 2px por bit + 1px gap). */
#define DIGIT_CHAR_W  11
/* Altura de um dígito em pixels (7 linhas * 2px por linha). */
#define DIGIT_CHAR_H  14

/* Desenha um único caractere (dígito, ':' ou '-') no LCD.
 * ch: índice no font map (0-9 = dígitos, 10 = ':', 11 = '-').
 * x, y: posição do canto superior esquerdo. */
static void DrawLcdChar(HDC hdc, int ch, int x, int y)
{
    if (ch < 0 || ch > 11)
        return;

    const unsigned char *fontData;
    int rows = 7;

    if (ch == 10) /* ':' */
        fontData = g_colonChar;
    else if (ch == 11) /* '-' */
        fontData = g_dashChar;
    else
        fontData = g_digitFont[ch];

    /* Desenhar cada linha do caractere como blocos de 2x2 pixels. */
    for (int row = 0; row < rows; row++)
    {
        unsigned char bits = fontData[row];
        for (int col = 0; col < 5; col++)
        {
            if (bits & (0x40 >> col)) /* bit ativo — dados ocupam bits 6..2 */
            {
                RECT cell = {
                    x + col * DIGIT_CHAR_W / 5, y + row * DIGIT_CHAR_H / 7,
                    x + (col + 1) * DIGIT_CHAR_W / 5 + 1,
                    y + (row + 1) * DIGIT_CHAR_H / 7 + 1
                };
                HBRUSH brush = CreateSolidBrush(LCD_TEXT_COLOR);
                FillRect(hdc, &cell, brush);
                DeleteObject(brush);
            }
        }
    }
}

/* Desenha a string de tempo no display LCD. */
static void DrawLcdDisplay(HWND /*hwnd*/, HDC hdc, LPCWSTR timeStr)
{
    /* Fundo escuro do LCD. */
    RECT lcdRect = { LCD_X, LCD_Y, LCD_X + LCD_W, LCD_Y + LCD_H };
    HBRUSH bgBrush = CreateSolidBrush(LCD_BG_COLOR);
    FillRect(hdc, &lcdRect, bgBrush);
    DeleteObject(bgBrush);

    /* Borda sutil. */
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x22, 0x44, 0x22));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, lcdRect.left, lcdRect.top, lcdRect.right, lcdRect.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    /* Desenhar cada caractere centralizado no LCD. */
    int len = (int)wcslen(timeStr);
    int totalWidth = len * DIGIT_CHAR_W;
    int startX = LCD_X + (LCD_W - totalWidth) / 2;
    int startY = LCD_Y + (LCD_H - DIGIT_CHAR_H) / 2;

    /* Ajustar se o texto ultrapassar a área. */
    if (startX < LCD_X + 2)
        startX = LCD_X + 2;

    for (int i = 0; i < len; i++)
    {
        wchar_t wc = timeStr[i];
        int chIdx = -1;

        if (wc >= L'0' && wc <= L'9')
            chIdx = wc - L'0';
        else if (wc == L':')
            chIdx = 10;
        else if (wc == L'-')
            chIdx = 11;

        if (chIdx >= 0)
            DrawLcdChar(hdc, chIdx, startX + i * DIGIT_CHAR_W, startY);
    }
}

/* ------------------------------------------------------------------ */
/* ------------------------------------------------------------------ */
/* Helpers — barra de busca / sliders (M3-T4, M3-T5)                    */
/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */
/* Helpers — desenho escalado por DPI                                   */
/* ------------------------------------------------------------------ */

/* Desenha botão da titlebar com coordenadas escaladas. */
static void DrawTitleButtonScaled(HDC hdc, TitleBarButton btn)
{
    RECT r{};
    switch (btn)
    {
    case BTN_MINIMIZE:
        r = { MainWnd_DPIScale(BTN_MINIMIZE_X), MainWnd_DPIScale(2),
              MainWnd_DPIScale(BTN_MINIMIZE_X + BTN_SIZE), MainWnd_DPIScale(2 + BTN_SIZE) };
        break;
    case BTN_CLOSE:
        r = { MainWnd_DPIScale(BTN_CLOSE_X), MainWnd_DPIScale(2),
              MainWnd_DPIScale(BTN_CLOSE_X + BTN_SIZE), MainWnd_DPIScale(2 + BTN_SIZE) };
        break;
    default:
        return;
    }

    HBRUSH brush = CreateSolidBrush(ButtonColor(btn));
    FillRect(hdc, &r, brush);
    DeleteObject(brush);

    int iconColor = (btn == BTN_CLOSE) ? RGB(0xff, 0x40, 0x40) : RGB(0x00, 0x00, 0x00);
    HPEN pen = CreatePen(PS_SOLID, MainWnd_DPIScale(1), iconColor);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    if (btn == BTN_MINIMIZE)
    {
        int yLine = r.top + r.bottom / 2;
        MoveToEx(hdc, r.left + MainWnd_DPIScale(4), yLine, NULL);
        LineTo(hdc, r.right - MainWnd_DPIScale(4), yLine);
    }
    else if (btn == BTN_CLOSE)
    {
        int pad = MainWnd_DPIScale(4);
        MoveToEx(hdc, r.left + pad, r.top + pad, NULL);
        LineTo(hdc, r.right - pad, r.top + r.bottom - pad);
        MoveToEx(hdc, r.right - pad, r.top + pad, NULL);
        LineTo(hdc, r.left + pad, r.top + r.bottom - pad);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

/* Desenha o display LCD com coordenadas escaladas. */
static void DrawLcdDisplayScaled(HDC hdc)
{
    int x = MainWnd_DPIScale(LCD_X);
    int y = MainWnd_DPIScale(LCD_Y);
    int w = MainWnd_DPIScale(LCD_W);
    int h = MainWnd_DPIScale(LCD_H);

    /* Fundo escuro do LCD. */
    RECT lcdRect = { x, y, x + w, y + h };
    HBRUSH bgBrush = CreateSolidBrush(LCD_BG_COLOR);
    FillRect(hdc, &lcdRect, bgBrush);
    DeleteObject(bgBrush);

    /* Borda sutil. */
    HPEN pen = CreatePen(PS_SOLID, MainWnd_DPIScale(1), RGB(0x22, 0x44, 0x22));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, lcdRect.left, lcdRect.top, lcdRect.right, lcdRect.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    /* Desenhar cada caractere centralizado no LCD. */
    wchar_t timeStr[8] = { 0 };
    double pos = Audio_GetPositionSeconds();
    double dur = Audio_GetDurationSeconds();
    FormatTime(pos, g_showRemaining, dur, timeStr, ARRAYSIZE(timeStr));

    int len = (int)wcslen(timeStr);
    int charW = MainWnd_DPIScale(DIGIT_CHAR_W);
    int charH = MainWnd_DPIScale(DIGIT_CHAR_H);
    int totalWidth = len * charW;
    int startX = x + (w - totalWidth) / 2;
    int startY = y + (h - charH) / 2;

    if (startX < x + MainWnd_DPIScale(2))
        startX = x + MainWnd_DPIScale(2);

    for (int i = 0; i < len; i++)
    {
        wchar_t wc = timeStr[i];
        int chIdx = -1;
        if (wc >= L'0' && wc <= L'9') chIdx = wc - L'0';
        else if (wc == L':') chIdx = 10;
        else if (wc == L'-') chIdx = 11;

        if (chIdx >= 0)
        {
            const unsigned char *fontData;
            if (chIdx == 10) fontData = g_colonChar;
            else if (chIdx == 11) fontData = g_dashChar;
            else fontData = g_digitFont[chIdx];

            int chX = startX + i * charW;
            int cellW = MainWnd_DPIScale(DIGIT_CHAR_W / 5);
            int cellH = MainWnd_DPIScale(DIGIT_CHAR_H / 7);
            if (cellW < 1) cellW = 1;
            if (cellH < 1) cellH = 1;

            for (int row = 0; row < 7; row++)
            {
                unsigned char bits = fontData[row];
                for (int col = 0; col < 5; col++)
                {
                    if (bits & (0x40 >> col)) /* bit ativo — dados ocupam bits 6..2 */
                    {
                        RECT cell = {
                            chX + col * cellW, startY + row * cellH,
                            chX + (col + 1) * cellW + 1,
                            startY + (row + 1) * cellH + 1
                        };
                        HBRUSH b = CreateSolidBrush(LCD_TEXT_COLOR);
                        FillRect(hdc, &cell, b);
                        DeleteObject(b);
                    }
                }
            }
        }
    }
}

/* Desenha slider horizontal com coordenadas de RECT escaladas. */
static void DrawHSliderScaled(HDC hdc, RECT *pr, float value)
{
    /* Track. */
    HBRUSH bgBrush = CreateSolidBrush(SEEK_BAR_BG);
    FillRect(hdc, pr, bgBrush);
    DeleteObject(bgBrush);

    /* Fill. */
    int fillW = (int)((float)(pr->right - pr->left) * value);
    if (fillW > 0)
    {
        RECT fillRect = { pr->left, pr->top, pr->left + fillW, pr->bottom };
        HBRUSH fillBrush = CreateSolidBrush(SEEK_BAR_FILL);
        FillRect(hdc, &fillRect, fillBrush);
        DeleteObject(fillBrush);
    }

    /* Thumb — largura escalada por DPI para manter proporção com a track;
     * clampada para não desenhar fora da track quando value == 0 ou 1. */
    int thumbHalf = MainWnd_DPIScale(3);
    if (thumbHalf < 1) thumbHalf = 1;
    int thumbX = pr->left + fillW;
    RECT thumbRect = { thumbX - thumbHalf, pr->top - MainWnd_DPIScale(2),
                        thumbX + thumbHalf, pr->bottom + MainWnd_DPIScale(2) };
    if (thumbRect.left < pr->left)   thumbRect.left  = pr->left;
    if (thumbRect.right > pr->right) thumbRect.right = pr->right;
    HBRUSH thumbBrush = CreateSolidBrush(SEEK_BAR_THUMB);
    FillRect(hdc, &thumbRect, thumbBrush);
    DeleteObject(thumbBrush);
}

/* ------------------------------------------------------------------ */
/* Helpers — botões de navegação (M3-T6)                                */
/* ------------------------------------------------------------------ */

static BOOL PtInNavBtn(int x, int y, int *pBtnId)
{
    /* Botão Prev. */
    RECT r = { BTN_PREV_X, NAV_Y, BTN_PREV_X + NAV_BTN_W, NAV_Y + NAV_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 1; return TRUE; }

    /* Botão Next. */
    r = { BTN_NEXT_X, NAV_Y, BTN_NEXT_X + NAV_BTN_W, NAV_Y + NAV_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 2; return TRUE; }

    /* Botão Shuffle. */
    r = { BTN_SHUFFLE_X, TOGGLE_Y, BTN_SHUFFLE_X + TOGGLE_W, TOGGLE_Y + TOGGLE_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 3; return TRUE; }

    /* Botão Repeat. */
    r = { BTN_REPEAT_X, TOGGLE_Y, BTN_REPEAT_X + TOGGLE_W, TOGGLE_Y + TOGGLE_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 4; return TRUE; }

    return FALSE;
}

/* ------------------------------------------------------------------ */
/* Helpers — hit-test para sliders e LCD                              */
/* ------------------------------------------------------------------ */

static BOOL PtInLcdArea(int x, int y)
{
    RECT r = { LCD_X, LCD_Y, LCD_X + LCD_W, LCD_Y + LCD_H };
    return PtInRect(&r, (POINT){ x, y });
}

static BOOL PtInSeekBar(int x, int y)
{
    RECT r = { SEEK_X, SEEK_Y, SEEK_X + SEEK_W, SEEK_Y + SEEK_H };
    return PtInRect(&r, (POINT){ x, y });
}

static BOOL PtInVolumeSlider(int x, int y)
{
    RECT r = { VOLUME_SLIDER_X, VOLPAN_Y,
               VOLUME_SLIDER_X + VOLUME_SLIDER_W, VOLPAN_Y + VOLPAN_H };
    return PtInRect(&r, (POINT){ x, y });
}

static BOOL PtInPanSlider(int x, int y)
{
    RECT r = { PAN_SLIDER_X, VOLPAN_Y,
               PAN_SLIDER_X + PAN_SLIDER_W, VOLPAN_Y + VOLPAN_H };
    return PtInRect(&r, (POINT){ x, y });
}

/* M4: Verifica se (x, y) está na área do visualizador. */
static BOOL PtInVizArea(int x, int y)
{
    RECT r = { VIZ_X, VIZ_Y, VIZ_X + VIZ_W, VIZ_Y + VIZ_H };
    return PtInRect(&r, (POINT){ x, y });
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
        /* Calcular fator de escala DPI para esta janela e redimensionar a
         * janela (criada em tamanho lógico 1x) para o tamanho físico real
         * antes do primeiro ShowWindow — senão o conteúdo desenhado em
         * coordenadas escaladas fica cortado dentro de uma janela pequena. */
        ApplyInterfaceScale(hwnd);

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

        /* M4-T1: Inicializar ring buffer de visualização. */
        Audio_VizInit();

        /* Timer para atualização periódica da posição (250ms). */
        SetTimer(hwnd, POSITION_TIMER_ID, 250, NULL);

        /* Timer para scroll do título (30ms — ~33fps suave). */
        SetTimer(hwnd, SCROLL_TIMER_ID, 30, NULL);

        /* Timer do visualizador (~60fps). */
        SetTimer(hwnd, VIZ_TIMER_ID, 16, NULL);

        break;
    }

    case WM_TIMER:
    {
        if (wParam == POSITION_TIMER_ID)
        {
            /* Se estiver tocando, sincronizar seek bar com posição real e
             * invalidar para redraw. */
            if (Audio_GetState() == AUDIO_STATE_PLAYING)
            {
                double dur = Audio_GetDurationSeconds();
                if (dur > 0.0)
                {
                    g_seekValue = (float)(Audio_GetPositionSeconds() / dur);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
        }
        else if (wParam == VIZ_TIMER_ID)
        {
            if (Audio_GetState() == AUDIO_STATE_PLAYING)
            {
                RECT vizRect = {
                    MainWnd_DPIScale(VIZ_X), MainWnd_DPIScale(VIZ_Y),
                    MainWnd_DPIScale(VIZ_X + VIZ_W), MainWnd_DPIScale(VIZ_Y + VIZ_H)
                };
                InvalidateRect(hwnd, &vizRect, FALSE);
            }
        }
        else if (wParam == SCROLL_TIMER_ID)
        {
            /* Atualizar contador de scroll do título. */
            g_scrollElapsedMs += 30;

            /* Invalidar apenas a área do título para redraw. */
            RECT titleRect = { TITLE_X, TITLE_Y, TITLE_X + TITLE_W, TITLE_Y + TITLE_H };
            InvalidateRect(hwnd, &titleRect, FALSE);
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
        int const mx = MainWnd_DPIUnscale((int)(short)LOWORD(lParam));
        int const my = MainWnd_DPIUnscale((int)(short)HIWORD(lParam));

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
        /* M3-T1: LCD toggle (decorrido/restante). */
        else if (PtInLcdArea(mx, my))
        {
            g_showRemaining = !g_showRemaining;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        /* M3-T4: Seek bar dragging. */
        else if (PtInSeekBar(mx, my))
        {
            g_draggingSlider = SLIDER_SEEK;
            /* Calcular valor a partir da posição do mouse. */
            int relX = mx - SEEK_X;
            g_seekValue = PixelToValue(relX, SEEK_W, 0.0f, 1.0f);
            double pos = g_seekValue * Audio_GetDurationSeconds();
            Audio_SeekSeconds(pos);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        /* M3-T5: Volume slider dragging. */
        else if (PtInVolumeSlider(mx, my))
        {
            g_draggingSlider = SLIDER_VOLUME;
            int relX = mx - VOLUME_SLIDER_X;
            g_volumeValue = PixelToValue(relX, VOLUME_SLIDER_W, 0.0f, 1.0f);
            Audio_SetVolume(g_volumeValue);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        /* M3-T5: Pan slider dragging. */
        else if (PtInPanSlider(mx, my))
        {
            g_draggingSlider = SLIDER_PAN;
            int relX = mx - PAN_SLIDER_X;
            g_panValue = PixelToValue(relX, PAN_SLIDER_W, 0.0f, 1.0f);
            /* Converter 0..1 para -1..1 */
            float panReal = g_panValue * 2.0f - 1.0f;
            Audio_SetPan(panReal);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        /* M4-T4: Visualizador toggle (osciloscópio/espectro). */
        else if (PtInVizArea(mx, my))
        {
            g_vizMode = (g_vizMode == VIZ_MODE_OSCILLOSCOPE)
                        ? VIZ_MODE_SPECTRUM
                        : VIZ_MODE_OSCILLOSCOPE;
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        /* M3-T6: Botões de navegação e toggles. */
        else if (my >= NAV_Y && my < NAV_Y + NAV_BTN_H + TOGGLE_GAP + TOGGLE_H)
        {
            int btnId = 0;
            if (PtInNavBtn(mx, my, &btnId))
            {
                switch (btnId)
                {
                case 1: /* Prev */
                    Audio_Previous();
                    break;
                case 2: /* Next */
                    Audio_Next(g_shuffleOn, g_repeatOn);
                    break;
                case 3: /* Shuffle toggle */
                    g_shuffleOn = !g_shuffleOn;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                case 4: /* Repeat toggle */
                    g_repeatOn = !g_repeatOn;
                    InvalidateRect(hwnd, NULL, FALSE);
                    return 0;
                }
            }
        }
        /* Clique na área de botões de controle — apenas captura o foco. */
        else if (my >= BOTTOM_CONTROLS_Y)
        {
            SetForegroundWindow(hwnd);
        }
        else
        {
            SetForegroundWindow(hwnd);
        }
        break;
    }

    /* ------------------------------------------------------------------
     * Movimento do mouse — slider dragging e hover.
     * ------------------------------------------------------------------ */
    case WM_MOUSEMOVE:
    {
        int const mx = MainWnd_DPIUnscale((int)(short)LOWORD(lParam));
        int const my = MainWnd_DPIUnscale((int)(short)HIWORD(lParam));

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

        /* Slider dragging. */
        if (g_draggingSlider != SLIDER_NONE)
        {
            switch (g_draggingSlider)
            {
            case SLIDER_SEEK:
            {
                int relX = mx - SEEK_X;
                g_seekValue = PixelToValue(relX, SEEK_W, 0.0f, 1.0f);
                double pos = g_seekValue * Audio_GetDurationSeconds();
                Audio_SeekSeconds(pos);
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
            case SLIDER_VOLUME:
            {
                int relX = mx - VOLUME_SLIDER_X;
                g_volumeValue = PixelToValue(relX, VOLUME_SLIDER_W, 0.0f, 1.0f);
                Audio_SetVolume(g_volumeValue);
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
            case SLIDER_PAN:
            {
                int relX = mx - PAN_SLIDER_X;
                g_panValue = PixelToValue(relX, PAN_SLIDER_W, 0.0f, 1.0f);
                float panReal = g_panValue * 2.0f - 1.0f;
                Audio_SetPan(panReal);
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
            default:
                break;
            }
        }
        break;
    }

    /* ------------------------------------------------------------------
     * Soltar botão — titlebar ou controles.
     * M2-T4/M3: chama funções de audio.h para botões de controle.
     * ------------------------------------------------------------------ */
    case WM_LBUTTONUP:
    {
        int const mx = MainWnd_DPIUnscale((int)(short)LOWORD(lParam));
        int const my = MainWnd_DPIUnscale((int)(short)HIWORD(lParam));

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

        /* Soltar slider — parar dragging. */
        if (g_draggingSlider != SLIDER_NONE && my >= SEEK_Y)
        {
            g_draggingSlider = SLIDER_NONE;
        }

        /* Verificar se soltou num botão de controle inferior. */
        if (my >= BOTTOM_CONTROLS_Y)
        {
            RECT r;
            /* Botão Abrir. */
            r = { BTN_OPEN_X, BTN_OPEN_Y, BTN_OPEN_X + BTN_OPEN_W, BTN_OPEN_Y + 16 };
            if (PtInRect(&r, (POINT){ mx, my }))
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
                    g_currentTrack = 0;
                    g_seekValue = 0.0f;
                    g_scrollElapsedMs = 0;
                }
            }
            else
            {
                /* Botão Play. */
                r = { BTN_PLAY_X, BTN_PLAY_Y, BTN_PLAY_X + 30, BTN_PLAY_Y + 16 };
                if (PtInRect(&r, (POINT){ mx, my }))
                {
                    Audio_Play();
                }
                else
                {
                    /* Botão Pause. */
                    r = { BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_X + 30, BTN_PAUSE_Y + 16 };
                    if (PtInRect(&r, (POINT){ mx, my }))
                    {
                        Audio_Pause();
                    }
                    else
                    {
                        /* Botão Stop. */
                        r = { BTN_STOP_X, BTN_STOP_Y, BTN_STOP_X + 30, BTN_STOP_Y + 16 };
                        if (PtInRect(&r, (POINT){ mx, my }))
                        {
                            Audio_Stop();
                            g_seekValue   = 0.0f;
                            g_scrollElapsedMs = 0;
                            /* Forçar redraw imediato — o timer para quando não está tocando,
                             * então InvalidateRect sozinho pode não gerar WM_PAINT rápido o suficiente. */
                            InvalidateRect(hwnd, NULL, FALSE);
                            UpdateWindow(hwnd);
                        }
                    }
                }
            }
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        /* Double buffer — desenhar em memória e copiar de uma vez.
         * Dimensões escaladas pelo DPI para telas high-DPI (4K etc.). */
        HDC memDC = CreateCompatibleDC(hdc);
        int scaledW = MainWnd_DPIScale(MAINWND_WIDTH);
        int scaledH = MainWnd_DPIScale(MAINWND_HEIGHT);
        HBITMAP memBM = CreateCompatibleBitmap(hdc, scaledW, scaledH);
        HGDIOBJ oldObj = SelectObject(memDC, memBM);

        /* Fundo da janela. */
        HBRUSH bgBrush = CreateSolidBrush(BG_COLOR);
        RECT fullRect = { 0, 0, scaledW, scaledH };
        FillRect(memDC, &fullRect, bgBrush);
        DeleteObject(bgBrush);

        /* ------------------------------------------------------------------
         * Barra de título (faixa no topo).
         * ------------------------------------------------------------------ */
        {
            RECT r = { 0, 0, scaledW, MainWnd_DPIScale(TITLEBAR_HEIGHT) };
            HBRUSH b = CreateSolidBrush(TITLEBAR_COLOR);
            FillRect(memDC, &r, b); DeleteObject(b);

            DrawTitleButtonScaled(memDC, BTN_MINIMIZE);
            DrawTitleButtonScaled(memDC, BTN_CLOSE);
        }

        /* ------------------------------------------------------------------
         * Display LCD de tempo — M3-T1
         * ------------------------------------------------------------------ */
        {
            wchar_t timeStr[8] = { 0 };
            double pos = Audio_GetPositionSeconds();
            double dur = Audio_GetDurationSeconds();
            FormatTime(pos, g_showRemaining, dur, timeStr, ARRAYSIZE(timeStr));
            DrawLcdDisplayScaled(memDC);
        }

        /* ------------------------------------------------------------------
         * Informações de áudio (kbps/kHz/estéreo) — M3-T3
         * ------------------------------------------------------------------ */
        {
            BOOL loaded    = Audio_HasTrack();
            int sampleRate = loaded ? Audio_GetFileSampleRate() : 0;
            int channels   = loaded ? Audio_GetChannels() : 0;
            int bitrate    = loaded ? Audio_GetBitrateKbps() : 0;

            RECT infoRect = {
                MainWnd_DPIScale(TITLE_X),
                MainWnd_DPIScale(TITLE_Y) + MainWnd_DPIScale(14),
                MainWnd_DPIScale(TITLE_X) + MainWnd_DPIScale(TITLE_W),
                MainWnd_DPIScale(TITLE_Y) + MainWnd_DPIScale(TITLE_H)
            };
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, AUDIO_INFO_COLOR);

            wchar_t infoBuf[48] = { 0 };
            wchar_t khzBuf[16] = { 0 };
            if (sampleRate > 0)
            {
                int tenths = (sampleRate + 50) / 100;
                if (tenths % 10 == 0)
                    wsprintfW(khzBuf, L"%d kHz", tenths / 10);
                else
                    wsprintfW(khzBuf, L"%d.%d kHz", tenths / 10, tenths % 10);
            }
            const wchar_t* stereo = channels == 2 ? L" st" : (channels == 1 ? L" mono" : L"");
            if (bitrate > 0 && sampleRate > 0)
                wsprintfW(infoBuf, L"%d kbps  %s%s", bitrate, khzBuf, stereo);
            else if (bitrate > 0)
                wsprintfW(infoBuf, L"%d kbps", bitrate);
            else if (sampleRate > 0)
                wsprintfW(infoBuf, L"%s%s", khzBuf, stereo);

            if (infoBuf[0] != L'\0')
                DrawTextW(memDC, infoBuf, (int)wcslen(infoBuf), &infoRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Título rolando — M3-T2
         * ------------------------------------------------------------------ */
        {
            wchar_t fileName[MAX_PATH] = { 0 };
            Audio_GetTrackName(fileName, ARRAYSIZE(fileName));
            SIZE textSize;
            GetTextExtentPoint32W(memDC, fileName, (int)wcslen(fileName), &textSize);

            /* Tudo em pixels reais; o texto é recortado à área do título
             * para nunca invadir o LCD nem a borda da janela. */
            RECT area = {
                MainWnd_DPIScale(TITLE_X), MainWnd_DPIScale(TITLE_Y),
                MainWnd_DPIScale(TITLE_X + TITLE_W), MainWnd_DPIScale(TITLE_Y) + MainWnd_DPIScale(14)
            };
            int areaW = area.right - area.left;

            if (textSize.cx > 0)
            {
                int scrollOffset = 0;
                CalcScrollOffset(&scrollOffset, textSize.cx, areaW, g_scrollElapsedMs,
                                 1500, TITLE_SCROLL_SPEED * (float)MainWnd_DPIScale(1));

                RECT textRect = area;
                if (textSize.cx <= areaW)
                    textRect.left = area.left + (areaW - textSize.cx) / 2;
                else
                    textRect.left = area.left - scrollOffset;
                textRect.right = textRect.left + textSize.cx;

                int savedDC = SaveDC(memDC);
                IntersectClipRect(memDC, area.left, area.top, area.right, area.bottom);
                SetBkMode(memDC, TRANSPARENT);
                SetTextColor(memDC, TITLE_TEXT_COLOR);
                DrawTextW(memDC, fileName, (int)wcslen(fileName), &textRect,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                RestoreDC(memDC, savedDC);
            }
        }

        /* ------------------------------------------------------------------
         * Barra de busca (seek) — M3-T4
         * ------------------------------------------------------------------ */
        {
            RECT seekR = { MainWnd_DPIScale(SEEK_X), MainWnd_DPIScale(SEEK_Y),
                           MainWnd_DPIScale(SEEK_X + SEEK_W), MainWnd_DPIScale(SEEK_Y + SEEK_H) };
            DrawHSliderScaled(memDC, &seekR, g_seekValue);

            wchar_t seekLabel[16] = { 0 };
            FormatTime(Audio_GetPositionSeconds(), FALSE, Audio_GetDurationSeconds(), seekLabel, ARRAYSIZE(seekLabel));
            RECT labelR = { seekR.left, seekR.bottom + MainWnd_DPIScale(1),
                            seekR.left + MainWnd_DPIScale(30), seekR.bottom + MainWnd_DPIScale(14) };
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, AUDIO_INFO_COLOR);
            DrawTextW(memDC, seekLabel, (int)wcslen(seekLabel), &labelR, DT_LEFT | DT_TOP | DT_SINGLELINE);

            wchar_t durLabel[16] = { 0 };
            FormatTime(Audio_GetDurationSeconds(), FALSE, Audio_GetDurationSeconds(), durLabel, ARRAYSIZE(durLabel));
            RECT durR = { seekR.right - MainWnd_DPIScale(35), seekR.bottom + MainWnd_DPIScale(1),
                          seekR.right, seekR.bottom + MainWnd_DPIScale(14) };
            DrawTextW(memDC, durLabel, (int)wcslen(durLabel), &durR, DT_RIGHT | DT_TOP | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Volume e balanço — M3-T5
         * ------------------------------------------------------------------ */
        {
            RECT volR = { MainWnd_DPIScale(VOLUME_SLIDER_X), MainWnd_DPIScale(VOLPAN_Y),
                          MainWnd_DPIScale(VOLUME_SLIDER_X + VOLUME_SLIDER_W), MainWnd_DPIScale(VOLPAN_Y + VOLPAN_H) };
            DrawHSliderScaled(memDC, &volR, g_volumeValue);

            RECT volLabelR = { volR.left, volR.top - 1, volR.left + MainWnd_DPIScale(20), volR.top + 1 };
            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, AUDIO_INFO_COLOR);
            DrawTextW(memDC, L"VOL", 3, &volLabelR, DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT panR = { MainWnd_DPIScale(PAN_SLIDER_X), MainWnd_DPIScale(VOLPAN_Y),
                          MainWnd_DPIScale(PAN_SLIDER_X + PAN_SLIDER_W), MainWnd_DPIScale(VOLPAN_Y + VOLPAN_H) };
            DrawHSliderScaled(memDC, &panR, g_panValue);

            RECT panLabelR = { panR.left, panR.top - 1, panR.left + MainWnd_DPIScale(20), panR.top + 1 };
            DrawTextW(memDC, L"BAL", 3, &panLabelR, DT_LEFT | DT_TOP | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Botões de navegação e toggles — M3-T6
         * ------------------------------------------------------------------ */
        {
            RECT prevR = { MainWnd_DPIScale(BTN_PREV_X), MainWnd_DPIScale(NAV_Y),
                           MainWnd_DPIScale(BTN_PREV_X + NAV_BTN_W), MainWnd_DPIScale(NAV_Y + NAV_BTN_H) };
            HBRUSH b = CreateSolidBrush(NAV_BG_NORMAL); FillRect(memDC, &prevR, b); DeleteObject(b);
            SetBkMode(memDC, TRANSPARENT); SetTextColor(memDC, RGB(0xff, 0xff, 0xcc));
            DrawTextW(memDC, L"<<", 2, &prevR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT nextR = { MainWnd_DPIScale(BTN_NEXT_X), MainWnd_DPIScale(NAV_Y),
                           MainWnd_DPIScale(BTN_NEXT_X + NAV_BTN_W), MainWnd_DPIScale(NAV_Y + NAV_BTN_H) };
            b = CreateSolidBrush(NAV_BG_NORMAL); FillRect(memDC, &nextR, b); DeleteObject(b);
            DrawTextW(memDC, L">>", 2, &nextR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT shufR = { MainWnd_DPIScale(BTN_SHUFFLE_X), MainWnd_DPIScale(TOGGLE_Y),
                           MainWnd_DPIScale(BTN_SHUFFLE_X + TOGGLE_W), MainWnd_DPIScale(TOGGLE_Y + TOGGLE_H) };
            b = CreateSolidBrush(g_shuffleOn ? TOGGLE_ON_BG : TOGGLE_OFF_BG); FillRect(memDC, &shufR, b); DeleteObject(b);
            SetTextColor(memDC, g_shuffleOn ? RGB(0xff, 0xff, 0xcc) : AUDIO_INFO_COLOR);
            DrawTextW(memDC, L"SHUF", 4, &shufR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT repR = { MainWnd_DPIScale(BTN_REPEAT_X), MainWnd_DPIScale(TOGGLE_Y),
                          MainWnd_DPIScale(BTN_REPEAT_X + TOGGLE_W), MainWnd_DPIScale(TOGGLE_Y + TOGGLE_H) };
            b = CreateSolidBrush(g_repeatOn ? TOGGLE_ON_BG : TOGGLE_OFF_BG); FillRect(memDC, &repR, b); DeleteObject(b);
            SetTextColor(memDC, g_repeatOn ? RGB(0xff, 0xff, 0xcc) : AUDIO_INFO_COLOR);
            DrawTextW(memDC, L"REP", 3, &repR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Botões de controle inferior (Abrir/Play/Pause/Stop).
         * ------------------------------------------------------------------ */
        {
            RECT openR = { MainWnd_DPIScale(BTN_OPEN_X), MainWnd_DPIScale(BTN_OPEN_Y),
                           MainWnd_DPIScale(BTN_OPEN_X + BTN_OPEN_W), MainWnd_DPIScale(BTN_OPEN_Y + 16) };
            HBRUSH b = CreateSolidBrush(CTRL_BG_NORMAL); FillRect(memDC, &openR, b); DeleteObject(b);
            SetBkMode(memDC, TRANSPARENT); SetTextColor(memDC, RGB(0xff, 0xff, 0xcc));
            DrawTextW(memDC, L"Abrir", 5, &openR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT playR = { MainWnd_DPIScale(BTN_PLAY_X), MainWnd_DPIScale(BTN_PLAY_Y),
                           MainWnd_DPIScale(BTN_PLAY_X + 30), MainWnd_DPIScale(BTN_PLAY_Y + 16) };
            b = CreateSolidBrush(CTRL_BG_NORMAL); FillRect(memDC, &playR, b); DeleteObject(b);
            DrawTextW(memDC, L">", 1, &playR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT pauseR = { MainWnd_DPIScale(BTN_PAUSE_X), MainWnd_DPIScale(BTN_PAUSE_Y),
                            MainWnd_DPIScale(BTN_PAUSE_X + 30), MainWnd_DPIScale(BTN_PAUSE_Y + 16) };
            b = CreateSolidBrush(CTRL_BG_NORMAL); FillRect(memDC, &pauseR, b); DeleteObject(b);
            DrawTextW(memDC, L"||", 2, &pauseR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT stopR = { MainWnd_DPIScale(BTN_STOP_X), MainWnd_DPIScale(BTN_STOP_Y),
                           MainWnd_DPIScale(BTN_STOP_X + 30), MainWnd_DPIScale(BTN_STOP_Y + 16) };
            b = CreateSolidBrush(CTRL_BG_NORMAL); FillRect(memDC, &stopR, b); DeleteObject(b);
            DrawTextW(memDC, L"[]", 2, &stopR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Visualizador — M4-T2 (osciloscópio) / M4-T3 (espectro).
         * ------------------------------------------------------------------ */
        {
            int vizX = MainWnd_DPIScale(VIZ_X);
            int vizY = MainWnd_DPIScale(VIZ_Y);
            int vizW = MainWnd_DPIScale(VIZ_W);
            int vizH = MainWnd_DPIScale(VIZ_H);

            if (g_vizMode == VIZ_MODE_OSCILLOSCOPE)
            {
                DrawOscilloscope(memDC, vizX, vizY, vizW, vizH);
            }
            else
            {
                DrawSpectrum(memDC, vizX, vizY, vizW, vizH);
            }
        }

        /* Copiar buffer para a tela. */
        BitBlt(hdc, 0, 0, scaledW, scaledH, memDC, 0, 0, SRCCOPY);

        SelectObject(memDC, oldObj);
        DeleteObject(memBM);
        DeleteDC(memDC);

        EndPaint(hwnd, &ps);
        break;
    }

    /* ------------------------------------------------------------------
     * Menu de contexto — Configurações > Aparência > Escala da interface.
     * Clique direito em qualquer ponto da janela.
     * ------------------------------------------------------------------ */
    case WM_CONTEXTMENU:
    {
        struct ScaleMenuItem { int id; int percent; const wchar_t *label; };
        static const ScaleMenuItem items[] = {
            { IDM_SCALE_AUTO, 0,   L"Automática (recomendado)" },
            { IDM_SCALE_80,   80,  L"80%" },
            { IDM_SCALE_90,   90,  L"90%" },
            { IDM_SCALE_100,  100, L"100%" },
            { IDM_SCALE_110,  110, L"110%" },
            { IDM_SCALE_125,  125, L"125%" },
            { IDM_SCALE_150,  150, L"150%" },
        };

        HMENU hScale = CreatePopupMenu();
        for (size_t i = 0; i < ARRAYSIZE(items); i++)
        {
            UINT flags = MF_STRING |
                (items[i].percent == g_userScalePercent ? MF_CHECKED : MF_UNCHECKED);
            AppendMenuW(hScale, flags, items[i].id, items[i].label);
        }

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hScale, L"Aparência > Escala da interface");

        int x = (int)(short)LOWORD(lParam);
        int y = (int)(short)HIWORD(lParam);
        if (x == -1 && y == -1)
        {
            RECT r{};
            GetWindowRect(hwnd, &r);
            x = r.left;
            y = r.top;
        }

        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, x, y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
        break;
    }

    case WM_COMMAND:
    {
        int cmdId = LOWORD(wParam);
        static const struct { int id; int percent; } scaleCmds[] = {
            { IDM_SCALE_AUTO, 0 },   { IDM_SCALE_80, 80 },   { IDM_SCALE_90, 90 },
            { IDM_SCALE_100, 100 }, { IDM_SCALE_110, 110 }, { IDM_SCALE_125, 125 },
            { IDM_SCALE_150, 150 },
        };
        for (size_t i = 0; i < ARRAYSIZE(scaleCmds); i++)
        {
            if (scaleCmds[i].id == cmdId)
            {
                g_userScalePercent = scaleCmds[i].percent;
                ApplyInterfaceScale(hwnd);
                break;
            }
        }
        break;
    }

    case WM_DESTROY:
    {
        KillTimer(hwnd, POSITION_TIMER_ID);
        KillTimer(hwnd, SCROLL_TIMER_ID);
        KillTimer(hwnd, VIZ_TIMER_ID);
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
