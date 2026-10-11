#include "playback.h"
#include "check.h"
#include <cwchar>
static AudioState state;static bool ended;static double position;static int loads;static bool broken;
static BOOL Load(const wchar_t* p){++loads;state=AUDIO_STATE_STOPPED;position=0;ended=false;return !broken && !wcsstr(p,L"bad");}
static void Play(){state=AUDIO_STATE_PLAYING;}static void Stop(){state=AUDIO_STATE_STOPPED;position=0;}
static void Unload(){Stop();}static void Seek(double p){position=p;ended=false;}static double Position(){return position;}
static AudioState State(){return state;}static BOOL End(){return ended;}
static PlaybackBackend Backend(){state=AUDIO_STATE_STOPPED;ended=false;position=0;loads=0;broken=false;return {Load,Play,Stop,Unload,Seek,Position,State,End};}
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
int main(){int cases=0;CASE(end_advances_stops);CASE(repeat_wrap_and_previous);CASE(missing_skip_and_all_bad_bound);
    CASE(paused_no_advance_and_remove);CASE(failed_explicit_and_clear);CASE(repeat_modes_cycle);
    CASE(repeat_one_eof_and_manual_navigation);CASE(repeat_one_preserves_shuffle_cycle);std::printf("%d playback cases\n",cases);}
