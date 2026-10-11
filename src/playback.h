#pragma once
#include "playlist.h"
#include "audio.h"

// Injected transport lets unit tests exercise this controller's real state
// transitions without duplicating its rules or opening a sound device.
struct PlaybackBackend {
    BOOL (*load)(const wchar_t*);
    void (*play)();
    void (*stop)();
    void (*unload)();
    void (*seek)(double);
    double (*position)();
    AudioState (*state)();
    BOOL (*atEnd)();
};
enum class RepeatMode { Off, All, One };
constexpr RepeatMode NextRepeatMode(RepeatMode mode) {
    return mode == RepeatMode::Off ? RepeatMode::All :
           mode == RepeatMode::All ? RepeatMode::One : RepeatMode::Off;
}
class PlaybackController {
public:
    PlaybackController(Playlist& list, PlaybackBackend backend) : list_(list), backend_(backend) {}
    bool PlayIndex(int index, bool favoritesOnly = false);
    bool Play();
    bool Play(bool favoritesOnly);
    bool Next(bool shuffle, bool repeat);
    bool Previous(bool shuffle, bool repeat);
    bool Update(bool shuffle, RepeatMode repeat);
    void RemoveAt(int index);
    void Clear();
private:
    bool LoadIndex(int index, bool resetNavigation);
    void SetScope(bool favoritesOnly);
    Playlist& list_;
    PlaybackBackend backend_;
    uint64_t loaded_ = 0;
    bool favoritesOnly_ = false;
};
Playlist& PlayerPlaylist();
PlaybackController& PlayerPlayback();
