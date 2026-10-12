// Regression probe of production layout/message paths; isolated persistence.
#include "ui_scale.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "ui_player.h"
#include "ui_tooltip.h"
#include "language_selection.h"
#include "skin.h"
#include "audio.h"
#include "check.h"
#include <cmath>
static HWND player,playlist,eq,themes;
static BOOL CALLBACK Find(HWND h,LPARAM){wchar_t c[80]{};GetClassNameW(h,c,80);
    if(!wcscmp(c,L"LiteSic.PlaylistWnd"))playlist=h;
    if(!wcscmp(c,L"LiteSic.EqualizerWnd"))eq=h;
    if(!wcscmp(c,L"LiteSic.ThemesWnd"))themes=h;
    return TRUE;}
static RECT Client(HWND h){RECT r{};CHECK(GetClientRect(h,&r));return r;}
static RECT Window(HWND h){RECT r{};CHECK(GetWindowRect(h,&r));return r;}
static void Dpi(HWND h,int dpi){auto r=Window(h);SendMessageW(h,WM_DPICHANGED,MAKELONG(dpi,dpi),reinterpret_cast<LPARAM>(&r));}
int main(){
    SetProcessDPIAware();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    Ui::ThemeConfigure(nullptr);I18n::InitializeLanguage(nullptr);CHECK(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));
    player=MainWnd_Create(GetModuleHandleW(nullptr),SW_HIDE);CHECK(player);Audio_Shutdown();
    CHECK(PlWnd_Create(GetModuleHandleW(nullptr),player));EqWnd_Toggle(player);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(eq&&playlist);
    int samples=0;
    for(int command:{IDM_SCALE_100,IDM_SCALE_125,IDM_SCALE_150})for(int dpi:{96,120,144,192}){
        SendMessageW(player,WM_COMMAND,command,0);Dpi(player,dpi);Dpi(eq,dpi);
        float scale=Ui::FitScale(dpi/96.f*MainWnd_UserScaleMultiplier(),460,290,Ui::MonitorWork(player));CHECK(std::abs(MainWnd_DPIScaleFloat(1)-scale)<.0001f);
        auto p=Client(player),e=Client(eq);CHECK(p.right==int(std::lround(460*scale)));CHECK(e.right==int(std::lround(460*scale)));
        for(int id=1;id<=16;++id)if(id!=15){auto r=PlayerControlRect(id);POINT center{int(std::lround((r.x+r.w/2)*scale)),int(std::lround((r.y+r.h/2)*scale))};CHECK(Ui::Tooltips::Resolve(player,center).id==id);}
        for(int i=0;i<11;++i)CHECK(Ui::Tooltips::Resolve(eq,{int(std::lround((60+i*37)*scale)),int(std::lround(100*scale))}).id==i+4);
        printf("DPI_LAYOUT dpi=%d zoom=%.2f combined=%.3f player_width=%ld eq_width=%ld hit_tests=PASS\n",dpi,MainWnd_UserScaleMultiplier(),scale,p.right,e.right);++samples;
    }
    SendMessageW(player,WM_COMMAND,IDM_SCALE_100,0);Dpi(player,96);
    Ui::ShowThemes(player);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(themes);
    auto before=Client(themes);Dpi(themes,192);auto after=Client(themes);CHECK(after.right>before.right);
    printf("FIX_VERIFIED appearance_dpi_change old_width=%ld new_width=%ld\n",before.right,after.right);DestroyWindow(themes);
    Dpi(eq,192);auto own=Client(eq);Dpi(player,120);auto owner=Client(eq);CHECK(own.right==owner.right);
    printf("FIX_VERIFIED independent_eq_dpi own192_width=%ld after_owner120_width=%ld\n",own.right,owner.right);
    RECT negative{-900,-600,-325,-237},work{-1920,-1080,0,0};WindowSnap::KeepVisible(negative,work);CHECK(negative.left==-900&&negative.top==-600);
    float fitted=Ui::FitScale(2.f,460,290,{0,0,800,600});CHECK(460*fitted<=800.01f&&290*fitted<=600.01f);
    printf("FIX_VERIFIED negative_workarea_and_compact_fit scale=%.4f\n",fitted);
    DestroyWindow(player);CoUninitialize();printf("AUDIT_PROBE layout_samples=%d fixed_gaps=3 compact_fit=PASS PASS\n",samples);
}
