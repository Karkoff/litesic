#include "mainwnd.h"
#include "ui_player.h"
#include "audio.h"
#include "m3u.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "check.h"
#include <cmath>
static HWND window;
static LPARAM Point(float x,float y) {return MAKELPARAM(MainWnd_DPIScale(static_cast<int>(x)),MainWnd_DPIScale(static_cast<int>(y)));}
static void Click(int id,bool visible=true) {
    auto r=PlayerControlRect(id,visible); LPARAM p=Point(r.x+r.w/2,r.y+r.h/2);
    SendMessageW(window,WM_LBUTTONDOWN,MK_LBUTTON,p);SendMessageW(window,WM_LBUTTONUP,0,p);
}
static COLORREF VizPixel() {
    RECT r{};GetClientRect(window,&r);HDC screen=GetDC(window),dc=CreateCompatibleDC(screen);
    HBITMAP bitmap=CreateCompatibleBitmap(screen,r.right,r.bottom);auto old=SelectObject(dc,bitmap);
    CHECK(PrintWindow(window,dc,0));COLORREF color=GetPixel(dc,MainWnd_DPIScale(20),MainWnd_DPIScale(230));
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(window,screen);return color;
}
static void transport_and_seek_use_real_audio() {
    CHECK(Audio_TestInit(48000));
    PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));
    CHECK(PlayerPlayback().PlayIndex(0)); Click(7);
    Click(5); CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
    Click(6); CHECK(Audio_GetState()==AUDIO_STATE_PAUSED);
    ShowWindow(window,SW_SHOWNOACTIVATE);
    COLORREF visible=VizPixel();CHECK(visible!=Ui::NativeColor(Ui::Background));
    RECT expanded{},compact{},restored{};GetWindowRect(window,&expanded);
    Click(16);GetWindowRect(window,&compact);
    CHECK(compact.left==expanded.left && compact.top==expanded.top);
    CHECK(compact.right==expanded.right);
    CHECK(compact.bottom-compact.top==MainWnd_DPIScale(PlayerLayoutHeight(false)));
    CHECK(Audio_GetState()==AUDIO_STATE_PAUSED);
    auto button=PlayerControlRect(16,false);
    CHECK(PlayerControlAt(button.x+button.w/2,button.y+button.h/2,false)==16);
    CHECK(PlayerControlAt(384,VIZ_Y+10,false)!=13);
    Click(16,false);GetWindowRect(window,&restored);
    CHECK(EqualRect(&expanded,&restored));CHECK(VizPixel()==visible);CHECK(Audio_GetState()==AUDIO_STATE_PAUSED);
    SendMessageW(window,WM_LBUTTONDOWN,MK_LBUTTON,Point(SEEK_X+SEEK_W/2,SEEK_Y+10));
    SendMessageW(window,WM_LBUTTONUP,0,Point(SEEK_X+SEEK_W/2,SEEK_Y+10));
    CHECK(std::abs(Audio_GetPositionSeconds()-1)<.02);
    Click(7); CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);CHECK(Audio_GetPositionSeconds()==0);
}
static void capture_cancel_and_release_outside() {
    CHECK(Audio_Load(FullPath(L"tests\\fixtures\\native-44100.wav").c_str()));
    SendMessageW(window,WM_LBUTTONDOWN,MK_LBUTTON,Point(SEEK_X+10,SEEK_Y+10));
    SendMessageW(window,WM_MOUSEMOVE,MK_LBUTTON,Point(SEEK_X+SEEK_W+40,SEEK_Y+10));
    CHECK(std::abs(Audio_GetPositionSeconds()-2)<.02);
    SendMessageW(window,WM_LBUTTONUP,0,Point(SEEK_X+SEEK_W+40,SEEK_Y+10));
    double position=Audio_GetPositionSeconds();
    SendMessageW(window,WM_MOUSEMOVE,0,Point(SEEK_X,SEEK_Y+10));
    CHECK(Audio_GetPositionSeconds()==position);
    auto r=PlayerControlRect(5);
    SendMessageW(window,WM_LBUTTONDOWN,MK_LBUTTON,Point(r.x+10,r.y+10));
    SendMessageW(window,WM_LBUTTONUP,0,Point(450,280));
    CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);
}
static void geometry_dpi_and_keyboard_seek() {
    for(float scale : {1.f,1.25f,1.5f,2.f}) {
        for(int id=1;id<=14;++id) {
            auto r=PlayerControlRect(id);
            CHECK(r.x>=0 && r.y>=32 && r.x+r.w<=MAINWND_WIDTH && r.y+r.h<=MAINWND_HEIGHT);
            CHECK(PlayerControlAt(r.x+r.w/2,r.y+r.h/2)==id);
            CHECK(std::lround((r.x+r.w/2)*scale)/scale>=r.x);
        }
    }
    Click(8);
    SendMessageW(window,WM_KEYDOWN,VK_HOME,0);CHECK(Audio_GetPositionSeconds()==0);
    SendMessageW(window,WM_KEYDOWN,VK_RIGHT,0);CHECK(Audio_GetPositionSeconds()>0);
    SendMessageW(window,WM_KEYDOWN,VK_END,0);CHECK(std::abs(Audio_GetPositionSeconds()-2)<.02);
}
static HWND listWindow,eqWindow;
static BOOL CALLBACK FindPanels(HWND hwnd,LPARAM) {
    wchar_t name[80]{};GetClassNameW(hwnd,name,80);
    if(!wcscmp(name,L"LiteSic.PlaylistWnd")) listWindow=hwnd;
    if(!wcscmp(name,L"LiteSic.EqualizerWnd")) eqWindow=hwnd;
    return TRUE;
}
static void appearance_persists_across_window_recreation() {
    auto ini=FullPath(L"build\\window-appearance-"+std::to_wstring(sizeof(void*)*8)+
#ifdef DEBUG
        L"-debug.ini"
#else
        L"-release.ini"
#endif
    );
    DestroyWindow(window);DeleteFileW(ini.c_str());Ui::ThemeConfigure(ini.c_str());
    auto instance=GetModuleHandleW(nullptr);window=MainWnd_Create(instance,SW_SHOWNOACTIVATE);CHECK(window);
    CHECK(PlWnd_Create(instance,window));CHECK(PlWnd_IsVisible());
    Click(13);Click(16);PlWnd_Toggle();CHECK(!PlWnd_IsVisible());
    EqWnd_Toggle(window);EnumThreadWindows(GetCurrentThreadId(),FindPanels,0);CHECK(eqWindow && IsWindowVisible(eqWindow));
    SendMessageW(eqWindow,WM_CLOSE,0,0);CHECK(!Ui::LoadAppearance().equalizerVisible);EqWnd_Toggle(window);
    SendMessageW(window,WM_COMMAND,IDM_SCALE_90,0);
    DestroyWindow(window);Ui::ThemeConfigure(nullptr);Ui::ThemeConfigure(ini.c_str());
    window=MainWnd_Create(instance,SW_SHOWNOACTIVATE);CHECK(window);
    CHECK(PlWnd_Create(instance,window));CHECK(!PlWnd_IsVisible());
    EqWnd_Restore(window);eqWindow=nullptr;EnumThreadWindows(GetCurrentThreadId(),FindPanels,0);
    CHECK(eqWindow && IsWindowVisible(eqWindow));CHECK(std::abs(MainWnd_UserScaleMultiplier()-.9f)<1e-6f);
    RECT r{};GetClientRect(window,&r);CHECK(r.bottom==MainWnd_DPIScale(PlayerLayoutHeight(false)));
    auto state=Ui::LoadAppearance();CHECK(state.spectrum && !state.visualizerVisible);
    Click(16,false);Click(11);CHECK(PlWnd_IsVisible());
    SendMessageW(listWindow,WM_CLOSE,0,0);CHECK(!Ui::LoadAppearance().playlistVisible);
    Ui::ThemeConfigure(nullptr);DeleteFileW(ini.c_str());
}
int main() {
    HINSTANCE instance=GetModuleHandleW(nullptr);CHECK(MainWnd_RegisterClass(instance));
    window=MainWnd_Create(instance,SW_HIDE);CHECK(window);
    // Use the same offline production audio adapter as integration tests.
    Audio_Shutdown(); int cases=0;
    CASE(transport_and_seek_use_real_audio);CASE(capture_cancel_and_release_outside);CASE(geometry_dpi_and_keyboard_seek);CASE(appearance_persists_across_window_recreation);
    DestroyWindow(window);std::printf("%d player window cases\n",cases);
}
