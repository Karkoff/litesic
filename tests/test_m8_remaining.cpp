#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "audio.h"
#include "playback.h"
#include "ui_theme.h"
#include "ui_scale.h"
#include "language_selection.h"
#include "runtime.h"
#include "skin.h"
#include "eq_state.h"
#include "m3u.h"
#include "check.h"
#include "isolated_desktop.h"
#include <oleacc.h>
#include <cmath>
static HWND player,playlist,eq,themes,information;
static BOOL CALLBACK Find(HWND hwnd,LPARAM){wchar_t cls[80]{};GetClassNameW(hwnd,cls,80);if(!wcscmp(cls,L"LiteSic.PlaylistWnd"))playlist=hwnd;if(!wcscmp(cls,L"LiteSic.EqualizerWnd"))eq=hwnd;if(!wcscmp(cls,L"LiteSic.ThemesWnd"))themes=hwnd;if(!wcscmp(cls,L"LiteSic.TrackInfoWnd"))information=hwnd;return TRUE;}
static void Key(HWND hwnd,WPARAM key){SendMessageW(hwnd,WM_KEYDOWN,key,0);}
static RECT Rect(HWND hwnd){RECT r{};GetWindowRect(hwnd,&r);return r;}
static void Dpi(HWND hwnd,int dpi){auto r=Rect(hwnd);SendMessageW(hwnd,WM_DPICHANGED,MAKELONG(dpi,dpi),reinterpret_cast<LPARAM>(&r));}
static void shortcuts_and_media_on_three_panels(){
    CHECK(Audio_TestInit(48000));PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));
    for(HWND hwnd:{player,playlist,eq}){Key(hwnd,'X');CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);Key(hwnd,'C');CHECK(Audio_GetState()==AUDIO_STATE_PAUSED);Key(hwnd,'V');CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);
        CHECK(SendMessageW(hwnd,WM_APPCOMMAND,0,LPARAM(APPCOMMAND_MEDIA_PLAY_PAUSE)<<16));CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
        CHECK(SendMessageW(hwnd,WM_APPCOMMAND,0,LPARAM(APPCOMMAND_MEDIA_STOP)<<16));CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);
        Key(hwnd,'B');CHECK(PlayerPlaylist().GetCurrentIndex()==1);Key(hwnd,'Z');CHECK(PlayerPlaylist().GetCurrentIndex()==0);
        CHECK(SendMessageW(hwnd,WM_APPCOMMAND,0,LPARAM(APPCOMMAND_MEDIA_NEXTTRACK)<<16));CHECK(PlayerPlaylist().GetCurrentIndex()==1);CHECK(SendMessageW(hwnd,WM_APPCOMMAND,0,LPARAM(APPCOMMAND_MEDIA_PREVIOUSTRACK)<<16));CHECK(PlayerPlaylist().GetCurrentIndex()==0);
    }
    MSG edit{GetDlgItem(playlist,520),WM_KEYDOWN,'X',0,0,{0,0}};CHECK(!MainWnd_TranslateShortcut(&edit));
}
static void hotkey_conflict_rolls_back_and_releases(){
    HWND blocker=CreateWindowExW(0,L"STATIC",L"",WS_POPUP,0,0,1,1,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(blocker);
    CHECK(RegisterHotKey(blocker,6200,MOD_CONTROL|MOD_ALT,VK_F11));CHECK(!MainWnd_SetGlobalHotkeys(player,true));
    CHECK(RegisterHotKey(blocker,6201,MOD_CONTROL|MOD_ALT,VK_F9));UnregisterHotKey(blocker,6201);UnregisterHotKey(blocker,6200);
    CHECK(MainWnd_SetGlobalHotkeys(player,true));CHECK(!RegisterHotKey(blocker,6202,MOD_CONTROL|MOD_ALT,VK_F9));CHECK(MainWnd_SetGlobalHotkeys(player,false));CHECK(RegisterHotKey(blocker,6202,MOD_CONTROL|MOD_ALT,VK_F9));UnregisterHotKey(blocker,6202);DestroyWindow(blocker);
}
static void playlist_keyboard_reaches_actions(){
    SendMessageW(playlist,WM_LBUTTONDOWN,0,MAKELPARAM(MainWnd_DPIScale(80),MainWnd_DPIScale(100)));SendMessageW(playlist,WM_LBUTTONUP,0,MAKELPARAM(MainWnd_DPIScale(80),MainWnd_DPIScale(100)));
    HWND edit=GetDlgItem(playlist,520);IAccessible* native=nullptr;CHECK(SUCCEEDED(AccessibleObjectFromWindow(edit,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&native))));VARIANT self{};self.vt=VT_I4;self.lVal=0;BSTR nativeName=nullptr;CHECK(native->get_accName(self,&nativeName)==S_OK&&nativeName&&*nativeName);SysFreeString(nativeName);native->Release();Key(playlist,VK_TAB);CHECK(GetFocus()==playlist); // favorite of selected row
    IAccessible* provider=nullptr;CHECK(SUCCEEDED(AccessibleObjectFromWindow(playlist,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&provider))));VARIANT focus{};CHECK(provider->get_accFocus(&focus)==S_OK&&focus.lVal>=1000);bool favorite=PlayerPlaylist().At(focus.lVal-1000)->favorite;Key(playlist,VK_SPACE);CHECK(PlayerPlaylist().At(focus.lVal-1000)->favorite!=favorite);
    VARIANT search{};search.vt=VT_I4;search.lVal=520;CHECK(provider->accSelect(SELFLAG_TAKEFOCUS,search)==S_OK&&GetFocus()==edit);Key(edit,VK_TAB);CHECK(GetFocus()==playlist);provider->Release();
}
static void topmost_owns_all_panels(){SendMessageW(player,WM_COMMAND,902,0);for(auto hwnd:{player,playlist,eq})CHECK(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TOPMOST);SendMessageW(player,WM_COMMAND,902,0);for(auto hwnd:{player,playlist,eq})CHECK(!(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_TOPMOST));}
static void CALLBACK OpenInformation(HWND,UINT,UINT_PTR id,DWORD){KillTimer(playlist,id);EndMenu();PostMessageW(playlist,WM_COMMAND,PL_COMMAND_INFORMATION,0);}
static void information_dpi_and_zoom_keep_native_controls(){
    CHECK(PlayerPlayback().PlayIndex(0));PlWnd_Refresh(true);CHECK(SetTimer(playlist,888,20,OpenInformation));
    SendMessageW(playlist,WM_CONTEXTMENU,reinterpret_cast<WPARAM>(playlist),MAKELPARAM(-1,-1));
    DWORD start=GetTickCount();while(!information&&GetTickCount()-start<2000){MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}EnumThreadWindows(GetCurrentThreadId(),Find,0);Sleep(1);}CHECK(information);
    for(int dpi:{96,120,144,192}){Dpi(information,dpi);RECT client{};CHECK(GetClientRect(information,&client));float scale=Ui::FitScale(dpi/96.f*MainWnd_UserScaleMultiplier(),560,388,Ui::MonitorWork(information));CHECK(std::abs(client.right-int(std::lround(560*scale)))<=2);LOGFONTW font{};CHECK(GetObjectW(reinterpret_cast<HFONT>(SendDlgItemMessageW(information,801,WM_GETFONT,0,0)),sizeof(font),&font));CHECK(std::abs(font.lfHeight+int(std::lround(12*scale)))<=1);}
    auto before=Rect(information);SendMessageW(player,WM_COMMAND,IDM_SCALE_125,0);auto after=Rect(information);CHECK(after.right-after.left>before.right-before.left);
    DestroyWindow(information);information=nullptr;SendMessageW(player,WM_COMMAND,IDM_SCALE_100,0);
}
static void independent_dpi_and_open_appearance(){
    Skin::Attachment(Skin::Equalizer)={};Skin::Attachment(Skin::Playlist)={};
    Dpi(eq,192);auto before=Rect(eq);Dpi(player,120);auto after=Rect(eq);CHECK(after.right-after.left==before.right-before.left);
    Ui::ShowThemes(player);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(themes);Dpi(themes,96);before=Rect(themes);Dpi(themes,192);after=Rect(themes);float expected=Ui::FitScale(2*MainWnd_UserScaleMultiplier(),520,430,Ui::MonitorWork(themes));CHECK(after.right-after.left==int(520*expected+.5f));CHECK(after.right-after.left>before.right-before.left);DestroyWindow(themes);
}
static void zoom_repaints_old_equalizer_pixels(){
    // The resized strips alone have a full bounding box but omit the old interior.
    // Verify the update region itself, including pixels already painted at 100%.
    SendMessageW(player,WM_COMMAND,IDM_SCALE_100,0);
    ValidateRect(eq,nullptr);
    SendMessageW(player,WM_COMMAND,IDM_SCALE_125,0);
    HRGN dirty=CreateRectRgn(0,0,0,0);CHECK(dirty);
    CHECK(GetUpdateRgn(eq,dirty,FALSE)!=ERROR);
    CHECK(PtInRegion(dirty,30,30));CHECK(PtInRegion(dirty,100,100));
    RECT client{};CHECK(GetClientRect(eq,&client));
    CHECK(PtInRegion(dirty,client.right-2,client.bottom-2));DeleteObject(dirty);
    SendMessageW(player,WM_COMMAND,IDM_SCALE_100,0);
}
static void accessible_names_roles_actions_and_lifetime(){
    for(auto hwnd:{player,playlist,eq}){IAccessible* accessible=nullptr;CHECK(SUCCEEDED(AccessibleObjectFromWindow(hwnd,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&accessible))));LONG count=0;CHECK(SUCCEEDED(accessible->get_accChildCount(&count))&&count>0);
        VARIANT self{},child{};self.vt=VT_I4;self.lVal=0;CHECK(accessible->accNavigate(NAVDIR_FIRSTCHILD,self,&child)==S_OK);BSTR name=nullptr;CHECK(accessible->get_accName(child,&name)==S_OK&&name&&*name);SysFreeString(name);VARIANT role{};CHECK(accessible->get_accRole(child,&role)==S_OK&&role.vt==VT_I4);LONG x,y,w,h;CHECK(accessible->accLocation(&x,&y,&w,&h,child)==S_OK&&w>0&&h>0);IEnumVARIANT* enumeration=nullptr;CHECK(SUCCEEDED(accessible->QueryInterface(IID_IEnumVARIANT,reinterpret_cast<void**>(&enumeration))));ULONG fetched=0;VARIANT item{};CHECK(enumeration->Next(1,&item,&fetched)==S_OK&&fetched==1);CHECK(accessible->get_accName(item,&name)==S_OK);SysFreeString(name);enumeration->Release();accessible->Release();}
    IAccessible* modes=nullptr;CHECK(SUCCEEDED(AccessibleObjectFromWindow(player,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&modes))));VARIANT fft{};fft.vt=VT_I4;fft.lVal=13;CHECK(modes->accDoDefaultAction(fft)==S_OK);VARIANT checked{};CHECK(modes->get_accState(fft,&checked)==S_OK&&(checked.lVal&STATE_SYSTEM_CHECKED));fft.lVal=14;CHECK(modes->get_accState(fft,&checked)==S_OK&&!(checked.lVal&STATE_SYSTEM_CHECKED));modes->Release();
    IAccessible* playerProvider=nullptr;CHECK(SUCCEEDED(AccessibleObjectFromWindow(player,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&playerProvider))));VARIANT volume{};volume.vt=VT_I4;volume.lVal=9;BSTR value=SysAllocString(L"25");CHECK(playerProvider->put_accValue(volume,value)==S_OK);SysFreeString(value);value=SysAllocString(L"NaN");CHECK(playerProvider->put_accValue(volume,value)==E_INVALIDARG);SysFreeString(value);CHECK(playerProvider->get_accValue(volume,&value)==S_OK&&!wcscmp(value,L"25"));SysFreeString(value);playerProvider->Release();
    IAccessible* provider=nullptr;CHECK(SUCCEEDED(AccessibleObjectFromWindow(eq,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&provider))));VARIANT first{};first.vt=VT_I4;first.lVal=1;VARIANT band{};band.vt=VT_I4;band.lVal=9;BSTR gain=SysAllocString(L"2");CHECK(provider->put_accValue(band,gain)==S_OK);CHECK(Eq::State().gains[4]==2&&Eq::State().gains[5]==0);SysFreeString(gain);
    bool enabled=Eq::State().enabled;CHECK(provider->accDoDefaultAction(first)==S_OK);CHECK(Eq::State().enabled!=enabled);DestroyWindow(eq);LONG count=0;CHECK(provider->get_accChildCount(&count)==CO_E_OBJNOTCONNECTED);provider->Release();
}
static void ipc_is_bounded_and_preserves_files(){auto files=Runtime::Encode({FullPath(L"tests\\fixtures\\native-44100.wav")});COPYDATASTRUCT data{Runtime::Packet,DWORD(files.size()*2),files.data()};int before=PlayerPlaylist().Count();CHECK(SendMessageW(player,WM_COPYDATA,0,reinterpret_cast<LPARAM>(&data)));CHECK(PlayerPlaylist().Count()==before+1);data.cbData=3;CHECK(!SendMessageW(player,WM_COPYDATA,0,reinterpret_cast<LPARAM>(&data)));CHECK(PlayerPlaylist().Count()==before+1);}
static int RunTests(int,char**){SetProcessDPIAware();CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Ui::ThemeConfigure(nullptr);I18n::InitializeLanguage(nullptr);auto instance=GetModuleHandleW(nullptr);CHECK(MainWnd_RegisterClass(instance));player=MainWnd_Create(instance,SW_HIDE);CHECK(player&&PlWnd_Create(instance,player));EqWnd_Toggle(player);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(playlist&&eq);int cases=0;CASE(shortcuts_and_media_on_three_panels);CASE(topmost_owns_all_panels);CASE(hotkey_conflict_rolls_back_and_releases);CASE(playlist_keyboard_reaches_actions);CASE(information_dpi_and_zoom_keep_native_controls);CASE(independent_dpi_and_open_appearance);CASE(zoom_repaints_old_equalizer_pixels);CASE(ipc_is_bounded_and_preserves_files);CASE(accessible_names_roles_actions_and_lifetime);DestroyWindow(player);CoUninitialize();printf("%d remaining Windows integration cases\n",cases);return 0;}

int main(int argc,char** argv){return OnIsolatedDesktop(RunTests,argc,argv);}
