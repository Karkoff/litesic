#pragma once
#include "i18n.h"
#include <windows.h>

namespace I18n {
constexpr UINT WM_LANGUAGE_CHANGED=WM_APP+89;
constexpr int LanguageAutomaticCommand=920, LanguageFirstCommand=921;
struct LanguagePreference {
    bool automatic=true;
    std::string selected;
};
using PreferenceProvider=std::vector<std::string>(*)();
std::vector<std::string> ParseWindowsLanguages(const wchar_t* buffer,size_t length);
std::vector<std::string> WindowsLanguagePreferences();
LanguagePreference ReadLanguagePreference(const std::wstring& path);
bool WriteLanguagePreference(const std::wstring& path,const LanguagePreference& preference);
// UI thread only. nullptr path disables persistence; provider permits isolated OS simulation.
void InitializeLanguage(const wchar_t* path,PreferenceProvider provider=nullptr);
LanguagePreference CurrentLanguagePreference();
// Fallback to embedded English on unavailable resources; nonempty diagnostic is explicit.
bool ChangeLanguage(bool automatic,const std::string& selected,std::string& diagnostic);
HMENU CreateLanguageMenu();
bool IsLanguageCommand(int command);
bool ExecuteLanguageCommand(int command,std::string& diagnostic);
void UpdateEditDirection(HWND edit);
}
