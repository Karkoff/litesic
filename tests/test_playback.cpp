#include "playback.h"
#include "check.h"
#include <cwchar>
static AudioState state;static bool ended;static double position;static int loads;static bool broken;static bool noOutput,silent;
static BOOL Load(const wchar_t* p){++loads;state=AUDIO_STATE_STOPPED;position=0;ended=false;return !noOutput && !broken && !wcsstr(p,L"bad");}
static void Play(){if(!silent)state=AUDIO_STATE_PLAYING;}static void Stop(){state=AUDIO_STATE_STOPPED;position=0;}
static void Unload(){Stop();}static void Seek(double p){position=p;ended=false;}static double Position(){return position;}
static AudioState State(){return state;}static BOOL End(){return ended;}static BOOL Output(){return !noOutput;}
static PlaybackBackend Backend(){state=AUDIO_STATE_STOPPED;ended=false;position=0;loads=0;broken=false;noOutput=silent=false;return {Load,Play,Stop,Unload,Seek,Position,State,End,Output};}
static void end_advances_stops(){Playlist p;p.Add(L"a.wav");p.Add(L"b.wav");PlaybackController c(p,Backend());
    CHECK(c.Play());CHECK(p.GetCurrentIndex()==0);ended=true;CHECK(c.Update(false,RepeatMode::Off));CHECK(p.GetCurrentIndex()==1);
    ended=true;CHECK(c.Update(false,RepeatMode::Off));CHECK(state==AUDIO_STATE_STOPPED);CHECK(!c.Update(false,RepeatMode::Off));CHECK(loads==2);}
static void repeat_wrap_and_previous(){Playlist p;p.Add(L"a.wav");p.Add(L"b.wav");PlaybackController c(p,Backend());
    CHECK(c.PlayIndex(1));ended=true;CHECK(c.Update(false,RepeatMode::All));CHECK(p.GetCurrentIndex()==0);
    position=4;CHECK(c.Previous(false,true));CHECK(position==0);CHECK(p.GetCurrentIndex()==0);
    CHECK(c.Previous(false,true));CHECK(p.GetCurrentIndex()==1);}
static void missing_skip_and_all_bad_bound(){Playlist p;p.Add(L"a.wav");p.Add(L"bad.wav");p.Add(L"b.wav");PlaybackController c(p,Backend());
    CHECK(c.Play());CHECK(c.Next(false,false));CHECK(p.GetCurrentIndex()==2);CHECK(p.At(1)->failed);
    broken=true;loads=0;CHECK(!c.Next(true,true));CHECK(loads<=p.Count());CHECK(state==AUDIO_STATE_STOPPED);}
static void paused_no_advance_and_remove(){Playlist p;p.Add(L"a.wav");p.Add(L"b.wav");PlaybackController c(p,Backend());
    CHECK(c.Play());state=AUDIO_STATE_PAUSED;ended=true;CHECK(!c.Update(false,RepeatMode::All));CHECK(p.GetCurrentIndex()==0);
    c.RemoveAt(1);CHECK(state==AUDIO_STATE_PAUSED);c.RemoveAt(0);CHECK(state==AUDIO_STATE_STOPPED);CHECK(p.Count()==0);CHECK(!c.Play());}
static void failed_explicit_and_clear(){Playlist p;p.Add(L"bad.wav");PlaybackController c(p,Backend());CHECK(!c.PlayIndex(0));CHECK(p.At(0)->failed);
    CHECK(!c.PlayIndex(-1));c.Clear();CHECK(p.Count()==0);CHECK(!c.Next(true,true));}
