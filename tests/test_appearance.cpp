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
    CHECK(Ui::SaveAppearanceValue(L"VisualizerVisible",0));
    CHECK(Ui::LoadAppearance().visualizerVisible);
}
static void preferences_roundtrip_preserves_theme() {
    DeleteFileW(ini.c_str());Ui::ThemeConfigure(ini.c_str());
    CHECK(Ui::SelectTheme(L"cyberpunk"));
    CHECK(Ui::SaveAppearanceValue(L"VisualizerVisible",0));
    CHECK(Ui::SaveAppearanceValue(L"PlaylistVisible",0));
    CHECK(Ui::SaveAppearanceValue(L"EqualizerVisible",1));
    CHECK(Ui::SaveAppearanceValue(L"Spectrum",1));
    CHECK(Ui::SaveAppearanceValue(L"ScalePercent",125));
    Ui::ThemeConfigure(nullptr);Ui::ThemeConfigure(ini.c_str());
    auto state=Ui::LoadAppearance();
    CHECK(!state.visualizerVisible && !state.playlistVisible && state.equalizerVisible);
    CHECK(state.spectrum && state.scalePercent==125);
    CHECK(!wcscmp(Ui::ActiveTheme().id,L"cyberpunk"));
}
static void invalid_values_use_defaults() {
    for(auto key:{L"VisualizerVisible",L"PlaylistVisible",L"EqualizerVisible",L"Spectrum",L"ScalePercent"}) {
        for(auto value:{L"garbage",L"-1",L"999999999999999999999",L""}) {
            CHECK(WritePrivateProfileStringW(L"Appearance",key,value,ini.c_str()));
            auto state=Ui::LoadAppearance();
            if(!wcscmp(key,L"VisualizerVisible")) CHECK(state.visualizerVisible);
            if(!wcscmp(key,L"PlaylistVisible")) CHECK(state.playlistVisible);
            if(!wcscmp(key,L"EqualizerVisible")) CHECK(!state.equalizerVisible);
            if(!wcscmp(key,L"Spectrum")) CHECK(!state.spectrum);
            if(!wcscmp(key,L"ScalePercent")) CHECK(state.scalePercent==0);
        }
    }
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
    Ui::ThemeConfigure(nullptr);DeleteFileW(ini.c_str());
    std::printf("%d appearance cases\n",cases);
}
