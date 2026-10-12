#pragma once
#include "playlist.h"
#include <string>
#include <vector>

std::wstring FullPath(const std::wstring& path);
bool DecodeText(const std::string& bytes, std::wstring& text, bool legacy = false);
std::string EncodeUtf8(const std::wstring& text);
bool ParseM3U(const std::wstring& text, const std::wstring& playlistPath,
              std::vector<Track>& result);
std::wstring WriteM3U(const std::vector<Track>& tracks);
bool LoadM3U(const std::wstring& path, std::vector<Track>& result, std::wstring& error,std::string* errorId=nullptr);
bool SaveM3U(const std::wstring& path, const std::vector<Track>& tracks, std::wstring& error,std::string* errorId=nullptr);
