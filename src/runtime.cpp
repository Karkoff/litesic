#include "runtime.h"
#include <algorithm>
#include <cwctype>
#include <cstring>
#include <shlobj.h>
namespace Runtime {
bool Parse(const std::vector<std::wstring>& args,Options& out){
    out={};bool filesOnly=false;
    for(const auto& arg:args){
        if(!filesOnly&&arg==L"--"){filesOnly=true;continue;}
        if(!filesOnly&&arg==L"--portable"){out.portable=true;continue;}
        if(!filesOnly&&arg==L"--validation"){out.validation=true;continue;}
        if(!filesOnly&&arg.rfind(L"--profile=",0)==0){out.profile=arg.substr(10);if(out.profile.empty())return false;continue;}
        if(!filesOnly&&arg.rfind(L"--locale=",0)==0){out.locale=arg.substr(9);continue;}
        if(!filesOnly&&arg.rfind(L"--",0)==0)return false;
        DWORD count=GetFullPathNameW(arg.c_str(),0,nullptr,nullptr);if(!count)return false;
        std::wstring path(count,L'\0');if(!GetFullPathNameW(arg.c_str(),count,&path[0],nullptr))return false;
        path.resize(wcslen(path.c_str()));out.files.push_back(path);
    }
    return !(out.portable&&!out.profile.empty());
}
bool CurrentPackageFamily(std::wstring& family){
    family.clear();
    using QueryFamily=LONG(WINAPI*)(UINT32*,PWSTR);
    auto kernel=GetModuleHandleW(L"kernel32.dll");
    FARPROC procedure=kernel?GetProcAddress(kernel,"GetCurrentPackageFamilyName"):nullptr;
    QueryFamily query=nullptr;
    static_assert(sizeof(query)==sizeof(procedure),"Windows function pointer size");
    std::memcpy(&query,&procedure,sizeof(query));
    if(!query)return true; // Windows 7 has no package identity API.
    UINT32 length=0;LONG result=query(&length,nullptr);
    if(result==APPMODEL_ERROR_NO_PACKAGE)return true;
    if(result!=ERROR_INSUFFICIENT_BUFFER||length<2||length>128)return false;
    std::wstring value(length,L'\0');
    if(query(&length,&value[0])!=ERROR_SUCCESS||length<2||length>value.size()||value[length-1]!=L'\0')return false;
    value.resize(length-1);family=std::move(value);return true;
}
std::wstring ProfileDirectory(const std::wstring& executable,const std::wstring& localData,const Options& options,const std::wstring& packageFamily){
    if(!options.profile.empty()){
        DWORD n=GetFullPathNameW(options.profile.c_str(),0,nullptr,nullptr);if(!n)return {};
        std::wstring p(n,L'\0');GetFullPathNameW(options.profile.c_str(),n,&p[0],nullptr);p.resize(wcslen(p.c_str()));return p;
    }
    // Explicit profile is a developer override. Normal MSIX launches never use
    // the existing desktop profile or attempt portable writes in WindowsApps.
    if(!packageFamily.empty())return localData.empty()?std::wstring{}:localData+L"\\Packages\\"+packageFamily+L"\\LocalState\\LiteSic";
    auto parent=executable.substr(0,executable.find_last_of(L"\\/"));
    if(options.validation){for(int i=0;i<3;++i)parent=parent.substr(0,parent.find_last_of(L"\\/"));return parent+L"\\build\\gui\\m6-session";}
    // Desktop portable archives opt in without requiring a command-line flag.
    // Resolve the marker beside the executable, never in the working directory.
    DWORD marker=GetFileAttributesW((parent+L"\\resources\\portable.flag").c_str());
    bool portable=options.portable||(marker!=INVALID_FILE_ATTRIBUTES&&!(marker&FILE_ATTRIBUTE_DIRECTORY));
    return portable?parent+L"\\data":localData.empty()?std::wstring{}:localData+L"\\LiteSic";
}
bool PrepareProfile(const std::wstring& directory){
    if(directory.empty())return false;
    // Create only the explicitly selected directory tree; no migration or deletion.
    int created=SHCreateDirectoryExW(nullptr,directory.c_str(),nullptr);
    if(created!=ERROR_SUCCESS&&created!=ERROR_ALREADY_EXISTS&&created!=ERROR_FILE_EXISTS)return false;
    for(auto file:{L"settings.ini",L"playlist.m3u8",L"playlist.m3u8.ini"}){
        DWORD attributes=GetFileAttributesW((directory+L"\\"+file).c_str());
        if(attributes==INVALID_FILE_ATTRIBUTES){DWORD error=GetLastError();if(error!=ERROR_FILE_NOT_FOUND&&error!=ERROR_PATH_NOT_FOUND)return false;continue;}
        if(attributes&FILE_ATTRIBUTE_READONLY)return false;
        auto existing=directory+L"\\"+file;
        HANDLE access=CreateFileW(existing.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(access==INVALID_HANDLE_VALUE)return false;
        CloseHandle(access);
    }
    auto probe=directory+L"\\.write-check-"+std::to_wstring(GetCurrentProcessId());
    HANDLE f=CreateFileW(probe.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
    if(f==INVALID_HANDLE_VALUE)return false;
    CloseHandle(f);return true;
}
std::wstring Identity(const std::wstring& directory){
    std::wstring canonical=directory;
    HANDLE folder=CreateFileW(directory.c_str(),0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,nullptr);
    if(folder!=INVALID_HANDLE_VALUE){DWORD n=GetFinalPathNameByHandleW(folder,nullptr,0,FILE_NAME_NORMALIZED);if(n){std::wstring resolved(n,L'\0');DWORD written=GetFinalPathNameByHandleW(folder,&resolved[0],n,FILE_NAME_NORMALIZED);if(written&&written<n){resolved.resize(written);canonical=resolved;}}CloseHandle(folder);}
    if(canonical.rfind(L"\\\\?\\UNC\\",0)==0)canonical=L"\\\\"+canonical.substr(8);
    else if(canonical.rfind(L"\\\\?\\",0)==0)canonical=canonical.substr(4);
    while(canonical.size()>3&&(canonical.back()==L'\\'||canonical.back()==L'/'))canonical.pop_back();
    unsigned long long hash=14695981039346656037ull;
    for(wchar_t c:canonical){c=std::towlower(c==L'/'?L'\\':c);hash^=unsigned(c);hash*=1099511628211ull;}
    return L"Local\\LiteSic.Profile."+std::to_wstring(hash);
}
std::vector<wchar_t> Encode(const std::vector<std::wstring>& files){
    std::vector<wchar_t> data;for(const auto& file:files){data.insert(data.end(),file.begin(),file.end());data.push_back(0);}data.push_back(0);if(data.size()==1)data.push_back(0);return data;
}
bool Decode(const COPYDATASTRUCT* data,std::vector<std::wstring>& files){
    files.clear();if(!data||data->dwData!=Packet||!data->lpData||data->cbData<4||data->cbData>1024*1024||data->cbData%sizeof(wchar_t))return false;
    const auto p=static_cast<const wchar_t*>(data->lpData);size_t n=data->cbData/sizeof(wchar_t);
    if(p[n-1]||p[n-2])return false;
    for(size_t i=0;i<n-1;){size_t end=i;while(end<n&&p[end])++end;if(end==i)return end==n-2||end==n-1;files.emplace_back(p+i,end-i);i=end+1;}
    return true;
}
struct Search { const wchar_t* property; HWND window=nullptr; };
static BOOL CALLBACK Find(HWND hwnd,LPARAM param){auto s=reinterpret_cast<Search*>(param);if(GetPropW(hwnd,s->property)){s->window=hwnd;return FALSE;}return TRUE;}
Instance::~Instance(){if(mutex){ReleaseMutex(mutex);CloseHandle(mutex);}}
int Instance::Enter(const std::wstring& directory,const std::vector<std::wstring>& files){
    identity=Identity(directory);mutex=CreateMutexW(nullptr,FALSE,identity.c_str());if(!mutex)return -1;
    DWORD acquired=WaitForSingleObject(mutex,0);if(acquired==WAIT_OBJECT_0||acquired==WAIT_ABANDONED)return 1;
    auto payload=Encode(files);if(payload.size()*sizeof(wchar_t)>1024*1024)return -1;
    DWORD start=GetTickCount();
    while(GetTickCount()-start<5000){
        Search search{identity.c_str()};EnumWindows(Find,reinterpret_cast<LPARAM>(&search));
        if(search.window){COPYDATASTRUCT data{Packet,DWORD(payload.size()*sizeof(wchar_t)),payload.data()};DWORD_PTR result=0;
            DWORD process=0;GetWindowThreadProcessId(search.window,&process);AllowSetForegroundWindow(process);
            if(SendMessageTimeoutW(search.window,WM_COPYDATA,0,reinterpret_cast<LPARAM>(&data),SMTO_ABORTIFHUNG|SMTO_BLOCK,2000,&result)&&result)return 0;
            return -1;
        }
        acquired=WaitForSingleObject(mutex,50);if(acquired==WAIT_OBJECT_0||acquired==WAIT_ABANDONED)return 1;
    }return -1;
}
void Instance::Publish(HWND hwnd){SetPropW(hwnd,identity.c_str(),reinterpret_cast<HANDLE>(1));}
}
