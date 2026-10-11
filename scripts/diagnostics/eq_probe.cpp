// Offline production graph + real window paint workload, isolated configuration.
#include "audio.h"
#include "eq_state.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "ui_theme.h"
#include "ui_draw.h"
#include "visualizer.h"
#include "ui_player.h"
#include <psapi.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <tlhelp32.h>
static void Require(bool ok){if(!ok)ExitProcess(2);}
static unsigned long long CPU(){FILETIME c,e,k,u;GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u);ULARGE_INTEGER a{},b{};
    a.LowPart=k.dwLowDateTime;a.HighPart=k.dwHighDateTime;b.LowPart=u.dwLowDateTime;b.HighPart=u.dwHighDateTime;return a.QuadPart+b.QuadPart;}
static void PaintWindow(HWND h){RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);}
static void ClickControl(HWND h,int id){auto r=PlayerControlRect(id);LPARAM p=MAKELPARAM(MainWnd_DPIScale(int(r.x+r.w/2)),MainWnd_DPIScale(int(r.y+r.h/2)));
    SendMessageW(h,WM_LBUTTONDOWN,MK_LBUTTON,p);SendMessageW(h,WM_LBUTTONUP,0,p);}
static BOOL CALLBACK PaintPanel(HWND h,LPARAM){wchar_t c[80];GetClassNameW(h,c,80);
    if(!wcscmp(c,L"LiteSic.EqualizerWnd")||!wcscmp(c,L"LiteSic.PlaylistWnd"))PaintWindow(h);
    return TRUE;
}
static unsigned Threads(){unsigned count=0;HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);Require(snapshot!=INVALID_HANDLE_VALUE);
    THREADENTRY32 e{};e.dwSize=sizeof(e);if(Thread32First(snapshot,&e))do{if(e.th32OwnerProcessID==GetCurrentProcessId())++count;}while(Thread32Next(snapshot,&e));
    CloseHandle(snapshot);return count;}
int main(int argc,char** argv){
    int mode=argc>1?std::atoi(argv[1]):0;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Ui::ThemeConfigure(nullptr);
    Require(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));HWND window=MainWnd_Create(GetModuleHandleW(nullptr),SW_HIDE);Require(window!=nullptr);
    Audio_Shutdown();Eq::ApplyPreset(mode>=2?1:0);Eq::SetEnabled(mode!=0);Require(Audio_TestInit(48000));Require(Audio_VizInit());
    if(mode!=7){Require(Audio_Load(L"tests\\eq_fixtures\\eq-tone-2.wav"));Audio_Play();}if(mode==6)Audio_TestSkipEq(TRUE);
    if(mode==5||mode==8||mode==10){Require(PlWnd_Create(GetModuleHandleW(nullptr),window));if(mode!=8)EqWnd_Toggle(window);}
    if(mode==9||mode==10){PlayerPlaylist().Add(L"tests\\eq_fixtures\\eq-tone-2.wav");PlayerPlaylist().SetCurrentIndex(0);
        PlayerPlaylist().At(0)->displayTitle=L"Título longo do LiteSic — Unicode 音楽 — auditoria integrada de rolagem com equalizador ativo";}
    ShowWindow(window,SW_SHOWNOACTIVATE);
    if(mode<=2||mode==6||mode==7)ClickControl(window,16);
    if(mode==3)ClickControl(window,13);
    PaintWindow(window);
    HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,690,435);auto old=SelectObject(dc,bitmap);ReleaseDC(nullptr,screen);
    Ui::Canvas canvas;RECT rect{0,0,690,435};float pcm[1024];LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);
    std::vector<double> times;times.reserve(2000);
    auto cpuStart=CPU();LARGE_INTEGER wallStart,wallEnd;QueryPerformanceCounter(&wallStart);
    const int seconds=argc>2?std::atoi(argv[2]):0;
    auto soakStart=GetTickCount64();double maxCpuPct=0;auto lastCpu=CPU();auto lastTick=GetTickCount64();
    for(int i=0;seconds?GetTickCount64()-soakStart<static_cast<unsigned long long>(seconds)*1000:i<2100;++i){if(i%128==0&&mode!=7)Audio_SeekSeconds(.2);
        LARGE_INTEGER a,b;QueryPerformanceCounter(&a);Require(Audio_TestRead(pcm,512));QueryPerformanceCounter(&b);
        if(i>=100)times.push_back(double(b.QuadPart-a.QuadPart)*1e6/frequency.QuadPart);
        if(i%4==0 && mode>=3 && mode<=5){Require(canvas.Begin(dc,rect,1.5f));DrawVisualizerD2D(canvas,{12,196,436,68},mode==3?VIZ_MODE_SPECTRUM:VIZ_MODE_OSCILLOSCOPE);Require(canvas.End());}
        if(i%4==0){PaintWindow(window);if(mode>=5)EnumThreadWindows(GetCurrentThreadId(),PaintPanel,0);}
        if(mode==10&&i%30==0){Ui::SelectTheme(Ui::ThemeAt(i/30%36).id,false);Eq::ApplyPreset(i/30%Eq::PresetCount());Eq::SetEnabled(i/30%4!=0);}
        if(seconds){MSG m{};while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}
            auto now=GetTickCount64();if(now-lastTick>=1000){auto cpu=CPU();double pct=double(cpu-lastCpu)/(now-lastTick)/100.;maxCpuPct=std::max(maxCpuPct,pct);lastCpu=cpu;lastTick=now;
                if((now-soakStart)/1000%30==0){PROCESS_MEMORY_COUNTERS_EX mem{};mem.cb=sizeof(mem);GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&mem),sizeof(mem));
                    std::printf("SAMPLE seconds=%llu private=%llu ws=%llu gdi=%lu user=%lu threads=%u cpu_one_core_pct=%.3f\n",(now-soakStart)/1000,(unsigned long long)mem.PrivateUsage,(unsigned long long)mem.WorkingSetSize,GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS),Threads(),pct);std::fflush(stdout);}
            }
            Sleep(10);
        }
    }
    double sum=0;for(double t:times)sum+=t;std::sort(times.begin(),times.end());
    QueryPerformanceCounter(&wallEnd);auto cpuEnd=CPU();
    PROCESS_MEMORY_COUNTERS_EX m{};m.cb=sizeof(m);Require(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&m),sizeof(m)));
    Require(!times.empty());
    std::printf("{\"mode\":%d,\"frames\":512,\"rate\":48000,\"channels\":2,\"mean_us\":%.3f,\"p99_us\":%.3f,\"max_us\":%.3f,\"audio_budget_pct\":%.3f,\"process_cpu_ms\":%.3f,\"wall_ms\":%.3f,\"working_set\":%llu,\"private_bytes\":%llu,\"gdi\":%lu,\"user\":%lu}\n",
        mode,sum/times.size(),times[times.size()*99/100],times.back(),sum/times.size()/10666.6667*100,(cpuEnd-cpuStart)/10000.,
        double(wallEnd.QuadPart-wallStart.QuadPart)*1000/frequency.QuadPart,(unsigned long long)m.WorkingSetSize,(unsigned long long)m.PrivateUsage,
        (unsigned long)GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),(unsigned long)GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS));
    std::printf("COUNTERS threads=%u samples=%llu peak_cpu_one_core_pct=%.3f\n",Threads(),(unsigned long long)times.size(),maxCpuPct);
    canvas.DiscardTarget();SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);DestroyWindow(window);CoUninitialize();
}
