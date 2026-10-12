// Empty foreground window used to test LiteSic global shortcuts without other apps.
#include <windows.h>
static LRESULT CALLBACK Proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}return DefWindowProcW(hwnd,msg,wp,lp);}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int){WNDCLASSW cls{};cls.lpfnWndProc=Proc;cls.hInstance=instance;cls.lpszClassName=L"M8.ForegroundProbe";cls.hbrBackground=GetSysColorBrush(COLOR_WINDOW);if(!RegisterClassW(&cls))return 1;HWND hwnd=CreateWindowW(cls.lpszClassName,L"M8 isolated foreground",WS_OVERLAPPEDWINDOW,100,100,480,240,nullptr,nullptr,instance,nullptr);if(!hwnd)return 2;ShowWindow(hwnd,SW_SHOW);MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;}
