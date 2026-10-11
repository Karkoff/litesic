#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Track {
    uint64_t id = 0;
    std::wstring path, displayTitle;
    double duration = -1;
    bool metadataPending = true, metadataLoaded = false, failed = false;
};

bool IsSupportedAudioFile(const wchar_t* path);
std::wstring FileTitle(const std::wstring& path);
// Indices are normalized, sorted and unique. Shift uses an explicit anchor.
std::vector<int> SelectRows(std::vector<int> selected, int count, int clicked,
                            int anchor, bool ctrl, bool shift);

class Playlist {
public:
    explicit Playlist(uint32_t seed = 0x4c495445u) : rng_(seed ? seed : 1) {}
    uint64_t Add(const std::wstring& path);
    bool RemoveAt(int index);
    bool MoveTo(int from, int to);
    void MoveSelection(const std::vector<int>& indices, int insertion);
    void Sort(bool byDuration, bool ascending);
    void ShuffleVisible();
    void Clear();
    int Count() const { return static_cast<int>(tracks_.size()); }
    int GetCurrentIndex() const { return Find(current_); }
    bool SetCurrentIndex(int index);
    int Next(bool shuffle, bool repeat);
    int Previous(bool shuffle = false, bool repeat = false);
    int Find(uint64_t id) const;
    const Track* At(int index) const;
    Track* At(int index);
    const std::vector<Track>& Tracks() const { return tracks_; }
private:
    uint32_t Random();
    void ResetNavigation();
    std::vector<Track> tracks_;
    uint64_t nextId_ = 1, current_ = 0;
    uint32_t rng_;
    std::vector<uint64_t> visited_, history_;
};
