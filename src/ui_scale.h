#pragma once
#include <windows.h>
#include <algorithm>
#include <cstring>
namespace Ui {
inline RECT MonitorWork(HWND hwnd){MONITORINFO m{};m.cbSize=sizeof(m);if(GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&m))return m.rcWork;RECT r{};SystemParametersInfoW(SPI_GETWORKAREA,0,&r,0);return r;}
inline RECT MonitorWork(const RECT& rect){MONITORINFO m{};m.cbSize=sizeof(m);if(GetMonitorInfoW(MonitorFromRect(&rect,MONITOR_DEFAULTTONEAREST),&m))return m.rcWork;return MonitorWork(nullptr);}
inline float FitScale(float requested,int width,int height,const RECT& work){
    if(requested<=0)requested=1;
    if(width<=0||height<=0||work.right<=work.left||work.bottom<=work.top)return requested;
    return std::min(requested,std::min(float(work.right-work.left)/width,float(work.bottom-work.top)/height));
}
inline float WindowDpi(HWND hwnd){
    using GetDpi=UINT(WINAPI*)(HWND);static GetDpi get=[](){FARPROC symbol=GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow");GetDpi pointer=nullptr;static_assert(sizeof(pointer)==sizeof(symbol),"pointer size");std::memcpy(&pointer,&symbol,sizeof(pointer));return pointer;}();
    if(get){UINT dpi=get(hwnd);if(dpi)return dpi/96.f;}
    HDC dc=GetDC(hwnd);int dpi=GetDeviceCaps(dc,LOGPIXELSX);ReleaseDC(hwnd,dc);return dpi>0?dpi/96.f:1.f;
}
}
