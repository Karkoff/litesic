#include "i18n.h"
#include "m3u.h"
#include <windows.h>
#include <sstream>
#include <cmath>
#include <cwctype>
#include <iomanip>
#include <locale>

std::wstring FullPath(const std::wstring& path) {
    DWORD size = GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
    if (!size) return path;
    std::vector<wchar_t> buffer(size);
    DWORD n = GetFullPathNameW(path.c_str(), size, buffer.data(), nullptr);
    return n && n < size ? std::wstring(buffer.data(), n) : path;
}
bool DecodeText(const std::string& bytes, std::wstring& text, bool legacy) {
    text.clear();
    if (bytes.empty()) return true;
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(),
                               static_cast<int>(bytes.size()), nullptr, 0);
    UINT cp = CP_UTF8;
    if (!n && legacy) {
        cp = 1252;
        n = MultiByteToWideChar(cp, 0, bytes.data(), static_cast<int>(bytes.size()), nullptr, 0);
    }
    if (!n) return false;
    text.resize(n);
    MultiByteToWideChar(cp, cp == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0,
        bytes.data(), static_cast<int>(bytes.size()), &text[0], n);
    if (!text.empty() && text[0] == 0xfeff) text.erase(0, 1);
    return true;
}
std::string EncodeUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                                nullptr, 0, nullptr, nullptr);
    std::string bytes(n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                        &bytes[0], n, nullptr, nullptr);
    return bytes;
}
static std::wstring Trim(std::wstring text) {
    auto a = text.find_first_not_of(L" \t\r\n");
    if (a == std::wstring::npos) return {};
    return text.substr(a, text.find_last_not_of(L" \t\r\n") - a + 1);
}
bool ParseM3U(const std::wstring& text, const std::wstring& playlistPath, std::vector<Track>& result) {
    result.clear();
    auto base = FullPath(playlistPath);
    base.resize(base.find_last_of(L"/\\") + 1);
    std::wistringstream input(text);
    std::wstring line, title;
    double duration = -1;
    bool first = true;
    bool favorite = false;
    while (std::getline(input, line)) {
        if (first && !line.empty() && line[0] == 0xfeff) line.erase(0, 1);
        first = false; line = Trim(line);
        if (line.empty()) continue;
        if(line==L"#LITESIC-FAVORITE:1"){favorite=true;continue;}
        if (line.compare(0, 8, L"#EXTINF:") == 0) {
            auto comma = line.find(L',', 8);
            title.clear(); duration = -1;
            if (comma != std::wstring::npos) {
                std::wistringstream number(line.substr(8, comma - 8));
                number.imbue(std::locale::classic());
                double value;
                if (number >> value && std::isfinite(value) && value >= 0) duration = value;
                title = Trim(line.substr(comma + 1));
            }
            continue;
        }
        if (line[0] == L'#') continue;
        if (line.size() >= 2 && line.front() == L'"' && line.back() == L'"')
            line = line.substr(1, line.size() - 2);
        // M5 accepts local supported files, never network streams.
        if (line.find(L"://") == std::wstring::npos && IsSupportedAudioFile(line.c_str())) {
            bool absolute = line.size() > 1 && (line[1] == L':' ||
                (line[0] == L'\\' && line[1] == L'\\'));
            if (!absolute && !line.empty() && (line[0] == L'\\' || line[0] == L'/'))
                line = base.substr(0, 2) + line;
            else if (!absolute) line = base + line;
            Track track;
            track.path = FullPath(line);
            track.displayTitle = title.empty() ? FileTitle(track.path) : title;
            track.duration = duration;
            track.favorite = favorite;
            result.push_back(std::move(track));
        }
        title.clear(); duration = -1; favorite = false;
    }
    return true;
}
std::wstring WriteM3U(const std::vector<Track>& tracks) {
    std::wostringstream output;
    output.imbue(std::locale::classic());
    output << L"#EXTM3U\r\n" << std::setprecision(12);
    for (const auto& track : tracks) {
        std::wstring title = track.displayTitle;
        for (auto& c : title) if (c == L'\r' || c == L'\n') c = L' ';
        if (track.path.find_first_of(L"\r\n") != std::wstring::npos) continue;
        if(track.favorite)output << L"#LITESIC-FAVORITE:1\r\n";
        output << L"#EXTINF:" << (std::isfinite(track.duration) ? track.duration : -1)
               << L"," << title << L"\r\n" << FullPath(track.path) << L"\r\n";
    }
    return output.str();
}
bool LoadM3U(const std::wstring& path, std::vector<Track>& result, std::wstring& error,std::string* errorId) {
    if(errorId)errorId->clear();
    error.clear(); result.clear();
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { if(errorId)*errorId="errors.m3u_open";error = I18n::Text("errors.m3u_open").c_str(); return false; }
    LARGE_INTEGER size{};
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 && size.QuadPart <= 16 * 1024 * 1024;
    std::string bytes;
    if (ok) {
        bytes.resize(static_cast<size_t>(size.QuadPart));
        DWORD read = 0;
        ok = ReadFile(file, bytes.empty() ? nullptr : &bytes[0], static_cast<DWORD>(bytes.size()), &read, nullptr)
             && read == bytes.size();
    }
    CloseHandle(file);
    std::wstring text;
    if (!ok || !DecodeText(bytes, text, true)) {
        if(errorId)*errorId="errors.m3u_invalid";
        error = I18n::Text("errors.m3u_invalid").c_str(); return false;
    }
    return ParseM3U(text, path, result);
}
bool SaveM3U(const std::wstring& path, const std::vector<Track>& tracks, std::wstring& error,std::string* errorId) {
    if(errorId)errorId->clear();
    error.clear();
    std::wstring full = FullPath(path), directory = full.substr(0, full.find_last_of(L"/\\") + 1);
    wchar_t temporary[MAX_PATH];
    if (!GetTempFileNameW(directory.c_str(), L"LSC", 0, temporary)) {
        if(errorId)*errorId="errors.temporary";
        error = I18n::Text("errors.temporary").c_str(); return false;
    }
    HANDLE file = CreateFileW(temporary, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    bool ok = file != INVALID_HANDLE_VALUE;
    if (ok) {
        // UTF-8 without BOM: legacy WMP otherwise silently imports zero items.
        std::string bytes = EncodeUtf8(WriteM3U(tracks));
        DWORD written = 0;
        ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)
            && written == bytes.size() && FlushFileBuffers(file);
        CloseHandle(file);
    }
    if (ok) ok = MoveFileExW(temporary, full.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok) { DeleteFileW(temporary);if(errorId)*errorId="errors.m3u_save"; error = I18n::Text("errors.m3u_save").c_str(); }
    return ok;
}
