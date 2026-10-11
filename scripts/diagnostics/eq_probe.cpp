// Offline production graph + real window paint workload, isolated configuration.
#include "audio.h"
#include "eq_state.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "ui_theme.h"
#include "ui_draw.h"
#include "visualizer.h"
#include <psapi.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
static void Require(bool ok){if(!ok)ExitProcess(2);}
static unsigned long long CPU(){FILETIME c,e,k,u;GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u);ULARGE_INTEGER a{},b{};
    a.LowPart=k.dwLowDateTime;a.HighPart=k.dwHighDateTime;b.LowPart=u.dwLowDateTime;b.HighPart=u.dwHighDateTime;return a.QuadPart+b.QuadPart;}
static BOOL CALLBACK PaintPanel(HWND h,LPARAM p){wchar_t c[80];GetClassNameW(h,c,80);
    if(!wcscmp(c,L"LiteSic.EqualizerWnd")||!wcscmp(c,L"LiteSic.PlaylistWnd"))PrintWindow(h,reinterpret_cast<HDC>(p),0);
    return TRUE;
}
int main(int argc,char** argv){
    int mode=argc>1?std::atoi(argv[1]):0;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Ui::ThemeConfigure(nullptr);
    Require(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));HWND window=MainWnd_Create(GetModuleHandleW(nullptr),SW_HIDE);Require(window!=nullptr);
    Audio_Shutdown();Eq::ApplyPreset(mode>=2?1:0);Eq::SetEnabled(mode!=0);Require(Audio_TestInit(48000));Require(Audio_VizInit());
    Require(Audio_Load(L"tests\\eq_fixtures\\eq-tone-2.wav"));Audio_Play();if(mode==6)Audio_TestSkipEq(TRUE);
    if(mode==5){Require(PlWnd_Create(GetModuleHandleW(nullptr),window));EqWnd_Toggle(window);}
    HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,690,435);auto old=SelectObject(dc,bitmap);ReleaseDC(nullptr,screen);
    Ui::Canvas canvas;RECT rect{0,0,690,435};float pcm[1024];LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    std::vector<double> times;times.reserve(2000);
    auto cpuStart=CPU();LARGE_INTEGER wallStart,wallEnd;QueryPerformanceCounter(&wallStart);
    for(int i=0;i<2100;++i){if(i%128==0)Audio_SeekSeconds(.2);
        LARGE_INTEGER a,b;QueryPerformanceCounter(&a);Require(Audio_TestRead(pcm,512));QueryPerformanceCounter(&b);
        if(i>=100)times.push_back(double(b.QuadPart-a.QuadPart)*1e6/frequency.QuadPart);
        if(i%4==0 && mode>=3 && mode<=5){Require(canvas.Begin(dc,rect,1.5f));DrawVisualizerD2D(canvas,{12,196,436,68},mode==3?VIZ_MODE_SPECTRUM:VIZ_MODE_OSCILLOSCOPE);Require(canvas.End());}
        if(i%4==0&&mode==5){PrintWindow(window,dc,0);EnumThreadWindows(GetCurrentThreadId(),PaintPanel,reinterpret_cast<LPARAM>(dc));}
    }
    double sum=0;for(double t:times)sum+=t;std::sort(times.begin(),times.end());
    QueryPerformanceCounter(&wallEnd);auto cpuEnd=CPU();
    PROCESS_MEMORY_COUNTERS_EX m{};m.cb=sizeof(m);Require(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&m),sizeof(m)));
    std::printf("{\"mode\":%d,\"frames\":512,\"rate\":48000,\"channels\":2,\"blocks\":2000,\"mean_us\":%.3f,\"p99_us\":%.3f,\"max_us\":%.3f,\"audio_budget_pct\":%.3f,\"process_cpu_ms\":%.3f,\"wall_ms\":%.3f,\"working_set\":%llu,\"private_bytes\":%llu,\"gdi\":%lu,\"user\":%lu}\n",
        mode,sum/times.size(),times[1980],times.back(),sum/times.size()/10666.6667*100,(cpuEnd-cpuStart)/10000.,
        double(wallEnd.QuadPart-wallStart.QuadPart)*1000/frequency.QuadPart,(unsigned long long)m.WorkingSetSize,(unsigned long long)m.PrivateUsage,
        (unsigned long)GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),(unsigned long)GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS));
    canvas.DiscardTarget();SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);DestroyWindow(window);CoUninitialize();
}
