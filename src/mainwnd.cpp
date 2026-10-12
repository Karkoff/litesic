#include "ui_accessibility.h"
#include <oleacc.h>
#include "ui_scale.h"
#include "runtime.h"
#include "ui_tooltip.h"
#include "i18n.h"
#include "language_selection.h"
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
#include "playback.h"
#include "plwnd.h"
#include "ui_player.h"
#include "eqwnd.h"
#include "skin.h"

#include <windows.h>
#include <cmath>
#include <algorithm>
#include <commctrl.h>

static void UpdateTooltipRects(HWND hwnd);

/* DPI scale factor — inicializa em WM_CREATE. */
static float g_dpiScale = 1.0f;
static bool g_visualizerVisible=true;

/* Escala de interface definida pelo usuário (Configurações > Aparência).
 * 0 = Automática (segue só o DPI do monitor); caso contrário, percentual
 * (80/90/100/110/125/150) multiplicado pelo fator de DPI do monitor.
 * Persistida com as demais preferências de aparência. */
static int g_userScalePercent = 0;

/* ------------------------------------------------------------------ */
/* DPI scaling — M8-T3                                                 */
/* ------------------------------------------------------------------ */

static float GetDpiScaleFactor(HWND hwnd)
{
    return Ui::WindowDpi(hwnd);
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
float MainWnd_UserScaleMultiplier() { return UserScaleMultiplier(); }

/* Recalcula g_dpiScale (DPI do monitor x escala do usuário) e redimensiona/
 * reposiciona a janela para o novo tamanho físico, mantendo-a centrada em
 * torno do centro atual. Usado tanto na criação quanto ao trocar a escala
 * pelo menu de contexto. */
static void ApplyInterfaceScale(HWND hwnd)
{
    g_dpiScale = Ui::FitScale(GetDpiScaleFactor(hwnd) * UserScaleMultiplier(),MAINWND_WIDTH,PlayerLayoutHeight(g_visualizerVisible),Ui::MonitorWork(hwnd));

    int scaledW = MainWnd_DPIScale(MAINWND_WIDTH);
    int scaledH = MainWnd_DPIScale(PlayerLayoutHeight(g_visualizerVisible));

    RECT cur{};
    GetWindowRect(hwnd, &cur);
    int cx = cur.left + (cur.right - cur.left) / 2;
    int cy = cur.top + (cur.bottom - cur.top) / 2;
    int x = cx - scaledW / 2;
    int y = cy - scaledH / 2;
    RECT fitted{x,y,x+scaledW,y+scaledH};WindowSnap::KeepVisible(fitted,Ui::MonitorWork(hwnd));x=fitted.left;y=fitted.top;

    SetWindowPos(hwnd, NULL, x, y, scaledW, scaledH, SWP_NOZORDER | SWP_NOACTIVATE);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateTooltipRects(hwnd);
}

/* ------------------------------------------------------------------ */
/* Classe da janela — nome único                                        */
/* ------------------------------------------------------------------ */

#define CLASS_NAME L"LiteSic.MainWnd"

/* Cor de fundo da janela (placeholder — arte real vem depois). */
#define BG_COLOR (Ui::NativeColor(Ui::Background))

/* Cor da barra de título (placeholder). */
#define TITLEBAR_COLOR (Ui::NativeColor(Ui::Border))

/* Cores dos botões. */
#define BTN_BG_NORMAL (Ui::NativeColor(Ui::Surface))
#define BTN_BG_HOVER (Ui::NativeColor(Ui::Hover))
#define BTN_BG_PRESSED (Ui::NativeColor(Ui::palette.pressed))

/* Cor do ícone "X" (fechar) — vermelho. */
#define CLOSE_ICON_COLOR (Ui::NativeColor(Ui::Danger))

/* Cores dos botões de controle (play/pause/stop). */
#define CTRL_BG_NORMAL (Ui::NativeColor(Ui::Surface))
#define CTRL_BG_HOVER (Ui::NativeColor(Ui::Hover))

/* Cores do display LCD. */
#define LCD_BG_COLOR (Ui::NativeColor(Ui::Display))
#define LCD_TEXT_COLOR (Ui::NativeColor(Ui::Green)) /* verde LCD */
#define LCD_DIM_COLOR (Ui::NativeColor(Ui::Active))

/* Cor do título rolando. */
#define TITLE_TEXT_COLOR (Ui::NativeColor(Ui::Text))

/* Cores dos indicadores de áudio (kbps/kHz). */
#define AUDIO_INFO_COLOR (Ui::NativeColor(Ui::Muted))

/* Cor da barra de busca. */
#define SEEK_BAR_BG (Ui::NativeColor(Ui::palette.sliderTrack))
#define SEEK_BAR_FILL (Ui::NativeColor(Ui::palette.sliderFill))
#define SEEK_BAR_THUMB (Ui::NativeColor(Ui::Text))

/* Cores dos sliders de volume/pan. */
#define SLIDER_BG (Ui::NativeColor(Ui::palette.sliderTrack))
#define SLIDER_FILL (Ui::NativeColor(Ui::palette.sliderFill))
#define SLIDER_THUMB (Ui::NativeColor(Ui::Text))

/* Cores dos botões de navegação. */
#define NAV_BG_NORMAL (Ui::NativeColor(Ui::Surface))
#define NAV_BG_HOVER (Ui::NativeColor(Ui::Hover))

/* Cores dos toggles. */
#define TOGGLE_ON_BG (Ui::NativeColor(Ui::Active))
#define TOGGLE_OFF_BG (Ui::NativeColor(Ui::Surface))

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
static DWORD g_scrollTick = 0;
static bool g_scrollTimerActive = false;
static bool g_titleOverflow = false;
static std::wstring g_scrollTitle;
static void UpdateScrollTimer(HWND hwnd) {
    bool needed=g_titleOverflow && IsWindowVisible(hwnd) && !IsIconic(hwnd);
    if(needed==g_scrollTimerActive)return;
    if(needed){g_scrollTick=GetTickCount();g_scrollTimerActive=SetTimer(hwnd,SCROLL_TIMER_ID,30,nullptr)!=0;}
    else {KillTimer(hwnd,SCROLL_TIMER_ID);g_scrollTimerActive=false;}
}

/* M3-T4/M3-T5: estado de arraste do slider. */
static SliderType g_draggingSlider = SLIDER_NONE;
static float    g_seekValue   = 0.0f; /* 0..1 normalizado para seek */
static float    g_volumeValue = 0.8f; /* 0..1 */
static float    g_panValue    = 0.5f; /* -1..1 (0.5 = centro, panReal = 0.0) */

/* M3-T6: estado dos toggles. */
static BOOL g_shuffleOn = FALSE;
static RepeatMode g_repeatMode = RepeatMode::Off;

static uint64_t g_displayTrack = 0;
static Ui::Canvas g_canvas;
static int g_controlHover=0, g_controlPressed=0, g_controlFocus=0;
static bool g_playbackError=false;
static void CreatePlayerTooltips(HWND hwnd);
static void UpdateTooltipRects(HWND hwnd) { Ui::Tooltips::Hide(hwnd); }

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
        HPEN pen = CreatePen(PS_SOLID, 1, Ui::NativeColor(Ui::Text));
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
    HPEN pen = CreatePen(PS_SOLID, 1, Ui::NativeColor(Ui::Border));
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

    int iconColor = (btn == BTN_CLOSE) ? Ui::NativeColor(Ui::Danger) : Ui::NativeColor(Ui::Text);
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
    HPEN pen = CreatePen(PS_SOLID, MainWnd_DPIScale(1), Ui::NativeColor(Ui::Border));
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

    r = { BTN_PLAY_X, NAV_Y, BTN_PLAY_X + CTRL_BTN_W, NAV_Y + CTRL_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 5; return TRUE; }
    r = { BTN_PAUSE_X, NAV_Y, BTN_PAUSE_X + CTRL_BTN_W, NAV_Y + CTRL_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 6; return TRUE; }
    r = { BTN_STOP_X, NAV_Y, BTN_STOP_X + CTRL_BTN_W, NAV_Y + CTRL_BTN_H };
    if (PtInRect(&r, (POINT){ x, y })) { *pBtnId = 7; return TRUE; }

    return FALSE;
}

