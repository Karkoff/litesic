#pragma once
#include "playlist.h"

// Presentation mapping only. Playback retains the canonical Playlist indices.
class PlaylistView {
public:
    void Update(const Playlist& list,const std::wstring& query);
    int Count() const { return static_cast<int>(indices_.size()); }
    int Canonical(int row) const { return row>=0 && row<Count()?indices_[row]:-1; }
    int Row(int canonical) const;
    const std::vector<int>& Indices() const { return indices_; }
private:
    std::vector<int> indices_;
};
