#include "i18n.h"
#include "ui_draw.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "eq_state.h"
#include "playback.h"
#include "audio.h"
#include "m3u.h"
#include "check.h"
#include <cstring>
static void all_scripts_themes_and_scales_render() {
    HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    HBITMAP bitmap=CreateCompatibleBitmap(screen,1040,600);ReleaseDC(nullptr,screen);auto old=SelectObject(dc,bitmap);
    Ui::Canvas canvas;std::string error;
    for(auto& language:I18n::Languages()) {
        CHECK(I18n::Select(language.code,error));
        for(size_t t=0;t<Ui::ThemeCount();++t) {
            CHECK(Ui::SelectTheme(Ui::ThemeAt(t).id,false));
            for(float scale:{1.f,1.25f,1.5f,2.f}) {
                RECT r{0,0,int(460*scale),int(290*scale)};CHECK(canvas.Begin(dc,r,scale));
                canvas.Fill({0,0,460,290},Ui::Background);
                canvas.TextAt({12,0,376,32},I18n::Text("playlist.title"),Ui::Text,Ui::Font::Heading);
                canvas.TextAt({12,38,436,30},I18n::Text("player.welcome"));
                canvas.TextAt({12,74,436,30},I18n::Count("playlist.tracks",112));
                canvas.TextAt({12,110,436,30},I18n::Format("info.field",{{"label",I18n::Text("info.file")},{"value",L"21 - é 🎵 العربية हिन्दी.mp3"}}));
                canvas.TextAt({12,150,52,18},I18n::Text("equalizer.reset")); // deliberate overflow exercises trimming
                CHECK(canvas.End());
            }
        }
    }
    canvas.Reset();SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
    CHECK(I18n::Select("en-US",error));CHECK(Ui::SelectTheme(L"original",false));
}
static HWND playlist,eq;
static BOOL CALLBACK Find(HWND hwnd,LPARAM){wchar_t cls[80]{};GetClassNameW(hwnd,cls,80);
    if(!wcscmp(cls,L"LiteSic.PlaylistWnd"))playlist=hwnd;
    if(!wcscmp(cls,L"LiteSic.EqualizerWnd"))eq=hwnd;
    return TRUE;}
static RECT Rect(HWND h){RECT r{};CHECK(GetWindowRect(h,&r));return r;}
static void programmatic_catalog_changes_preserve_functional_state() {
    auto instance=GetModuleHandleW(nullptr);CHECK(MainWnd_RegisterClass(instance));Ui::ThemeConfigure(nullptr);Eq::Configure(nullptr);
    HWND main=MainWnd_Create(instance,SW_SHOWNOACTIVATE);CHECK(main);CHECK(PlWnd_Create(instance,main,nullptr));
    EqWnd_Toggle(main);EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(playlist&&eq);
    PlayerPlaylist().Clear();auto path=FullPath(L"tests\\fixtures\\native-44100.wav");
    PlayerPlaylist().Add(path);PlayerPlaylist().At(0)->displayTitle=L"🎵 é हिन्दी العربية";
    Audio_Shutdown();CHECK(Audio_TestInit(48000));CHECK(PlayerPlayback().PlayIndex(0));Eq::ApplyPreset(1);Eq::SetEnabled(true);
    auto p=Rect(main),pl=Rect(playlist),e=Rect(eq);auto id=PlayerPlaylist().At(0)->id;auto state=Eq::State();
    std::string error;float samples[960]{};auto initialCursor=Audio_TestCursor();
    for(auto& language:I18n::Languages()) {
        auto cursor=Audio_TestCursor();CHECK(I18n::Select(language.code,error));
        // T7 will issue refresh notifications. This test exercises the prepared drawing paths.
        for(HWND h:{main,playlist,eq})RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_UPDATENOW);
        CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);CHECK(Audio_TestCursor()==cursor);
        CHECK(Audio_TestRead(samples,480));CHECK(Audio_TestCursor()>=cursor);
        CHECK(PlayerPlaylist().Count()==1&&PlayerPlaylist().At(0)->id==id);
        CHECK(PlayerPlaylist().At(0)->path==path&&PlayerPlaylist().At(0)->displayTitle==L"🎵 é हिन्दी العربية");
        CHECK(Eq::State().enabled==state.enabled&&Eq::State().preset==state.preset&&Eq::State().preamp==state.preamp);
        for(unsigned b=0;b<Eq::Bands;++b)CHECK(Eq::State().gains[b]==state.gains[b]);
        auto actual=Rect(main);CHECK(EqualRect(&p,&actual));actual=Rect(playlist);CHECK(EqualRect(&pl,&actual));actual=Rect(eq);CHECK(EqualRect(&e,&actual));
    }
    CHECK(Audio_TestCursor()>initialCursor);
    Audio_Shutdown();DestroyWindow(main);CHECK(I18n::Select("en-US",error));
}
int main(){int cases=0;CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    CASE(all_scripts_themes_and_scales_render);CASE(programmatic_catalog_changes_preserve_functional_state);
    CoUninitialize();std::printf("%d i18n window tests passed.\n",cases);return 0;}
