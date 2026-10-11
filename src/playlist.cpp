#include "playlist.h"
#include <algorithm>
#include <cwctype>

std::wstring FileTitle(const std::wstring& path) {
    auto start = path.find_last_of(L"/\\");
    std::wstring name = path.substr(start == std::wstring::npos ? 0 : start + 1);
    auto dot = name.find_last_of(L'.');
    if (dot != std::wstring::npos && dot != 0) name.resize(dot);
    return name;
}
bool IsSupportedAudioFile(const wchar_t* path) {
    if (!path) return false;
    std::wstring value(path);
    auto dot = value.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;
    value = value.substr(dot);
    for (auto& ch : value) ch = static_cast<wchar_t>(towlower(ch));
    return value == L".mp3" || value == L".wav" || value == L".flac" || value == L".ogg";
}
std::vector<int> SelectRows(std::vector<int> selected, int count, int clicked,
                            int anchor, bool ctrl, bool shift) {
    selected.erase(std::remove_if(selected.begin(), selected.end(),
        [count](int i) { return i < 0 || i >= count; }), selected.end());
    if (clicked < 0 || clicked >= count) return selected;
    if (shift && anchor >= 0 && anchor < count) {
        if (!ctrl) selected.clear();
        for (int i = std::min(anchor, clicked); i <= std::max(anchor, clicked); ++i)
            selected.push_back(i);
    } else if (ctrl) {
        auto it = std::find(selected.begin(), selected.end(), clicked);
        if (it == selected.end()) selected.push_back(clicked); else selected.erase(it);
    } else selected = {clicked};
    std::sort(selected.begin(), selected.end());
    selected.erase(std::unique(selected.begin(), selected.end()), selected.end());
    return selected;
}
const Track* Playlist::At(int index) const {
    return index >= 0 && index < Count() ? &tracks_[index] : nullptr;
}
Track* Playlist::At(int index) {
    return index >= 0 && index < Count() ? &tracks_[index] : nullptr;
}
int Playlist::Find(uint64_t id) const {
    if (id) for (int i = 0; i < Count(); ++i) if (tracks_[i].id == id) return i;
    return -1;
}
uint32_t Playlist::Random() {
    rng_ ^= rng_ << 13; rng_ ^= rng_ >> 17; rng_ ^= rng_ << 5;
    return rng_;
}
void Playlist::ResetNavigation() { visited_.clear(); history_.clear(); }
uint64_t Playlist::Add(const std::wstring& path) {
    if (path.empty()) return 0;
    Track track;
    track.id = nextId_++; track.path = path; track.displayTitle = FileTitle(path);
    tracks_.push_back(track);
    return track.id;
}
bool Playlist::SetCurrentIndex(int index) {
    if (!At(index)) return false;
    current_ = tracks_[index].id;
    ResetNavigation();
    return true;
}
bool Playlist::RemoveAt(int index) {
    if (!At(index)) return false;
    auto id = tracks_[index].id;
    if (current_ == id) current_ = 0;
    tracks_.erase(tracks_.begin() + index);
    visited_.erase(std::remove(visited_.begin(), visited_.end(), id), visited_.end());
    history_.erase(std::remove(history_.begin(), history_.end(), id), history_.end());
    return true;
}
bool Playlist::MoveTo(int from, int to) {
    if (!At(from) || !At(to)) return false;
    Track track = std::move(tracks_[from]);
    tracks_.erase(tracks_.begin() + from);
    tracks_.insert(tracks_.begin() + to, std::move(track));
    return true;
}
void Playlist::MoveSelection(const std::vector<int>& indices, int insertion) {
    insertion = std::max(0, std::min(insertion, Count()));
    std::vector<Track> moved, kept;
    int target = insertion;
    for (int i = 0; i < Count(); ++i) {
        if (std::find(indices.begin(), indices.end(), i) != indices.end()) {
            moved.push_back(std::move(tracks_[i]));
            if (i < insertion) --target;
        } else kept.push_back(std::move(tracks_[i]));
    }
    kept.insert(kept.begin() + target, moved.begin(), moved.end());
    tracks_ = std::move(kept);
}
void Playlist::Sort(bool duration, bool ascending) {
    std::stable_sort(tracks_.begin(), tracks_.end(), [=](const Track& a, const Track& b) {
        if (duration) return ascending ? a.duration < b.duration : a.duration > b.duration;
        auto less = [](const std::wstring& left, const std::wstring& right) {
            return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end(),
                [](wchar_t x, wchar_t y) { return towlower(x) < towlower(y); });
        };
        return ascending ? less(a.displayTitle, b.displayTitle) : less(b.displayTitle, a.displayTitle);
    });
}
void Playlist::ShuffleVisible() {
    for (int i = Count() - 1; i > 0; --i)
        std::swap(tracks_[i], tracks_[Random() % (i + 1)]);
}
void Playlist::Clear() { tracks_.clear(); current_ = 0; ResetNavigation(); }
int Playlist::Next(bool shuffle, bool repeat, bool favoritesOnly) {
    if (!Count()) return -1;
    int current = GetCurrentIndex(), next = -1;
    auto eligible=[&](int i){return !favoritesOnly||tracks_[i].favorite;};
    int eligibleCount=0;for(int i=0;i<Count();++i)if(eligible(i))++eligibleCount;
    if(!eligibleCount)return -1;
    if (shuffle) {
        // One shuffled cycle visits every identity once. Repeat starts a new
        // cycle, excluding the just-played track from the first draw (if > 1).
        if (current >= 0 && std::find(visited_.begin(), visited_.end(), current_) == visited_.end())
            visited_.push_back(current_);
        std::vector<int> candidates;
        for (int i = 0; i < Count(); ++i)
            if (eligible(i)&&std::find(visited_.begin(), visited_.end(), tracks_[i].id) == visited_.end())
                candidates.push_back(i);
        if (candidates.empty()) {
            if (!repeat) return -1;
            visited_.clear();
            for (int i = 0; i < Count(); ++i)
                if (eligible(i)&&(eligibleCount == 1 || i != current)) candidates.push_back(i);
        }
        next = candidates[Random() % candidates.size()];
    } else {
        for(int i=current+1;i<Count();++i)if(eligible(i)){next=i;break;}
        if(next<0&&repeat)for(int i=0;i<Count();++i)if(eligible(i)){next=i;break;}
        if(next<0)return -1;
    }
    if (shuffle && current >= 0) {
        history_.push_back(current_);
        if (history_.size() > tracks_.size()) history_.erase(history_.begin());
    }
    current_ = tracks_[next].id;
    if (shuffle) visited_.push_back(current_);
    return next;
}
int Playlist::Previous(bool shuffle, bool repeat, bool favoritesOnly) {
    if (!Count()) return -1;
    if (shuffle) {
        while (!history_.empty()) {
            uint64_t id = history_.back(); history_.pop_back();
            int index=Find(id);
            if (index>=0&&(!favoritesOnly||tracks_[index].favorite)) { current_ = id; return index; }
        }
        return -1;
    }
    int previous=-1;
    for(int i=GetCurrentIndex()-1;i>=0;--i)if(!favoritesOnly||tracks_[i].favorite){previous=i;break;}
    if(previous<0&&repeat)for(int i=Count()-1;i>=0;--i)if(!favoritesOnly||tracks_[i].favorite){previous=i;break;}
    if(previous<0)return -1;
    current_ = tracks_[previous].id;
    return previous;
}
