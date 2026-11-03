#include "ui_scale.h"
#include "mainwnd.h"
// Real Win32 message integration, in this test process. No cross-process
// pointer injection and no mouse/keyboard input to the user's desktop.
#include "plwnd.h"
#include "playback.h"
#include "m3u.h"
#include "playlist_worker.h"
#include "ui_theme.h"
#include "check.h"
#include <shellapi.h>
#include <shlobj.h>
#include <cstring>
#include <cmath>
static HWND playlist;
static HWND information;
static BOOL CALLBACK FindInformation(HWND hwnd,LPARAM){wchar_t name[80]{};GetClassNameW(hwnd,name,80);if(!wcscmp(name,L"LiteSic.TrackInfoWnd"))information=hwnd;return TRUE;}
static void CALLBACK OpenInformationCommand(HWND,UINT,UINT_PTR id,DWORD){KillTimer(playlist,id);EndMenu();PostMessageW(playlist,WM_COMMAND,PL_COMMAND_INFORMATION,0);}
static BOOL CALLBACK FindPlaylist(HWND hwnd, LPARAM) {
    wchar_t name[64]; GetClassNameW(hwnd,name,64);
    if (!wcscmp(name,L"LiteSic.PlaylistWnd")) playlist=hwnd;
    return TRUE;
}
static void Drop(const std::vector<std::wstring>& paths) {
    std::wstring names;
    for (auto& path:paths) { names+=FullPath(path); names+=L'\0'; }
    names+=L'\0';
    HGLOBAL handle=GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT,sizeof(DROPFILES)+names.size()*sizeof(wchar_t)); CHECK(handle);
    auto header=static_cast<DROPFILES*>(GlobalLock(handle)); CHECK(header);
    header->pFiles=sizeof(DROPFILES); header->fWide=TRUE;
    std::memcpy(reinterpret_cast<char*>(header)+sizeof(DROPFILES),names.data(),names.size()*sizeof(wchar_t));
    GlobalUnlock(handle);
    SendMessageW(playlist,WM_DROPFILES,reinterpret_cast<WPARAM>(handle),0); // DragFinish frees the owned HDROP.
}
static void AwaitMetadata() {
    DWORD deadline=GetTickCount()+10000;
    for (;;) {
        MSG message;
        while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {TranslateMessage(&message);DispatchMessageW(&message);}
        bool pending=false;
        for(auto& track:PlayerPlaylist().Tracks()) pending|=track.metadataPending;
        if(!pending) return;
        CHECK(static_cast<LONG>(GetTickCount()-deadline)<0);
        MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);
    }
}
static void dropped_files_and_invalid_filter() {
    Drop({L"tests\\fixtures\\native-44100.wav",L"tests\\fixtures\\tagged.mp3",L"README.md"});
    CHECK(PlayerPlaylist().Count()==2); AwaitMetadata();
    CHECK(PlayerPlaylist().At(0)->duration==2); CHECK(PlayerPlaylist().At(1)->metadataLoaded);
    PlayerPlayback().Clear();
}
static void dropped_folder_recursive_and_missing_file() {
    Drop({L"tests\\fixtures"});
    DWORD deadline=GetTickCount()+10000;
    while(PlayerPlaylist().Count()!=7) {
        MSG message;while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) DispatchMessageW(&message);
        CHECK(static_cast<LONG>(GetTickCount()-deadline)<0);
        MsgWaitForMultipleObjects(0,nullptr,FALSE,10,QS_ALLINPUT);
    }
    AwaitMetadata(); Drop({L"tests\\fixtures\\absent.wav"}); AwaitMetadata();
    CHECK(PlayerPlaylist().Count()==8);CHECK(PlayerPlaylist().At(7)->failed);
    PlayerPlayback().Clear();
}
static void scale_layout_and_overlap() {
    HWND owner=GetWindow(playlist,GW_OWNER);
    SetWindowPos(owner,nullptr,100,100,600,400,SWP_NOZORDER|SWP_NOACTIVATE);
    SetWindowPos(playlist,nullptr,120,120,320,300,SWP_NOZORDER|SWP_NOACTIVATE);
    PlWnd_ScaleChanged();
    RECT parent{},list{},overlap{};GetWindowRect(owner,&parent);GetWindowRect(playlist,&list);
    CHECK(!IntersectRect(&overlap,&parent,&list));
    RECT suggested={120,510,600,960};
    SendMessageW(playlist,WM_DPICHANGED,MAKELONG(144,144),reinterpret_cast<LPARAM>(&suggested));
    MINMAXINFO minimum{};SendMessageW(playlist,WM_GETMINMAXINFO,0,reinterpret_cast<LPARAM>(&minimum));
    CHECK(minimum.ptMinTrackSize.x==413);CHECK(minimum.ptMinTrackSize.y==270);
    GetWindowRect(playlist,&list);CHECK(list.right-list.left==480);CHECK(list.bottom-list.top==450);
}
static void dock_follow_detach_and_reattach() {
    HWND owner=GetWindow(playlist,GW_OWNER);
    SetWindowPos(owner,nullptr,500,500,400,200,SWP_NOZORDER|SWP_NOACTIVATE);
    RECT proposed={510,705,990,1155};
    // Exercise production WM_MOVING/EXITSIZEMOVE handlers in-process.
    SendMessageW(playlist,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
    CHECK(proposed.top==700);
    SetWindowPos(playlist,nullptr,proposed.left,proposed.top,480,450,SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(playlist,WM_EXITSIZEMOVE,0,0);
    SetWindowPos(owner,nullptr,550,530,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);PlWnd_MainMoved();
    RECT list{};GetWindowRect(playlist,&list);CHECK(list.left==550);CHECK(list.top==730);
    proposed={1100,900,1580,1350};
    SendMessageW(playlist,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
    SetWindowPos(playlist,nullptr,1100,900,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(playlist,WM_EXITSIZEMOVE,0,0);
    SetWindowPos(owner,nullptr,500,500,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);PlWnd_MainMoved();
    GetWindowRect(playlist,&list);CHECK(list.left==1100);CHECK(list.top==900);
    // Reattach on each facing edge, then move and resize the main window.
    for(int side=0;side<4;++side) {
        RECT parent{};GetWindowRect(owner,&parent);
        int x=parent.left+10,y=parent.bottom+5;
        if(side==1)y=parent.top-450-5;
        if(side==2){x=parent.right+5;y=parent.top+10;}
        if(side==3){x=parent.left-480-5;y=parent.top+10;}
        proposed={x,y,x+480,y+450};
        SendMessageW(playlist,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
        SetWindowPos(playlist,nullptr,proposed.left,proposed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(playlist,WM_EXITSIZEMOVE,0,0);
        SetWindowPos(owner,nullptr,parent.left+20,parent.top+30,420,220,SWP_NOZORDER|SWP_NOACTIVATE);
        PlWnd_MainMoved();GetWindowRect(owner,&parent);GetWindowRect(playlist,&list);
        if(side==0){CHECK(list.top==parent.bottom);CHECK(list.left==parent.left);}
        if(side==1){CHECK(list.bottom==parent.top);CHECK(list.left==parent.left);}
        if(side==2){CHECK(list.left==parent.right);CHECK(list.top==parent.top);}
        if(side==3){CHECK(list.right==parent.left);CHECK(list.top==parent.top);}
        RECT overlap{};CHECK(!IntersectRect(&overlap,&parent,&list));
    }
}
static void dock_follow_stays_inside_work_area() {
    HWND owner=GetWindow(playlist,GW_OWNER);MONITORINFO m{};m.cbSize=sizeof(m);
    CHECK(GetMonitorInfoW(MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST),&m));
    SetWindowPos(owner,nullptr,m.rcWork.left+600,m.rcWork.top+300,400,200,SWP_NOZORDER|SWP_NOACTIVATE);
    RECT parent{},panel{};GetWindowRect(owner,&parent);GetWindowRect(playlist,&panel);
    RECT proposed{parent.right,parent.top,parent.right+panel.right-panel.left,parent.top+panel.bottom-panel.top};
    SendMessageW(playlist,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
    SetWindowPos(playlist,nullptr,proposed.left,proposed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(playlist,WM_EXITSIZEMOVE,0,0);
    SetWindowPos(owner,nullptr,m.rcWork.right-400,m.rcWork.top+300,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    PlWnd_MainMoved();GetWindowRect(playlist,&panel);
    CHECK(panel.left>=m.rcWork.left&&panel.right<=m.rcWork.right&&panel.top>=m.rcWork.top&&panel.bottom<=m.rcWork.bottom);
}
static void filter_selection_removes_canonical_item() {
    PlayerPlayback().Clear();
    PlayerPlaylist().Add(L"Alpha.wav"); PlayerPlaylist().Add(L"Beta.wav"); PlayerPlaylist().Add(L"Gamma.wav");
    PlWnd_Refresh();
    HWND edit=GetDlgItem(playlist,520); CHECK(edit);
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(80,270));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(80,270));
    SetWindowTextW(edit,L"BETA");
    SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0); CHECK(PlayerPlaylist().Count()==3);
    // The window was set to 144 DPI by the previous production DPI test.
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(80,140));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(80,140));
    SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0);
    CHECK(PlayerPlaylist().Count()==2); CHECK(PlayerPlaylist().At(0)->displayTitle==L"Alpha");
    CHECK(PlayerPlaylist().At(1)->displayTitle==L"Gamma");
    RECT client{};GetClientRect(playlist,&client);
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(client.right-33,78));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(client.right-33,78));
    CHECK(GetWindowTextLengthW(edit)==0);
    PlayerPlayback().Clear(); PlWnd_Refresh();
}
static void play_selection_and_center_current() {
    PlayerPlayback().Clear(); CHECK(Audio_TestInit(48000));
    for(int i=0;i<100;++i) PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));
    // Known client geometry at the 150% scale used by the DPI test.
    SetWindowPos(playlist,nullptr,0,0,480,450,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    PlWnd_Refresh();
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(80,140));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(80,140));
    CHECK(PlWnd_PlaySelected()); CHECK(PlayerPlaylist().GetCurrentIndex()==0);
    // Playing a visible bottom row must not move it. Clicking the same
    // coordinates afterwards must still select exactly the same track.
    for(auto point : {MAKELPARAM(80,350),MAKELPARAM(80,390),MAKELPARAM(80,140)}) {
        SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,point);
        SendMessageW(playlist,WM_LBUTTONUP,0,point);
        CHECK(PlWnd_PlaySelected());
        int expected=PlayerPlaylist().GetCurrentIndex();
        CHECK(expected==(HIWORD(point)==350?6:HIWORD(point)==390?7:0));
        SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,point);
        SendMessageW(playlist,WM_LBUTTONUP,0,point);
        CHECK(PlWnd_PlaySelected());CHECK(PlayerPlaylist().GetCurrentIndex()==expected);
    }
    CHECK(PlayerPlayback().PlayIndex(80)); PlWnd_Refresh(true);
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(80,250));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(80,250));
    CHECK(PlWnd_PlaySelected());CHECK(PlayerPlaylist().GetCurrentIndex()==80);
    CHECK(PlayerPlayback().PlayIndex(50)); PlWnd_Refresh(true);
    // Seven complete rows: the current track must occupy the fourth row.
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(80,250));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(80,250));
    CHECK(PlWnd_PlaySelected()); CHECK(PlayerPlaylist().GetCurrentIndex()==50);
    Audio_Pause(); CHECK(PlWnd_PlaySelected()); CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
    SetWindowTextW(GetDlgItem(playlist,520),L"does-not-match");
    PlWnd_Refresh(true);
    CHECK(GetWindowTextLengthW(GetDlgItem(playlist,520))==0);
    // Revealing also selects the track, even when a filter hid it.
    SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0);
    CHECK(PlayerPlaylist().Count()==99); CHECK(PlayerPlaylist().GetCurrentIndex()==-1);
    PlayerPlayback().Clear(); Audio_Shutdown(); PlWnd_Refresh();
}
static void session_survives_window_restart() {
    HWND owner=GetWindow(playlist,GW_OWNER); HINSTANCE instance=GetModuleHandleW(nullptr);
    std::wstring path=FullPath(L"build\\playlist-session-test.m3u8");
    DeleteFileW(path.c_str()); PlWnd_Shutdown(); PlayerPlayback().Clear();
    CHECK(PlWnd_Create(instance,owner,path.c_str()));
    std::wstring unicodeTrack=FullPath(L"build\\faixa-音.wav");
    CHECK(CopyFileW(FullPath(L"tests\\fixtures\\native-44100.wav").c_str(),unicodeTrack.c_str(),FALSE));
    PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));
    PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\missing.wav"));
    PlayerPlaylist().Add(unicodeTrack);
    PlayerPlaylist().At(0)->displayTitle=L"Título com acentuação";
    PlayerPlaylist().At(0)->favorite=true;
    PlayerPlaylist().MoveTo(1,2);
    auto expected=PlayerPlaylist().Tracks();
    EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
    PlWnd_Refresh();SendMessageW(playlist,WM_KEYDOWN,VK_HOME,0);SendMessageW(playlist,WM_KEYDOWN,VK_DOWN,0);
    CHECK(PlayerPlaylist().GetCurrentIndex()==-1); // Selection alone never starts audio.
    PlWnd_Shutdown(); PlayerPlayback().Clear();
    CHECK(PlWnd_Create(instance,owner,path.c_str()));
    CHECK(PlayerPlaylist().Count()==3);
    for(int i=0;i<3;++i) {
        CHECK(PlayerPlaylist().At(i)->path==expected[i].path);
        CHECK(PlayerPlaylist().At(i)->displayTitle==expected[i].displayTitle);
        CHECK(PlayerPlaylist().At(i)->favorite==expected[i].favorite);
    }
    EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
    CHECK(Audio_TestInit(48000));CHECK(Audio_GetState()!=AUDIO_STATE_PLAYING);
    CHECK(PlWnd_PlaySelected());CHECK(PlayerPlaylist().GetCurrentIndex()==1);
    Audio_Shutdown();
    PlWnd_Shutdown();PlayerPlayback().Clear();
    CHECK(WritePrivateProfileStringW(L"Selection",L"Path",L"stale-entry.wav",(path+L".ini").c_str()));
    CHECK(PlWnd_Create(instance,owner,path.c_str()));
    EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
    CHECK(Audio_TestInit(48000));CHECK(PlWnd_PlaySelected());
    CHECK(PlayerPlaylist().GetCurrentIndex()==0); // Stale companion state cannot select a different entry.
    Audio_Shutdown();
    // Saving a deliberately cleared playlist must not resurrect old entries.
    PlayerPlayback().Clear(); PlWnd_Shutdown();
    CHECK(PlWnd_Create(instance,owner,path.c_str())); CHECK(PlayerPlaylist().Count()==0);
    PlWnd_Shutdown(); CHECK(DeleteFileW(path.c_str()));DeleteFileW((path+L".ini").c_str());
    CHECK(DeleteFileW(unicodeTrack.c_str()));
}
static void favorites_tabs_and_track_information(){
    PlayerPlayback().Clear();CHECK(Audio_TestInit(48000));
    for(auto path:{L"tests\\fixtures\\native-44100.wav",L"tests\\fixtures\\tagged.mp3",L"tests\\fixtures\\tagged.flac"})PlayerPlaylist().Add(FullPath(path));
    auto track=PlayerPlaylist().At(1);track->metadata=ReadTrackMetadata(track->path);track->metadataPending=false;track->duration=track->metadata.duration;
    CHECK(PlayerPlayback().PlayIndex(0));PlWnd_Refresh();
    RECT r{};GetClientRect(playlist,&r);
    auto click=[](int x,int y){SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(x,y));SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(x,y));};
    // Playlist remains at 150% after the production DPI test.
    click(r.right-147,180);CHECK(PlayerPlaylist().At(1)->favorite);CHECK(PlayerPlaylist().GetCurrentIndex()==0&&Audio_GetState()==AUDIO_STATE_PLAYING);
    click(168,78);click(80,140);CHECK(PlWnd_PlaySelected());CHECK(PlayerPlaylist().GetCurrentIndex()==1);
    CHECK(SetTimer(playlist,888,20,OpenInformationCommand));
    SendMessageW(playlist,WM_CONTEXTMENU,reinterpret_cast<WPARAM>(playlist),MAKELPARAM(-1,-1));
    DWORD deadline=GetTickCount()+2000;
    while(!information&&GetTickCount()<deadline){MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}EnumThreadWindows(GetCurrentThreadId(),FindInformation,0);Sleep(1);}
    CHECK(information);
    float originalZoom=MainWnd_UserScaleMultiplier();
    for(int dpi:{96,120,144,192}){
        RECT proposed{};CHECK(GetWindowRect(information,&proposed));
        SendMessageW(information,WM_DPICHANGED,MAKELONG(dpi,dpi),reinterpret_cast<LPARAM>(&proposed));
        RECT client{};CHECK(GetClientRect(information,&client));
        float expected=Ui::FitScale(dpi/96.f*originalZoom,560,388,Ui::MonitorWork(information));
        CHECK(std::abs(client.right-int(std::lround(560*expected)))<=2);
        RECT editRect{};CHECK(GetWindowRect(GetDlgItem(information,801),&editRect));
        CHECK(editRect.right>editRect.left&&editRect.bottom>editRect.top);
    }
    wchar_t text[4096]{};GetWindowTextW(GetDlgItem(information,801),text,4096);
    CHECK(wcsstr(text,L"Canção M5")&&wcsstr(text,L"Artista: LiteSic")&&wcsstr(text,L"MP3")&&wcsstr(text,L"Hz")&&wcsstr(text,L"tagged.mp3"));
    CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);MSG escape{};escape.hwnd=GetDlgItem(information,801);escape.message=WM_KEYDOWN;escape.wParam=VK_ESCAPE;
    CHECK(PlWnd_DialogMessage(&escape));CHECK(!IsWindow(information));information=nullptr;
    click(r.right-147,140);CHECK(!PlayerPlaylist().At(1)->favorite);CHECK(PlayerPlaylist().Count()==3&&Audio_GetState()==AUDIO_STATE_PLAYING);
    click(50,78);click(80,140);CHECK(PlWnd_PlaySelected());CHECK(PlayerPlaylist().GetCurrentIndex()==0);
    PlayerPlayback().Clear();Audio_Shutdown();PlWnd_Refresh();
}
static void large_search_ignores_stale_metadata_notifications(){
    PlayerPlayback().Clear();
    for(int i=0;i<10453;++i)PlayerPlaylist().Add(L"synthetic\\Band - Song "+std::to_wstring(i)+L".mp3");
    PlayerPlaylist().At(7654)->displayTitle=L"Match ey";PlWnd_Refresh();
    HWND edit=GetDlgItem(playlist,520);CHECK(edit);SetWindowTextW(edit,L"ey");
    DWORD start=GetTickCount();
    // Old messages must not trigger thousands of full-library filter scans.
    for(int i=0;i<1000;++i){SendMessageW(playlist,WM_PLAYLIST_RESULTS,0,0);SendMessageW(playlist,WM_TIMER,524,0);}
    CHECK(GetTickCount()-start<2000);
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(80,140));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(80,140));
    SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0);
    CHECK(PlayerPlaylist().Count()==10452);
    for(const auto& track:PlayerPlaylist().Tracks())CHECK(track.displayTitle!=L"Match ey");
    SetWindowTextW(edit,L"");PlayerPlayback().Clear();PlWnd_Refresh();
}
static void playback_follows_tab_that_started_play(){
    PlayerPlayback().Clear();CHECK(Audio_TestInit(48000));
    for(int i=0;i<3;++i)PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));
    PlayerPlaylist().At(1)->favorite=true;PlWnd_Refresh();
    auto click=[](int x,int y){SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(x,y));SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(x,y));};
    click(168,78);click(80,140);CHECK(PlWnd_PlaySelected());CHECK(PlayerPlaylist().GetCurrentIndex()==1);
    click(50,78);CHECK(!PlayerPlayback().Next(false,false));CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);
    // Explicit Play of that same track from General changes the playback scope.
    click(80,180);CHECK(PlWnd_PlaySelected());CHECK(PlayerPlaylist().GetCurrentIndex()==1);
    CHECK(PlayerPlayback().Next(false,false));CHECK(PlayerPlaylist().GetCurrentIndex()==2);
    click(168,78);SendMessageW(playlist,WM_LBUTTONDBLCLK,MK_LBUTTON,MAKELPARAM(80,140));
    CHECK(PlayerPlaylist().GetCurrentIndex()==1);CHECK(!PlayerPlayback().Next(false,false));
    SendMessageW(playlist,WM_KEYDOWN,VK_RETURN,0);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
    CHECK(!PlayerPlayback().Next(false,false));CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);
    click(50,78);PlayerPlayback().Clear();Audio_Shutdown();PlWnd_Refresh();
}
static void geometry_survives_restart(){
    PlWnd_Shutdown();PlayerPlayback().Clear();
    HINSTANCE instance=GetModuleHandleW(nullptr);
    HWND owner=CreateWindowExW(0,L"STATIC",L"geometry test owner",WS_POPUP,0,0,260,100,nullptr,nullptr,instance,nullptr);CHECK(owner);
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    CHECK(GetMonitorInfoW(MonitorFromWindow(owner,MONITOR_DEFAULTTONEAREST),&monitor));
    const RECT work=monitor.rcWork;
    const auto path=FullPath(L"build\\playlist-geometry-test.m3u8"),ini=path+L".ini";
    DeleteFileW(path.c_str());DeleteFileW(ini.c_str());
    for(int side=0;side<5;++side){
        CHECK(PlWnd_Create(instance,owner,path.c_str()));EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
        int mx=work.left+(work.right-work.left-260)/2,my=work.top+(work.bottom-work.top-100)/2;
        SetWindowPos(owner,nullptr,mx,my,260,100,SWP_NOZORDER|SWP_NOACTIVATE);
        RECT panel{mx,my+100,mx+300,my+280};
        if(side==1)panel={mx,my-180,mx+300,my};
        if(side==2)panel={mx+260,my,mx+560,my+180};
        if(side==3)panel={mx-300,my,mx,my+180};
        if(side==4)panel={work.left+10,work.top+10,work.left+310,work.top+190};
        SendMessageW(playlist,WM_MOVING,0,reinterpret_cast<LPARAM>(&panel));
        SetWindowPos(playlist,nullptr,panel.left,panel.top,300,180,SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(playlist,WM_EXITSIZEMOVE,0,0);
        RECT expected{},expectedOwner{};GetWindowRect(playlist,&expected);GetWindowRect(owner,&expectedOwner);
        // Hiding/reopening the panel and restarting the session both retain its position.
        SendMessageW(playlist,WM_CLOSE,0,0);PlWnd_Show();
        RECT actual{};GetWindowRect(playlist,&actual);CHECK(EqualRect(&expected,&actual));
        PlWnd_Shutdown();
        SetWindowPos(owner,nullptr,work.left,work.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        CHECK(PlWnd_Create(instance,owner,path.c_str()));EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
        GetWindowRect(playlist,&actual);CHECK(EqualRect(&expected,&actual));
        GetWindowRect(owner,&actual);CHECK(EqualRect(&expectedOwner,&actual));
        RECT moved{expectedOwner.left+5,expectedOwner.top+5,expectedOwner.right+5,expectedOwner.bottom+5};
        SetWindowPos(owner,nullptr,moved.left,moved.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);PlWnd_MainMoved(true);
        GetWindowRect(playlist,&actual);
        if(side!=4)OffsetRect(&expected,5,5);
        CHECK(EqualRect(&expected,&actual));PlWnd_Shutdown();
    }
    CHECK(WritePrivateProfileStringW(L"Geometry",L"Left",L"100000",ini.c_str()));
    CHECK(WritePrivateProfileStringW(L"Geometry",L"Top",L"100000",ini.c_str()));
    CHECK(PlWnd_Create(instance,owner,path.c_str()));EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
    RECT actual{};GetWindowRect(playlist,&actual);
    CHECK(actual.left>=work.left&&actual.top>=work.top&&actual.right<=work.right&&actual.bottom<=work.bottom);
    PlWnd_Shutdown();DeleteFileW(path.c_str());DeleteFileW(ini.c_str());DestroyWindow(owner);
}
static void active_tab_survives_restart(){
    PlWnd_Shutdown();PlayerPlayback().Clear();
    HINSTANCE instance=GetModuleHandleW(nullptr);
    HWND owner=CreateWindowExW(0,L"STATIC",L"tab persistence test",WS_POPUP,100,100,300,100,nullptr,nullptr,instance,nullptr);CHECK(owner);
    const auto ini=FullPath(L"build\\playlist-tab-"+std::to_wstring(sizeof(void*)*8)+
#ifdef DEBUG
        L"-debug.ini");
#else
        L"-release.ini");
#endif
    DeleteFileW(ini.c_str());Ui::ThemeConfigure(ini.c_str());
    auto create=[&](){CHECK(PlWnd_Create(instance,owner));EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
        PlayerPlaylist().Add(L"General.wav");PlayerPlaylist().Add(L"Favorite.wav");PlayerPlaylist().At(1)->favorite=true;PlWnd_Refresh();};
    auto click=[](int x,int y){SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(x,y));SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(x,y));};
    create();click(112,52);CHECK(Ui::LoadAppearance().playlistFavoritesOnly);
    SendMessageW(playlist,WM_CLOSE,0,0);PlWnd_Show();CHECK(Ui::LoadAppearance().playlistFavoritesOnly);
    PlWnd_Shutdown();PlayerPlayback().Clear();Ui::ThemeConfigure(ini.c_str());create();
    click(80,96);SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0);
    CHECK(PlayerPlaylist().Count()==1&&PlayerPlaylist().At(0)->path==L"General.wav");
    click(38,52);CHECK(!Ui::LoadAppearance().playlistFavoritesOnly);
    PlWnd_Shutdown();PlayerPlayback().Clear();Ui::ThemeConfigure(ini.c_str());create();
    click(80,96);SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0);
    CHECK(PlayerPlaylist().Count()==1&&PlayerPlaylist().At(0)->path==L"Favorite.wav");
    PlWnd_Shutdown();PlayerPlayback().Clear();Ui::ThemeConfigure(nullptr);DeleteFileW(ini.c_str());DestroyWindow(owner);
}
static void export_general_and_favorites_m3u(){
    HINSTANCE instance=GetModuleHandleW(nullptr);
    HWND owner=CreateWindowExW(0,L"STATIC",L"export integration owner",WS_POPUP,0,0,1,1,nullptr,nullptr,instance,nullptr);CHECK(owner);
    Ui::ThemeConfigure(nullptr);PlayerPlayback().Clear();
    CHECK(PlWnd_Create(instance,owner));EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);
    for(auto path:{L"tests\\fixtures\\native-44100.wav",L"tests\\fixtures\\tagged.mp3",L"tests\\fixtures\\tagged.flac"})PlayerPlaylist().Add(FullPath(path));
    PlayerPlaylist().At(1)->favorite=true;PlayerPlaylist().At(2)->favorite=true;
    auto expected=PlayerPlaylist().Tracks();
    for(int scenario=0;scenario<4;++scenario){
        const bool favorites=scenario!=0;
        if(scenario==3)for(int i=0;i<PlayerPlaylist().Count();++i)PlayerPlaylist().At(i)->favorite=false;
        // Commands and clicks target only windows created by this test.
        SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(favorites?112:38,52));
        SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(favorites?112:38,52));
        // A search with no matches must not silently export an empty collection.
        SetWindowTextW(GetDlgItem(playlist,520),L"no-matching-track");
        const auto exportPath=FullPath(scenario==2?L"build\\export-favorites.m3u8":favorites?L"build\\export-favorites.m3u":L"build\\export-general.m3u");
        DeleteFileW(exportPath.c_str());
        // Exercise the same snapshot, worker and atomic writer as the Save dialog.
        CHECK(PlWnd_SavePlaylist(exportPath));
        DWORD deadline=GetTickCount()+10000;
        while(GetFileAttributesW(exportPath.c_str())==INVALID_FILE_ATTRIBUTES){
            MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
            CHECK(static_cast<LONG>(GetTickCount()-deadline)<0);Sleep(10);
        }
        std::vector<Track> loaded;std::wstring error;CHECK(LoadM3U(exportPath,loaded,error));
        CHECK(loaded.size()==(scenario==3?0u:favorites?2u:3u));
        for(size_t i=0;i<loaded.size();++i){
            const auto& track=expected[i+(favorites?1:0)];
            CHECK(loaded[i].path==track.path);CHECK(loaded[i].favorite==track.favorite);
        }
        CHECK(PlayerPlaylist().Count()==3);CHECK(DeleteFileW(exportPath.c_str()));
    }
    PlWnd_Shutdown();PlayerPlayback().Clear();DestroyWindow(owner);
    CHECK(!PlWnd_SavePlaylist(L"build\\export-without-window.m3u"));
}
int main(int argc,char**) {
    setvbuf(stdout,nullptr,_IONBF,0);
    if(argc>1){export_general_and_favorites_m3u();return 0;}
    HINSTANCE instance=GetModuleHandleW(nullptr);
    HWND owner=CreateWindowExW(0,L"STATIC",L"playlist integration owner",WS_POPUP,0,0,1,1,nullptr,nullptr,instance,nullptr);CHECK(owner);
    CHECK(PlWnd_Create(instance,owner)); EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);CHECK(playlist);
    ShowWindow(playlist,SW_HIDE);
    int cases=0;CASE(dropped_files_and_invalid_filter);CASE(dropped_folder_recursive_and_missing_file);CASE(scale_layout_and_overlap);
    CASE(dock_follow_detach_and_reattach);
    CASE(dock_follow_stays_inside_work_area);
    CASE(filter_selection_removes_canonical_item);
    CASE(play_selection_and_center_current);
    CASE(large_search_ignores_stale_metadata_notifications);
    CASE(favorites_tabs_and_track_information);
    CASE(playback_follows_tab_that_started_play);
    CASE(session_survives_window_restart);
    CASE(geometry_survives_restart);
    CASE(active_tab_survives_restart);
    CASE(export_general_and_favorites_m3u);
    PlWnd_Shutdown();DestroyWindow(owner);std::printf("%d playlist window integration cases\n",cases);
}
