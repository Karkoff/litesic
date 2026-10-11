// Real Win32 message integration, in this test process. No cross-process
// pointer injection and no mouse/keyboard input to the user's desktop.
#include "plwnd.h"
#include "playback.h"
#include "m3u.h"
#include "check.h"
#include <shellapi.h>
#include <shlobj.h>
#include <cstring>
static HWND playlist;
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
    RECT list{};GetWindowRect(playlist,&list);CHECK(list.left==560);CHECK(list.top==730);
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
        if(side==0){CHECK(list.top==parent.bottom);CHECK(list.left==parent.left+10);}
        if(side==1){CHECK(list.bottom==parent.top);CHECK(list.left==parent.left+10);}
        if(side==2){CHECK(list.left==parent.right);CHECK(list.top==parent.top+10);}
        if(side==3){CHECK(list.right==parent.left);CHECK(list.top==parent.top+10);}
        RECT overlap{};CHECK(!IntersectRect(&overlap,&parent,&list));
    }
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
    SetWindowTextW(edit,L""); PlayerPlayback().Clear(); PlWnd_Refresh();
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
    CHECK(PlayerPlayback().PlayIndex(50)); PlWnd_Refresh(true);
    // Six complete rows: the current track must occupy the fourth row.
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(80,240));
    SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(80,240));
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
int main() {
    HINSTANCE instance=GetModuleHandleW(nullptr);
    HWND owner=CreateWindowExW(0,L"STATIC",L"playlist integration owner",WS_POPUP,0,0,1,1,nullptr,nullptr,instance,nullptr);CHECK(owner);
    CHECK(PlWnd_Create(instance,owner)); EnumThreadWindows(GetCurrentThreadId(),FindPlaylist,0);CHECK(playlist);
    ShowWindow(playlist,SW_HIDE);
    int cases=0;CASE(dropped_files_and_invalid_filter);CASE(dropped_folder_recursive_and_missing_file);CASE(scale_layout_and_overlap);
    CASE(dock_follow_detach_and_reattach);
    CASE(filter_selection_removes_canonical_item);
    CASE(play_selection_and_center_current);
    CASE(session_survives_window_restart);
    PlWnd_Shutdown();DestroyWindow(owner);std::printf("%d playlist window integration cases\n",cases);
}
