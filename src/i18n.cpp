#include "i18n.h"
#include "i18n_english.h"
#include <windows.h>
#include <algorithm>
#include <set>
#include <sstream>
#include <stdexcept>

namespace I18n {
const std::vector<Language>& Languages() {
    static const std::vector<Language> list={
        {"pt-BR",L"Português (Brasil)",false,nullptr},{"en-US",L"English",false,nullptr},
        {"es-ES",L"Español",false,nullptr},{"fr-FR",L"Français",false,nullptr},
        {"de-DE",L"Deutsch",false,nullptr},{"it-IT",L"Italiano",false,nullptr},
        {"pt-PT",L"Português (Portugal)",false,nullptr},{"nl-NL",L"Nederlands",false,nullptr},
        {"pl-PL",L"Polski",false,nullptr},{"ru-RU",L"Русский",false,nullptr},
        {"ja-JP",L"日本語",false,L"Meiryo"},{"zh-CN",L"简体中文",false,L"Microsoft YaHei"},
        {"zh-TW",L"繁體中文",false,L"Microsoft JhengHei"},{"ko-KR",L"한국어",false,L"Malgun Gothic"},
        {"tr-TR",L"Türkçe",false,nullptr},{"ar-SA",L"العربية",true,L"Tahoma"},
        {"hi-IN",L"हिन्दी",false,L"Mangal"},{"sv-SE",L"Svenska",false,nullptr}};
    return list;
}
static std::string Lower(std::string s) {
    for(auto& c:s) { if(c=='_')c='-'; if(c>='A'&&c<='Z')c+=32; }
    return s;
}
std::string Resolve(const std::vector<std::string>& preferences) {
    for(auto pref:preferences) {
        pref=Lower(pref);
        bool valid=!pref.empty();size_t part=0;
        for(char c:pref) {
            if(c=='-'){valid&=part>0&&part<=8;part=0;}
            else {valid&=(c>='a'&&c<='z')||(c>='0'&&c<='9');++part;}
        }
        if(!valid||part==0||part>8)continue;
        for(auto& l:Languages()) if(pref==Lower(l.code))return l.code;
        auto split=pref.find('-');auto base=pref.substr(0,split);
        if(base=="zh") {
            // Explicit script wins; incompatible/unknown explicit scripts never cross fallback.
            if(pref.find("-hant")!=std::string::npos)return "zh-TW";
            if(pref.find("-hans")!=std::string::npos)return "zh-CN";
            if(pref=="zh-tw"||pref=="zh-hk"||pref=="zh-mo")return "zh-TW";
            if(pref=="zh"||pref=="zh-cn"||pref=="zh-sg")return "zh-CN";
            continue;
        }
        std::string script;std::istringstream subtags(pref);std::string tag;
        std::getline(subtags,tag,'-');
        while(std::getline(subtags,tag,'-'))if(tag.size()==4)script=tag;
        const std::string expected=base=="ru"?"cyrl":base=="ar"?"arab":base=="hi"?"deva":
            base=="ja"?"jpan":base=="ko"?"kore":"latn";
        if(!script.empty()&&script!=expected)continue;
        for(auto& l:Languages()) if(base==Lower(l.code).substr(0,Lower(l.code).find('-')))return l.code;
    }
    return "en-US";
}
std::string PluralCategory(const std::string& language,uint64_t n) {
    auto l=Lower(language).substr(0,2);
    if(l=="ja"||l=="zh"||l=="ko")return "other";
    if(l=="ar") {if(n==0)return "zero";if(n==1)return "one";if(n==2)return "two";
        if(n%100>=3&&n%100<=10)return "few";
        if(n%100>=11&&n%100<=99)return "many";
        return "other";}
    if(l=="ru"||l=="pl") {
        if((l=="ru"&&n%10==1&&n%100!=11)||(l=="pl"&&n==1))return "one";
        if(n%10>=2&&n%10<=4&&(n%100<12||n%100>14))return "few";
        return "many";
    }
    if((l=="fr"||l=="pt"||l=="it"||l=="es") && n!=0 && n%1000000==0)return "many";
    if(l=="fr"||l=="hi"||Lower(language)=="pt-br")return n<=1?"one":"other";
    return n==1?"one":"other";
}
bool DecodeUtf8(const std::string& bytes,std::wstring& out) {
    out.clear();if(bytes.empty())return true;if(bytes.size()>1024*1024)return false;
    int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(bytes.size()),nullptr,0);
    if(!size)return false;
    out.resize(size);
    return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,bytes.data(),int(bytes.size()),&out[0],size)==size;
}
static bool Key(const std::string& key) {
    if(key.empty())return false;
    for(char c:key)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;
    return true;
}
static bool Placeholders(const std::wstring& s,std::set<std::string>& names) {
    for(size_t i=0;i<s.size();++i) {
        if(s[i]==L'}')return false;
        if(s[i]!=L'{')continue;
        auto end=s.find(L'}',i+1);if(end==s.npos)return false;
        std::string name;
        for(size_t j=i+1;j<end;++j){if(s[j]>127)return false;name+=char(s[j]);}
        if(!Key(name))return false;
        names.insert(name);i=end;
    }return true;
}
bool Parse(const std::string& bytes,Catalog& result,std::string& error) {
    result.clear();error.clear();std::wstring decoded;
    if(!DecodeUtf8(bytes,decoded)){error="invalid UTF-8 or catalog larger than 1 MiB";return false;}
    std::string input=bytes;if(input.compare(0,3,"\xEF\xBB\xBF")==0)input.erase(0,3);
    std::istringstream stream(input);std::string line;Catalog parsed;unsigned number=0;
    while(std::getline(stream,line)) {
        ++number;if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()||line[0]=='#')continue;
        auto tab=line.find('\t');std::string key=line.substr(0,tab);
        auto fail=[&](){error="invalid catalog line "+std::to_string(number);return false;};
        if(tab==line.npos||!Key(key))return fail();
        std::wstring value;if(!DecodeUtf8(line.substr(tab+1),value)||value.empty())return fail();
        std::wstring unescaped;
        for(size_t i=0;i<value.size();++i) {
            wchar_t c=value[i];
            if(c==L'\\') {if(++i==value.size())return fail();c=value[i];
                if(c==L'n'){unescaped+=L"\r\n";continue;}if(c!=L'\\')return fail();}
            if(c<32||c==127||c==0xfeff)return fail();
            unescaped+=c;
        }
        if(unescaped.find_first_not_of(L" \r\n\t")==unescaped.npos)return fail();
        std::set<std::string> params;if(!Placeholders(unescaped,params)||!parsed.emplace(key,unescaped).second)return fail();
    }
    if(parsed.empty()){error="empty catalog";return false;}result.swap(parsed);return true;
}
bool Validate(const Catalog& catalog,const Catalog& reference,const std::string& language,bool complete,std::string& error) {
    error.clear();
    for(const auto& item:catalog) {
        auto found=reference.find(item.first);std::set<std::string> a,b;
        bool unicode=true;
        for(size_t i=0;i<item.second.size();++i){auto c=item.second[i];
            if(c>=0xd800&&c<=0xdbff){if(++i==item.second.size()||item.second[i]<0xdc00||item.second[i]>0xdfff){unicode=false;break;}}
            else if((c>=0xdc00&&c<=0xdfff)||c==0||(c<32&&c!=L'\r'&&c!=L'\n')){unicode=false;break;}
        }
        if(found==reference.end()||!unicode||item.second.find_first_not_of(L" \r\n\t")==item.second.npos||!Placeholders(item.second,a)||
           !Placeholders(found->second,b)||a!=b){error="invalid key/parameters: "+item.first;return false;}
    }
    if(complete)for(const auto& item:reference)if(!catalog.count(item.first)){error="missing key: "+item.first;return false;}
    // Each plural family declares all six categories, including equivalent forms.
    for(const auto& item:catalog) if(item.first.size()>6&&item.first.substr(item.first.size()-6)==".other") {
        auto base=item.first.substr(0,item.first.size()-6);
        for(auto category:{"zero","one","two","few","many","other"})
            if(!catalog.count(base+"."+category)){error="incomplete plural: "+base+" ("+language+")";return false;}
    }
    return true;
}
bool Read(const std::wstring& path,Catalog& result,std::string& error) {
    result.clear();HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){error="cannot open catalog";return false;}
    LARGE_INTEGER size{};bool ok=GetFileSizeEx(file,&size)&&size.QuadPart>0&&size.QuadPart<=1024*1024;
    std::string bytes;DWORD got=0;
    if(ok){bytes.resize(size_t(size.QuadPart));ok=ReadFile(file,&bytes[0],DWORD(bytes.size()),&got,nullptr)&&got==bytes.size();}
    CloseHandle(file);if(!ok){error="cannot read catalog (limit 1 MiB)";return false;}return Parse(bytes,result,error);
}
std::wstring ExecutableLocales() {
    std::vector<wchar_t> path(32768);DWORD n=GetModuleFileNameW(nullptr,path.data(),DWORD(path.size()));
    if(!n||n>=path.size())return {};
    std::wstring exe(path.data(),n);return exe.substr(0,exe.find_last_of(L"\\/"))+L"\\locales";
}
Service::Service() {
    std::string error;if(!Parse(EnglishCatalog,fallback_,error))throw std::logic_error(error);
}
bool Service::Install(const std::string& language,Catalog catalog,bool complete,std::string& error) {
    bool known=false;for(auto& l:Languages())known|=language==l.code;
    if(!known){error="unsupported catalog language";return false;}
    if(!Validate(catalog,fallback_,language,complete,error))return false;
    active_.swap(catalog);language_=language;++generation_;return true;
}
bool Service::Select(const std::string& language,const std::wstring& root,std::string& error) {
    auto resolved=Resolve({language});
    if(resolved=="en-US"){Unload();error.clear();return true;}
    Catalog catalog;if(root.empty()){error="empty locales root";return false;}
    std::wstring code(resolved.begin(),resolved.end());
    if(!Read(root+L"\\"+code+L"\\messages.tsv",catalog,error))return false;
    return Install(resolved,std::move(catalog),true,error);
}
void Service::Unload(){Catalog{}.swap(active_);language_="en-US";++generation_;}
std::wstring Service::Text(const std::string& id) const {
    auto found=active_.find(id);if(found!=active_.end())return found->second;
    found=fallback_.find(id);if(found!=fallback_.end())return found->second;
    OutputDebugStringA(("LiteSic unknown translation ID: "+id+"\n").c_str());
    return fallback_.at("errors.translation");
}
bool Service::Format(const std::string& id,const Arguments& args,std::wstring& result,std::string& error) const {
    auto text=Text(id);std::set<std::string> needed;Placeholders(text,needed);error.clear();result.clear();
    if(needed.size()!=args.size()){error="argument count mismatch: "+id;return false;}
    for(auto& name:needed)if(!args.count(name)){error="missing argument: "+name;return false;}
    for(size_t i=0;i<text.size();++i) {
        if(text[i]!=L'{'){result+=text[i];continue;}
        size_t end=text.find(L'}',i);std::string name;
        for(size_t j=i+1;j<end;++j)name+=char(text[j]);
        // Unicode directional embedding works on Win7 (unlike depending on isolate support).
        if(language_=="ar-SA")result+=wchar_t(0x202a);
        result+=args.at(name);
        if(language_=="ar-SA")result+=wchar_t(0x202c);
        i=end;
    }return true;
}
std::wstring Service::Count(const std::string& id,uint64_t n) const {
    auto key=id+"."+PluralCategory(language_,n);
    // A missing Arabic/Russian form must use English grammar as well as English text.
    if(!active_.count(key))key=id+"."+PluralCategory("en-US",n);
    std::wstring result;std::string error;
    if(!Format(key,{{"count",std::to_wstring(n)}},result,error))return Text("errors.translation");
    return result;
}
size_t Service::RetainedUnits() const {
    size_t n=0;for(auto c:{&fallback_,&active_})for(auto& p:*c)n+=p.first.size()+2*p.second.size();return n;
}
namespace {
struct Shared {
    CRITICAL_SECTION lock;Service service;
    Shared(){
        InitializeCriticalSection(&lock);
        // Preserve the pre-i18n application default until T7 implements preferences.
        std::string error;
        if(!service.Select("pt-BR",ExecutableLocales(),error))OutputDebugStringA(error.c_str());
    }
    ~Shared(){DeleteCriticalSection(&lock);}
};
Shared& Global(){static Shared shared;return shared;}
struct Guard {Shared& shared;Guard():shared(Global()){EnterCriticalSection(&shared.lock);}~Guard(){LeaveCriticalSection(&shared.lock);}};
}
bool Select(const std::string& language,std::string& error){Guard guard;return guard.shared.service.Select(language,ExecutableLocales(),error);}
std::wstring Text(const std::string& id){Guard guard;return guard.shared.service.Text(id);}
std::wstring Format(const std::string& id,const Arguments& args){Guard guard;std::wstring result;std::string error;
    if(!guard.shared.service.Format(id,args,result,error)){OutputDebugStringA(error.c_str());return guard.shared.service.Text("errors.translation");}return result;}
std::wstring Count(const std::string& id,uint64_t n){Guard guard;return guard.shared.service.Count(id,n);}
std::string Active(){Guard guard;return guard.shared.service.Active();}
unsigned Generation(){Guard guard;return guard.shared.service.Generation();}
bool IsRtl(){return Active()=="ar-SA";}
std::wstring LocaleName(){auto code=Active();return {code.begin(),code.end()};}
const wchar_t* ScriptFont(){auto code=Active();for(auto& l:Languages())if(code==l.code)return l.font;return nullptr;}
}
