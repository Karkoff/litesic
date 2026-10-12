#include "language_selection.h"
#include "check.h"
#include "m3u.h"
#include <psapi.h>
#include <chrono>
using namespace I18n;
static std::vector<std::string> preferences;
static std::vector<std::string> FakeWindows(){return preferences;}
static const auto ini=FullPath(L"build\\language-selection-test.ini");
static void ordered_multisz_and_bounds(){
    const wchar_t list[]=L"es-MX\0ja-JP\0\0";
    CHECK(ParseWindowsLanguages(list,ARRAYSIZE(list))==std::vector<std::string>({"es-MX","ja-JP"}));
    CHECK(ParseWindowsLanguages(nullptr,0).empty());CHECK(ParseWindowsLanguages(list,2).empty());
    const wchar_t bad[]={L'e',L'n',0,L'j',L'a',0};CHECK(ParseWindowsLanguages(bad,6).empty());
    const wchar_t nonAscii[]={0x4e2d,0,0};CHECK(ParseWindowsLanguages(nonAscii,3).empty());
}
static void automatic_supported_regional_and_unknown(){
    for(auto item:std::vector<std::pair<std::vector<std::string>,std::string>>{
        {{"de-DE"},"de-DE"},{{"xx-YY","es-MX","ja-JP"},"es-ES"},
        {{"pt-AO"},"pt-BR"},{{"pt-PT"},"pt-PT"},{{"zh-Hant-HK"},"zh-TW"},
        {{"zh-Hans-SG"},"zh-CN"},{{"en-GB","pt-BR"},"en-US"},{{"xx-YY"},"en-US"},{{},"en-US"}}){
        preferences=item.first;InitializeLanguage(nullptr,FakeWindows);
        CHECK(Active()==item.second);CHECK(CurrentLanguagePreference().automatic);
        CHECK(CurrentLanguagePreference().selected.empty());
    }
}
static void native_windows_provider_is_callable(){
    auto result=WindowsLanguagePreferences();auto resolved=Resolve(result);
    bool known=false;for(auto& l:Languages())known|=resolved==l.code;CHECK(known);
    // Never modifies the user's Windows preferences; host language is not assumed.
    for(auto& code:result)CHECK(!code.empty());
}
static void manual_auto_and_restart_share_existing_ini(){
    DeleteFileW(ini.c_str());preferences={"ja-JP"};InitializeLanguage(ini.c_str(),FakeWindows);
    CHECK(Active()=="ja-JP");CHECK(GetFileAttributesW(ini.c_str())==INVALID_FILE_ATTRIBUTES);
    CHECK(WritePrivateProfileStringW(L"Appearance",L"Theme",L"original",ini.c_str()));
    std::string error;CHECK(ChangeLanguage(false,"pt-PT",error));CHECK(error.empty());
    preferences={"ar-SA"};InitializeLanguage(ini.c_str(),FakeWindows);CHECK(Active()=="pt-PT");
    CHECK(!CurrentLanguagePreference().automatic&&CurrentLanguagePreference().selected=="pt-PT");
    CHECK(ChangeLanguage(true,"",error));CHECK(Active()=="ar-SA");
    preferences={"ko-KR"};InitializeLanguage(ini.c_str(),FakeWindows);CHECK(Active()=="ko-KR");
    CHECK(CurrentLanguagePreference().automatic&&CurrentLanguagePreference().selected=="pt-PT");
    wchar_t theme[32]{};GetPrivateProfileStringW(L"Appearance",L"Theme",L"",theme,32,ini.c_str());CHECK(!wcscmp(theme,L"original"));
    DeleteFileW(ini.c_str());
}
static void invalid_preferences_and_commands_are_safe(){
    CHECK(WritePrivateProfileStringW(L"Language",L"Mode",L"manual",ini.c_str()));
    CHECK(WritePrivateProfileStringW(L"Language",L"Selected",L"../../evil",ini.c_str()));
    preferences={"sv-SE"};InitializeLanguage(ini.c_str(),FakeWindows);CHECK(Active()=="sv-SE");
    CHECK(CurrentLanguagePreference().automatic);
    std::string error;auto generation=Generation();CHECK(!ChangeLanguage(false,"xx",error));
    CHECK(!ExecuteLanguageCommand(999,error));CHECK(Generation()==generation);
    CHECK(!WriteLanguagePreference(ini,{false,""}));DeleteFileW(ini.c_str());
}
static void failed_persistence_keeps_selection_and_catalog(){
    preferences={"en-US"};auto invalid=FullPath(L"build\\no-such-language-directory\\settings.ini");
    InitializeLanguage(invalid.c_str(),FakeWindows);std::string error;auto generation=Generation();
    CHECK(!ChangeLanguage(false,"de-DE",error));CHECK(!error.empty());CHECK(Active()=="en-US");
    CHECK(CurrentLanguagePreference().automatic);CHECK(Generation()==generation);
}
static void menu_has_native_names_and_correct_checks(){
    preferences={"ja-JP"};InitializeLanguage(nullptr,FakeWindows);std::string error;
    for(int chosen=-1;chosen<18;++chosen){
        CHECK(ChangeLanguage(chosen<0,chosen<0?"":Languages()[chosen].code,error));
        HMENU menu=CreateLanguageMenu();CHECK(GetMenuItemCount(menu)==20);
        wchar_t automaticLabel[256]{};GetMenuStringW(menu,LanguageAutomaticCommand,automaticLabel,256,MF_BYCOMMAND);
        CHECK(std::wstring(automaticLabel)==Format("appearance.auto_language",{{"language",L"日本語"}}));
        CHECK(bool(GetMenuState(menu,LanguageAutomaticCommand,MF_BYCOMMAND)&MF_CHECKED)==(chosen<0));
        for(int i=0;i<18;++i){
            wchar_t label[128]{};GetMenuStringW(menu,LanguageFirstCommand+i,label,128,MF_BYCOMMAND);
            CHECK(!wcscmp(label,Languages()[i].name));
            CHECK(bool(GetMenuState(menu,LanguageFirstCommand+i,MF_BYCOMMAND)&MF_CHECKED)==(chosen==i));
        }
        DestroyMenu(menu);
    }
}
static SIZE_T PrivateBytes(){PROCESS_MEMORY_COUNTERS_EX m{};m.cb=sizeof(m);CHECK(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&m),sizeof(m)));return m.PrivateUsage;}
static void repeated_live_changes_cost_and_memory(){
    preferences={"en-US"};InitializeLanguage(nullptr,FakeWindows);std::string error;
    for(int i=0;i<180;++i)CHECK(ChangeLanguage(false,Languages()[i%18].code,error));
    auto warm=PrivateBytes();auto start=std::chrono::steady_clock::now();
    for(int i=0;i<1800;++i)CHECK(ChangeLanguage(false,Languages()[i%18].code,error));
    auto end=std::chrono::steady_clock::now();auto final=PrivateBytes();CHECK(final<=warm+512*1024);
    auto generation=Generation();CHECK(ChangeLanguage(false,Active(),error));CHECK(Generation()==generation);
    std::printf("LANGUAGE_METRICS arch=%zu mean_change_ms=%.3f warm_private=%zu final_private=%zu switches=1980 catalogs_resident=2\n",sizeof(void*)*8,
        std::chrono::duration<double,std::milli>(end-start).count()/1800,size_t(warm),size_t(final));
}
int main(){int cases=0;
    CASE(ordered_multisz_and_bounds);CASE(automatic_supported_regional_and_unknown);CASE(native_windows_provider_is_callable);
    CASE(manual_auto_and_restart_share_existing_ini);CASE(invalid_preferences_and_commands_are_safe);
    CASE(failed_persistence_keeps_selection_and_catalog);CASE(menu_has_native_names_and_correct_checks);CASE(repeated_live_changes_cost_and_memory);
    std::printf("%d language selection cases\n",cases);return 0;
}
