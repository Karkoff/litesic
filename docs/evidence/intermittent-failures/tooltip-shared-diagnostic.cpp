#include "ui_tooltip.h"
#include "ui_player.h"
#include "i18n.h"
#include "language_selection.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "eq_state.h"
#include "audio.h"
#include "playback.h"
#include "check.h"
#include <psapi.h>
#include <chrono>
#include <cmath>
using namespace Ui::Tooltips;
static HWND mainWindow,playlist,eq,themes;
static BOOL CALLBACK Find(HWND h,LPARAM){wchar_t name[80]{};GetClassNameW(h,name,80);
    if(!wcscmp(name,L"LiteSic.EqualizerWnd"))eq=h;
    if(!wcscmp(name,L"LiteSic.PlaylistWnd"))playlist=h;
    if(!wcscmp(name,L"LiteSic.ThemesWnd"))themes=h;
    return TRUE;}
static POINT P(float x,float y){return {MainWnd_DPIScale(int(x)),MainWnd_DPIScale(int(y))};}
static std::vector<std::string> English(){return {"en-US"};}
static void click(HWND h,int x,int y){auto p=P(x,y);auto l=MAKELPARAM(p.x,p.y);SendMessageW(h,WM_LBUTTONDOWN,MK_LBUTTON,l);SendMessageW(h,WM_LBUTTONUP,0,l);}
static void names_and_regions(){
    CHECK(Resolve(mainWindow,P(0,0)).id==0);
    for(int id=1;id<=16;++id)if(id!=15){auto r=PlayerControlRect(id);CHECK(Resolve(mainWindow,P(r.x+2,r.y+2)).id==id);CHECK(!Description(mainWindow,id).empty());CHECK(!PlayerControlName(id).empty());}
    CHECK(Resolve(mainWindow,P(LCD_X+2,LCD_Y+2)).id==19);
    CHECK(Resolve(mainWindow,P(BTN_CLOSE_X+2,8)).id==18);
    CHECK(Resolve(mainWindow,P(BTN_MINIMIZE_X+2,8)).id==17);
    for(int i=0;i<11;++i)CHECK(Resolve(eq,P(60+i*37,100)).id==i+4);
    for(int i=0;i<3;++i)CHECK(Resolve(eq,P(80+i*128,200)).id==i+1);
    for(int id:{501+11,503,501+12,521,522,523,520,530,531})CHECK(!Description(playlist,id).empty());
}
static void dynamic_transport_and_visibility(){
    CHECK(Description(mainWindow,3)==I18n::Text("tooltip.shuffle_on"));click(mainWindow,BTN_SHUFFLE_X+5,TOGGLE_Y+5);
    CHECK(Description(mainWindow,3)==I18n::Text("tooltip.shuffle_off"));click(mainWindow,BTN_SHUFFLE_X+5,TOGGLE_Y+5);
    const char* keys[]={"tooltip.repeat_off","tooltip.repeat_all","tooltip.repeat_one"};
    for(auto key:keys){CHECK(Description(mainWindow,4)==I18n::Text(key));click(mainWindow,BTN_REPEAT_X+5,TOGGLE_Y+5);}
    auto before=Description(mainWindow,12);EqWnd_Toggle(mainWindow);CHECK(Description(mainWindow,12)!=before);EqWnd_Toggle(mainWindow);
    auto r=PlayerControlRect(16);click(mainWindow,int(r.x+5),int(r.y+5));CHECK(Resolve(mainWindow,P(370,VIZ_Y+5)).id!=13);
    r=PlayerControlRect(16,false);click(mainWindow,int(r.x+5),int(r.y+5));CHECK(Resolve(mainWindow,P(370,VIZ_Y+5)).id==13);
}
static void dsp_values_and_unavailable_bands(){
    Eq::ApplyPreset(1);CHECK(Description(eq,4).find(L"-4.0")!=std::wstring::npos);
    Eq::SetGain(4,3);CHECK(Description(eq,9).find(L"1000")!=std::wstring::npos);CHECK(Description(eq,9).find(L"+3.0")!=std::wstring::npos);
    Eq::SetGain(-1,-2);CHECK(Description(eq,4).find(L"-2.0")!=std::wstring::npos);
    Eq::SetEnabled(true);CHECK(Description(eq,1)==I18n::Text("tooltip.eq_off"));Eq::SetEnabled(false);CHECK(Description(eq,1)==I18n::Text("tooltip.eq_on"));
    CHECK(Audio_TestInit(8000));CHECK(Description(eq,14).find(I18n::Format("tooltip.band_unavailable",{{"frequency",L"16000"},{"gain",L"+1.0"}}))!=std::wstring::npos);Audio_Shutdown();
}
static void all_catalogs_and_themes(){
    std::string error;
    for(const auto& language:I18n::Languages()){
        CHECK(I18n::ChangeLanguage(false,language.code,error));
        for(int id=1;id<=19;++id)CHECK(!Description(mainWindow,id).empty());
        for(int id=1;id<=15;++id)CHECK(!Description(eq,id).empty());
        for(size_t i=0;i<Ui::ThemeCount();++i){CHECK(Ui::SelectTheme(Ui::ThemeAt(i).id,false));CHECK(Ui::Contrast(Ui::Surface,Ui::Text)>=4.5);CHECK(Description(mainWindow,5)==I18n::Text("tooltip.play"));}
    }
    CHECK(I18n::ChangeLanguage(false,"en-US",error));
}
static void edge_placement_and_dpi(){
    for(float dpi:{1.f,1.25f,1.5f,2.f})for(float zoom:{.8f,1.f,1.5f}){
        SIZE size{int(370*dpi*zoom),int(60*dpi*zoom)};RECT work{-1280,-200,0,824};
        for(RECT anchor: {RECT{-1280,-200,-1200,-180},RECT{-80,800,0,824},RECT{-500,200,-400,230}}){
            RECT p=Place(anchor,size,work,6);CHECK(p.left>=work.left&&p.right<=work.right&&p.top>=work.top&&p.bottom<=work.bottom);
        }
    }
    auto p=Place({0,0,20,20},{5000,5000},{0,0,100,100},6);CHECK(p.right==100&&p.bottom==100);
}
static void native_controls_and_lifecycle(){
    size_t initial=AttachedCount();Ui::ShowThemes(mainWindow);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(themes);
    for(int id:{702,703,704,705,706})CHECK(!Description(GetDlgItem(themes,id),1).empty());
    CHECK(AttachedCount()==initial+6);DestroyWindow(themes);CHECK(AttachedCount()==initial);
    for(int i=0;i<40;++i){EqWnd_Shutdown();CHECK(AttachedCount()==initial-1);EqWnd_Toggle(mainWindow);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(AttachedCount()==initial);}
}
static void favorites_use_real_track(){
    PlayerPlaylist().Clear();auto id=PlayerPlaylist().Add(L"tests/fixtures/native-48000.wav");auto track=PlayerPlaylist().At(PlayerPlaylist().Find(id));
    track->favorite=false;CHECK(Description(playlist,1000)==I18n::Text("tooltip.favorite_on"));track->favorite=true;CHECK(Description(playlist,1000)==I18n::Text("tooltip.favorite_off"));PlayerPlaylist().Clear();
}
static void audio_and_incremental_resources(){
    CHECK(Audio_TestInit(48000));CHECK(Audio_Load(L"tests/fixtures/native-48000.wav"));Audio_Play();Eq::SetEnabled(true);Eq::SetGain(4,3);
    float samples[512]{};auto render=[&]{CHECK(Audio_TestRead(samples,16));};for(int i=0;i<180;++i){for(int id=1;id<=15;++id)Description(eq,id);render();}
    PROCESS_MEMORY_COUNTERS_EX a{},b{};a.cb=b.cb=sizeof(a);CHECK(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&a),sizeof(a)));
    auto gdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),user=GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS);auto cursor=Audio_TestCursor();auto start=std::chrono::steady_clock::now();
    for(int i=0;i<1800;++i){for(int id=1;id<=15;++id)CHECK(!Description(eq,id).empty());render();}
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    CHECK(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&b),sizeof(b)));
    auto afterGdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),afterUser=GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS);
    std::fprintf(stderr,"SHARED_RESOURCES private_delta=%lld GDI=%lu->%lu USER=%lu->%lu\n",static_cast<long long>(b.PrivateUsage)-a.PrivateUsage,gdi,afterGdi,user,afterUser);
    CHECK(b.PrivateUsage<=a.PrivateUsage+512*1024);CHECK(afterGdi==gdi);CHECK(afterUser==user);
    CHECK(Audio_TestCursor()>cursor);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);CHECK(Eq::State().enabled&&Eq::State().gains[4]==3);
    printf("TOOLTIP_METRICS descriptions=27000 elapsed_ms=%.3f private_delta=%lld GDI=%lu USER=%lu\n",ms,static_cast<long long>(b.PrivateUsage)-a.PrivateUsage,static_cast<unsigned long>(gdi),static_cast<unsigned long>(user));Audio_Shutdown();
}

