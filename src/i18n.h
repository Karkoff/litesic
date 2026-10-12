#pragma once
#include <string>
#include <vector>
#include <map>
#include <cstdint>

// Catalog operations are independent of HWND/audio. Never call from the audio callback.
namespace I18n {
struct Language { const char* code; const wchar_t* name; bool rtl; const wchar_t* font; };
const std::vector<Language>& Languages();
std::string Resolve(const std::vector<std::string>& preferences);
std::string PluralCategory(const std::string& language, uint64_t count);
using Catalog = std::map<std::string,std::wstring>;
using Arguments = std::map<std::string,std::wstring>;
bool DecodeUtf8(const std::string& bytes,std::wstring& result);
bool Parse(const std::string& bytes,Catalog& result,std::string& error);
bool Validate(const Catalog& catalog,const Catalog& reference,const std::string& language,
              bool complete,std::string& error);
bool Read(const std::wstring& path,Catalog& result,std::string& error);
std::wstring ExecutableLocales();
class Service {
public:
    Service();
    bool Select(const std::string& language,const std::wstring& root,std::string& error);
    // Partial catalogs are accepted only by this explicit API, for fallback tests/extensions.
    bool Install(const std::string& language,Catalog catalog,bool complete,std::string& error);
    void Unload();
    std::wstring Text(const std::string& id) const;
    bool Format(const std::string& id,const Arguments& args,std::wstring& result,std::string& error) const;
    std::wstring Count(const std::string& id,uint64_t count) const;
    const std::string& Active() const { return language_; }
    unsigned Generation() const { return generation_; }
    size_t RetainedUnits() const;
    const Catalog& Fallback() const { return fallback_; }
private:
    Catalog fallback_,active_;
    std::string language_="en-US";
    unsigned generation_=1;
};
// Global facade serializes worker error lookups with UI catalog changes using CRITICAL_SECTION.
bool Select(const std::string& language,std::string& error);
std::wstring Text(const std::string& id);
std::wstring Format(const std::string& id,const Arguments& args);
std::wstring Count(const std::string& id,uint64_t count);
std::string Active();
unsigned Generation();
bool IsRtl();
std::wstring LocaleName();
const wchar_t* ScriptFont();
}
