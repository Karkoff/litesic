#include "ui_theme.h"
#include "m3u.h"
#include "check.h"
#include <string>
static std::wstring ini;
static void defaults_without_settings() {
    Ui::ThemeConfigure(nullptr);
    auto state=Ui::LoadAppearance();
    CHECK(state.visualizerVisible && state.playlistVisible && !state.equalizerVisible);
    CHECK(!state.spectrum && state.scalePercent==0);
    CHECK(state.volumePermille==800);
    CHECK(!state.playlistFavoritesOnly);
    CHECK(Ui::SaveAppearanceValue(L"VisualizerVisible",0));
    CHECK(Ui::LoadAppearance().visualizerVisible);
}
static void preferences_roundtrip_preserves_theme() {
    DeleteFileW(ini.c_str());Ui::ThemeConfigure(ini.c_str());
    CHECK(Ui::SelectTheme(L"cyberpunk"));
    CHECK(Ui::SaveAppearanceValue(L"VisualizerVisible",0));
    CHECK(Ui::SaveAppearanceValue(L"PlaylistVisible",0));
    CHECK(Ui::SaveAppearanceValue(L"PlaylistFavoritesOnly",1));
    CHECK(Ui::SaveAppearanceValue(L"EqualizerVisible",1));
    CHECK(Ui::SaveAppearanceValue(L"Spectrum",1));
    CHECK(Ui::SaveAppearanceValue(L"ScalePercent",125));
    CHECK(Ui::SaveAppearanceValue(L"VolumePermille",310));
    Ui::ThemeConfigure(nullptr);Ui::ThemeConfigure(ini.c_str());
    auto state=Ui::LoadAppearance();
    CHECK(!state.visualizerVisible && !state.playlistVisible && state.equalizerVisible);
    CHECK(state.spectrum && state.scalePercent==125);
    CHECK(state.volumePermille==310);
    CHECK(state.playlistFavoritesOnly);
    CHECK(Ui::SaveAppearanceValue(L"PlaylistFavoritesOnly",0));
    CHECK(!Ui::LoadAppearance().playlistFavoritesOnly);
    for(int volume:{0,1,310,999,1000}){
        CHECK(Ui::SaveAppearanceValue(L"VolumePermille",volume));
        CHECK(Ui::LoadAppearance().volumePermille==volume);
    }
    CHECK(!wcscmp(Ui::ActiveTheme().id,L"cyberpunk"));
}
static void invalid_values_use_defaults() {
    for(auto key:{L"VisualizerVisible",L"PlaylistVisible",L"PlaylistFavoritesOnly",L"EqualizerVisible",L"Spectrum",L"ScalePercent",L"VolumePermille"}) {
        for(auto value:{L"garbage",L"-1",L"999999999999999999999",L""}) {
            CHECK(WritePrivateProfileStringW(L"Appearance",key,value,ini.c_str()));
            auto state=Ui::LoadAppearance();
            if(!wcscmp(key,L"VisualizerVisible")) CHECK(state.visualizerVisible);
            if(!wcscmp(key,L"PlaylistVisible")) CHECK(state.playlistVisible);
            if(!wcscmp(key,L"PlaylistFavoritesOnly")) CHECK(!state.playlistFavoritesOnly);
            if(!wcscmp(key,L"EqualizerVisible")) CHECK(!state.equalizerVisible);
            if(!wcscmp(key,L"Spectrum")) CHECK(!state.spectrum);
            if(!wcscmp(key,L"ScalePercent")) CHECK(state.scalePercent==0);
            if(!wcscmp(key,L"VolumePermille")) CHECK(state.volumePermille==800);
        }
    }
}
static void equalizer_position_roundtrip_and_validation(){
    Ui::ThemeConfigure(nullptr);Ui::PanelPosition actual;
    CHECK(!Ui::LoadEqualizerPosition(actual));CHECK(Ui::SaveEqualizerPosition({-200,300,4,2}));
    CHECK(!Ui::LoadEqualizerPosition(actual));Ui::ThemeConfigure(ini.c_str());
    for(int side=0;side<=4;++side)for(int anchor=0;anchor<=2;++anchor){
        CHECK(Ui::SaveEqualizerPosition({-200,300,side,anchor}));
        Ui::ThemeConfigure(ini.c_str());CHECK(Ui::LoadEqualizerPosition(actual));
        CHECK(actual.left==-200&&actual.top==300&&actual.side==side&&actual.anchor==anchor);
    }
    CHECK(Ui::SaveEqualizerPosition({-200,300,4,2,1}));CHECK(Ui::LoadEqualizerPosition(actual));CHECK(actual.parent==1);
    CHECK(WritePrivateProfileStringW(L"EqualizerWindow",L"Parent",nullptr,ini.c_str()));
    CHECK(Ui::LoadEqualizerPosition(actual));CHECK(actual.parent==0); // Legacy INI attaches to player.
    CHECK(WritePrivateProfileStringW(L"EqualizerWindow",L"Parent",L"2",ini.c_str()));CHECK(!Ui::LoadEqualizerPosition(actual));
    for(auto key:{L"Version",L"Left",L"Top",L"Dock",L"Anchor"})for(auto value:{L"garbage",L"999999999999999999999",L""}){
        CHECK(Ui::SaveEqualizerPosition({-200,300,4,2}));
        CHECK(WritePrivateProfileStringW(L"EqualizerWindow",key,value,ini.c_str()));
        CHECK(!Ui::LoadEqualizerPosition(actual));
    }
    CHECK(!wcscmp(Ui::ActiveTheme().id,L"cyberpunk"));
}
int main() {
    std::wstring variant=std::to_wstring(sizeof(void*)*8);
#ifdef DEBUG
    variant+=L"-debug";
#else
    variant+=L"-release";
#endif
    ini=FullPath(L"build\\appearance-tests-"+variant+L".ini");int cases=0;
    CASE(defaults_without_settings);CASE(preferences_roundtrip_preserves_theme);CASE(invalid_values_use_defaults);
    CASE(equalizer_position_roundtrip_and_validation);
    Ui::ThemeConfigure(nullptr);DeleteFileW(ini.c_str());
    std::printf("%d appearance cases\n",cases);
}
