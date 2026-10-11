#include "playlist_view.h"
#include <windows.h>
#include <algorithm>

static std::wstring Lower(std::wstring value) {
    if(!value.empty()) CharLowerBuffW(value.data(),static_cast<DWORD>(value.size()));
    return value;
}
void PlaylistView::Update(const Playlist& list,const std::wstring& query,bool favoritesOnly) {
    indices_.clear(); indices_.reserve(list.Count());
    auto needle=Lower(query);
    for(int i=0;i<list.Count();++i) {
        const Track& track=*list.At(i);
        if(favoritesOnly&&!track.favorite)continue;
        if(needle.empty() || Lower(track.displayTitle).find(needle)!=std::wstring::npos ||
            Lower(FileTitle(track.path)).find(needle)!=std::wstring::npos) indices_.push_back(i);
    }
}
int PlaylistView::Row(int canonical) const {
    auto found=std::find(indices_.begin(),indices_.end(),canonical);
    return found==indices_.end()?-1:static_cast<int>(found-indices_.begin());
}
