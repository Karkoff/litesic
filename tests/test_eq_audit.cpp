#include "eq_dsp.h"
#include "eq_state.h"
#include "audio.h"
#include "ui_theme.h"
#include "check.h"
#include <vector>
#include <complex>
#include <cmath>
#include <cstring>
#include <string>

constexpr double tau=6.283185307179586;
// Measurement oracles consume production PCM, never duplicate coefficient math.
static double Tone(unsigned rate,const Eq::Parameters& p,double hz){
    Eq::Processor dsp;dsp.Initialize(rate,1,p);
    std::vector<float> in(rate*2),out(in.size());
    for(unsigned i=0;i<in.size();++i)in[i]=float(1e-5*std::sin(tau*hz*i/rate));
    dsp.Process(out.data(),in.data(),unsigned(in.size()));double dry=0,wet=0;
    for(unsigned i=rate;i<in.size();++i){dry+=double(in[i])*in[i];wet+=double(out[i])*out[i];}
    return 10*std::log10(wet/dry);
}
static double Impulse(unsigned rate,const Eq::Parameters& p,double hz){
    Eq::Processor dsp;dsp.Initialize(rate,1,p);std::vector<float> in(rate*2),out(in.size());
    in[0]=1e-4f;dsp.Process(out.data(),in.data(),unsigned(in.size()));std::complex<double> sum{};
    for(unsigned i=0;i<out.size();++i)sum+=double(out[i])*std::polar(1.,-tau*hz*i/rate);
    return 20*std::log10(std::abs(sum)/in[0]);
}
static void impulse_fourier_centers(){
    for(unsigned rate:{32000u,44100u,48000u,96000u})for(unsigned b=0;b<Eq::Bands;++b)
        if(Eq::Available(b,rate))for(float gain:{-12.f,-6.f,6.f,12.f}){
            Eq::Parameters p;p.enabled=true;p.gains[b]=gain;
            CHECK(std::abs(Impulse(rate,p,Eq::Frequencies[b])-gain)<.02);
        }
}
static void cascade_matches_measured_individual_filters(){
    for(unsigned rate:{32000u,44100u,48000u,96000u})for(int scenario=0;scenario<3;++scenario){
        Eq::Parameters p;p.enabled=true;p.preamp=scenario==2?-3.f:0.f;
        for(unsigned b=0;b<Eq::Bands;++b)p.gains[b]=scenario==0?12.f:scenario==1?-12.f:float(int(b)%5*3-6);
        for(double hz:{60.,1000.,8000.,14000.}){
            double expected=p.preamp;
            for(unsigned b=0;b<Eq::Bands;++b){Eq::Parameters single;single.enabled=true;single.gains[b]=p.gains[b];expected+=Tone(rate,single,hz);}
            double actual=Tone(rate,p,hz);CHECK(std::abs(actual-expected)<.05);
            CHECK(std::abs(Impulse(rate,p,hz)-actual)<.05);
            std::printf("MEASURE rate=%u scenario=%d hz=%.0f db=%.5f error=%.6f\n",rate,scenario,hz,actual,actual-expected);
        }
    }
}
static void preamp_positive_zero_negative_and_headroom(){
    for(unsigned rate:{32000u,44100u,48000u,96000u})for(float db:{-12.f,-6.f,0.f,6.f,12.f}){
        Eq::Parameters p;p.enabled=true;p.preamp=db;CHECK(std::abs(Tone(rate,p,1000)-db)<.00001);
    }
    Eq::Parameters p;p.enabled=true;p.preamp=12;Eq::Processor dsp;dsp.Initialize(48000,1,p);
    float in=.5f,out=0;dsp.Process(&out,&in,1);CHECK(out>1.9f&&out<2.f); // no hidden limiter
}
static void partition_inplace_reset_and_full_channel_isolation(){
    Eq::Parameters p;p.enabled=true;for(float& g:p.gains)g=6;
    std::vector<float> in(4096*Eq::MaxChannels),whole(in.size()),chunked(in.size());in[31]=.01f;
    Eq::Processor a,b;a.Initialize(48000,Eq::MaxChannels,p);b.Initialize(48000,Eq::MaxChannels,p);
    a.Process(whole.data(),in.data(),4096);chunked=in;
    for(unsigned at=0;at<4096;){unsigned n=std::min(37u,4096-at);b.Process(chunked.data()+at*32,chunked.data()+at*32,n);at+=n;}
    CHECK(!std::memcmp(whole.data(),chunked.data(),whole.size()*sizeof(float)));
    for(unsigned f=0;f<4096;++f)for(unsigned c=0;c<31;++c)CHECK(whole[f*32+c]==0);
    b.RequestReset();std::fill(in.begin(),in.end(),0);b.Process(chunked.data(),in.data(),4096);
    for(float v:chunked)CHECK(v==0);
}
static void preset_custom_roundtrip_before_engine_without_eq_window(){
    wchar_t tmp[MAX_PATH]{},file[MAX_PATH]{};CHECK(GetTempPathW(MAX_PATH,tmp));CHECK(GetTempFileNameW(tmp,L"leq",0,file));
    Ui::ThemeConfigure(file);Eq::SetEnabled(true);Eq::ApplyPreset(1);Eq::SetGain(0,2.5);Eq::SetGain(4,6);Eq::SetGain(-1,-2);
    auto expected=Eq::State();CHECK(Eq::Save());Ui::ThemeConfigure(nullptr);Ui::ThemeConfigure(file);
    CHECK(Eq::State().enabled&&Eq::State().preset==-1&&Eq::State().preamp==-2);
    for(unsigned b=0;b<10;++b)CHECK(Eq::State().gains[b]==expected.gains[b]);
    CHECK(Audio_TestInit(96000));CHECK(Audio_GetSampleRate()==96000);CHECK(Audio_Load(L"tests\\eq_fixtures\\eq-tone-2.wav"));
    CHECK(Audio_GetFileSampleRate()==44100);Audio_Play();std::vector<float> pcm(96000);CHECK(Audio_TestRead(pcm.data(),48000));
    double energy=0;for(unsigned i=48000;i<96000;++i)energy+=double(pcm[i])*pcm[i];CHECK(energy>1e-3);
    Audio_Shutdown();Eq::SetEnabled(false);CHECK(Eq::Save());Eq::Configure(file);CHECK(!Eq::State().enabled&&Eq::State().gains[4]==6);
    CHECK(WritePrivateProfileSectionW(L"Equalizer",L"Enabled=bad\0Band0=nan\0Band1=13\0Preset=Rock\0\0",file));
    Eq::Configure(file);CHECK(!Eq::State().enabled&&Eq::State().preamp==0&&Eq::State().gains[0]==0&&Eq::State().gains[1]==0&&Eq::State().preset==-1);
    Ui::ThemeConfigure(nullptr);CHECK(DeleteFileW(file));
}
static void rapid_sine_transitions_are_bounded_and_settle_to_bypass(){
    Eq::Processor dsp;Eq::Parameters p;p.enabled=true;p.preamp=6;p.gains[4]=6;
    dsp.Initialize(48000,1,p);float previous=0;double maxStep=0;unsigned at=0;
    float in[240],out[240];
    for(unsigned block=0;block<200;++block){
        if(block){p.enabled=block%7<5;p.preamp=block%3?6.f:-6.f;p.gains[4]=block%5?6.f:-6.f;dsp.Publish(p);}
        for(unsigned i=0;i<240;++i)in[i]=float(.001*std::sin(tau*1000*(at++)/48000));
        dsp.Process(out,in,240);
        for(float v:out){CHECK(std::isfinite(v));maxStep=std::max(maxStep,double(std::abs(v-previous)));previous=v;}
    }
    // Max steady amplitude <= 0.001 * 10^(12/20); max sine step 0.000521.
    // 0.0006 permits crossfade slope and state settling, without hiding full-scale spikes.
    CHECK(maxStep>.0002 && maxStep<.0006);std::printf("MEASURE transition_max_step=%.9f\n",maxStep);
    p.enabled=false;dsp.Publish(p);
    for(int b=0;b<12;++b){for(unsigned i=0;i<240;++i)in[i]=float(.001*std::sin(tau*1000*(at++)/48000));dsp.Process(out,in,240);}
    CHECK(!std::memcmp(in,out,sizeof(in)));
}
int main(){int cases=0;CASE(impulse_fourier_centers);CASE(cascade_matches_measured_individual_filters);
    CASE(preamp_positive_zero_negative_and_headroom);CASE(partition_inplace_reset_and_full_channel_isolation);
    CASE(preset_custom_roundtrip_before_engine_without_eq_window);CASE(rapid_sine_transitions_are_bounded_and_settle_to_bypass);
    std::printf("%d EQ audit cases\n",cases);}
