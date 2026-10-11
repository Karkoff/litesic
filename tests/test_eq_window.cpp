#include "mainwnd.h"
#include "eqwnd.h"
#include "ui_player.h"
#include "window_snap.h"
#include "eq_state.h"
#include "audio.h"
#include "ui_theme.h"
#include "check.h"
#include "plwnd.h"
#include "m3u.h"
#include <vector>
#include <string>
#include <cmath>
static HWND mainWindow,eqWindow;
static BOOL CALLBACK Find(HWND h,LPARAM){wchar_t c[80];GetClassNameW(h,c,80);if(!wcscmp(c,L"LiteSic.EqualizerWnd")&&GetWindow(h,GW_OWNER)==mainWindow)eqWindow=h;return TRUE;}
static LPARAM Point(int x,int y){return MAKELPARAM(MainWnd_DPIScale(x),MainWnd_DPIScale(y));}
static void Click(int x,int y){SendMessageW(eqWindow,WM_LBUTTONDOWN,MK_LBUTTON,Point(x,y));SendMessageW(eqWindow,WM_LBUTTONUP,0,Point(x,y));}
static void Screenshot(unsigned theme){
    if(!CaptureAllowed()) { UpdateWindow(eqWindow); return; }
    RECT r{};GetClientRect(eqWindow,&r);HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* pixels=nullptr;
    HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);CHECK(bitmap);auto old=SelectObject(dc,bitmap);
    CHECK(PrintWindow(eqWindow,dc,0));GdiFlush();CHECK(GetPixel(dc,2,MainWnd_DPIScale(35))==Ui::NativeColor(Ui::Background));
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+r.right*r.bottom*4;
    CreateDirectoryW(L"build\\m6",nullptr);std::wstring name=L"build\\m6\\eq-theme-"+std::to_wstring(theme)+L"-"+std::to_wstring(sizeof(void*)*8)+
#ifdef DEBUG
    L"-debug.bmp";
#else
    L"-release.bmp";
#endif
    FILE* file=_wfopen(name.c_str(),L"wb");CHECK(file);fwrite(&header,sizeof(header),1,file);fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,file);fwrite(pixels,r.right*r.bottom*4,1,file);fclose(file);
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(nullptr,screen);
}
static double Render(){std::vector<float> pcm(9600);CHECK(Audio_TestRead(pcm.data(),4800));double sum=0;for(unsigned i=4800;i<9600;++i)sum+=pcm[i]*pcm[i];return std::sqrt(sum/4800);}
static void controls_publish_dsp_and_hidden_processing(){Eq::Configure(nullptr);CHECK(Audio_TestInit(48000));CHECK(Audio_Load(L"tests\\eq_fixtures\\eq-tone-2.wav"));Audio_Play();double dry=Render();
    Click(136,201);CHECK(Eq::State().enabled);
    SendMessageW(eqWindow,WM_LBUTTONDOWN,MK_LBUTTON,Point(245,100));
    SendMessageW(eqWindow,WM_MOUSEMOVE,MK_LBUTTON,Point(245,85));SendMessageW(eqWindow,WM_LBUTTONUP,0,Point(245,85));
    CHECK(std::abs(Eq::State().gains[4]-6)<=.25f&&Eq::State().preset==-1); // One physical-pixel rounding at the current scale.
    double boost=Render();CHECK(boost/dry>1.9&&boost/dry<2.2);SendMessageW(eqWindow,WM_CLOSE,0,0);CHECK(!IsWindowVisible(eqWindow));CHECK(Eq::State().enabled);
    CHECK(!EqWnd_IsVisible());
    CHECK(Render()/dry>1.9);EqWnd_Toggle(mainWindow);CHECK(EqWnd_IsVisible());CHECK(std::abs(Eq::State().gains[4]-6)<=.25f);
    Click(392,201);CHECK(Eq::State().preset==0&&Eq::State().gains[4]==0);Render();CHECK(std::abs(Render()/dry-1)<.01);
    Click(60,100);SendMessageW(eqWindow,WM_KEYDOWN,VK_UP,0);CHECK(Eq::State().preamp==.5f);
    SendMessageW(eqWindow,WM_KEYDOWN,VK_HOME,0);CHECK(Eq::State().preamp==12);
    SendMessageW(eqWindow,WM_KEYDOWN,VK_END,0);CHECK(Eq::State().preamp==-12);Audio_Shutdown();}
