#include "skin.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "ui_theme.h"
#include "eq_state.h"
#include "playback.h"
#include "audio.h"
#include "m3u.h"
#include "check.h"
#include <string>
static HWND player,playlist,eq;
static BOOL CALLBACK Find(HWND h,LPARAM){
    if(GetWindow(h,GW_OWNER)!=player)return TRUE;
    wchar_t name[80]{};GetClassNameW(h,name,80);
    if(!wcscmp(name,L"LiteSic.PlaylistWnd"))playlist=h;
    if(!wcscmp(name,L"LiteSic.EqualizerWnd"))eq=h;
    return TRUE;
}
static RECT Rect(HWND h){RECT r{};CHECK(GetWindowRect(h,&r));return r;}
static void Move(HWND h,int x,int y){
    RECT r=Rect(h);OffsetRect(&r,x-r.left,y-r.top);
    SendMessageW(h,WM_MOVING,0,reinterpret_cast<LPARAM>(&r));
    SetWindowPos(h,nullptr,r.left,r.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(h,WM_EXITSIZEMOVE,0,0);
}
static void Reset(){
    Skin::Attachment(Skin::Playlist)={};Skin::Attachment(Skin::Equalizer)={};
    Skin::RestoreParent(Skin::Playlist,Skin::Player);Skin::RestoreParent(Skin::Equalizer,Skin::Player);
    ShowWindow(player,SW_SHOWNOACTIVATE);ShowWindow(playlist,SW_SHOWNOACTIVATE);ShowWindow(eq,SW_SHOWNOACTIVATE);
    SendMessageW(player,WM_COMMAND,IDM_SCALE_100,0);
    MONITORINFO m{};m.cbSize=sizeof(m);CHECK(GetMonitorInfoW(MonitorFromWindow(player,MONITOR_DEFAULTTONEAREST),&m));
    SetWindowPos(player,nullptr,m.rcWork.left+100,m.rcWork.top+100,460,290,SWP_NOZORDER|SWP_NOACTIVATE);
}
static void Chain(bool playlistFirst){
    Reset();RECT p=Rect(player);
    if(playlistFirst){Move(playlist,p.left,p.bottom);RECT pl=Rect(playlist);Move(eq,pl.right,pl.top);CHECK(Skin::Parent(Skin::Equalizer)==Skin::Playlist);}
    else {Move(eq,p.right,p.top);RECT e=Rect(eq);Move(playlist,e.left,e.bottom);CHECK(Skin::Parent(Skin::Playlist)==Skin::Equalizer);}
}
static void transitive_move_in_both_orders(){
    for(bool order:{false,true}){Chain(order);auto p=Rect(player),pl=Rect(playlist),e=Rect(eq);
        SetWindowPos(player,nullptr,p.left+35,p.top+27,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        auto now=Rect(player);OffsetRect(&pl,now.left-p.left,now.top-p.top);OffsetRect(&e,now.left-p.left,now.top-p.top);
        auto actual=Rect(playlist);CHECK(EqualRect(&pl,&actual));actual=Rect(eq);CHECK(EqualRect(&e,&actual));}
}
static void detached_component_moves_and_reattaches(){
    Chain(false);auto p=Rect(player);Move(eq,p.right+200,p.top+100);
    CHECK(Skin::Attachment(Skin::Equalizer).side==WindowSnap::Side::None);
    auto e=Rect(eq),pl=Rect(playlist);CHECK(pl.top==e.bottom);
    SetWindowPos(player,nullptr,p.left+20,p.top+20,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    auto actual=Rect(eq);CHECK(EqualRect(&e,&actual));p=Rect(player);Move(eq,p.right,p.top);
    CHECK(Skin::Parent(Skin::Equalizer)==Skin::Player);CHECK(Skin::Attachment(Skin::Equalizer).side==WindowSnap::Side::Right);
    CHECK(Rect(playlist).top==Rect(eq).bottom);
}
static void player_snaps_to_independent_panel(){
    Reset();auto p=Rect(player);Move(playlist,p.right+250,p.top+300);ShowWindow(eq,SW_HIDE);
    CHECK(Skin::Attachment(Skin::Playlist).side==WindowSnap::Side::None);
    auto pl=Rect(playlist);Move(player,pl.left,pl.top-(p.bottom-p.top)+8);
    CHECK(Rect(player).bottom==Rect(playlist).top);CHECK(Skin::Parent(Skin::Playlist)==Skin::Player);
    p=Rect(player);pl=Rect(playlist);SetWindowPos(player,nullptr,p.left+12,p.top+13,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    auto now=Rect(player);OffsetRect(&pl,now.left-p.left,now.top-p.top);auto actual=Rect(playlist);CHECK(EqualRect(&pl,&actual));
}
static void hidden_bridge_and_minimize_restore_preserve_audio(){
    Chain(false);CHECK(Audio_TestInit(48000));CHECK(Audio_Load(FullPath(L"tests\\fixtures\\native-44100.wav").c_str()));Audio_Play();Eq::ApplyPreset(1);Eq::SetEnabled(true);
    SendMessageW(eq,WM_CLOSE,0,0);CHECK(!EqWnd_IsVisible());auto p=Rect(player),pl=Rect(playlist);
    SetWindowPos(player,nullptr,p.left+15,p.top+11,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);auto now=Rect(player);
    OffsetRect(&pl,now.left-p.left,now.top-p.top);auto actual=Rect(playlist);CHECK(EqualRect(&pl,&actual));
    ShowWindow(player,SW_MINIMIZE);ShowWindow(player,SW_RESTORE);Skin::Reconcile();
    SendMessageW(playlist,WM_CLOSE,0,0);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING&&Eq::State().enabled);
    float pcm[960]{};auto cursor=Audio_TestCursor();CHECK(Audio_TestRead(pcm,480));CHECK(Audio_TestCursor()>cursor);
    PlWnd_Toggle();EqWnd_Toggle(player);CHECK(PlWnd_IsVisible()&&EqWnd_IsVisible());Audio_Shutdown();
}
static void visibility_compacts_restores_and_manual_drag_redefines(){
    for(bool order:{false,true}) {
        Chain(order);HWND middle=order?playlist:eq,leaf=order?eq:playlist;
        auto p=Rect(player),expanded=Rect(leaf);
        SendMessageW(middle,WM_CLOSE,0,0);auto compact=Rect(leaf);
        CHECK(order?compact.left==p.right:compact.top==p.bottom);
        CHECK(Skin::Parent(order?Skin::Equalizer:Skin::Playlist)==(order?Skin::Playlist:Skin::Equalizer));
        SetWindowPos(player,nullptr,p.left+13,p.top+17,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        OffsetRect(&compact,13,17);auto actual=Rect(leaf);CHECK(EqualRect(&compact,&actual));
        if(order)PlWnd_Toggle();else EqWnd_Toggle(player);
        OffsetRect(&expanded,13,17);actual=Rect(leaf);CHECK(EqualRect(&expanded,&actual));
        SendMessageW(middle,WM_CLOSE,0,0);p=Rect(player);
        Move(leaf,p.right+180,p.top+220);auto manual=Rect(leaf);
        CHECK(Skin::Attachment(order?Skin::Equalizer:Skin::Playlist).side==WindowSnap::Side::None);
        if(order)PlWnd_Toggle();else EqWnd_Toggle(player);
        actual=Rect(leaf);CHECK(EqualRect(&manual,&actual));
    }
}
static void hidden_middle_stays_compact_after_minimize_restore(){
    for(bool order:{false,true}){
        Reset();auto p=Rect(player);
        SetWindowPos(playlist,nullptr,0,0,460,180,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        HWND middle=order?playlist:eq,leaf=order?eq:playlist;
        Move(leaf,p.right+250,p.top+300);Move(middle,p.left,p.bottom);
        auto m=Rect(middle);Move(leaf,m.left,m.bottom);
        CHECK(Rect(middle).top==Rect(player).bottom);CHECK(Rect(leaf).top==Rect(middle).bottom);
        SendMessageW(middle,WM_CLOSE,0,0);CHECK(!IsWindowVisible(middle));
        CHECK(Rect(leaf).top==Rect(player).bottom);
        for(int cycle=0;cycle<3;++cycle){
            ShowWindow(player,SW_MINIMIZE);CHECK(IsIconic(player));
            ShowWindow(player,SW_RESTORE);
            MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
            CHECK(!IsWindowVisible(middle));CHECK(IsWindowVisible(leaf));
            CHECK(Rect(leaf).top==Rect(player).bottom);
        }
        if(order)PlWnd_Toggle();else EqWnd_Toggle(player);
        CHECK(Rect(middle).top==Rect(player).bottom);CHECK(Rect(leaf).top==Rect(middle).bottom);
    }
}
static void chain_resize_dpi_and_display_recovery(){
    Chain(true);
    for(int dpi:{96,120,144,192}){
        RECT proposed=Rect(player);SendMessageW(player,WM_DPICHANGED,MAKELONG(dpi,dpi),reinterpret_cast<LPARAM>(&proposed));
        auto p=Rect(player),pl=Rect(playlist),e=Rect(eq);CHECK(pl.top==p.bottom);CHECK(e.left==pl.right&&e.top==pl.top);
        auto width=pl.right-pl.left;SetWindowPos(playlist,nullptr,0,0,width+20,pl.bottom-pl.top+10,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        CHECK(Rect(eq).left==Rect(playlist).right);
    }
    Skin::Attachment(Skin::Equalizer)={};SetWindowPos(eq,nullptr,90000,90000,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(player,WM_DISPLAYCHANGE,0,0);MONITORINFO m{};m.cbSize=sizeof(m);auto e=Rect(eq);
    CHECK(GetMonitorInfoW(MonitorFromRect(&e,MONITOR_DEFAULTTONEAREST),&m));CHECK(e.left>=m.rcWork.left&&e.top>=m.rcWork.top&&e.right<=m.rcWork.right&&e.bottom<=m.rcWork.bottom);
}
static void peer_attachment_persists_and_invalid_cycle_falls_back(){
    auto ini=FullPath(L"build\\m7\\integration-"+std::to_wstring(sizeof(void*)*8)+L".ini");
    auto session=ini+L".m3u8";Ui::ThemeConfigure(ini.c_str());
    Chain(false);auto expected=Rect(playlist);PlWnd_Shutdown();EqWnd_Shutdown();
    CHECK(PlWnd_Create(GetModuleHandleW(nullptr),player,session.c_str()));EqWnd_Toggle(player);EnumThreadWindows(GetCurrentThreadId(),Find,0);Chain(false);
    expected=Rect(playlist);PlWnd_Shutdown();EqWnd_Shutdown();
    CHECK(PlWnd_Create(GetModuleHandleW(nullptr),player,session.c_str()));EqWnd_Restore(player);EnumThreadWindows(GetCurrentThreadId(),Find,0);
    CHECK(Skin::Parent(Skin::Playlist)==Skin::Equalizer);auto actual=Rect(playlist);CHECK(EqualRect(&expected,&actual));
    auto plDock=Skin::Attachment(Skin::Playlist);Skin::Attachment(Skin::Equalizer)={WindowSnap::Side::Right,WindowSnap::Anchor::Start};
    Skin::RestoreParent(Skin::Equalizer,Skin::Playlist);CHECK(Skin::Attachment(Skin::Equalizer).side==WindowSnap::Side::None);
    CHECK(Skin::Attachment(Skin::Playlist).side==plDock.side);Skin::Reconcile();
    PlWnd_Shutdown();EqWnd_Shutdown();PlayerPlayback().Clear();Ui::ThemeConfigure(nullptr);
    DeleteFileW(ini.c_str());DeleteFileW(session.c_str());DeleteFileW((session+L".ini").c_str());
}
int main(){
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Ui::ThemeConfigure(nullptr);CHECK(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));
    player=MainWnd_Create(GetModuleHandleW(nullptr),SW_HIDE);CHECK(player);Audio_Shutdown();
    CHECK(PlWnd_Create(GetModuleHandleW(nullptr),player));EqWnd_Toggle(player);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(playlist&&eq);
    int cases=0;CASE(transitive_move_in_both_orders);CASE(detached_component_moves_and_reattaches);CASE(player_snaps_to_independent_panel);
    CASE(hidden_bridge_and_minimize_restore_preserve_audio);CASE(visibility_compacts_restores_and_manual_drag_redefines);CASE(hidden_middle_stays_compact_after_minimize_restore);CASE(chain_resize_dpi_and_display_recovery);CASE(peer_attachment_persists_and_invalid_cycle_falls_back);
    DestroyWindow(player);CoUninitialize();std::printf("%d window integration cases\n",cases);
}
