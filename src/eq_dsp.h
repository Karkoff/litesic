#pragma once
#include <atomic>
#include <cstdint>

namespace Eq {
constexpr unsigned Bands=10, MaxChannels=32;
inline constexpr double Frequencies[Bands]={60,170,310,600,1000,3000,6000,12000,14000,16000};
struct Parameters { bool enabled=false; float preamp=0; float gains[Bands]{}; int preset=0; };
float SafeGain(float value);
struct Coefficients { double b0=1,b1=0,b2=0,a1=0,a2=0; bool active=false; };
struct Snapshot { Coefficients filters[Bands]; float preamp=1; bool enabled=false,flat=true; };
bool Available(unsigned band,unsigned rate);
Snapshot Prepare(const Parameters& parameters,unsigned rate);
// Single producer (UI), single consumer (audio). Three exclusive ownership slots.
class Processor {
public:
    void Initialize(unsigned rate,unsigned channels,const Parameters& parameters);
    void Publish(const Parameters& parameters); // producer only, coefficient math here
    void RequestReset() { reset_.fetch_add(1,std::memory_order_release); }
    void Process(float* output,const float* input,unsigned frames); // consumer only
private:
    struct Delay { double z1=0,z2=0; };
    struct Bank { Snapshot snapshot; Delay delays[MaxChannels][Bands]{}; };
    float Sample(Bank& bank,float input,unsigned channel);
    Snapshot slots_[3];
    std::atomic<unsigned> middle_{1},reset_{0};
    unsigned back_=2,front_=0,seenReset_=0,rate_=48000,channels_=2;
    Bank banks_[2]; unsigned current_=0,remaining_=0,length_=960;
};
}