static void all_themes_preserve_parameters(){CHECK(Audio_TestInit(48000));Eq::ApplyPreset(1);Eq::SetEnabled(true);auto before=Eq::State();
    for(unsigned i=0;i<Ui::ThemeCount();++i){CHECK(Ui::SelectTheme(Ui::ThemeAt(i).id,false));Screenshot(i);
        CHECK(Eq::State().enabled==before.enabled&&Eq::State().preamp==before.preamp);for(unsigned b=0;b<10;++b)CHECK(Eq::State().gains[b]==before.gains[b]);}
    for(int percent:{80,90,100,110,125,150}){SendMessageW(mainWindow,WM_COMMAND,percent==80?IDM_SCALE_80:percent==90?IDM_SCALE_90:percent==100?IDM_SCALE_100:percent==110?IDM_SCALE_110:percent==125?IDM_SCALE_125:IDM_SCALE_150,0);
        RECT r{};GetClientRect(eqWindow,&r);CHECK(r.right==MainWnd_DPIScale(460)&&r.bottom==MainWnd_DPIScale(PlayerLayoutHeight(false)));}
    Audio_Shutdown();}
static void docking_follow_detach_and_scale(){
    SetWindowPos(mainWindow,nullptr,800,500,400,200,SWP_NOZORDER|SWP_NOACTIVATE);
    for(int side=0;side<4;++side){
        RECT parent{},panel{};GetWindowRect(mainWindow,&parent);GetWindowRect(eqWindow,&panel);
        int w=panel.right-panel.left,h=panel.bottom-panel.top;
        int x=parent.left+8,y=parent.bottom+20;
        if(side==1)y=parent.top-h-20;
        if(side==2){x=parent.right+20;y=parent.top+8;}
        if(side==3){x=parent.left-w-20;y=parent.top+8;}
        RECT proposed{x,y,x+w,y+h};
        SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
        SetWindowPos(eqWindow,nullptr,proposed.left,proposed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
        SetWindowPos(mainWindow,nullptr,parent.left+30,parent.top+40,420,220,SWP_NOZORDER|SWP_NOACTIVATE);
        EqWnd_ScaleChanged();GetWindowRect(mainWindow,&parent);GetWindowRect(eqWindow,&panel);
        if(side==0){CHECK(panel.top==parent.bottom);CHECK(panel.left==parent.left);}
        if(side==1){CHECK(panel.bottom==parent.top);CHECK(panel.left==parent.left);}
        if(side==2){CHECK(panel.left==parent.right);CHECK(panel.top==parent.top);}
        if(side==3){CHECK(panel.right==parent.left);CHECK(panel.top==parent.top);}
    }
    RECT detached{1600,1500,1600+MainWnd_DPIScale(460),1500+MainWnd_DPIScale(PlayerLayoutHeight(false))};
    SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&detached));
    SetWindowPos(eqWindow,nullptr,detached.left,detached.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
    SetWindowPos(mainWindow,nullptr,500,500,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    RECT panel{};GetWindowRect(eqWindow,&panel);CHECK(panel.left==1600&&panel.top==1500);
}
static void docked_panel_remains_reachable_at_monitor_edge(){
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    CHECK(GetMonitorInfoW(MonitorFromWindow(mainWindow,MONITOR_DEFAULTTONEAREST),&monitor));
    RECT work=monitor.rcWork;
    SetWindowPos(mainWindow,nullptr,work.left+100,work.top+100,300,200,SWP_NOZORDER|SWP_NOACTIVATE);
    RECT parent{},panel{};GetWindowRect(mainWindow,&parent);GetWindowRect(eqWindow,&panel);
    RECT proposed{parent.right,parent.top,parent.right+panel.right-panel.left,parent.top+panel.bottom-panel.top};
    SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
    SetWindowPos(eqWindow,nullptr,proposed.left,proposed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
    SetWindowPos(mainWindow,nullptr,work.right-300,work.top+100,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    EqWnd_MainMoved();GetWindowRect(eqWindow,&panel);
    CHECK(panel.left>=work.left && panel.top>=work.top && panel.right<=work.right && panel.bottom<=work.bottom);
}
static void simultaneous_panels_hide_without_stopping_audio(){
    SetWindowPos(mainWindow,nullptr,1000,300,MainWnd_DPIScale(460),MainWnd_DPIScale(290),SWP_NOZORDER|SWP_NOACTIVATE);
    CHECK(PlWnd_Create(GetModuleHandleW(nullptr),mainWindow));
    HWND list=nullptr;for(HWND h=GetWindow(mainWindow,GW_HWNDFIRST);h;h=GetWindow(h,GW_HWNDNEXT)){
        wchar_t cls[80]{};GetClassNameW(h,cls,80);if(!wcscmp(cls,L"LiteSic.PlaylistWnd")&&GetWindow(h,GW_OWNER)==mainWindow)list=h;
    }CHECK(list);
    RECT parent{},panel{},overlap{},pl{};GetWindowRect(mainWindow,&parent);GetWindowRect(eqWindow,&panel);
    RECT proposed{parent.left,parent.bottom,parent.left+panel.right-panel.left,parent.bottom+panel.bottom-panel.top};
    SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
    SetWindowPos(eqWindow,nullptr,proposed.left,proposed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
    GetWindowRect(eqWindow,&panel);GetWindowRect(list,&pl);CHECK(!IntersectRect(&overlap,&panel,&pl));
    PlWnd_Toggle();CHECK(!PlWnd_IsVisible());
    proposed={parent.left,parent.bottom,parent.left+panel.right-panel.left,parent.bottom+panel.bottom-panel.top};
    SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
    SetWindowPos(eqWindow,nullptr,proposed.left,proposed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
    PlWnd_Toggle();CHECK(PlWnd_IsVisible());GetWindowRect(eqWindow,&panel);GetWindowRect(list,&pl);
    CHECK(!IntersectRect(&overlap,&panel,&pl));
    CHECK(Audio_TestInit(48000));CHECK(Audio_Load(L"tests\\eq_fixtures\\eq-tone-2.wav"));Eq::ApplyPreset(1);Eq::SetEnabled(true);Audio_Play();
    auto before=Audio_TestCursor();SendMessageW(list,WM_CLOSE,0,0);CHECK(!PlWnd_IsVisible());
    SendMessageW(eqWindow,WM_CLOSE,0,0);CHECK(!IsWindowVisible(eqWindow));CHECK(Eq::State().enabled);
    CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);Render();CHECK(Audio_TestCursor()>before);
    PlWnd_Shutdown();Audio_Shutdown();
}
static void docked_formation_survives_owner_moves(){
    using namespace WindowSnap;
    SendMessageW(mainWindow,WM_COMMAND,IDM_SCALE_100,0);
    CHECK(PlWnd_Create(GetModuleHandleW(nullptr),mainWindow));
    HWND list=nullptr;
    for(HWND h=GetWindow(mainWindow,GW_HWNDFIRST);h;h=GetWindow(h,GW_HWNDNEXT)){
        wchar_t cls[80]{};GetClassNameW(h,cls,80);
        if(GetWindow(h,GW_OWNER)==mainWindow&&!wcscmp(cls,L"LiteSic.PlaylistWnd"))list=h;
    }CHECK(list);
    auto attach=[&](HWND h,Side side){RECT owner{},r{};GetWindowRect(mainWindow,&owner);GetWindowRect(h,&r);
        Place(r,owner,{side,Anchor::Start});
        SendMessageW(h,WM_MOVING,0,reinterpret_cast<LPARAM>(&r));
        SetWindowPos(h,nullptr,r.left,r.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(h,WM_EXITSIZEMOVE,0,0);};
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    CHECK(GetMonitorInfoW(MonitorFromWindow(mainWindow,MONITOR_DEFAULTTONEAREST),&monitor));
    for(auto sides:{std::pair<Side,Side>{Side::Bottom,Side::Right},{Side::Top,Side::Left},
                    {Side::Left,Side::Bottom},{Side::Right,Side::Top}}){
        SendMessageW(eqWindow,WM_CLOSE,0,0);if(PlWnd_IsVisible())PlWnd_Toggle();
        SetWindowPos(mainWindow,nullptr,800,400,MainWnd_DPIScale(460),MainWnd_DPIScale(PlayerLayoutHeight(false)),SWP_NOZORDER|SWP_NOACTIVATE);
        EqWnd_Toggle(mainWindow);attach(eqWindow,sides.second);PlWnd_Toggle();attach(list,sides.first);
        for(POINT delta:{POINT{30,20},POINT{-55,-35},POINT{10000,0},POINT{-10000,0},POINT{0,10000},POINT{0,-10000}}){
            RECT owner{},eq{},pl{},next{},actual{};GetWindowRect(mainWindow,&owner);GetWindowRect(eqWindow,&eq);GetWindowRect(list,&pl);
            SetWindowPos(mainWindow,nullptr,owner.left+delta.x,owner.top+delta.y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
            GetWindowRect(mainWindow,&next);int dx=next.left-owner.left,dy=next.top-owner.top;
            OffsetRect(&eq,dx,dy);OffsetRect(&pl,dx,dy);
            GetWindowRect(eqWindow,&actual);CHECK(EqualRect(&eq,&actual));
            GetWindowRect(list,&actual);CHECK(EqualRect(&pl,&actual));
            for(auto r:{next,eq,pl})CHECK(r.left>=monitor.rcWork.left&&r.top>=monitor.rcWork.top&&r.right<=monitor.rcWork.right&&r.bottom<=monitor.rcWork.bottom);
        }
    }
    PlWnd_Shutdown();
}
static void position_survives_restart(){
    using namespace WindowSnap;
    EqWnd_Shutdown();SendMessageW(mainWindow,WM_COMMAND,IDM_SCALE_100,0);
    auto ini=FullPath(L"build\\eq-position-"+std::to_wstring(sizeof(void*)*8)+
#ifdef DEBUG
        L"-debug.ini");
#else
        L"-release.ini");
#endif
    DeleteFileW(ini.c_str());Ui::ThemeConfigure(ini.c_str());
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    CHECK(GetMonitorInfoW(MonitorFromWindow(mainWindow,MONITOR_DEFAULTTONEAREST),&monitor));
    RECT work=monitor.rcWork;
    for(auto side:{Side::Bottom,Side::Top,Side::Right,Side::Left,Side::None}){
        SetWindowPos(mainWindow,nullptr,work.left+(work.right-work.left-300)/2,work.top+(work.bottom-work.top-150)/2,
            300,150,SWP_NOZORDER|SWP_NOACTIVATE);
        EqWnd_Toggle(mainWindow);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(eqWindow);
        RECT owner{},panel{};GetWindowRect(mainWindow,&owner);GetWindowRect(eqWindow,&panel);
        Place(panel,owner,{side,Anchor::End});
        if(side==Side::None)panel={work.left+10,work.top+10,work.left+470,work.top+224};
        SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&panel));
        SetWindowPos(eqWindow,nullptr,panel.left,panel.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
        RECT expected{},actual{};GetWindowRect(eqWindow,&expected);
        SendMessageW(eqWindow,WM_CLOSE,0,0);EqWnd_Toggle(mainWindow);
        GetWindowRect(eqWindow,&actual);CHECK(EqualRect(&expected,&actual));
        EqWnd_Shutdown();Ui::ThemeConfigure(ini.c_str());EqWnd_Restore(mainWindow);
        EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(EqWnd_IsVisible());
        GetWindowRect(eqWindow,&actual);CHECK(EqualRect(&expected,&actual));
        GetWindowRect(mainWindow,&owner);
        SetWindowPos(mainWindow,nullptr,owner.left+5,owner.top+5,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        GetWindowRect(eqWindow,&actual);if(side!=Side::None)OffsetRect(&expected,5,5);
        CHECK(EqualRect(&expected,&actual));
        SendMessageW(eqWindow,WM_CLOSE,0,0);EqWnd_Shutdown();Ui::ThemeConfigure(ini.c_str());
        EqWnd_Restore(mainWindow);CHECK(!EqWnd_IsVisible());EqWnd_Toggle(mainWindow);
        EnumThreadWindows(GetCurrentThreadId(),Find,0);GetWindowRect(eqWindow,&actual);CHECK(EqualRect(&expected,&actual));EqWnd_Shutdown();
    }
    CHECK(Ui::SaveEqualizerPosition({100000,100000,0,0}));EqWnd_Toggle(mainWindow);
    EnumThreadWindows(GetCurrentThreadId(),Find,0);RECT actual{};GetWindowRect(eqWindow,&actual);
    CHECK(actual.left>=work.left&&actual.top>=work.top&&actual.right<=work.right&&actual.bottom<=work.bottom);
    EqWnd_Shutdown();Ui::ThemeConfigure(nullptr);DeleteFileW(ini.c_str());
}
int main(){CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Ui::ThemeConfigure(nullptr);CHECK(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));
    mainWindow=MainWnd_Create(GetModuleHandleW(nullptr),SW_HIDE);CHECK(mainWindow);Audio_Shutdown();EqWnd_Toggle(mainWindow);
    EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(eqWindow);int cases=0;CASE(controls_publish_dsp_and_hidden_processing);CASE(all_themes_preserve_parameters);CASE(docking_follow_detach_and_scale);
    CASE(docked_panel_remains_reachable_at_monitor_edge);
    CASE(simultaneous_panels_hide_without_stopping_audio);
    CASE(docked_formation_survives_owner_moves);
    CASE(position_survives_restart);
    DestroyWindow(mainWindow);CoUninitialize();std::printf("%d EQ window cases\n",cases);}

