#include "eq_dsp.h"
#include "eq_state.h"
#include "audio.h"
#include "ui_theme.h"
#include "check.h"
#include <vector>
#include <cmath>
#include <limits>
#include <cstring>
#include <string>

static double Response(unsigned rate,unsigned band,float gain,double frequency=0) {
    Eq::Processor dsp;Eq::Parameters p;p.enabled=true;p.gains[band]=gain;dsp.Initialize(rate,1,p);
    if(!frequency)frequency=Eq::Frequencies[band];
    std::vector<float> in(rate),out(rate);
    for(unsigned i=0;i<rate;++i)in[i]=float(.01*std::sin(6.283185307179586*frequency*i/rate));
    dsp.Process(out.data(),in.data(),rate);double a=0,b=0;
    for(unsigned i=rate/2;i<rate;++i){a+=in[i]*in[i];b+=out[i]*out[i];}
    return 10*std::log10(b/a);
}
static void all_centers_and_rates(){for(unsigned rate:{32000u,44100u,48000u,96000u})for(unsigned b=0;b<10;++b)
    if(Eq::Available(b,rate))for(float gain:{-12.f,-6.f,6.f,12.f})CHECK(std::abs(Response(rate,b,gain)-gain)<.08);}
static void octave_half_gain(){CHECK(std::abs(Response(96000,4,6,1000/std::sqrt(2.))-3)<.04);
    CHECK(std::abs(Response(96000,4,6,1000*std::sqrt(2.))-3)<.04);}
static void flat_bypass_and_preamp(){std::vector<float> in(4096),out(4096);for(unsigned i=0;i<in.size();++i)in[i]=float(std::sin(i*.27)*.01);
    Eq::Processor dsp;Eq::Parameters p;p.enabled=true;dsp.Initialize(48000,2,p);dsp.Process(out.data(),in.data(),2048);
    CHECK(!std::memcmp(in.data(),out.data(),in.size()*sizeof(float)));
    p.preamp=-6;dsp.Initialize(48000,2,p);dsp.Process(out.data(),in.data(),2048);
    for(unsigned i=0;i<in.size();++i)CHECK(std::abs(out[i]-in[i]*std::pow(10.,-6/20.))<1e-8);
    p.enabled=false;p.preamp=12;for(float& g:p.gains)g=12;dsp.Initialize(48000,2,p);dsp.Process(out.data(),in.data(),2048);
    CHECK(!std::memcmp(in.data(),out.data(),in.size()*sizeof(float)));}
static void nyquist_invalid_and_channel_isolation(){CHECK(!Eq::Available(9,32000));CHECK(Eq::Available(9,44100));
    Eq::Parameters p;p.enabled=true;p.gains[9]=12;Eq::Processor dsp;dsp.Initialize(32000,2,p);
    float in[1024]{},out[1024]{};in[0]=.01;dsp.Process(out,in,512);CHECK(!std::memcmp(in,out,sizeof(in)));
    p.gains[4]=12;dsp.Initialize(48000,2,p);dsp.Process(out,in,512);
    for(unsigned i=0;i<512;++i){CHECK(std::isfinite(out[i*2]));CHECK(out[i*2+1]==0);}
    p.preamp=std::numeric_limits<float>::infinity();p.gains[0]=std::numeric_limits<float>::quiet_NaN();
    CHECK(Eq::SafeGain(p.preamp)==0);CHECK(Eq::SafeGain(p.gains[0])==0);dsp.Initialize(48000,1,p);
    in[0]=std::numeric_limits<float>::infinity();dsp.Process(out,in,512);for(float v:out)CHECK(std::isfinite(v));
    p={};p.enabled=true;p.preamp=12;dsp.Initialize(48000,1,p);in[0]=std::numeric_limits<float>::max();dsp.Process(out,in,512);CHECK(std::isfinite(out[0]));}
static void extremes_and_sweep(){for(unsigned rate:{32000u,44100u,48000u,96000u})for(float g:{-12.f,12.f}){
    Eq::Parameters p;p.enabled=true;p.preamp=g;for(float& gain:p.gains)gain=g;
    Eq::Processor dsp;dsp.Initialize(rate,1,p);std::vector<float> in(rate),out(rate);
    double phase=0;for(unsigned i=0;i<rate;++i){double f=20*std::pow((rate*.45)/20.,double(i)/rate);phase+=6.283185307179586*f/rate;in[i]=float(.0001*std::sin(phase));}
    dsp.Process(out.data(),in.data(),rate);for(float v:out)CHECK(std::isfinite(v)&&std::abs(v)<10);
    std::fill(in.begin(),in.end(),0);in[0]=.0001;dsp.Process(out.data(),in.data(),rate);for(float v:out)CHECK(std::isfinite(v));}}
