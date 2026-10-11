#include "audio.h"
#include "playback.h"
#include "check.h"
#include <vector>
#include <cmath>
static void Render(unsigned frames){std::vector<float> pcm(frames*2);CHECK(Audio_TestRead(pcm.data(),frames));}
static void mismatched_rate_duration_seek_position(){
    CHECK(Audio_TestInit(48000));CHECK(Audio_Load(L"tests\\fixtures\\native-44100.wav"));
    CHECK(Audio_GetSampleRate()==48000);CHECK(Audio_GetFileSampleRate()==44100);
    CHECK(std::abs(Audio_GetDurationSeconds()-2)<0.00001);
    Audio_SeekSeconds(1);Audio_Play();Render(4800);
    CHECK(std::abs(Audio_GetPositionSeconds()-1.1)<0.02);
    CHECK(std::abs(static_cast<double>(Audio_TestCursor())/48000-1.1)<0.02);
    Audio_Pause();double paused=Audio_GetPositionSeconds();Render(4800);CHECK(Audio_GetPositionSeconds()==paused);
    Audio_Stop();Render(1);CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);CHECK(Audio_GetPositionSeconds()<0.001);
    Audio_Shutdown();
}
static void no_track_failure_and_reload(){CHECK(Audio_TestInit(48000));Audio_Play();CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);
    CHECK(!Audio_Load(L"absent.wav"));Audio_Play();CHECK(!Audio_HasTrack());CHECK(Audio_GetDurationSeconds()==0);
    CHECK(Audio_Load(L"tests\\fixtures\\native-48000.wav"));CHECK(Audio_HasTrack());
    CHECK(!Audio_Load(L"README.md"));CHECK(!Audio_HasTrack());CHECK(Audio_GetChannels()==0);
    CHECK(Audio_Load(L"tests\\fixtures\\native-44100.wav"));Audio_Unload();CHECK(!Audio_HasTrack());Audio_Shutdown();Audio_Shutdown();}
static void decoder_formats(){CHECK(Audio_TestInit(48000));
    for(const wchar_t* path:{L"tests\\fixtures\\tagged.mp3",L"tests\\fixtures\\tagged.flac",L"tests\\fixtures\\tagged.ogg"}) {
        CHECK(Audio_Load(path));CHECK(Audio_GetDurationSeconds()>0.24);Audio_Play();Render(4800);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
    }Audio_Shutdown();}
static void real_controller_natural_end(){CHECK(Audio_TestInit(48000));Playlist p;p.Add(L"tests\\fixtures\\tagged.flac");p.Add(L"tests\\fixtures\\native-44100.wav");
    PlaybackController c(p,{Audio_Load,Audio_Play,Audio_Stop,Audio_Unload,Audio_SeekSeconds,Audio_GetPositionSeconds,Audio_GetState,Audio_IsAtEnd});
    CHECK(c.Play());Render(24000);CHECK(Audio_IsAtEnd());CHECK(c.Update(false,RepeatMode::Off));CHECK(p.GetCurrentIndex()==1);
    Render(120000);CHECK(c.Update(false,RepeatMode::Off));CHECK(Audio_GetState()==AUDIO_STATE_STOPPED);
    Render(1);CHECK(Audio_GetPositionSeconds()<0.001);c.Clear();Audio_Shutdown();}
static DWORD WINAPI RenderThread(void*){Render(256);return 0;}
static void real_repeat_one_restarts_loaded_sound(){
    CHECK(Audio_TestInit(48000));Playlist p;p.Add(L"tests\\fixtures\\native-44100.wav");p.Add(L"tests\\fixtures\\native-48000.wav");
    PlaybackController c(p,{Audio_Load,Audio_Play,Audio_Stop,Audio_Unload,Audio_SeekSeconds,Audio_GetPositionSeconds,Audio_GetState,Audio_IsAtEnd});
    CHECK(c.Play());
    for(int i=0;i<3;++i){Render(120000);CHECK(Audio_IsAtEnd());CHECK(c.Update(true,RepeatMode::One));
        CHECK(p.GetCurrentIndex()==0);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);Render(4800);
        CHECK(std::abs(Audio_GetPositionSeconds()-0.1)<0.02);}
    CHECK(c.Next(false,false));CHECK(p.GetCurrentIndex()==1);c.Clear();Audio_Shutdown();
}
static void visualization_callback_does_not_wait(){CHECK(Audio_TestInit(48000));CHECK(Audio_VizInit());CHECK(Audio_Load(L"tests\\fixtures\\native-44100.wav"));Audio_Play();
    Audio_TestVizLock(TRUE);HANDLE thread=CreateThread(nullptr,0,RenderThread,nullptr,0,nullptr);CHECK(thread);
    DWORD wait=WaitForSingleObject(thread,2000);Audio_TestVizLock(FALSE);CHECK(wait==WAIT_OBJECT_0);CloseHandle(thread);Audio_Shutdown();}
static DWORD WINAPI ContinuousRender(void*) {
    for(int i=0;i<3000;++i)Render(256);
    return 0;
}
static void mp3_duration_queries_during_render(){
    CHECK(Audio_TestInit(48000));CHECK(Audio_Load(L"tests\\fixtures\\tagged.mp3"));
    double duration=Audio_GetDurationSeconds();CHECK(duration>.24);Audio_Play();
    HANDLE thread=CreateThread(nullptr,0,ContinuousRender,nullptr,0,nullptr);CHECK(thread);
    for(int i=0;i<10000;++i)CHECK(Audio_GetDurationSeconds()==duration);
    CHECK(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0);CloseHandle(thread);
    CHECK(Audio_IsAtEnd());Audio_Unload();CHECK(Audio_GetDurationSeconds()==0);
    CHECK(!Audio_Load(L"tests\\fixtures\\absent.mp3"));CHECK(Audio_GetDurationSeconds()==0);
    Audio_Shutdown();
}
static void real_eof_skips_invalid_file(){
    CHECK(Audio_TestInit(48000));Playlist p;
    p.Add(L"tests\\fixtures\\tagged.mp3");p.Add(L"README.md");p.Add(L"tests\\fixtures\\native-44100.wav");
    PlaybackController c(p,{Audio_Load,Audio_Play,Audio_Stop,Audio_Unload,Audio_SeekSeconds,Audio_GetPositionSeconds,Audio_GetState,Audio_IsAtEnd});
    CHECK(c.Play());Render(24000);CHECK(Audio_IsAtEnd());CHECK(c.Update(false,RepeatMode::Off));
    CHECK(p.At(1)->failed);CHECK(p.GetCurrentIndex()==2);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
    Render(4800);CHECK(Audio_GetPositionSeconds()>.09);c.Clear();Audio_Shutdown();
}
int main(){int cases=0;CASE(mismatched_rate_duration_seek_position);CASE(no_track_failure_and_reload);CASE(decoder_formats);
    CASE(real_controller_natural_end);CASE(real_repeat_one_restarts_loaded_sound);
    CASE(visualization_callback_does_not_wait);CASE(mp3_duration_queries_during_render);CASE(real_eof_skips_invalid_file);
    std::printf("%d real audio cases\n",cases);}
