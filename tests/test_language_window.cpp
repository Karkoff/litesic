#include "ui_tooltip.h"
#include "language_selection.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "eq_state.h"
#include "playback.h"
#include "audio.h"
#include "m3u.h"
#include "skin.h"
#include "ui_draw.h"
#include "check.h"
#include <commctrl.h>
#include <cstring>
#include <chrono>
static HWND mainWindow,playlist,eq,information,themes,tooltip;
static std::vector<std::string> FakeWindows(){return {"en-US"};}
static BOOL CALLBACK Find(HWND hwnd,LPARAM){
    wchar_t cls[80]{};GetClassNameW(hwnd,cls,80);
    if(!wcscmp(cls,L"LiteSic.PlaylistWnd"))playlist=hwnd;
    if(!wcscmp(cls,L"LiteSic.EqualizerWnd"))eq=hwnd;
    if(!wcscmp(cls,L"LiteSic.TrackInfoWnd"))information=hwnd;
    if(!wcscmp(cls,L"LiteSic.ThemesWnd"))themes=hwnd;
    if(!wcscmp(cls,TOOLTIPS_CLASSW)&&GetWindow(hwnd,GW_OWNER)==mainWindow)tooltip=hwnd;
    return TRUE;
}
static RECT Rect(HWND h){RECT r{};CHECK(GetWindowRect(h,&r));return r;}
static std::wstring WindowText(HWND h){std::wstring s(GetWindowTextLengthW(h)+1,L'\0');GetWindowTextW(h,&s[0],int(s.size()));s.resize(wcslen(s.c_str()));return s;}
static void Draw(HWND h){CHECK(RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN));}
static void click(HWND h,int x,int y){auto p=MAKELPARAM(MainWnd_DPIScale(x),MainWnd_DPIScale(y));SendMessageW(h,WM_LBUTTONDOWN,MK_LBUTTON,p);SendMessageW(h,WM_LBUTTONUP,0,p);}
static void CALLBACK InfoCommand(HWND,UINT,UINT_PTR id,DWORD){KillTimer(playlist,id);EndMenu();PostMessageW(playlist,WM_COMMAND,PL_COMMAND_INFORMATION,0);}
static void Pump(){MSG m;while(PeekMessageW(&m,nullptr,0,0,PM_REMOVE)){TranslateMessage(&m);DispatchMessageW(&m);}}
static void live_change_preserves_audio_selection_search_eq_and_docking(){
    CHECK(Audio_TestInit(48000));PlayerPlaylist().Clear();
    for(int i=0;i<3;++i){PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));PlayerPlaylist().At(i)->displayTitle=L"track é 🎵 العربية हिन्दी "+std::to_wstring(i);}
    CHECK(PlayerPlayback().PlayIndex(0));Audio_SeekSeconds(.5);Audio_SetVolume(.4f);Audio_SetPan(-.2f);
    Eq::ApplyPreset(2);Eq::SetEnabled(true);auto state=Eq::State();PlWnd_Refresh();
    HWND search=GetDlgItem(playlist,520);SetWindowTextW(search,L"track");SendMessageW(search,EM_SETSEL,1,3);
    SendMessageW(playlist,WM_KEYDOWN,VK_HOME,0);SendMessageW(playlist,WM_KEYDOWN,VK_DOWN,0);
    auto selected=PlayerPlaylist().At(1)->id;auto tracks=PlayerPlaylist().Tracks();
    auto beforeMain=Rect(mainWindow),beforeList=Rect(playlist),beforeEq=Rect(eq);
    auto parentList=Skin::Parent(Skin::Playlist),parentEq=Skin::Parent(Skin::Equalizer);
    auto dockList=Skin::Attachment(Skin::Playlist),dockEq=Skin::Attachment(Skin::Equalizer);
    std::wstring theme=Ui::ActiveTheme().id;
    std::string error;double total=0;float samples[960]{};auto initial=Audio_TestCursor();
    DWORD warmGdi=0,warmUser=0;
    for(bool spectrum:{false,true}){
        click(mainWindow,spectrum?374:418,206);
        CHECK(Ui::LoadAppearance().scalePercent==0);
        for(auto& language:I18n::Languages()){
            auto cursor=Audio_TestCursor();auto start=std::chrono::steady_clock::now();
            CHECK(I18n::ExecuteLanguageCommand(I18n::LanguageFirstCommand+int(&language-&I18n::Languages()[0]),error));CHECK(error.empty());
            total+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
            CHECK(Audio_TestCursor()==cursor);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
            SendMessageW(mainWindow,WM_TIMER,VIZ_TIMER_ID,0);for(HWND h:{mainWindow,playlist,eq})Draw(h);
            CHECK(Audio_TestCursor()==cursor);CHECK(Audio_TestRead(samples,480));
            CHECK(PlayerPlaylist().Count()==3&&PlayerPlaylist().GetCurrentIndex()==0);
            for(int i=0;i<3;++i){auto track=PlayerPlaylist().At(i);CHECK(track->id==tracks[i].id&&track->path==tracks[i].path&&track->displayTitle==tracks[i].displayTitle);}
            CHECK(WindowText(search)==L"track");DWORD a=0,b=0;SendMessageW(search,EM_GETSEL,reinterpret_cast<WPARAM>(&a),reinterpret_cast<LPARAM>(&b));CHECK(a==1&&b==3);
            CHECK(Eq::State().enabled==state.enabled&&Eq::State().preset==state.preset&&Eq::State().preamp==state.preamp);
            for(unsigned band=0;band<Eq::Bands;++band)CHECK(Eq::State().gains[band]==state.gains[band]);
            CHECK(theme==Ui::ActiveTheme().id);auto r=Rect(mainWindow);CHECK(EqualRect(&r,&beforeMain));r=Rect(playlist);CHECK(EqualRect(&r,&beforeList));r=Rect(eq);CHECK(EqualRect(&r,&beforeEq));
            CHECK(Skin::Parent(Skin::Playlist)==parentList&&Skin::Parent(Skin::Equalizer)==parentEq);
            CHECK(Skin::Attachment(Skin::Playlist).side==dockList.side&&Skin::Attachment(Skin::Playlist).anchor==dockList.anchor);
            CHECK(Skin::Attachment(Skin::Equalizer).side==dockEq.side&&Skin::Attachment(Skin::Equalizer).anchor==dockEq.anchor);
            CHECK(WindowText(playlist)==I18n::Format("window.caption",{{"title",I18n::Text("playlist.title")}}));
            CHECK(WindowText(eq)==I18n::Format("window.caption",{{"title",I18n::Text("equalizer.title")}}));
            CHECK(bool(GetWindowLongPtrW(search,GWL_EXSTYLE)&WS_EX_RTLREADING)==I18n::IsRtl());
            CHECK(Ui::Tooltips::Resolve(mainWindow,{MainWnd_DPIScale(BTN_PLAY_X+10),MainWnd_DPIScale(NAV_Y+10)}).id==5);
            CHECK(Ui::Tooltips::Description(mainWindow,5)==I18n::Text("tooltip.play"));
        }
        auto gdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),user=GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS);
        if(!spectrum){warmGdi=gdi;warmUser=user;}
        else {CHECK(gdi<=warmGdi+8);CHECK(user<=warmUser+4);
            std::printf("LANGUAGE_RESOURCES arch=%zu warm_gdi=%lu final_gdi=%lu warm_user=%lu final_user=%lu\n",sizeof(void*)*8,static_cast<unsigned long>(warmGdi),static_cast<unsigned long>(gdi),static_cast<unsigned long>(warmUser),static_cast<unsigned long>(user));}
    }
    CHECK(Audio_TestCursor()>initial);
    SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0);CHECK(PlayerPlaylist().Count()==2&&PlayerPlaylist().Find(selected)<0);
    CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
    std::printf("LANGUAGE_UI_METRICS arch=%zu mean_notify_ms=%.3f switches=36 windows=3\n",sizeof(void*)*8,total/36);
    SetWindowTextW(search,L"");Audio_Shutdown();
}
static void auxiliary_windows_tooltips_and_theme_interaction(){
    CHECK(Audio_TestInit(48000));CHECK(PlayerPlayback().PlayIndex(0));PlWnd_Refresh();
    SendMessageW(playlist,WM_KEYDOWN,VK_HOME,0);
    CHECK(SetTimer(playlist,888,20,InfoCommand));
    SendMessageW(playlist,WM_CONTEXTMENU,reinterpret_cast<WPARAM>(playlist),MAKELPARAM(-1,-1));
    auto deadline=GetTickCount()+2000;while(!information&&GetTickCount()<deadline){Pump();EnumThreadWindows(GetCurrentThreadId(),Find,0);Sleep(1);}CHECK(information);
    Ui::ShowThemes(mainWindow);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(themes);
    auto infoRect=Rect(information),themeRect=Rect(themes);auto id=PlayerPlaylist().At(0)->id;
    // Preserve category, selected preview row and scroll position, including a filtered category.
    HWND category=GetDlgItem(themes,701),list=GetDlgItem(themes,702);
    SendMessageW(category,CB_SETCURSEL,1,0);SendMessageW(themes,WM_COMMAND,MAKEWPARAM(701,CBN_SELCHANGE),reinterpret_cast<LPARAM>(category));
    auto top=SendMessageW(list,LB_GETTOPINDEX,0,0),row=SendMessageW(list,LB_GETCURSEL,0,0);
    std::string error;
    for(size_t i=0;i<I18n::Languages().size();++i){
        CHECK(I18n::ChangeLanguage(false,I18n::Languages()[i].code,error));
        auto text=WindowText(GetDlgItem(information,801));CHECK(text.find(PlayerPlaylist().At(0)->displayTitle)!=text.npos);
        CHECK(text.find(I18n::Text("info.artist"))!=text.npos);CHECK(WindowText(information)==I18n::Text("info.caption"));
        CHECK(WindowText(GetDlgItem(information,802))==I18n::Text("common.close"));
        CHECK(WindowText(themes)==I18n::Text("themes.title"));CHECK(WindowText(GetDlgItem(themes,703))==I18n::Text("themes.restore"));
        CHECK(WindowText(GetDlgItem(themes,706))==I18n::Text("themes.essential"));
        CHECK(SendMessageW(category,CB_GETCURSEL,0,0)==1&&SendMessageW(list,LB_GETTOPINDEX,0,0)==top&&SendMessageW(list,LB_GETCURSEL,0,0)==row);
        auto r=Rect(information);CHECK(EqualRect(&r,&infoRect));r=Rect(themes);CHECK(EqualRect(&r,&themeRect));
        auto cursor=Audio_TestCursor();CHECK(Ui::SelectTheme(Ui::ThemeAt(i*2).id,false));CHECK(Audio_TestCursor()==cursor);
        auto theme=std::wstring(Ui::ActiveTheme().id);CHECK(I18n::ChangeLanguage(false,I18n::Languages()[(i+1)%18].code,error));CHECK(theme==Ui::ActiveTheme().id);
        for(HWND h:{mainWindow,playlist,eq,themes,information})Draw(h);
        CHECK(PlayerPlaylist().At(0)->id==id&&Audio_GetState()==AUDIO_STATE_PLAYING);
    }
    DestroyWindow(themes);DestroyWindow(information);Audio_Shutdown();
}
int main(){int cases=0;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);auto instance=GetModuleHandleW(nullptr);
    Ui::ThemeConfigure(nullptr);I18n::InitializeLanguage(nullptr,FakeWindows);std::string startupError;
    CHECK(I18n::ChangeLanguage(false,"ar-SA",startupError));CHECK(MainWnd_RegisterClass(instance));
    mainWindow=MainWnd_Create(instance,SW_SHOWNOACTIVATE);CHECK(mainWindow);CHECK(PlWnd_Create(instance,mainWindow,nullptr));
    EqWnd_Toggle(mainWindow);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(playlist&&eq);
    CHECK(GetWindowLongPtrW(GetDlgItem(playlist,520),GWL_EXSTYLE)&WS_EX_RTLREADING);
    CHECK(GetWindowLongPtrW(GetDlgItem(playlist,520),GWL_STYLE)&ES_RIGHT);Audio_Shutdown();
    CASE(live_change_preserves_audio_selection_search_eq_and_docking);CASE(auxiliary_windows_tooltips_and_theme_interaction);
    DestroyWindow(mainWindow);CoUninitialize();std::printf("%d language window cases\n",cases);return 0;
}
