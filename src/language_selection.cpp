#include "language_selection.h"
#include <algorithm>

namespace I18n {
namespace {
std::wstring settings;
LanguagePreference preference;
PreferenceProvider provider=WindowsLanguagePreferences;
DWORD ownerThread=0;
bool changing=false;
bool Known(const std::string& code){for(const auto& l:Languages())if(code==l.code)return true;return false;}
BOOL CALLBACK Notify(HWND hwnd,LPARAM){
    wchar_t cls[80]{};GetClassNameW(hwnd,cls,80);
    if(wcsncmp(cls,L"LiteSic.",8)==0)SendMessageW(hwnd,WM_LANGUAGE_CHANGED,0,0);
    return TRUE;
}
std::string Effective(const LanguagePreference& p){return p.automatic?Resolve(provider()):p.selected;}
void Apply(const LanguagePreference& next,std::string& diagnostic){
    const auto code=Effective(next);
    if(code!=Active()&&!Select(code,diagnostic)){
        OutputDebugStringA(("LiteSic language fallback: "+diagnostic+"\n").c_str());
        std::string ignored;Select("en-US",ignored);
    }
}
}
std::vector<std::string> ParseWindowsLanguages(const wchar_t* buffer,size_t length){
    std::vector<std::string> result;
    if(!buffer||length<2||buffer[length-1]||buffer[length-2])return result;
    size_t offset=0;
    while(offset+1<length&&buffer[offset]){
        std::string code;
        while(offset<length&&buffer[offset]){
            wchar_t c=buffer[offset++];if(c>127)return {};
            code+=static_cast<char>(c);
        }
        if(code.size()>=LOCALE_NAME_MAX_LENGTH)return {};
        result.push_back(std::move(code));++offset;
    }
    return result;
}
std::vector<std::string> WindowsLanguagePreferences(){
    ULONG count=0,length=0;
    if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME,&count,nullptr,&length)&&length>=2&&length<=65536){
        // Retry if the user changes the list between the size and data calls.
        for(int attempt=0;attempt<3;++attempt){
            std::vector<wchar_t> buffer(length);ULONG actual=length;
            if(GetUserPreferredUILanguages(MUI_LANGUAGE_NAME,&count,buffer.data(),&actual)){
                if(actual>buffer.size())break;
                auto result=ParseWindowsLanguages(buffer.data(),actual);
                if(!result.empty())return result;
                break;
            }
            if(GetLastError()!=ERROR_INSUFFICIENT_BUFFER||actual>65536||actual<2)break;
            length=actual;
        }
    }
    wchar_t name[LOCALE_NAME_MAX_LENGTH]{};
    if(LCIDToLocaleName(MAKELCID(GetUserDefaultUILanguage(),SORT_DEFAULT),name,LOCALE_NAME_MAX_LENGTH,0)>1){
        std::wstring wide=name;return {std::string(wide.begin(),wide.end())};
    }
    return {}; // Resolve will choose English, never a geographic locale.
}
LanguagePreference ReadLanguagePreference(const std::wstring& path){
    LanguagePreference p;if(path.empty())return p;
    wchar_t mode[32]{},selected[LOCALE_NAME_MAX_LENGTH]{};
    GetPrivateProfileStringW(L"Language",L"Mode",L"automatic",mode,32,path.c_str());
    GetPrivateProfileStringW(L"Language",L"Selected",L"",selected,LOCALE_NAME_MAX_LENGTH,path.c_str());
    std::wstring wide=selected;
    if(std::all_of(wide.begin(),wide.end(),[](wchar_t c){return c<128;}))p.selected.assign(wide.begin(),wide.end());
    if(!Known(p.selected))p.selected.clear();
    p.automatic=wcscmp(mode,L"manual")!=0||p.selected.empty();return p;
}
bool WriteLanguagePreference(const std::wstring& path,const LanguagePreference& p){
    if((!p.automatic&&p.selected.empty())||(!p.selected.empty()&&!Known(p.selected)))return false;
    if(path.empty())return true;
    std::wstring data=std::wstring(L"Mode=")+(p.automatic?L"automatic":L"manual");data+=L'\0';
    if(!p.selected.empty()){data+=L"Selected=";data.append(p.selected.begin(),p.selected.end());data+=L'\0';}
    data+=L'\0';return WritePrivateProfileSectionW(L"Language",data.c_str(),path.c_str())!=FALSE;
}
void InitializeLanguage(const wchar_t* path,PreferenceProvider source){
    settings=path?path:L"";provider=source?source:WindowsLanguagePreferences;
    ownerThread=GetCurrentThreadId();preference=ReadLanguagePreference(settings);
    std::string diagnostic;Apply(preference,diagnostic);
}
LanguagePreference CurrentLanguagePreference(){return preference;}
bool ChangeLanguage(bool automatic,const std::string& selected,std::string& diagnostic){
    diagnostic.clear();
    if(changing||(ownerThread&&ownerThread!=GetCurrentThreadId())){diagnostic="language change must run on the UI thread without reentrancy";return false;}
    if(!automatic&&!Known(selected)){diagnostic="unsupported manual language";return false;}
    struct Changing{Changing(){changing=true;}~Changing(){changing=false;}} guard;
    LanguagePreference next{automatic,automatic?preference.selected:selected};
    // Persist a single section before changing visible state. Failure leaves UI and preference intact.
    if(!WriteLanguagePreference(settings,next)){diagnostic="cannot persist language preference";return false;}
    Apply(next,diagnostic);preference=next;
    EnumThreadWindows(GetCurrentThreadId(),Notify,0);
    return true;
}
HMENU CreateLanguageMenu(){
    HMENU menu=CreatePopupMenu();std::wstring name;
    const auto automaticCode=preference.automatic?Active():Resolve(provider());
    for(const auto& l:Languages())if(l.code==automaticCode)name=l.name;
    auto automatic=Format("appearance.auto_language",{{"language",name}});
    AppendMenuW(menu,MF_STRING|(preference.automatic?MF_CHECKED:0),LanguageAutomaticCommand,automatic.c_str());
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    int id=LanguageFirstCommand;
    for(const auto& l:Languages())AppendMenuW(menu,MF_STRING|(!preference.automatic&&preference.selected==l.code?MF_CHECKED:0),id++,l.name);
    return menu;
}
bool IsLanguageCommand(int command){return command>=LanguageAutomaticCommand&&command<LanguageFirstCommand+static_cast<int>(Languages().size());}
bool ExecuteLanguageCommand(int command,std::string& diagnostic){
    if(!IsLanguageCommand(command)){diagnostic="invalid language command";return false;}
    return ChangeLanguage(command==LanguageAutomaticCommand,command==LanguageAutomaticCommand?"":Languages()[command-LanguageFirstCommand].code,diagnostic);
}
void UpdateEditDirection(HWND edit){
    auto style=GetWindowLongPtrW(edit,GWL_STYLE),ex=GetWindowLongPtrW(edit,GWL_EXSTYLE);
    auto desired=(style&~static_cast<LONG_PTR>(ES_RIGHT))|(IsRtl()?ES_RIGHT:0);
    auto desiredEx=(ex&~static_cast<LONG_PTR>(WS_EX_RTLREADING))|(IsRtl()?WS_EX_RTLREADING:0);
    if(style!=desired||ex!=desiredEx){
        SetWindowLongPtrW(edit,GWL_STYLE,desired);SetWindowLongPtrW(edit,GWL_EXSTYLE,desiredEx);
        SetWindowPos(edit,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
    }
}
}
