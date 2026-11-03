#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace Runtime {
struct Options { bool portable=false,validation=false; std::wstring profile,locale; std::vector<std::wstring> files; };
bool Parse(const std::vector<std::wstring>& args, Options& options);
// Dynamic lookup keeps the traditional binary loadable on Windows 7.
bool CurrentPackageFamily(std::wstring& family);
std::wstring ProfileDirectory(const std::wstring& executable, const std::wstring& localData, const Options& options, const std::wstring& packageFamily = {});
bool PrepareProfile(const std::wstring& directory);
std::wstring Identity(const std::wstring& directory);
std::vector<wchar_t> Encode(const std::vector<std::wstring>& files);
bool Decode(const COPYDATASTRUCT* data,std::vector<std::wstring>& files);
constexpr ULONG_PTR Packet=0x4c534938;
class Instance {
    HANDLE mutex=nullptr; std::wstring identity;
public:
    ~Instance();
    // 1 primary, 0 forwarded, -1 failure. Never writes profile before election.
    int Enter(const std::wstring& directory,const std::vector<std::wstring>& files);
    void Publish(HWND hwnd);
};
}
