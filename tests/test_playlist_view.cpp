#include "playlist_view.h"
#include "check.h"
static void filter_preserves_canonical_playback() {
    Playlist list; list.Add(L"Alpha.wav"); list.Add(L"Beta.wav"); list.Add(L"Gamma.wav"); list.SetCurrentIndex(0);
    PlaylistView view; view.Update(list,L"BETA"); CHECK(view.Count()==1); CHECK(view.Canonical(0)==1);
    CHECK(list.Count()==3); CHECK(list.GetCurrentIndex()==0); CHECK(list.Next(false,false)==1);
    view.Update(list,L""); CHECK(view.Count()==3); CHECK(view.Row(2)==2); CHECK(view.Row(5)==-1);
}
static void unicode_metadata_and_empty_results() {
    Playlist list; list.Add(L"audio.flac"); list.At(0)->displayTitle=L"Música · Coração";
    PlaylistView view; view.Update(list,L"CORAÇÃO"); CHECK(view.Count()==1);
    view.Update(list,L"ausente"); CHECK(view.Count()==0); CHECK(view.Canonical(0)==-1);
    view.Update(list,L"audio"); CHECK(view.Count()==1); list.RemoveAt(0); view.Update(list,L""); CHECK(view.Count()==0);
}
static void favorites_share_canonical_entries(){
    Playlist list;list.Add(L"Alpha.wav");list.Add(L"Beta.wav");list.Add(L"Gamma.wav");
    list.At(1)->favorite=true;list.At(2)->favorite=true;list.SetCurrentIndex(0);
    PlaylistView view;view.Update(list,L"",true);CHECK(view.Count()==2&&view.Canonical(0)==1&&view.Canonical(1)==2);
    view.Update(list,L"BETA",true);CHECK(view.Count()==1&&view.Canonical(0)==1);
    CHECK(list.GetCurrentIndex()==0&&list.Count()==3);list.MoveTo(1,0);
    view.Update(list,L"",true);CHECK(view.Canonical(0)==0&&list.At(0)->favorite);
    list.At(0)->favorite=false;view.Update(list,L"",true);CHECK(view.Count()==1);
    view.Update(list,L"");CHECK(view.Count()==3);
}
int main() { int cases=0; CASE(filter_preserves_canonical_playback); CASE(unicode_metadata_and_empty_results);CASE(favorites_share_canonical_entries); std::printf("%d playlist view cases\n",cases); }
