// Offline, same-process diagnostic. No input to other apps and no session file.
// Link once with production ui_draw.o and once with a generated GDI-only copy.
#include "mainwnd.h"
#include "plwnd.h"
#include "playback.h"
#include "eqwnd.h"
#include <windows.h>
#include <psapi.h>
#include <cstdio>
#include <string>

static HWND listWindow;
static BOOL CALLBACK FindList(HWND hwnd, LPARAM) {
    wchar_t name[64]{}; GetClassNameW(hwnd,name,64);
    if (!wcscmp(name,L"LiteSic.PlaylistWnd")) listWindow=hwnd;
    return TRUE;
}
static void Pump() {
    MSG msg;
    while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)) {
        if(msg.message!=WM_QUIT) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }
}
static void Paint(HWND main) {
    InvalidateRect(main,nullptr,FALSE); UpdateWindow(main);
    InvalidateRect(listWindow,nullptr,FALSE); UpdateWindow(listWindow); Pump();
}
static void Sample(const char* stage,int cycles) {
    PROCESS_MEMORY_COUNTERS_EX m{}; m.cb=sizeof(m);
    if(!GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&m),sizeof(m))) ExitProcess(2);
    DWORD handles=0; GetProcessHandleCount(GetCurrentProcess(),&handles);
    FILETIME c,e,k,u; GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u);
    ULARGE_INTEGER kernel{},user{}; kernel.LowPart=k.dwLowDateTime;kernel.HighPart=k.dwHighDateTime;
    user.LowPart=u.dwLowDateTime;user.HighPart=u.dwHighDateTime;
    std::printf("{\"stage\":\"%s\",\"cycles\":%d,\"working_set\":%llu,\"private_bytes\":%llu,\"handles\":%lu,\"gdi\":%lu,\"user\":%lu,\"cpu_100ns\":%llu}\n",
        stage,cycles,(unsigned long long)m.WorkingSetSize,(unsigned long long)m.PrivateUsage,
        (unsigned long)handles,(unsigned long)GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),
        (unsigned long)GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS),(unsigned long long)(kernel.QuadPart+user.QuadPart));
    std::fflush(stdout);
}
int main() {
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    Sample("process",0);
    HINSTANCE instance=GetModuleHandleW(nullptr);
    if(!MainWnd_RegisterClass(instance)) return 1;
    HWND main=MainWnd_Create(instance,SW_SHOWNOACTIVATE); if(!main) return 1;
    if(!PlWnd_Create(instance,main)) return 1;
    EnumThreadWindows(GetCurrentThreadId(),FindList,0); if(!listWindow) return 1;
    // A fixed 150% simulation on the same desktop for both render backends.
    RECT mainRect{0,0,690,435},listRect{0,435,690,975};
    SendMessageW(main,WM_DPICHANGED,MAKELONG(144,144),reinterpret_cast<LPARAM>(&mainRect));
    SendMessageW(listWindow,WM_DPICHANGED,MAKELONG(144,144),reinterpret_cast<LPARAM>(&listRect));
    for(int i=0;i<60;++i) { Paint(main); Sleep(10); }
    Sample("empty_warm",60);
    HWND edit=GetDlgItem(listWindow,520);
    for(int i=0;i<2000;++i) PlayerPlaylist().Add(L"C:\\diagnostic\\Faixa-"+std::to_wstring(i)+L".wav");
    PlWnd_Refresh(); Paint(main); Sample("loaded_2000",0);
    for(int batch=1;batch<=10;++batch) {
        for(int j=0;j<100;++j) {
            SetWindowTextW(edit,j%2?L"Faixa-1":L"");
            PlWnd_Refresh(); Paint(main);
        }
        Sample("filter_repaint",batch*100);
    }
    SetWindowTextW(edit,L"");
    for(int batch=1;batch<=10;++batch) {
        for(int j=0;j<20;++j) {
            SetWindowPos(listWindow,nullptr,0,0,j%2?690:1000,j%2?540:800,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
            Paint(main);
        }
        Sample("resize",batch*20);
    }
    PlayerPlayback().Clear(); PlWnd_Refresh(); Paint(main);
    Sample("cleared",0);
    for(int i=0;i<100;++i) { Paint(main); Sleep(10); }
    Sample("settled",100);
    DestroyWindow(main); Pump(); Sample("windows_destroyed",0);
    CoUninitialize();
}