static void PumpFor(DWORD ms){DWORD start=GetTickCount();do{MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}Sleep(5);}while(GetTickCount()-start<ms);}
static HWND Hover(HWND hwnd,POINT p,bool wait=false){
    ShowWindow(GetAncestor(hwnd,GA_ROOT),SW_SHOW);ShowWindow(hwnd,SW_SHOW);SetForegroundWindow(GetAncestor(hwnd,GA_ROOT));SetFocus(hwnd);PumpFor(20);
    POINT screen=p;ClientToScreen(hwnd,&screen);CHECK(SetCursorPos(screen.x,screen.y));PumpFor(30);
    SendMessageW(hwnd,WM_MOUSEMOVE,0,MAKELPARAM(p.x,p.y));
    if(wait)PumpFor(750);else SendMessageW(hwnd,WM_TIMER,0x4c535454,0);
    HWND popup=Popup(hwnd);if(!popup||!IsWindowVisible(popup)){POINT cur{};GetCursorPos(&cur);POINT local=cur;ScreenToClient(hwnd,&local);std::fprintf(stderr,"HOVER surface=%p root=%p foreground=%p under=%p capture=%p local=%ld,%ld hit=%d\n",hwnd,GetAncestor(hwnd,GA_ROOT),GetForegroundWindow(),WindowFromPoint(cur),GetCapture(),local.x,local.y,Resolve(hwnd,local).id);}CHECK(popup&&IsWindowVisible(popup));CHECK(GetForegroundWindow()==GetAncestor(hwnd,GA_ROOT));
    CHECK(SendMessageW(popup,WM_NCHITTEST,0,0)==HTTRANSPARENT);CHECK(SendMessageW(popup,WM_MOUSEACTIVATE,0,0)==MA_NOACTIVATE);return popup;
}
static std::wstring TipText(HWND h){wchar_t s[2048]{};GetWindowTextW(h,s,2048);return s;}
static void popup_delay_dismissal_live_update_and_expiry(){
    ShowWindow(mainWindow,SW_SHOW);SetForegroundWindow(mainWindow);SetFocus(mainWindow);PumpFor(30);
    POINT p=P(BTN_SHUFFLE_X+10,TOGGLE_Y+10),screen=p;ClientToScreen(mainWindow,&screen);CHECK(SetCursorPos(screen.x,screen.y));
    SendMessageW(mainWindow,WM_MOUSEMOVE,0,MAKELPARAM(p.x,p.y));PumpFor(250);CHECK(!Popup(mainWindow)||!IsWindowVisible(Popup(mainWindow)));
    PumpFor(500);HWND tip=Popup(mainWindow);CHECK(tip&&IsWindowVisible(tip));CHECK(TipText(tip)==Description(mainWindow,3));
    std::string error;CHECK(I18n::ChangeLanguage(false,"ar-SA",error));CHECK(IsWindowVisible(tip));CHECK(TipText(tip)==Description(mainWindow,3));
    CHECK(Ui::SelectTheme(Ui::ThemeAt(2).id,false));CHECK(IsWindowVisible(tip));
    SendMessageW(mainWindow,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(p.x,p.y));CHECK(!IsWindowVisible(tip));SendMessageW(mainWindow,WM_LBUTTONUP,0,MAKELPARAM(p.x,p.y));
    tip=Hover(mainWindow,p,true);CHECK(TipText(tip)==I18n::Text("tooltip.shuffle_off"));
    for(UINT msg:{WM_MOUSELEAVE,WM_KILLFOCUS,WM_CANCELMODE,WM_ENTERSIZEMOVE,WM_ENTERMENULOOP,WM_KEYDOWN}){
        SendMessageW(mainWindow,msg,msg==WM_KEYDOWN?VK_F24:0,0);CHECK(!IsWindowVisible(tip));tip=Hover(mainWindow,p);
    }
    SetCapture(mainWindow);SendMessageW(mainWindow,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(p.x,p.y));CHECK(!IsWindowVisible(tip));ReleaseCapture();
    tip=Hover(mainWindow,p);PumpFor(8250);CHECK(!IsWindowVisible(tip));
    SendMessageW(mainWindow,WM_TIMER,0x4c535454,0);CHECK(!IsWindowVisible(tip));Hide(mainWindow);
    CHECK(I18n::ChangeLanguage(false,"en-US",error));
    RECT eqClient{};CHECK(GetClientRect(eq,&eqClient));
    // EQ keeps its local monitor DPI; player P() is not its coordinate converter.
    float eqScale=eqClient.right/460.f;
    tip=Hover(eq,{int(std::lround(60*eqScale)),int(std::lround(100*eqScale))});Eq::SetGain(-1,-2);PumpFor(180);CHECK(TipText(tip)==Description(eq,4));
    ShowWindow(eq,SW_HIDE);CHECK(!IsWindowVisible(tip));EqWnd_Toggle(mainWindow);
    tip=Hover(GetDlgItem(playlist,520),{2,2});CHECK(TipText(tip)==I18n::Text("tooltip.search"));Hide(GetDlgItem(playlist,520));
}
static void popup_render_matrix_and_resource_reuse(){
    WNDCLASSW cls{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"LiteSic.TooltipFixture";CHECK(RegisterClassW(&cls));
    HWND surface=CreateWindowExW(0,cls.lpszClassName,L"Tooltip integration fixture",WS_POPUP|WS_VISIBLE,200,200,500,200,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(surface);
    float scale=1;CHECK(Attach(surface,[](POINT p){return Target{p.x>=0&&p.x<100&&p.y>=0&&p.y<100?1:0,{0,0,100,100}};},[](int){return I18n::Format("tooltip.band_unavailable",{{"frequency",L"16000"},{"gain",L"+3.0"}});},[&]{return scale;}));
    HWND tip=Hover(surface,{30,30});std::string error;int frames=0;
    auto paint=[&]{RECT r{};GetClientRect(tip,&r);HDC dc=GetDC(tip),memory=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,r.right,r.bottom);auto old=SelectObject(memory,bitmap);
        SendMessageW(tip,WM_PRINTCLIENT,reinterpret_cast<WPARAM>(memory),PRF_CLIENT);CHECK(GetPixel(memory,4,4)==Ui::NativeColor(Ui::Surface));
        SelectObject(memory,old);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(tip,dc);
        RECT rect{};GetWindowRect(tip,&rect);MONITORINFO info{};info.cbSize=sizeof(info);CHECK(GetMonitorInfoW(MonitorFromWindow(tip,MONITOR_DEFAULTTONEAREST),&info));
        CHECK(rect.left>=info.rcWork.left&&rect.right<=info.rcWork.right&&rect.top>=info.rcWork.top&&rect.bottom<=info.rcWork.bottom);++frames;};
    for(const auto& language:I18n::Languages()){
        CHECK(I18n::ChangeLanguage(false,language.code,error));
        for(size_t theme=0;theme<Ui::ThemeCount();++theme){Hide(surface);CHECK(Hover(surface,{30,30})==tip);CHECK(Ui::SelectTheme(Ui::ThemeAt(theme).id,false));
            for(float value:{.8f,1.25f,1.5f,2.f}){scale=value;Refresh();CHECK(Popup(surface)==tip);CHECK(TipText(tip)==Description(surface,1));paint();}
        }
    }
    auto gdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),user=GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS);
    PROCESS_MEMORY_COUNTERS_EX a{},b{};a.cb=b.cb=sizeof(a);GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&a),sizeof(a));
    for(int i=0;i<500;++i){Hide(surface);CHECK(Hover(surface,{30,30})==tip);paint();}
    GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&b),sizeof(b));
    CHECK(b.PrivateUsage<=a.PrivateUsage+512*1024);CHECK(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)==gdi);CHECK(GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS)==user);
    printf("TOOLTIP_GUI frames=%d hover_cycles=500 private_delta=%lld GDI=%lu USER=%lu\n",frames,static_cast<long long>(b.PrivateUsage)-a.PrivateUsage,static_cast<unsigned long>(gdi),static_cast<unsigned long>(user));
    DestroyWindow(surface);CHECK(!IsWindow(tip));
}
int main(int argc,char** argv){SetProcessDPIAware();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Ui::ThemeConfigure(nullptr);I18n::InitializeLanguage(nullptr,English);CHECK(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));
    mainWindow=MainWnd_Create(GetModuleHandleW(nullptr),SW_HIDE);CHECK(mainWindow);Audio_Shutdown();CHECK(PlWnd_Create(GetModuleHandleW(nullptr),mainWindow,nullptr));EqWnd_Toggle(mainWindow);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(playlist&&eq);int cases=0;
    CASE(names_and_regions);CASE(dynamic_transport_and_visibility);CASE(dsp_values_and_unavailable_bands);CASE(all_catalogs_and_themes);CASE(edge_placement_and_dpi);CASE(native_controls_and_lifecycle);CASE(favorites_use_real_track);CASE(audio_and_incremental_resources);
    if(argc==2&&std::string(argv[1])=="--gui"){CASE(popup_delay_dismissal_live_update_and_expiry);CASE(popup_render_matrix_and_resource_reuse);}
    if(argc==2&&std::string(argv[1])=="--matrix"){CASE(popup_render_matrix_and_resource_reuse);}
    DestroyWindow(mainWindow);CHECK(AttachedCount()==0);CoUninitialize();printf("%d tooltip cases\n",cases);return 0;
}
