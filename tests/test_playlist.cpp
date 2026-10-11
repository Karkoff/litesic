#include "playlist.h"
#include "check.h"
#include <set>
#include <algorithm>
static Playlist Three() { Playlist p(123); p.Add(L"c.wav");p.Add(L"a.mp3");p.Add(L"b.flac");return p; }
static void add_remove_identity() {
    auto p=Three(); auto id=p.At(1)->id; CHECK(p.SetCurrentIndex(1));
    CHECK(p.RemoveAt(0)); CHECK(p.GetCurrentIndex()==0); CHECK(p.At(0)->id==id);
    CHECK(p.RemoveAt(0)); CHECK(p.GetCurrentIndex()==-1); CHECK(p.Count()==1);
}
static void invalid_operations() {
    Playlist p; CHECK(!p.Add(L"")); CHECK(!p.RemoveAt(-1)); CHECK(!p.SetCurrentIndex(0));
    CHECK(!p.MoveTo(0,0)); CHECK(p.Next(true,true)==-1); CHECK(p.Previous(false,true)==-1);
    p.Add(L"file.wav"); CHECK(!p.MoveTo(0,1)); CHECK(!p.RemoveAt(1)); CHECK(!p.At(1));
}
static void reorder_current() {
    auto p=Three(); p.SetCurrentIndex(0); auto id=p.At(0)->id;
    CHECK(p.MoveTo(0,2)); CHECK(p.GetCurrentIndex()==2); CHECK(p.At(2)->id==id);
    CHECK(p.MoveTo(2,0)); CHECK(p.GetCurrentIndex()==0);
}
static void multiple_reorder() {
    auto p=Three(); p.Add(L"d.ogg"); auto ids=std::vector<uint64_t>{p.At(0)->id,p.At(2)->id};
    p.SetCurrentIndex(2); p.MoveSelection({0,2},4);
    CHECK(p.At(2)->id==ids[0]); CHECK(p.At(3)->id==ids[1]); CHECK(p.GetCurrentIndex()==3);
    p.MoveSelection({2,3},0); CHECK(p.At(0)->id==ids[0]); CHECK(p.GetCurrentIndex()==1);
}
static void sequential_limits() {
    auto p=Three(); CHECK(p.Previous()==-1); CHECK(p.Next(false,false)==0);
    CHECK(p.Previous()==-1); CHECK(p.Next(false,false)==1); CHECK(p.Next(false,false)==2);
    CHECK(p.Next(false,false)==-1); CHECK(p.GetCurrentIndex()==2); CHECK(p.Previous()==1);
}
static void sequential_repeat() {
    auto p=Three();p.SetCurrentIndex(2);CHECK(p.Next(false,true)==0);CHECK(p.Previous(false,true)==2);
    Playlist one;one.Add(L"a.wav");CHECK(one.Next(true,true)==0);CHECK(one.Next(true,true)==0);
}
static void shuffled_cycle() {
    auto p=Three(); p.SetCurrentIndex(0); std::set<uint64_t> ids{p.At(0)->id};
    for(int i=0;i<2;++i) {int n=p.Next(true,false);CHECK(n>=0);CHECK(ids.insert(p.At(n)->id).second);}
    CHECK(p.Next(true,false)==-1); auto last=p.At(p.GetCurrentIndex())->id;
    int n=p.Next(true,true); CHECK(n>=0); CHECK(p.At(n)->id!=last);
}
static void shuffled_history_and_add() {
    auto p=Three();p.SetCurrentIndex(0);CHECK(p.Next(true,false)>=0);CHECK(p.Previous(true)==0);
    p.Add(L"new.wav"); CHECK(p.Next(true,false)>=0); CHECK(p.Next(true,false)>=0);CHECK(p.Next(true,false)==-1);
}
static void sort_and_visible_shuffle() {
    auto p=Three();p.SetCurrentIndex(0);auto id=p.At(0)->id;
    p.Sort(false,true);CHECK(p.At(0)->path==L"a.mp3");CHECK(p.GetCurrentIndex()==2);
    p.Sort(false,false);CHECK(p.At(0)->id==id);
    p.At(0)->duration=5;p.At(1)->duration=10;p.At(2)->duration=1;
    p.Sort(true,true);CHECK(p.At(0)->duration==1);p.Sort(true,false);CHECK(p.At(0)->duration==10);
    p.ShuffleVisible();CHECK(p.At(p.GetCurrentIndex())->id==id);
    p.Clear();CHECK(p.Count()==0);CHECK(p.GetCurrentIndex()==-1);
}
static void selection_range_toggle() {
    CHECK(SelectRows({},10,5,2,false,true)==std::vector<int>({2,3,4,5}));
    CHECK(SelectRows({},10,2,5,false,true)==std::vector<int>({2,3,4,5}));
    CHECK(SelectRows({2,3},10,2,2,true,false)==std::vector<int>({3}));
    CHECK(SelectRows({0},10,5,3,true,true)==std::vector<int>({0,3,4,5}));
    CHECK(SelectRows({-1,11,2},10,50,0,false,false)==std::vector<int>({2}));
}
static void extensions_and_titles() {
    CHECK(IsSupportedAudioFile(L"C:\\áudio\\song.MP3"));CHECK(IsSupportedAudioFile(L"x.FlAc"));
    CHECK(IsSupportedAudioFile(L"x.wav"));CHECK(IsSupportedAudioFile(L"x.OGG"));
    CHECK(!IsSupportedAudioFile(nullptr));CHECK(!IsSupportedAudioFile(L"x.mp3.exe"));
    CHECK(!IsSupportedAudioFile(L"folder.mp3\\song"));CHECK(FileTitle(L"C:\\p\\canção.live.mp3")==L"canção.live");
}
int main(){int cases=0;CASE(add_remove_identity);CASE(invalid_operations);CASE(reorder_current);CASE(multiple_reorder);
    CASE(sequential_limits);CASE(sequential_repeat);CASE(shuffled_cycle);CASE(shuffled_history_and_add);
    CASE(sort_and_visible_shuffle);CASE(selection_range_toggle);CASE(extensions_and_titles);std::printf("%d playlist cases\n",cases);}