// A missing output is not the files' fault: no track is marked failed, and
// navigation stops at the first attempt instead of loading the whole list.
static void missing_output_neither_marks_nor_scans(){
    Playlist p;for(auto name:{L"a.wav",L"b.wav",L"c.wav",L"d.wav"})p.Add(name);PlaybackController c(p,Backend());
    noOutput=true;CHECK(!c.PlayIndex(1));CHECK(!p.At(1)->failed);CHECK(loads==1);
    loads=0;CHECK(!c.Next(false,true));CHECK(loads==1);CHECK(state==AUDIO_STATE_STOPPED);
    loads=0;CHECK(!c.Previous(false,true));CHECK(loads==1);
    for(int i=0;i<p.Count();++i)CHECK(!p.At(i)->failed);
    noOutput=false;silent=true;loads=0;CHECK(!c.Next(false,true));CHECK(loads==1);
    for(int i=0;i<p.Count();++i)CHECK(!p.At(i)->failed);
    silent=false;CHECK(c.Play());CHECK(state==AUDIO_STATE_PLAYING); // Output back: same selection plays.
}
static void repeat_modes_cycle(){
    CHECK(NextRepeatMode(RepeatMode::Off)==RepeatMode::All);
    CHECK(NextRepeatMode(RepeatMode::All)==RepeatMode::One);
    CHECK(NextRepeatMode(RepeatMode::One)==RepeatMode::Off);
}
static void repeat_one_eof_and_manual_navigation(){
    Playlist p;p.Add(L"a.wav");p.Add(L"b.wav");PlaybackController c(p,Backend());
    CHECK(c.PlayIndex(1));
    for(int i=0;i<3;++i){position=120;ended=true;CHECK(c.Update(false,RepeatMode::One));
        CHECK(p.GetCurrentIndex()==1);CHECK(position==0);CHECK(!ended);CHECK(state==AUDIO_STATE_PLAYING);}
    CHECK(loads==1); // No reload or reset of navigation on EOF.
    state=AUDIO_STATE_PAUSED;ended=true;CHECK(!c.Update(true,RepeatMode::One));
    CHECK(c.Previous(false,false));CHECK(p.GetCurrentIndex()==0);
    CHECK(c.Next(false,false));CHECK(p.GetCurrentIndex()==1);
    ended=true;CHECK(c.Update(false,RepeatMode::Off));CHECK(state==AUDIO_STATE_STOPPED);
}
static void repeat_one_preserves_shuffle_cycle(){
    Playlist p(1);p.Add(L"a.wav");p.Add(L"b.wav");p.Add(L"c.wav");PlaybackController c(p,Backend());
    CHECK(c.Play());CHECK(c.Next(true,false));int current=p.GetCurrentIndex();
    ended=true;CHECK(c.Update(true,RepeatMode::One));CHECK(p.GetCurrentIndex()==current);
    CHECK(c.Previous(true,false));CHECK(p.GetCurrentIndex()==0);
    c.Clear();ended=true;state=AUDIO_STATE_PLAYING;
    CHECK(c.Update(true,RepeatMode::One));CHECK(state==AUDIO_STATE_STOPPED);
}
static void favorites_sequence_repeat_and_general_scope(){
    Playlist p;for(auto name:{L"general0.wav",L"fav1.wav",L"general2.wav",L"fav3.wav",L"general4.wav"})p.Add(name);
    p.At(1)->favorite=p.At(3)->favorite=true;PlaybackController c(p,Backend());
    CHECK(c.PlayIndex(1,true));ended=true;CHECK(c.Update(false,RepeatMode::Off));CHECK(p.GetCurrentIndex()==3);
    ended=true;CHECK(c.Update(false,RepeatMode::Off));CHECK(state==AUDIO_STATE_STOPPED&&p.GetCurrentIndex()==3);
    CHECK(c.PlayIndex(3,true));ended=true;CHECK(c.Update(false,RepeatMode::All));CHECK(p.GetCurrentIndex()==1);
    int before=loads;ended=true;CHECK(c.Update(false,RepeatMode::One));CHECK(p.GetCurrentIndex()==1&&loads==before);
    CHECK(c.Previous(false,true));CHECK(p.GetCurrentIndex()==3);
    CHECK(c.Previous(false,false));CHECK(p.GetCurrentIndex()==1);
    CHECK(c.PlayIndex(1,false));ended=true;CHECK(c.Update(false,RepeatMode::Off));CHECK(p.GetCurrentIndex()==2);
    CHECK(c.Play(true));CHECK(p.GetCurrentIndex()==1); // Current general track cannot leak into Favorites.
    CHECK(!c.PlayIndex(2,true));CHECK(p.GetCurrentIndex()==1);
    p.At(1)->favorite=p.At(3)->favorite=false;CHECK(!c.Play(true));CHECK(state==AUDIO_STATE_STOPPED);
}
static void favorites_shuffle_history_and_edits(){
    Playlist p(1);for(int i=0;i<6;++i)p.Add(L"song.wav");
    for(int i:{1,3,5})p.At(i)->favorite=true;
    PlaybackController c(p,Backend());CHECK(c.PlayIndex(1,true));
    bool seen[6]{};seen[1]=true;
    for(int i=0;i<2;++i){CHECK(c.Next(true,false));int current=p.GetCurrentIndex();CHECK(p.At(current)->favorite&&!seen[current]);seen[current]=true;}
    CHECK(!c.Next(true,false));CHECK(state==AUDIO_STATE_STOPPED);
    CHECK(c.Play());ended=true;CHECK(c.Update(true,RepeatMode::All));CHECK(p.At(p.GetCurrentIndex())->favorite);
    CHECK(c.PlayIndex(1,true));CHECK(c.Next(true,false));CHECK(c.Previous(true,false));CHECK(p.GetCurrentIndex()==1);
    p.At(1)->favorite=false;ended=true;CHECK(c.Update(false,RepeatMode::One));CHECK(p.GetCurrentIndex()==3);
    c.RemoveAt(3);CHECK(c.Next(false,false));CHECK(p.At(p.GetCurrentIndex())->favorite);
    CHECK(!c.Next(false,false));CHECK(state==AUDIO_STATE_STOPPED);
}
static void favorites_skip_invalid_and_bound_repeat(){
    Playlist p;p.Add(L"general.wav");p.Add(L"bad.wav");p.Add(L"fav.wav");p.Add(L"general2.wav");
    p.At(1)->favorite=p.At(2)->favorite=true;PlaybackController c(p,Backend());
    CHECK(!c.PlayIndex(1,true));CHECK(c.Next(false,false));CHECK(p.GetCurrentIndex()==2);
    broken=true;loads=0;CHECK(!c.Next(true,true));CHECK(loads<=p.Count());CHECK(state==AUDIO_STATE_STOPPED);
    p.At(1)->favorite=p.At(2)->favorite=false;loads=0;
    CHECK(!c.Next(true,true));CHECK(loads==0);CHECK(!c.Previous(false,true));
}
int main(){int cases=0;CASE(end_advances_stops);CASE(repeat_wrap_and_previous);CASE(missing_skip_and_all_bad_bound);
    CASE(paused_no_advance_and_remove);CASE(failed_explicit_and_clear);CASE(repeat_modes_cycle);
    CASE(repeat_one_eof_and_manual_navigation);CASE(repeat_one_preserves_shuffle_cycle);
    CASE(favorites_sequence_repeat_and_general_scope);CASE(favorites_shuffle_history_and_edits);CASE(favorites_skip_invalid_and_bound_repeat);
    CASE(missing_output_neither_marks_nor_scans);
    std::printf("%d playback cases\n",cases);}
