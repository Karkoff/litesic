#include "eqwnd.h"
#include "mainwnd.h"
#include "ui_draw.h"
#include <windowsx.h>
#include <cmath>
#include <algorithm>

namespace {
HWND window=nullptr;
float scale=1;
Ui::Canvas canvas;
void Hide(HWND hwnd) { ShowWindow(hwnd,SW_HIDE); Ui::SaveAppearanceValue(L"EqualizerVisible",0); }
int S(float value) { return static_cast<int>(std::lround(value*scale)); }
LRESULT CALLBACK Proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case WM_ERASEBKGND:return 1;
    case Ui::WM_THEME_CHANGED: InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_NCHITTEST: {
        POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)}; ScreenToClient(hwnd,&p);
        if(p.y<S(32) && p.x<S(428)) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_LBUTTONUP:
        if(GET_X_LPARAM(lp)>=S(428) && GET_Y_LPARAM(lp)<S(32)) Hide(hwnd);
        return 0;
    case WM_KEYDOWN:if(wp==VK_ESCAPE) Hide(hwnd);return 0;
    case WM_CLOSE:Hide(hwnd);return 0;
    case WM_DPICHANGED: {
        scale=LOWORD(wp)/96.f*MainWnd_UserScaleMultiplier();
        auto r=reinterpret_cast<RECT*>(lp);
        SetWindowPos(hwnd,nullptr,r->left,r->top,S(460),S(210),SWP_NOZORDER|SWP_NOACTIVATE); return 0;
    }
    case WM_PAINT: {
        using namespace Ui;
        PAINTSTRUCT ps{}; HDC dc=BeginPaint(hwnd,&ps); RECT r{};GetClientRect(hwnd,&r);
        if(canvas.Begin(dc,r,scale)) {
            canvas.Fill({0,0,460,210},Background);
            canvas.TitleBar(460,L"LITESIC · EQUALIZADOR",false);
            canvas.Button({12,40,100,28},L"Indisponível",false,false,false,false);
            canvas.Button({124,40,168,28},L"Preset: Flat",false,false,false,false);
            canvas.Button({304,40,144,28},L"Redefinir",false,false,false,false);
            canvas.TextAt({12,70,436,20},L"DSP pendente de implementação e testes na M6",Muted,Font::Small,true);
            canvas.Fill({8,94,444,108},Surface,3);
            canvas.TextAt({12,94,32,20},L"+12 dB",Muted,Font::Small);
            canvas.TextAt({12,128,32,20},L"0 dB",Muted,Font::Small);
            canvas.TextAt({12,164,32,20},L"−12 dB",Muted,Font::Small);
            const wchar_t* names[]={L"PRE",L"31",L"62",L"125",L"250",L"500",L"1k",L"2k",L"4k",L"8k",L"16k"};
            for(int i=0;i<11;++i) {
                float x=60+i*37;
                canvas.Line(x,104,x,174,Border,3);canvas.Circle(x,139,4,Bronze,Muted);
                canvas.TextAt({x-16,178,32,20},names[i],Muted,Font::Small,true);
            }
            if(!canvas.End()) InvalidateRect(hwnd,nullptr,FALSE);
        } else {
            // Clearly unavailable fallback, never an apparently working EQ.
            SetBkMode(dc,OPAQUE); SetBkColor(dc,Ui::NativeColor(Ui::Surface)); SetTextColor(dc,Ui::NativeColor(Ui::Text));
            DrawTextW(dc,L"Equalizador indisponível — DSP pendente",-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        }
        EndPaint(hwnd,&ps);return 0;
    }
    case WM_DESTROY:window=nullptr;canvas.DiscardTarget();return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
}
void EqWnd_Toggle(HWND main) {
    if(!window) {
        WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);cls.lpfnWndProc=Proc;
        cls.hInstance=GetModuleHandleW(nullptr);cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.lpszClassName=L"LiteSic.EqualizerWnd";
        if(!RegisterClassExW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return;
        scale=MainWnd_DPIScaleFloat(1);
        RECT parent{};GetWindowRect(main,&parent);
        MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);GetMonitorInfoW(MonitorFromWindow(main,MONITOR_DEFAULTTONEAREST),&monitor);
        int x=std::max<int>(monitor.rcWork.left,std::min<int>(parent.right+S(8),monitor.rcWork.right-S(460)));
        int y=std::max<int>(monitor.rcWork.top,std::min<int>(parent.top,monitor.rcWork.bottom-S(210)));
        window=CreateWindowExW(0,cls.lpszClassName,L"LiteSic — Equalizador (DSP pendente)",WS_POPUP,x,y,S(460),S(210),main,nullptr,cls.hInstance,nullptr);
    }
    if(window) { ShowWindow(window,IsWindowVisible(window)?SW_HIDE:SW_SHOWNOACTIVATE); Ui::SaveAppearanceValue(L"EqualizerVisible",IsWindowVisible(window)!=FALSE); InvalidateRect(window,nullptr,FALSE); }
}
void EqWnd_ScaleChanged() {
    scale=MainWnd_DPIScaleFloat(1);
    if(window) SetWindowPos(window,nullptr,0,0,S(460),S(210),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
}
void EqWnd_Shutdown() { if(window) DestroyWindow(window); }
void EqWnd_Restore(HWND main) { if(Ui::LoadAppearance().equalizerVisible) EqWnd_Toggle(main); }