static void ExecutePlayerControl(HWND hwnd,int id) {
    switch(id) {
    case 1: PlayerPlayback().Previous(g_shuffleOn,g_repeatMode==RepeatMode::All); PlWnd_Refresh(true); break;
    case 2: PlayerPlayback().Next(g_shuffleOn,g_repeatMode==RepeatMode::All); PlWnd_Refresh(true); break;
    case 3: g_shuffleOn=!g_shuffleOn; break;
    case 4: g_repeatMode=NextRepeatMode(g_repeatMode); break;
    case 5: g_playbackError=!PlWnd_PlaySelected(); break;
    case 6: Audio_Pause(); break;
    case 7: Audio_Stop(); g_seekValue=0; g_scrollElapsedMs=0; g_playbackError=false; break;
    case 11: PlWnd_Toggle(); break;
    case 12: EqWnd_Toggle(hwnd); break;
    case 13: g_vizMode=VIZ_MODE_SPECTRUM; Ui::SaveAppearanceValue(L"Spectrum",1); break;
    case 14: g_vizMode=VIZ_MODE_OSCILLOSCOPE; Ui::SaveAppearanceValue(L"Spectrum",0); break;
    case 16:
        g_visualizerVisible=!g_visualizerVisible;
        Ui::SaveAppearanceValue(L"VisualizerVisible",g_visualizerVisible);
        g_dpiScale=Ui::FitScale(g_dpiScale,MAINWND_WIDTH,PlayerLayoutHeight(g_visualizerVisible),Ui::MonitorWork(hwnd));
        SetWindowPos(hwnd,nullptr,0,0,MainWnd_DPIScale(MAINWND_WIDTH),
            MainWnd_DPIScale(PlayerLayoutHeight(g_visualizerVisible)),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        UpdateTooltipRects(hwnd);
        break;
    }
}

namespace {
bool topmost=false,globalHotkeys=false;
constexpr UINT PlaybackCommand=WM_APP+97;
HWND playerWindow=nullptr;
void ReleaseHotkeys(HWND hwnd){for(int i=0;i<4;++i)UnregisterHotKey(hwnd,6100+i);}
bool RegisterPlaybackHotkeys(HWND hwnd){
    ReleaseHotkeys(hwnd);
    const UINT keys[]={VK_F9,VK_F10,VK_F11,VK_F12};
    for(int i=0;i<4;++i)if(!RegisterHotKey(hwnd,6100+i,MOD_CONTROL|MOD_ALT|MOD_NOREPEAT,keys[i])){ReleaseHotkeys(hwnd);return false;}
    return true;
}
BOOL CALLBACK TopmostWindow(HWND hwnd,LPARAM value){
    wchar_t name[80]{};GetClassNameW(hwnd,name,80);
    if(wcsncmp(name,L"LiteSic.",8)==0&&wcscmp(name,L"LiteSic.TooltipWnd"))
        SetWindowPos(hwnd,value?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
    return TRUE;
}
}
bool MainWnd_SetGlobalHotkeys(HWND hwnd,bool enabled){
    if(enabled&&!RegisterPlaybackHotkeys(hwnd))return false;
    if(!enabled)ReleaseHotkeys(hwnd);
    globalHotkeys=enabled;Ui::SaveAppearanceValue(L"GlobalHotkeys",enabled);return true;
}
void MainWnd_ApplyTopmost(HWND){EnumThreadWindows(GetCurrentThreadId(),TopmostWindow,topmost);}
bool MainWnd_TranslateShortcut(MSG* msg){
    if(msg->message!=WM_KEYDOWN||!playerWindow)return false;
    HWND root=GetAncestor(msg->hwnd,GA_ROOT);wchar_t cls[80]{},child[80]{};
    GetClassNameW(root,cls,80);GetClassNameW(msg->hwnd,child,80);
    if(wcscmp(cls,L"LiteSic.MainWnd")&&wcscmp(cls,L"LiteSic.PlaylistWnd")&&wcscmp(cls,L"LiteSic.EqualizerWnd"))return false;
    if(!wcscmp(child,L"Edit")||!wcscmp(child,L"EDIT"))return false;
    bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0;
    if((GetKeyState(VK_MENU)&0x8000)||(GetKeyState(VK_SHIFT)&0x8000))return false;
    int action=0;
    if(ctrl){if(msg->wParam=='D')action=3;else return false;}
    else switch(msg->wParam){case 'Z':action=1;break;case 'B':action=2;break;case 'X':action=5;break;case 'C':action=6;break;case 'V':action=7;break;case 'L':action=20;break;
        // EQ and playlist retain arrow navigation; player sliders retain adjustment.
        case VK_LEFT:case VK_RIGHT:if((root==playerWindow&&!(g_controlFocus>=8&&g_controlFocus<=10))||!wcscmp(cls,L"LiteSic.PlaylistWnd"))action=msg->wParam==VK_LEFT?21:22;break;}
    if(!action)return false;
    SendMessageW(playerWindow,PlaybackCommand,action,0);return true;
}
static PlayerViewState PlayerSnapshot() {
    PlayerViewState s;
    s.loaded=Audio_HasTrack(); s.playback=Audio_GetState();
    s.shuffle=g_shuffleOn; s.repeat=g_repeatMode; s.visualization=g_vizMode;
    s.visualizerVisible=g_visualizerVisible;
    s.seek=g_seekValue; s.volume=g_volumeValue; s.pan=g_panValue;
    s.hover=g_controlHover; s.pressed=g_controlPressed; s.focus=g_controlFocus;
    s.titleHover=g_hoverBtn; s.titlePressed=g_pressedBtn;
    s.playlistVisible=PlWnd_IsVisible(); s.equalizerVisible=EqWnd_IsVisible(); s.hasPlaylist=PlayerPlaylist().Count()>0;
    wchar_t time[16]{},duration[16]{},name[MAX_PATH]{};
    FormatTime(Audio_GetPositionSeconds(),g_showRemaining,Audio_GetDurationSeconds(),time,ARRAYSIZE(time));
    FormatTime(Audio_GetDurationSeconds(),FALSE,Audio_GetDurationSeconds(),duration,ARRAYSIZE(duration));
    s.time=time; s.duration=s.loaded?duration:L"--:--";
    Audio_GetTrackName(name,ARRAYSIZE(name)); s.title=name;
    if(const Track* track=PlayerPlaylist().At(PlayerPlaylist().GetCurrentIndex())) s.title=track->displayTitle;
    if(!s.loaded) s.title=I18n::Text("player.welcome").c_str();
    if(s.title!=g_scrollTitle){g_scrollTitle=s.title;g_scrollElapsedMs=0;}
    s.titleElapsedMs=g_scrollElapsedMs;
    s.subtitle=!s.loaded?I18n::Text("player.add_hint").c_str():s.playback==AUDIO_STATE_PLAYING?I18n::Text("player.playing").c_str():
        s.playback==AUDIO_STATE_PAUSED?I18n::Text("player.paused").c_str():I18n::Text("player.ready").c_str();
    if(g_playbackError) s.subtitle=I18n::Text("errors.play_track").c_str();
    if(s.loaded) {
        wchar_t info[96]{};
        int rate=Audio_GetFileSampleRate(),bitrate=Audio_GetBitrateKbps(),channels=Audio_GetChannels();
        swprintf(info,ARRAYSIZE(info),L"%.1f kHz   ·   %ls",rate/1000.0,
            channels==2?I18n::Text("audio.stereo").c_str():channels==1?I18n::Text("audio.mono").c_str():I18n::Text("audio.multichannel").c_str());
        s.technical=info;
        if(bitrate>0) s.technical+=L"   ·   "+std::to_wstring(bitrate)+L" kbps";
    } else s.technical=I18n::Text("player.native").c_str();
    return s;
}

static std::wstring PlayerTooltip(int id) {
    const char* key=nullptr;
    switch(id){
    case 1:key="tooltip.previous";break;case 2:key="tooltip.next";break;
    case 3:key=g_shuffleOn?"tooltip.shuffle_off":"tooltip.shuffle_on";break;
    case 4:key=g_repeatMode==RepeatMode::One?"tooltip.repeat_one":g_repeatMode==RepeatMode::All?"tooltip.repeat_all":"tooltip.repeat_off";break;
    case 5:key="tooltip.play";break;case 6:key="tooltip.pause";break;case 7:key="tooltip.stop";break;
    case 8:key="tooltip.seek";break;
    case 9:return I18n::Format("tooltip.volume",{{"value",std::to_wstring(int(std::lround(g_volumeValue*100)))}});
    case 10:return I18n::Format("tooltip.balance",{{"value",std::to_wstring(int(std::lround((g_panValue*2-1)*100)))}});
    case 11:key=PlWnd_IsVisible()?"tooltip.hide_playlist":"tooltip.show_playlist";break;
    case 12:key=EqWnd_IsVisible()?"tooltip.hide_eq":"tooltip.show_eq";break;
    case 13:key="tooltip.fft";break;case 14:key="tooltip.osc";break;
    case 15:return PlayerSnapshot().title;
    case 16:key=g_visualizerVisible?"tooltip.hide_viz":"tooltip.show_viz";break;
    case 17:key="tooltip.minimize";break;case 18:key="tooltip.close";break;case 19:key="tooltip.time";break;
    }
    return key?I18n::Text(key):std::wstring{};
}
static void CreatePlayerTooltips(HWND hwnd) {
    Ui::Tooltips::Attach(hwnd,[](POINT point){
        float x=point.x/MainWnd_DPIScaleFloat(1),y=point.y/MainWnd_DPIScaleFloat(1);
        int id=PlayerControlAt(x,y,g_visualizerVisible);
        Ui::Rect r=PlayerControlRect(id,g_visualizerVisible);
        if(!id){
            if((r={TITLE_X,TITLE_Y,TITLE_W,TITLE_H}).Contains(x,y))id=15;
            else if((r={BTN_MINIMIZE_X,4,BTN_SIZE,24}).Contains(x,y))id=17;
            else if((r={BTN_CLOSE_X,4,BTN_SIZE,24}).Contains(x,y))id=18;
            else if((r={LCD_X,LCD_Y,LCD_W,LCD_H}).Contains(x,y))id=19;
        }
        float scale=MainWnd_DPIScaleFloat(1);
        return Ui::Tooltips::Target{id,{int(r.x*scale),int(r.y*scale),int((r.x+r.w)*scale),int((r.y+r.h)*scale)}};
    },PlayerTooltip,[]{return MainWnd_DPIScaleFloat(1);});
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
    return g_visualizerVisible && PtInRect(&r, (POINT){ x, y });
}

/* ------------------------------------------------------------------ */
/* WndProc                                                              */
/* ------------------------------------------------------------------ */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        playerWindow=hwnd;
        Skin::Bind(Skin::Player,hwnd);
    {
        /* Calcular fator de escala DPI para esta janela e redimensionar a
         * janela (criada em tamanho lógico 1x) para o tamanho físico real
         * antes do primeiro ShowWindow — senão o conteúdo desenhado em
         * coordenadas escaladas fica cortado dentro de uma janela pequena. */
        ApplyInterfaceScale(hwnd);
        CreatePlayerTooltips(hwnd);
        Ui::Accessibility::Attach(hwnd,[hwnd]{
            std::vector<Ui::Accessibility::Control> controls;auto state=PlayerSnapshot();
            for(int id=1;id<=19;++id){Ui::Rect r=PlayerControlRect(id,g_visualizerVisible);
                if(id==15)r={TITLE_X,TITLE_Y,TITLE_W,TITLE_H};
                if(id==17)r={BTN_MINIMIZE_X,4,BTN_SIZE,24};
                if(id==18)r={BTN_CLOSE_X,4,BTN_SIZE,24};
                if(id==19)r={LCD_X,LCD_Y,LCD_W,LCD_H};
                if(r.w<=0||r.h<=0)continue;
                LONG flags=id<=14||id==16?STATE_SYSTEM_FOCUSABLE:0;
                if(GetFocus()==hwnd&&g_controlFocus==id)flags|=STATE_SYSTEM_FOCUSED;
                if((id==3&&state.shuffle)||(id==4&&state.repeat!=RepeatMode::Off)||(id==11&&state.playlistVisible)||(id==12&&state.equalizerVisible)||(id==16&&state.visualizerVisible))flags|=STATE_SYSTEM_CHECKED;
                if(((id==6||id==7||id==8)&&!state.loaded)||((id==1||id==2||id==5)&&!state.hasPlaylist))flags|=STATE_SYSTEM_UNAVAILABLE;
                if((id==13&&state.visualization==VIZ_MODE_SPECTRUM)||(id==14&&state.visualization==VIZ_MODE_OSCILLOSCOPE))flags|=STATE_SYSTEM_CHECKED;
                std::wstring value=id==8?std::to_wstring(int(state.seek*100)):id==9?std::to_wstring(int(state.volume*100)):id==10?std::to_wstring(int((state.pan*2-1)*100)):L"";
                controls.push_back({id,{MainWnd_DPIScale(int(r.x)),MainWnd_DPIScale(int(r.y)),MainWnd_DPIScale(int(r.x+r.w)),MainWnd_DPIScale(int(r.y+r.h))},id==15?state.title:id<=16?PlayerControlName(id):PlayerTooltip(id),PlayerTooltip(id),value,id>=8&&id<=10?ROLE_SYSTEM_SLIDER:id==15||id==19?ROLE_SYSTEM_STATICTEXT:id==3||id==4||id==11||id==12||id==16?ROLE_SYSTEM_CHECKBUTTON:id==13||id==14?ROLE_SYSTEM_RADIOBUTTON:ROLE_SYSTEM_PUSHBUTTON,flags});
            }return controls;
        },[hwnd](int id){if(id==17)ShowWindow(hwnd,SW_MINIMIZE);else if(id==18)PostMessageW(hwnd,WM_CLOSE,0,0);else ExecutePlayerControl(hwnd,id);InvalidateRect(hwnd,nullptr,FALSE);},[hwnd](int id){g_controlFocus=id;Ui::Accessibility::Focus(hwnd,id);InvalidateRect(hwnd,nullptr,FALSE);},[hwnd](int id,double value){
            if(id<8||id>10||value<(id==10?-100:0)||value>100)return false;
            if(id==8){if(!Audio_HasTrack())return false;g_seekValue=float(value/100);Audio_SeekSeconds(g_seekValue*Audio_GetDurationSeconds());}
            if(id==9){g_volumeValue=float(value/100);Audio_SetVolume(g_volumeValue);}
            if(id==10){g_panValue=float((value+100)/200);Audio_SetPan(g_panValue*2-1);}
            InvalidateRect(hwnd,nullptr,FALSE);return true;
        });


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
        Audio_SetVolume(g_volumeValue);
        Audio_SetPan(g_panValue*2-1);
        if(globalHotkeys&&!MainWnd_SetGlobalHotkeys(hwnd,true)){globalHotkeys=false;MessageBoxW(hwnd,I18n::Text("errors.hotkeys").c_str(),L"LiteSic",MB_OK|MB_ICONWARNING);}

        /* M4-T1: Inicializar ring buffer de visualização. */
        Audio_VizInit();

        /* Timer para atualização periódica da posição (250ms). */
        SetTimer(hwnd, POSITION_TIMER_ID, 250, NULL);

        /* Timer para scroll do título (30ms — ~33fps suave). */
        g_scrollTimerActive=false;
        g_titleOverflow=false;
        g_scrollTick=GetTickCount();

        /* Timer do visualizador (~60fps). */
        SetTimer(hwnd, VIZ_TIMER_ID, 33, NULL);

        break;
    }

    case WM_COPYDATA:{
        std::vector<std::wstring> files;if(!Runtime::Decode(reinterpret_cast<COPYDATASTRUCT*>(lParam),files))return FALSE;
        if(!PlWnd_AppendPaths(files))return FALSE;
        ShowWindow(hwnd,SW_RESTORE);SetForegroundWindow(hwnd);return TRUE;
    }
    case PlaybackCommand:
        if(wParam==20)PlWnd_AddFiles(hwnd,true);
        else if(wParam==21||wParam==22){if(Audio_HasTrack())Audio_SeekSeconds(std::clamp(Audio_GetPositionSeconds()+(wParam==21?-5.:5.),0.,Audio_GetDurationSeconds()));}
        else ExecutePlayerControl(hwnd,int(wParam));
        InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_APPCOMMAND:{
        int action=0;switch(GET_APPCOMMAND_LPARAM(lParam)){
        case APPCOMMAND_MEDIA_PREVIOUSTRACK:action=1;break;case APPCOMMAND_MEDIA_NEXTTRACK:action=2;break;
        case APPCOMMAND_MEDIA_STOP:action=7;break;case APPCOMMAND_MEDIA_PLAY_PAUSE:action=Audio_GetState()==AUDIO_STATE_PLAYING?6:5;break;}
        if(action){SendMessageW(hwnd,PlaybackCommand,action,0);return TRUE;}break;
    }
    case WM_HOTKEY:
        if(wParam>=6100&&wParam<=6103){const int actions[]={Audio_GetState()==AUDIO_STATE_PLAYING?6:5,7,1,2};SendMessageW(hwnd,PlaybackCommand,actions[wParam-6100],0);}return 0;
    case WM_ENTERSIZEMOVE:Skin::BeginMove(Skin::Player);return 0;
    case WM_MOVING:Skin::Moving(Skin::Player,*reinterpret_cast<RECT*>(lParam),MainWnd_DPIScale(24));return TRUE;
    case WM_EXITSIZEMOVE:Skin::EndMove(Skin::Player,MainWnd_DPIScale(24));return 0;
    case WM_WINDOWPOSCHANGING:Skin::Constrain(Skin::Player,*reinterpret_cast<WINDOWPOS*>(lParam));break;
    case WM_WINDOWPOSCHANGED:
        Skin::Changed(Skin::Player);UpdateScrollTimer(hwnd);
        return DefWindowProcW(hwnd,msg,wParam,lParam);
    case WM_DISPLAYCHANGE:ApplyInterfaceScale(hwnd);PlWnd_ScaleChanged();EqWnd_ScaleChanged();Ui::ThemeScaleChanged();Skin::Recover();return 0;
    case WM_ERASEBKGND: return 1;
    case WM_DPICHANGED: {
        if(!lParam)return 0;
        auto r=reinterpret_cast<RECT*>(lParam);
        g_dpiScale=Ui::FitScale(LOWORD(wParam)/96.f*UserScaleMultiplier(),MAINWND_WIDTH,PlayerLayoutHeight(g_visualizerVisible),Ui::MonitorWork(*r));
        SetWindowPos(hwnd,nullptr,r->left,r->top,MainWnd_DPIScale(MAINWND_WIDTH),MainWnd_DPIScale(PlayerLayoutHeight(g_visualizerVisible)),SWP_NOZORDER|SWP_NOACTIVATE);
        PlWnd_ScaleChanged(); EqWnd_ScaleChanged(); UpdateTooltipRects(hwnd); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    }
    case WM_KEYDOWN: {
        MSG key{hwnd,WM_KEYDOWN,wParam,lParam,0,{0,0}};if(MainWnd_TranslateShortcut(&key))return 0;
        const int order[]={1,5,6,7,2,8,9,10,3,4,16,11,12,13,14};
        if(wParam==VK_TAB) {
            int next=0;
            for(int i=0;i<15;++i) if(order[i]==g_controlFocus) next=(i+((GetKeyState(VK_SHIFT)&0x8000)?14:1))%15;
            if(!g_visualizerVisible) while(order[next]==13 || order[next]==14) next=(next+((GetKeyState(VK_SHIFT)&0x8000)?14:1))%15;
            g_controlFocus=order[next]; Ui::Accessibility::Focus(hwnd,g_controlFocus); InvalidateRect(hwnd,nullptr,FALSE); return 0;
        }
        if(wParam==VK_SPACE || wParam==VK_RETURN) {
            ExecutePlayerControl(hwnd,g_controlFocus?g_controlFocus:Audio_GetState()==AUDIO_STATE_PLAYING?6:5);
            InvalidateRect(hwnd,nullptr,FALSE); return 0;
        }
        if(g_controlFocus>=8 && g_controlFocus<=10 && (wParam==VK_LEFT || wParam==VK_RIGHT || wParam==VK_HOME || wParam==VK_END)) {
            if(g_controlFocus==8 && !Audio_HasTrack()) return 0;
            float& value=g_controlFocus==8?g_seekValue:g_controlFocus==9?g_volumeValue:g_panValue;
            value=wParam==VK_HOME?0:wParam==VK_END?1:std::clamp(value+(wParam==VK_LEFT?-.02f:.02f),0.f,1.f);
            if(g_controlFocus==8 && Audio_HasTrack()) Audio_SeekSeconds(value*Audio_GetDurationSeconds());
            if(g_controlFocus==9) Audio_SetVolume(value);
            if(g_controlFocus==10) Audio_SetPan(value*2-1);
            InvalidateRect(hwnd,nullptr,FALSE); return 0;
        }
        break;
    }
    case WM_KILLFOCUS: g_controlFocus=0; InvalidateRect(hwnd,nullptr,FALSE); break;
    case WM_CAPTURECHANGED: g_controlPressed=0; g_pressedBtn=BTN_NONE; g_draggingSlider=SLIDER_NONE; InvalidateRect(hwnd,nullptr,FALSE); break;
    case WM_MOUSELEAVE: g_controlHover=0; g_hoverBtn=BTN_NONE; InvalidateRect(hwnd,nullptr,FALSE); break;

    case WM_TIMER:
    {
        if (wParam == POSITION_TIMER_ID)
        {
            if (PlayerPlayback().Update(g_shuffleOn, g_repeatMode)) PlWnd_Refresh(true);
            const Track* current = PlayerPlaylist().At(PlayerPlaylist().GetCurrentIndex());
            uint64_t id = current ? current->id : 0;
            if (id != g_displayTrack) {
                g_displayTrack = id; g_seekValue = 0; g_scrollElapsedMs = 0;
                PlWnd_Refresh(); InvalidateRect(hwnd, nullptr, FALSE);
            }
            // EOF can stop without changing track identity. Synchronize the
            // slider even then, so it cannot retain the previous end position.
            float before = g_seekValue;
            if (g_draggingSlider != SLIDER_SEEK) {
                double dur = Audio_GetDurationSeconds();
                g_seekValue = dur > 0 ? static_cast<float>(Audio_GetPositionSeconds() / dur) : 0;
            }
            if (Audio_GetState() == AUDIO_STATE_PLAYING || before != g_seekValue)
                InvalidateRect(hwnd, NULL, FALSE);
        }
        else if (wParam == VIZ_TIMER_ID)
        {
            if (g_visualizerVisible && Audio_GetState() == AUDIO_STATE_PLAYING && IsWindowVisible(hwnd) && !IsIconic(hwnd))
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
            DWORD now=GetTickCount(),delta=now-g_scrollTick;g_scrollTick=now;
            if(g_titleOverflow && IsWindowVisible(hwnd) && !IsIconic(hwnd)) {
                g_scrollElapsedMs+=delta;
                RECT titleRect={MainWnd_DPIScale(TITLE_X),MainWnd_DPIScale(TITLE_Y),
                    MainWnd_DPIScale(TITLE_X+TITLE_W),MainWnd_DPIScale(TITLE_Y+23)};
                InvalidateRect(hwnd,&titleRect,FALSE);
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
        int const mx = MainWnd_DPIUnscale((int)(short)LOWORD(lParam));
        int const my = MainWnd_DPIUnscale((int)(short)HIWORD(lParam));

        SetFocus(hwnd);
        int control=PlayerControlAt(static_cast<float>(mx),static_cast<float>(my),g_visualizerVisible);
        if(!g_visualizerVisible && (control==13 || control==14)) control=0;
        g_controlFocus=control;
        if(control && (control<8 || control>10)) {
            g_controlPressed=control; SetCapture(hwnd);
            InvalidateRect(hwnd,nullptr,FALSE); return 0;
        }
        if(control>=8 && control<=10) {
            if(control==8 && !Audio_HasTrack()) return 0;
            SetCapture(hwnd);
        }

        /* Verificar se clicou num botão da titlebar primeiro. */
        if (my < (int)TITLEBAR_HEIGHT)
        {
            if (PtInButton(mx, my, BTN_CLOSE))
            {
                g_pressedBtn = BTN_CLOSE;
                SetCapture(hwnd);
                InvalidateButtons(hwnd);
                return 0;
            }
            if (PtInButton(mx, my, BTN_MINIMIZE))
            {
                g_pressedBtn = BTN_MINIMIZE;
                SetCapture(hwnd);
                InvalidateButtons(hwnd);
                return 0;
            }

            ReleaseCapture();
            SendMessageW(hwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
            return 0;
        }
        if (PtInLcdArea(mx, my)) {
            g_showRemaining = !g_showRemaining;
        } else if (PtInSeekBar(mx, my)) {
            g_draggingSlider = SLIDER_SEEK;
            g_seekValue = PixelToValue(mx - SEEK_X - 6, SEEK_W - 12, 0.0f, 1.0f);
            Audio_SeekSeconds(g_seekValue * Audio_GetDurationSeconds());
        } else if (PtInVolumeSlider(mx, my)) {
            g_draggingSlider = SLIDER_VOLUME;
            g_volumeValue = PixelToValue(mx - VOLUME_SLIDER_X - 6, VOLUME_SLIDER_W - 12, 0.0f, 1.0f);
            Audio_SetVolume(g_volumeValue);
        } else if (PtInPanSlider(mx, my)) {
            g_draggingSlider = SLIDER_PAN;
            g_panValue = PixelToValue(mx - PAN_SLIDER_X - 6, PAN_SLIDER_W - 12, 0.0f, 1.0f);
            Audio_SetPan(g_panValue * 2.0f - 1.0f);
        } else if (PtInVizArea(mx, my)) {
            g_vizMode = g_vizMode == VIZ_MODE_OSCILLOSCOPE ? VIZ_MODE_SPECTRUM : VIZ_MODE_OSCILLOSCOPE;
            Ui::SaveAppearanceValue(L"Spectrum",g_vizMode==VIZ_MODE_SPECTRUM);
        } else if (my >= NAV_Y && my < TOGGLE_Y + TOGGLE_H) {
            int btnId = 0;
            if (PtInNavBtn(mx, my, &btnId)) {
                switch (btnId) {
                case 1: PlayerPlayback().Previous(g_shuffleOn, g_repeatMode == RepeatMode::All); PlWnd_Refresh(true); break;
                case 2: PlayerPlayback().Next(g_shuffleOn, g_repeatMode == RepeatMode::All); PlWnd_Refresh(true); break;
                case 3: g_shuffleOn = !g_shuffleOn; break;
                case 4: g_repeatMode = NextRepeatMode(g_repeatMode); break;
                case 5: PlWnd_PlaySelected(); break;
                case 6: Audio_Pause(); break;
                case 7: Audio_Stop(); g_seekValue = 0; g_scrollElapsedMs = 0; break;
                }
            }
        } else
        {
            SetForegroundWindow(hwnd);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }

    /* ------------------------------------------------------------------
     * Movimento do mouse — slider dragging e hover.
     * ------------------------------------------------------------------ */
    case WM_MOUSEMOVE:
    {
        int const mx = MainWnd_DPIUnscale((int)(short)LOWORD(lParam));
        int const my = MainWnd_DPIUnscale((int)(short)HIWORD(lParam));

        int control=PlayerControlAt(static_cast<float>(mx),static_cast<float>(my),g_visualizerVisible);
        if(!g_visualizerVisible && (control==13 || control==14)) control=0;
        if(control!=g_controlHover) { g_controlHover=control; InvalidateRect(hwnd,nullptr,FALSE); }
        TRACKMOUSEEVENT tracking{sizeof(tracking),TME_LEAVE,hwnd,0}; TrackMouseEvent(&tracking);

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
                g_seekValue = PixelToValue(relX - 6, SEEK_W - 12, 0.0f, 1.0f);
                double pos = g_seekValue * Audio_GetDurationSeconds();
                Audio_SeekSeconds(pos);
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
            case SLIDER_VOLUME:
            {
                int relX = mx - VOLUME_SLIDER_X;
                g_volumeValue = PixelToValue(relX - 6, VOLUME_SLIDER_W - 12, 0.0f, 1.0f);
                Audio_SetVolume(g_volumeValue);
                InvalidateRect(hwnd, NULL, FALSE);
                break;
            }
            case SLIDER_PAN:
            {
                int relX = mx - PAN_SLIDER_X;
                g_panValue = PixelToValue(relX - 6, PAN_SLIDER_W - 12, 0.0f, 1.0f);
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

        int control=g_controlPressed; g_controlPressed=0;
        TitleBarButton titlePressed=g_pressedBtn;
        if(GetCapture()==hwnd) ReleaseCapture();
        if(control && control==PlayerControlAt(static_cast<float>(mx),static_cast<float>(my),g_visualizerVisible)) {
            ExecutePlayerControl(hwnd,control); InvalidateRect(hwnd,nullptr,FALSE); return 0;
        }

        /* Verificar se soltou na titlebar. */
        if (titlePressed != BTN_NONE && my < (int)TITLEBAR_HEIGHT)
        {
            if (PtInButton(mx, my, titlePressed))
            {
                switch (titlePressed)
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

        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT client{}; GetClientRect(hwnd,&client);
        if(PaintPlayer(g_canvas,hdc,client,g_dpiScale,PlayerSnapshot(),&g_titleOverflow)) {
            UpdateScrollTimer(hwnd);
            EndPaint(hwnd,&ps); break;
        }

        /* Double buffer — desenhar em memória e copiar de uma vez.
         * Dimensões escaladas pelo DPI para telas high-DPI (4K etc.). */
        HDC memDC = CreateCompatibleDC(hdc);
        int scaledW = MainWnd_DPIScale(MAINWND_WIDTH);
        int scaledH = MainWnd_DPIScale(PlayerLayoutHeight(g_visualizerVisible));
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
            if (const Track* track = PlayerPlaylist().At(PlayerPlaylist().GetCurrentIndex()))
                wcsncpy_s(fileName, ARRAYSIZE(fileName), track->displayTitle.c_str(), _TRUNCATE);
            SIZE textSize;
            GetTextExtentPoint32W(memDC, fileName, (int)wcslen(fileName), &textSize);

            /* Tudo em pixels reais; o texto é recortado à área do título
             * para nunca invadir o LCD nem a borda da janela. */
            RECT area = {
                MainWnd_DPIScale(TITLE_X), MainWnd_DPIScale(TITLE_Y),
                MainWnd_DPIScale(TITLE_X + TITLE_W), MainWnd_DPIScale(TITLE_Y) + MainWnd_DPIScale(14)
            };
            int areaW = area.right - area.left;
            g_titleOverflow=Audio_HasTrack() && textSize.cx>areaW;
            UpdateScrollTimer(hwnd);

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
            DrawTextW(memDC, I18n::Text("player.volume").c_str(), -1, &volLabelR, DT_LEFT | DT_TOP | DT_SINGLELINE);

            RECT panR = { MainWnd_DPIScale(PAN_SLIDER_X), MainWnd_DPIScale(VOLPAN_Y),
                          MainWnd_DPIScale(PAN_SLIDER_X + PAN_SLIDER_W), MainWnd_DPIScale(VOLPAN_Y + VOLPAN_H) };
            DrawHSliderScaled(memDC, &panR, g_panValue);

            RECT panLabelR = { panR.left, panR.top - 1, panR.left + MainWnd_DPIScale(20), panR.top + 1 };
            DrawTextW(memDC, I18n::Text("player.balance").c_str(), -1, &panLabelR, DT_LEFT | DT_TOP | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Botões de navegação e toggles — M3-T6
         * ------------------------------------------------------------------ */
        {
            RECT prevR = { MainWnd_DPIScale(BTN_PREV_X), MainWnd_DPIScale(NAV_Y),
                           MainWnd_DPIScale(BTN_PREV_X + NAV_BTN_W), MainWnd_DPIScale(NAV_Y + NAV_BTN_H) };
            HBRUSH b = CreateSolidBrush(NAV_BG_NORMAL); FillRect(memDC, &prevR, b); DeleteObject(b);
            SetBkMode(memDC, TRANSPARENT); SetTextColor(memDC, Ui::NativeColor(Ui::Text));
            DrawTextW(memDC, L"<<", 2, &prevR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT nextR = { MainWnd_DPIScale(BTN_NEXT_X), MainWnd_DPIScale(NAV_Y),
                           MainWnd_DPIScale(BTN_NEXT_X + NAV_BTN_W), MainWnd_DPIScale(NAV_Y + NAV_BTN_H) };
            b = CreateSolidBrush(NAV_BG_NORMAL); FillRect(memDC, &nextR, b); DeleteObject(b);
            DrawTextW(memDC, L">>", 2, &nextR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT shufR = { MainWnd_DPIScale(BTN_SHUFFLE_X), MainWnd_DPIScale(TOGGLE_Y),
                           MainWnd_DPIScale(BTN_SHUFFLE_X + TOGGLE_W), MainWnd_DPIScale(TOGGLE_Y + TOGGLE_H) };
            b = CreateSolidBrush(g_shuffleOn ? Ui::NativeColor(Ui::SelectedButtonFill()) : TOGGLE_OFF_BG); FillRect(memDC, &shufR, b); DeleteObject(b);
            SetTextColor(memDC, g_shuffleOn ? Ui::NativeColor(Ui::SelectedButtonText) : AUDIO_INFO_COLOR);
            DrawTextW(memDC, I18n::Text("player.shuffle_short").c_str(), -1, &shufR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT repR = { MainWnd_DPIScale(BTN_REPEAT_X), MainWnd_DPIScale(TOGGLE_Y),
                          MainWnd_DPIScale(BTN_REPEAT_X + TOGGLE_W), MainWnd_DPIScale(TOGGLE_Y + TOGGLE_H) };
            b = CreateSolidBrush(g_repeatMode != RepeatMode::Off ? Ui::NativeColor(Ui::SelectedButtonFill()) : TOGGLE_OFF_BG); FillRect(memDC, &repR, b); DeleteObject(b);
            SetTextColor(memDC, g_repeatMode != RepeatMode::Off ? Ui::NativeColor(Ui::SelectedButtonText) : AUDIO_INFO_COLOR);
            DrawTextW(memDC, g_repeatMode == RepeatMode::One ? I18n::Text("player.repeat_one").c_str() : I18n::Text("player.repeat_short").c_str(), -1, &repR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Play/Pause/Stop ao lado de Anterior/Próximo.
         * ------------------------------------------------------------------ */
        {
            HBRUSH b;
            SetBkMode(memDC, TRANSPARENT); SetTextColor(memDC, Ui::NativeColor(Ui::Text));
            RECT playR = { MainWnd_DPIScale(BTN_PLAY_X), MainWnd_DPIScale(BTN_PLAY_Y),
                           MainWnd_DPIScale(BTN_PLAY_X + CTRL_BTN_W), MainWnd_DPIScale(BTN_PLAY_Y + CTRL_BTN_H) };
            b = CreateSolidBrush(CTRL_BG_NORMAL); FillRect(memDC, &playR, b); DeleteObject(b);
            DrawTextW(memDC, L">", 1, &playR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT pauseR = { MainWnd_DPIScale(BTN_PAUSE_X), MainWnd_DPIScale(BTN_PAUSE_Y),
                            MainWnd_DPIScale(BTN_PAUSE_X + CTRL_BTN_W), MainWnd_DPIScale(BTN_PAUSE_Y + CTRL_BTN_H) };
            b = CreateSolidBrush(CTRL_BG_NORMAL); FillRect(memDC, &pauseR, b); DeleteObject(b);
            DrawTextW(memDC, L"||", 2, &pauseR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            RECT stopR = { MainWnd_DPIScale(BTN_STOP_X), MainWnd_DPIScale(BTN_STOP_Y),
                           MainWnd_DPIScale(BTN_STOP_X + CTRL_BTN_W), MainWnd_DPIScale(BTN_STOP_Y + CTRL_BTN_H) };
            b = CreateSolidBrush(CTRL_BG_NORMAL); FillRect(memDC, &stopR, b); DeleteObject(b);
            DrawTextW(memDC, L"[]", 2, &stopR, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        /* ------------------------------------------------------------------
         * Visualizador — M4-T2 (osciloscópio) / M4-T3 (espectro).
         * ------------------------------------------------------------------ */
        if(g_visualizerVisible) {
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

        // Footer controls remain available in the GDI fallback.
        SetBkMode(memDC,TRANSPARENT);SetTextColor(memDC,Ui::NativeColor(Ui::Text));
        for(int id:{16,11,12}) {
            auto button=PlayerControlRect(id,g_visualizerVisible);
            RECT rect{MainWnd_DPIScale(static_cast<int>(button.x)),MainWnd_DPIScale(static_cast<int>(button.y)),
                MainWnd_DPIScale(static_cast<int>(button.x+button.w)),MainWnd_DPIScale(static_cast<int>(button.y+button.h))};
            bool active=(id==16 && g_visualizerVisible) || (id==11 && PlWnd_IsVisible()) || (id==12 && EqWnd_IsVisible());
            HBRUSH brush=CreateSolidBrush(Ui::NativeColor(active?Ui::SelectedButtonFill():Ui::Surface));FillRect(memDC,&rect,brush);DeleteObject(brush);
            brush=CreateSolidBrush(Ui::NativeColor(active?Ui::Green:Ui::Bronze));FrameRect(memDC,&rect,brush);DeleteObject(brush);
            SetTextColor(memDC,Ui::NativeColor(active?Ui::SelectedButtonText:Ui::Text));
            DrawTextW(memDC,id==16?L"VIZ":id==11?I18n::Text("player.list_short").c_str():L"EQ",-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
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
    case WM_MEASUREITEM: case WM_DRAWITEM: {
        LRESULT result{};
        if(Ui::ThemeMenuMessage(msg,wParam,lParam,result)) return result;
        break;
    }

    case WM_CONTEXTMENU:
    {
        struct ScaleMenuItem { int id; int percent; std::wstring label; };
        const ScaleMenuItem items[] = {
            { IDM_SCALE_AUTO, 0,   I18n::Text("appearance.auto_scale").c_str() },
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
            AppendMenuW(hScale, flags, items[i].id, items[i].label.c_str());
        }

        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu,MF_STRING|(topmost?MF_CHECKED:0),902,I18n::Text("appearance.topmost").c_str());
        AppendMenuW(hMenu,MF_STRING|(globalHotkeys?MF_CHECKED:0),903,I18n::Text("appearance.hotkeys").c_str());
        AppendMenuW(hMenu, MF_STRING, 900, I18n::Text("playlist.title").c_str());
        AppendMenuW(hMenu, MF_STRING, 901, I18n::Text("appearance.themes_menu").c_str());
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hScale, I18n::Text("appearance.scale_menu").c_str());
        AppendMenuW(hMenu,MF_POPUP,reinterpret_cast<UINT_PTR>(I18n::CreateLanguageMenu()),I18n::Text("appearance.language").c_str());

        int x = (int)(short)LOWORD(lParam);
        int y = (int)(short)HIWORD(lParam);
        if (x == -1 && y == -1)
        {
            RECT r{};
            GetWindowRect(hwnd, &r);
            x = r.left;
            y = r.top;
        }

        Ui::TrackThemedMenu(hMenu, TPM_RIGHTBUTTON, x, y, hwnd, MainWnd_DPIScaleFloat(1));
        DestroyMenu(hMenu);
        break;
    }

    case WM_COMMAND:
    {
        int cmdId = LOWORD(wParam);
        if(I18n::IsLanguageCommand(cmdId)){
            std::string diagnostic;
            if(!I18n::ExecuteLanguageCommand(cmdId,diagnostic)||!diagnostic.empty())
                MessageBoxW(hwnd,I18n::Text("errors.language_change").c_str(),L"LiteSic",MB_OK|MB_ICONWARNING);
            break;
        }
        if(cmdId==902){topmost=!topmost;MainWnd_ApplyTopmost(hwnd);Ui::SaveAppearanceValue(L"Topmost",topmost);break;}
        if(cmdId==903){
            if(!MainWnd_SetGlobalHotkeys(hwnd,!globalHotkeys))MessageBoxW(hwnd,I18n::Text("errors.hotkeys").c_str(),L"LiteSic",MB_OK|MB_ICONWARNING);
            break;
        }
        if (cmdId == 900) { PlWnd_Show(); break; }
        if (cmdId == 901) { Ui::ShowThemes(hwnd); break; }
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
                Ui::SaveAppearanceValue(L"ScalePercent",g_userScalePercent);
                ApplyInterfaceScale(hwnd);
                PlWnd_ScaleChanged();
                EqWnd_ScaleChanged();
                Ui::ThemeScaleChanged();
                break;
            }
        }
        break;
    }

    case I18n::WM_LANGUAGE_CHANGED:
        Ui::Tooltips::Refresh();
        InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case Ui::WM_THEME_CHANGED:
    case WM_PLAYER_CHANGED:
        InvalidateRect(hwnd, nullptr, FALSE);
        break;

    case WM_DESTROY:
    {
        ReleaseHotkeys(hwnd);playerWindow=nullptr;
        Ui::SaveAppearanceValue(L"PanPermille",int(std::lround(g_panValue*1000)));
        Ui::SaveAppearanceValue(L"Shuffle",g_shuffleOn);
        Ui::SaveAppearanceValue(L"Repeat",int(g_repeatMode));
        RECT position{};GetWindowRect(hwnd,&position);
        Ui::SavePlayerPosition({position.left,position.top});
        Ui::SaveAppearanceValue(L"VolumePermille",static_cast<int>(std::lround(g_volumeValue*1000)));
        KillTimer(hwnd, POSITION_TIMER_ID);
        KillTimer(hwnd, SCROLL_TIMER_ID);
        KillTimer(hwnd, VIZ_TIMER_ID);
        PlWnd_Shutdown();
        EqWnd_Shutdown();
        Audio_Shutdown();
        g_canvas.Reset();
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
    auto appearance=Ui::LoadAppearance();
    g_volumeValue=appearance.volumePermille/1000.f;
    g_panValue=appearance.panPermille/1000.f;g_shuffleOn=appearance.shuffle;g_repeatMode=RepeatMode(appearance.repeat);
    topmost=appearance.topmost;globalHotkeys=appearance.globalHotkeys;
    g_visualizerVisible=appearance.visualizerVisible;
    g_userScalePercent=appearance.scalePercent;
    g_vizMode=appearance.spectrum?VIZ_MODE_SPECTRUM:VIZ_MODE_OSCILLOSCOPE;
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

    Ui::PanelPosition savedPosition;bool hasPosition=Ui::LoadPlayerPosition(savedPosition);
    if(hasPosition){x=savedPosition.left;y=savedPosition.top;}
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
        if(hasPosition){RECT r{};GetWindowRect(hwnd,&r);r={savedPosition.left,savedPosition.top,savedPosition.left+r.right-r.left,savedPosition.top+r.bottom-r.top};MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);if(GetMonitorInfoW(MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST),&monitor))WindowSnap::KeepVisible(r,monitor.rcWork);SetWindowPos(hwnd,nullptr,r.left,r.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}
        MainWnd_ApplyTopmost(hwnd);
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
    }

    return hwnd;
}
