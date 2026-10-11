#include "audio.h"
#include "eq_state.h"
#include "check.h"
#include <vector>
#include <cmath>
static double RMS(const std::vector<float>& v,unsigned channel=0){double sum=0;for(unsigned i=v.size()/4;i<v.size()/2;++i)sum+=v[i*2+channel]*v[i*2+channel];return std::sqrt(sum/(v.size()/4));}
static std::vector<float> Render(const wchar_t* file,bool enabled,float gain=0,float volume=1,float pan=0,unsigned rate=48000){
    Eq::ApplyPreset(0);Eq::SetEnabled(enabled);Eq::SetGain(4,gain);CHECK(Audio_TestInit(rate));CHECK(Audio_VizInit());
    CHECK(Audio_Load(file));CHECK(Audio_GetSampleRate()==int(rate));Audio_SetVolume(volume);Audio_SetPan(pan);Audio_Play();std::vector<float> out(rate);CHECK(Audio_TestRead(out.data(),rate/2));
    float viz[2048];CHECK(Audio_GetVisualizationSamples(viz,2048)==2048);double v=0;for(float x:viz)v+=x*x;
    CHECK(std::isfinite(v)&&v>0);Audio_Shutdown();return out;
}
static void codecs_and_mono_use_real_node(){for(unsigned rate:{32000u,44100u,48000u,96000u})for(const wchar_t* file:{L"tests\\eq_fixtures\\eq-tone-1.wav",L"tests\\eq_fixtures\\eq-tone-2.wav",L"tests\\eq_fixtures\\eq-tone.mp3",L"tests\\eq_fixtures\\eq-tone.flac",L"tests\\eq_fixtures\\eq-tone.ogg"}){
    auto dry=Render(file,false,0,1,0,rate);auto flat=Render(file,true,0,1,0,rate);auto boost=Render(file,true,6,1,0,rate);auto cut=Render(file,true,-6,1,0,rate);
    CHECK(RMS(dry)>.001);CHECK(std::abs(RMS(flat)/RMS(dry)-1)<1e-6);
    CHECK(std::abs(20*std::log10(RMS(boost)/RMS(dry))-6)<.1);CHECK(std::abs(20*std::log10(RMS(cut)/RMS(dry))+6)<.1);}}
static void volume_pan_and_transport(){auto full=Render(L"tests\\eq_fixtures\\eq-tone-2.wav",true,6);auto quiet=Render(L"tests\\eq_fixtures\\eq-tone-2.wav",true,6,.25f);
    CHECK(std::abs(RMS(quiet)/RMS(full)-.25)<.001);auto left=Render(L"tests\\eq_fixtures\\eq-tone-2.wav",true,6,1,-1);CHECK(RMS(left,1)<1e-8);CHECK(RMS(left,0)>.001);
    Eq::ApplyPreset(1);Eq::SetEnabled(true);CHECK(Audio_TestInit(48000));CHECK(Audio_Load(L"tests\\eq_fixtures\\eq-tone-2.wav"));Audio_Play();
    float block[1024];for(int i=0;i<80;++i){Eq::ApplyPreset(i%Eq::PresetCount());Eq::SetEnabled(i%3);Eq::SetGain(-1,float(i%25-12));
        CHECK(Audio_TestRead(block,512));for(float v:block)CHECK(std::isfinite(v)&&std::abs(v)<2);
        if(i%10==0){Audio_Pause();double position=Audio_GetPositionSeconds();CHECK(Audio_TestRead(block,512));CHECK(Audio_GetPositionSeconds()==position);Audio_Play();}
        if(i%11==0)Audio_SeekSeconds(.2);
        if(i%13==0){Audio_Stop();CHECK(Audio_GetPositionSeconds()<.001);Audio_Play();}
        if(i%17==0){CHECK(Audio_Load(L"tests\\eq_fixtures\\eq-tone-1.wav"));Audio_Play();}}
    Audio_Shutdown();Eq::Configure(nullptr);}
int main(){int cases=0;CASE(codecs_and_mono_use_real_node);CASE(volume_pan_and_transport);std::printf("%d real EQ audio cases\n",cases);}

