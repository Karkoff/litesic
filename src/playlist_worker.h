#pragma once
#include <windows.h>
#include <deque>
#include "tags.h"
#include "playlist.h"

constexpr UINT WM_PLAYLIST_RESULTS = WM_APP + 55;
struct PlaylistJob {
    enum Kind { Scan, Metadata, Open, Save } kind = Metadata;
    LONG generation = 0;
    uint64_t id = 0;
    std::wstring path;
    std::vector<Track> tracks;
};
struct PlaylistResult {
    PlaylistJob::Kind kind = PlaylistJob::Metadata;
    LONG generation = 0;
    uint64_t id = 0;
    std::vector<Track> tracks;
    TrackMetadata metadata;
    std::wstring error;
};
// A single worker owns disk I/O and decoder probes. Only the UI owns Playlist.
// Bounded result queue prevents unlimited messages/memory if the UI is busy.
class PlaylistWorker {
public:
    bool Start(HWND destination);
    void Stop();
    bool Submit(PlaylistJob job);
    LONG NewGeneration();
    LONG Generation() const { return InterlockedCompareExchange(&generation_, 0, 0); }
    bool Busy() const { return InterlockedCompareExchange(&pending_, 0, 0) != 0; }
    std::vector<PlaylistResult> Drain();
    ~PlaylistWorker() { Stop(); }
    PlaylistWorker() = default;
    PlaylistWorker(const PlaylistWorker&) = delete;
    PlaylistWorker& operator=(const PlaylistWorker&) = delete;
private:
    static DWORD WINAPI Entry(void* self);
    void Run();
    void ScanFolder(const PlaylistJob& job);
    bool Cancelled(LONG generation) const;
    bool Publish(PlaylistResult result);
    bool Notify();
    CRITICAL_SECTION lock_{};
    HANDLE thread_ = nullptr, stop_ = nullptr, ready_ = nullptr, space_ = nullptr;
    HWND destination_ = nullptr;
    bool initialized_ = false;
    bool notificationPending_ = false;
    mutable volatile LONG generation_ = 1, pending_ = 0;
    std::deque<PlaylistJob> jobs_;
    std::deque<PlaylistResult> results_;
};