static void transition_continuity(){Eq::Processor dsp;Eq::Parameters p;dsp.Initialize(48000,1,p);float in[256],out[256];for(float& v:in)v=.01f;
    p.enabled=true;p.preamp=12;dsp.Publish(p);float previous=.01;
    for(int b=0;b<8;++b){dsp.Process(out,in,256);for(float v:out){CHECK(std::abs(v-previous)<.00004);previous=v;}}
    CHECK(std::abs(previous-.01*std::pow(10.,12/20.))<1e-7);
    p.enabled=false;dsp.Publish(p);for(int b=0;b<8;++b)dsp.Process(out,in,256);CHECK(!std::memcmp(out+255,in+255,sizeof(float)));}
static Eq::Processor concurrent;
static DWORD WINAPI Consumer(void*){float in[512],out[512];for(unsigned i=0;i<512;++i)in[i]=float(.0001*std::sin(i*.2));
    for(unsigned b=0;b<20000;++b){concurrent.Process(out,in,256);for(float v:out)CHECK(std::isfinite(v)&&std::abs(v)<100);}return 0;}
static void concurrent_updates(){Eq::Parameters p;concurrent.Initialize(48000,2,p);HANDLE t=CreateThread(nullptr,0,Consumer,nullptr,0,nullptr);CHECK(t);
    for(int i=0;i<20000;++i){p.enabled=i%3;p.preamp=float(i%25-12);for(unsigned b=0;b<10;++b)p.gains[b]=float((i+b)%25-12);concurrent.Publish(p);if(i%7==0)concurrent.RequestReset();}
    CHECK(WaitForSingleObject(t,30000)==WAIT_OBJECT_0);CloseHandle(t);}
static void presets_and_persistence(){wchar_t path[MAX_PATH]{};
    std::wstring name=L"build\\eq-unit-"+std::to_wstring(sizeof(void*)*8)+
#ifdef DEBUG
        L"-debug.ini";
#else
        L"-release.ini";
#endif
    GetFullPathNameW(name.c_str(),MAX_PATH,path,nullptr);DeleteFileW(path);Ui::ThemeConfigure(path);
    Eq::SetEnabled(true);for(unsigned i=0;i<Eq::PresetCount();++i){Eq::ApplyPreset(i);CHECK(Eq::State().enabled);
        CHECK(Eq::State().preamp==Eq::PresetAt(i).preamp);for(unsigned b=0;b<10;++b)CHECK(Eq::State().gains[b]==Eq::PresetAt(i).gains[b]);}
    Eq::ApplyPreset(1);const float rock[]={4,3,1,-2,-1,2,3,2,1,1};CHECK(Eq::State().preamp==-4);
    for(unsigned b=0;b<10;++b)CHECK(Eq::State().gains[b]==rock[b]);
    Eq::ApplyPreset(0);Eq::SetGain(0,0);CHECK(Eq::Save());Eq::Configure(path);CHECK(Eq::State().preset==-1);
    Eq::SetGain(3,5.5f);CHECK(Eq::State().preset==-1);CHECK(Eq::Save());Ui::SaveAppearanceValue(L"ScalePercent",125);Eq::Configure(path);
    CHECK(Eq::State().enabled&&Eq::State().gains[3]==5.5f&&Eq::State().preset==-1);CHECK(Ui::LoadAppearance().scalePercent==125);
    WritePrivateProfileStringW(L"Equalizer",L"Band3",L"NaN",path);WritePrivateProfileStringW(L"Equalizer",L"Preamp",L"999",path);Eq::Configure(path);
    CHECK(Eq::State().gains[3]==0&&Eq::State().preamp==0);Ui::ThemeConfigure(nullptr);DeleteFileW(path);}
int main(){int cases=0;CASE(all_centers_and_rates);CASE(octave_half_gain);CASE(flat_bypass_and_preamp);CASE(nyquist_invalid_and_channel_isolation);
    CASE(extremes_and_sweep);CASE(transition_continuity);CASE(concurrent_updates);CASE(presets_and_persistence);std::printf("%d EQ DSP cases\n",cases);}
