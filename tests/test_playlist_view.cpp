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
int main() { int cases=0; CASE(filter_preserves_canonical_playback); CASE(unicode_metadata_and_empty_results); std::printf("%d playlist view cases\n",cases); }
