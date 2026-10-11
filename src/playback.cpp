#include "playback.h"

bool PlaybackController::LoadIndex(int index, bool resetNavigation) {
    Track* track = list_.At(index);
    if (!track) return false;
    if (resetNavigation) list_.SetCurrentIndex(index);
    if (!backend_.load(track->path.c_str())) {
        track->failed = true; loaded_ = 0; return false;
    }
    track->failed = false;
    loaded_ = track->id;
    backend_.play();
    return backend_.state() == AUDIO_STATE_PLAYING;
}
bool PlaybackController::PlayIndex(int index) { return LoadIndex(index, true); }
bool PlaybackController::Play() {
    int index = list_.GetCurrentIndex();
    if (index < 0) index = 0;
    const Track* track = list_.At(index);
    if (!track) return false;
    if (loaded_ != track->id) return PlayIndex(index);
    if (backend_.atEnd()) { backend_.stop(); backend_.seek(0); }
    backend_.play();
    return backend_.state() == AUDIO_STATE_PLAYING;
}
bool PlaybackController::Next(bool shuffle, bool repeat) {
    // Missing/corrupt entries are skipped, with a strict one-list bound even
    // under repeat. A list of broken files cannot spin the UI thread forever.
    for (int attempts = 0; attempts < list_.Count(); ++attempts) {
        int index = list_.Next(shuffle, repeat);
        if (index < 0) break;
        if (LoadIndex(index, false)) return true;
    }
    backend_.stop();
    return false;
}
bool PlaybackController::Previous(bool shuffle, bool repeat) {
    if (loaded_ && backend_.position() > 3) { backend_.seek(0); return true; }
    for (int attempts = 0; attempts < list_.Count(); ++attempts) {
        int index = list_.Previous(shuffle, repeat);
        if (index < 0) return false;
        if (LoadIndex(index, false)) return true;
    }
    backend_.stop(); return false;
}
bool PlaybackController::Update(bool shuffle, RepeatMode repeat) {
    if (backend_.state() != AUDIO_STATE_PLAYING || !backend_.atEnd()) return false;
    const Track* track = list_.At(list_.GetCurrentIndex());
    // Repeat-one is an EOF policy. Explicit navigation remains independent,
    // and restarting the loaded sound preserves shuffle's visit/history state.
    if (repeat == RepeatMode::One && track && loaded_ == track->id) {
        // The sound has ended internally, while the transport still reports
        // PLAYING until this timer update. Stop first so Play restarts it.
        backend_.stop();
        backend_.seek(0);
        backend_.play();
    } else Next(shuffle, repeat == RepeatMode::All);
    return true;
}
void PlaybackController::RemoveAt(int index) {
    const Track* track = list_.At(index);
    if (track && track->id == loaded_) { backend_.unload(); loaded_ = 0; }
    list_.RemoveAt(index);
}
void PlaybackController::Clear() { backend_.unload(); loaded_ = 0; list_.Clear(); }

Playlist& PlayerPlaylist() { static Playlist list(GetTickCount() | 1u); return list; }
PlaybackController& PlayerPlayback() {
    static PlaybackController controller(PlayerPlaylist(), {
        Audio_Load, Audio_Play, Audio_Stop, Audio_Unload, Audio_SeekSeconds,
        Audio_GetPositionSeconds, Audio_GetState, Audio_IsAtEnd});
    return controller;
}
