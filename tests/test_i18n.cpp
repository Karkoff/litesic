#include "i18n.h"
#include "check.h"
#include "heap_usage.h"
#include <windows.h>
#include <psapi.h>
#include <chrono>
#include <cstdio>
#include <set>
#include <cstring>
using namespace I18n;
static const auto root=ExecutableLocales();
static void catalogs_are_complete_and_unicode() {
    Service service;std::string error;CHECK(Languages().size()==18);
    CHECK(I18n::Active()=="pt-BR");CHECK(I18n::Text("info.artist")==L"Artista");
    for(auto& l:Languages()) {
        Catalog catalog;std::wstring code(l.code,l.code+strlen(l.code));
        CHECK(Read(root+L"\\"+code+L"\\messages.tsv",catalog,error));
        CHECK(Validate(catalog,service.Fallback(),l.code,true,error));
        CHECK(catalog.size()==241);CHECK(service.Install(l.code,std::move(catalog),true,error));
        CHECK(service.Active()==l.code);CHECK(!service.Text("playlist.title").empty());
    }
    CHECK(service.Select("ja-JP",root,error));CHECK(service.Text("player.play")==L"再生");
    CHECK(service.Select("zh-CN",root,error));CHECK(service.Text("info.file")==L"文件");
    CHECK(service.Select("zh-TW",root,error));CHECK(service.Text("info.file")==L"檔案");
    CHECK(service.Select("hi-IN",root,error));CHECK(service.Text("player.play")==L"चलाएँ");
}
static void utf8_rejects_invalid_and_preserves_supplementary() {
    std::wstring result;
    for(auto bytes:{"\xc0\xaf","\xed\xa0\x80","\xf4\x90\x80\x80","\xe2\x82","\xff"})CHECK(!DecodeUtf8(bytes,result));
    CHECK(DecodeUtf8(u8"A é 🎵 العربية हिन्दी",result));
    CHECK(result.find(L"e\x301")!=result.npos);CHECK(result.find(L"\xd83c\xdfb5")!=result.npos);
    CHECK(!DecodeUtf8(std::string(1024*1024+1,'a'),result));
}
static void parser_rejects_structural_errors_transactionally() {
    Catalog c;std::string error;
    for(auto bytes:{"x\tOne\nx\tTwo\n","x\t\n","x\t  \n","Invalid\tX\n","x X\n","x\t{broken\n","x\t{}\n","x\t\\q\n","x\tX\tY\n","# empty"}) {
        c["old"]=L"old";CHECK(!Parse(bytes,c,error));CHECK(!error.empty());CHECK(c.empty());
    }
    CHECK(!Parse(std::string("x\tA\0B\n",7),c,error));
    CHECK(Parse("\xef\xbb\xbf# comment\r\nx\tline 1\\nline 2\r\n",c,error));
    CHECK(c.at("x")==L"line 1\r\nline 2");
}
static void fallback_and_partial_catalog() {
    Service s;std::string error;Catalog partial{{"player.play",L"Jouer"}};
    CHECK(s.Install("fr-FR",partial,false,error));CHECK(s.Text("player.play")==L"Jouer");
    CHECK(s.Text("player.pause")==L"Pause");CHECK(s.Text("missing.id")==L"Text unavailable");
    CHECK(s.Count("playlist.tracks",2)==L"2 tracks");
    auto generation=s.Generation();CHECK(!s.Install("fr-FR",partial,true,error));CHECK(s.Generation()==generation);
    CHECK(!s.Install("bad",partial,false,error));CHECK(s.Active()=="fr-FR");
}
static void placeholders_are_checked_and_not_recursively_expanded() {
    Service s;std::string error;std::wstring result;
    CHECK(s.Format("info.field",{{"label",L"Title"},{"value",L"{label} 🎵"}},result,error));
    CHECK(result==L"Title: {label} 🎵");
    CHECK(!s.Format("info.field",{{"label",L"Title"}},result,error));
    CHECK(!s.Format("info.field",{{"label",L"Title"},{"wrong",L"data"}},result,error));
    CHECK(!s.Install("de-DE",{{"info.field",L"{label}: {bad}"}},false,error));
    CHECK(!s.Install("de-DE",{{"unknown",L"text"}},false,error));
    CHECK(!s.Install("de-DE",{{"player.play",std::wstring(1,wchar_t(0xd800))}},false,error));
    CHECK(!s.Install("de-DE",{{"player.play",L"   "}},false,error));
}
static void ordered_resolution_and_regional_variants() {
    for(auto pair:{std::pair<const char*,const char*>{"es-MX","es-ES"},{"pt-AO","pt-BR"},{"pt-PT","pt-PT"},
        {"EN_us","en-US"},{"zh-Hans","zh-CN"},{"zh-Hant","zh-TW"},{"zh-HK","zh-TW"},{"zh-SG","zh-CN"},
        {"ar-EG","ar-SA"},{"fr-CA","fr-FR"},{"zz-ZZ","en-US"},{"","en-US"}})CHECK(Resolve({pair.first})==pair.second);
    CHECK(Resolve({"unknown","de-AT","ja-JP"})=="de-DE");
    CHECK(Resolve({"unknown","en-US","pt-BR"})=="en-US");
    CHECK(Resolve({"zh-Latn","ru-RU"})=="ru-RU");
    CHECK(Resolve({"../../ru-RU","ja-JP"})=="ja-JP");
    CHECK(Resolve({"en-Arab","ru-RU"})=="ru-RU");
    CHECK(Resolve({"pt-../../bad","de-DE"})=="de-DE");
}
static void integer_plural_boundaries() {
    CHECK(PluralCategory("en-US",0)=="other");CHECK(PluralCategory("en-US",1)=="one");
    CHECK(PluralCategory("pt-BR",0)=="one");CHECK(PluralCategory("pt-PT",0)=="other");
    CHECK(PluralCategory("fr-FR",1000000)=="many");CHECK(PluralCategory("ja-JP",1)=="other");
    CHECK(PluralCategory("tr-TR",1)=="one");CHECK(PluralCategory("hi-IN",0)=="one");
    CHECK(PluralCategory("ru-RU",21)=="one");CHECK(PluralCategory("ru-RU",11)=="many");
    CHECK(PluralCategory("ru-RU",22)=="few");CHECK(PluralCategory("ru-RU",112)=="many");
    CHECK(PluralCategory("pl-PL",21)=="many");CHECK(PluralCategory("pl-PL",2)=="few");
    for(auto pair:{std::pair<uint64_t,const char*>{0,"zero"},{1,"one"},{2,"two"},{3,"few"},{10,"few"},{11,"many"},{99,"many"},{100,"other"},{102,"other"},{103,"few"}})
        CHECK(PluralCategory("ar-SA",pair.first)==pair.second);
    Service s;std::string error;CHECK(s.Select("ru-RU",root,error));CHECK(s.Count("playlist.tracks",21)==L"21 трек");
    CHECK(s.Select("pl-PL",root,error));CHECK(s.Count("playlist.tracks",12)==L"12 utworów");
    Catalog broken=s.Fallback();broken.erase("playlist.tracks.few");CHECK(!s.Install("pl-PL",broken,false,error));
}
static void arabic_composition_embeds_external_values() {
    Service s;std::string error;CHECK(s.Select("ar-SA",root,error));std::wstring result;
    CHECK(s.Format("info.field",{{"label",s.Text("info.file")},{"value",L"C:\\Music\\أغنية-21.mp3"}},result,error));
    CHECK(result.find(L"\x202a" L"C:\\Music\\أغنية-21.mp3\x202c")!=result.npos);
    CHECK(Languages()[15].rtl);
}
static void io_errors_preserve_active_catalog_and_unload() {
    Service s;std::string error;CHECK(s.Select("de-DE",root,error));auto before=s.Text("player.play");
    CHECK(!s.Select("ru-RU",L"nonexistent",error));CHECK(s.Text("player.play")==before);
    s.Unload();CHECK(s.Active()=="en-US");CHECK(s.Text("player.play")==L"Play");
    CHECK(s.Select("unsupported",L"",error));CHECK(s.Active()=="en-US");
}
static void executable_relative_loading_ignores_working_directory() {
    wchar_t original[32768]{};CHECK(GetCurrentDirectoryW(32768,original));
    wchar_t system[32768]{};CHECK(GetSystemDirectoryW(system,32768));CHECK(SetCurrentDirectoryW(system));
    Service s;std::string error;CHECK(s.Select("hi-IN",ExecutableLocales(),error));CHECK(s.Active()=="hi-IN");
    CHECK(SetCurrentDirectoryW(original));
}
static SIZE_T PrivateBytes(){PROCESS_MEMORY_COUNTERS_EX p{};p.cb=sizeof(p);CHECK(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&p),sizeof(p)));return p.PrivateUsage;}
static void repeated_switches_release_catalogs_and_measure_cost() {
    using Clock=std::chrono::steady_clock;
    auto baseline=PrivateBytes();auto start=Clock::now();Service s;
    auto initialized=Clock::now();auto fallbackMemory=PrivateBytes();std::string error;size_t maxUnits=0;
    double totalMs=0;
    for(auto& l:Languages()) {
        auto begin=Clock::now();CHECK(s.Select(l.code,root,error));
        totalMs+=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();
        maxUnits=std::max(maxUnits,s.RetainedUnits());
    }
    for(int i=0;i<180;++i)CHECK(s.Select(Languages()[i%18].code,root,error));
    SIZE_T warm=PrivateBytes(),warmLive=LiveHeapBytes();
    for(int i=0;i<1800;++i){CHECK(s.Select(Languages()[i%18].code,root,error));CHECK(s.RetainedUnits()<=maxUnits);}
    SIZE_T final=PrivateBytes(),finalLive=LiveHeapBytes();
    std::printf("I18N_LIVE_HEAP warm=%zu final=%zu limit=524288\n",size_t(warmLive),size_t(finalLive));
    CHECK(finalLive<=warmLive+512*1024);s.Unload();CHECK(s.RetainedUnits()<maxUnits);
    std::printf("I18N_METRICS arch=%zu init_ms=%.3f mean_select_ms=%.3f fallback_private_delta=%lld warm_private=%zu final_private=%zu max_payload_bytes=%zu switches=1980\n",
        sizeof(void*)*8,std::chrono::duration<double,std::milli>(initialized-start).count(),totalMs/18,
        static_cast<long long>(fallbackMemory)-static_cast<long long>(baseline),size_t(warm),size_t(final),maxUnits);
}
int main(){int cases=0;
    CASE(catalogs_are_complete_and_unicode);CASE(utf8_rejects_invalid_and_preserves_supplementary);
    CASE(parser_rejects_structural_errors_transactionally);CASE(fallback_and_partial_catalog);
    CASE(placeholders_are_checked_and_not_recursively_expanded);CASE(ordered_resolution_and_regional_variants);
    CASE(integer_plural_boundaries);CASE(arabic_composition_embeds_external_values);
    CASE(io_errors_preserve_active_catalog_and_unload);CASE(executable_relative_loading_ignores_working_directory);
    CASE(heap_accounting_detects_retention);CASE(repeated_switches_release_catalogs_and_measure_cost);
    std::printf("%d i18n tests passed.\n",cases);return 0;
}
