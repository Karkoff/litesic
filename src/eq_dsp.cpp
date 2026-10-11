#include "eq_dsp.h"
#include <cmath>
#include <algorithm>

static_assert(std::atomic<unsigned>::is_always_lock_free,"EQ requires lock-free 32-bit atomics");
namespace Eq {
float SafeGain(float v) { return std::isfinite(v)?std::clamp(v,-12.f,12.f):0.f; }
bool Available(unsigned band,unsigned rate) { return band<Bands && rate>=8000 && Frequencies[band]<rate*.475; }
Snapshot Prepare(const Parameters& p,unsigned rate) {
    Snapshot s; s.enabled=p.enabled;s.preamp=static_cast<float>(std::pow(10.,SafeGain(p.preamp)/20.));
    for(unsigned i=0;i<Bands;++i) {
        double gain=SafeGain(p.gains[i]);
        if(!Available(i,rate) || gain==0) continue;
        double w=6.283185307179586*Frequencies[i]/rate, sn=std::sin(w), cs=std::cos(w);
        // Cookbook bandwidth = one octave at half the peak gain, including BLT correction.
        double alpha=sn*std::sinh(std::log(2.)*.5*w/sn), a=std::pow(10.,gain/40.), a0=1+alpha/a;
        Coefficients c{(1+alpha*a)/a0,-2*cs/a0,(1-alpha*a)/a0,-2*cs/a0,(1-alpha/a)/a0,true};
        if(std::isfinite(c.b0)&&std::isfinite(c.b1)&&std::isfinite(c.b2)&&std::isfinite(c.a1)&&std::isfinite(c.a2)
           && std::abs(c.a2)<1 && 1+c.a1+c.a2>0 && 1-c.a1+c.a2>0) s.filters[i]=c;
    }
    s.flat=s.preamp==1;
    for(const auto& c:s.filters) s.flat=s.flat && !c.active;
    return s;
}
void Processor::Initialize(unsigned rate,unsigned channels,const Parameters& p) {
    rate_=rate;channels_=std::min(channels,MaxChannels);length_=std::max(1u,rate/50);
    back_=2;front_=0;middle_.store(1);reset_.store(0);seenReset_=0;current_=0;remaining_=0;
    banks_[0]=Bank{};banks_[1]=Bank{};
    banks_[0].snapshot=banks_[1].snapshot=Prepare(p,rate);
}
void Processor::Publish(const Parameters& p) {
    slots_[back_]=Prepare(p,rate_);
    back_=middle_.exchange(back_|4,std::memory_order_acq_rel)&3;
}
float Processor::Sample(Bank& bank,float input,unsigned channel) {
    if(!bank.snapshot.enabled || bank.snapshot.flat) return input;
    double x=input*bank.snapshot.preamp;
    for(unsigned i=0;i<Bands;++i) {
        const auto& c=bank.snapshot.filters[i];if(!c.active) continue;
        auto& d=bank.delays[channel][i];
        double y=c.b0*x+d.z1;d.z1=c.b1*x-c.a1*y+d.z2;d.z2=c.b2*x-c.a2*y;
        if(!std::isfinite(y)||!std::isfinite(d.z1)||!std::isfinite(d.z2)||std::abs(y)>1e12) {d={};y=0;}
        if(std::abs(d.z1)<1e-24) d.z1=0;
        if(std::abs(d.z2)<1e-24) d.z2=0;
        x=y;
    }
    return std::isfinite(x)&&std::abs(x)<=1e12?static_cast<float>(x):0.f;
}
void Processor::Process(float* out,const float* in,unsigned frames) {
    unsigned reset=reset_.load(std::memory_order_acquire);
    if(reset!=seenReset_) { for(auto& b:banks_) for(auto& channel:b.delays) for(auto& d:channel) d={};seenReset_=reset; }
    if(!remaining_ && (middle_.load(std::memory_order_acquire)&4)) {
        front_=middle_.exchange(front_,std::memory_order_acq_rel)&3;
        auto& next=banks_[1-current_];next=banks_[current_];next.snapshot=slots_[front_];
        // Disabled/identity stages must not retain stale history.
        for(unsigned c=0;c<channels_;++c) for(unsigned b=0;b<Bands;++b)
            if(!next.snapshot.filters[b].active || !banks_[current_].snapshot.enabled) next.delays[c][b]={};
        remaining_=length_;
    }
    for(unsigned f=0;f<frames;++f) {
        float t=remaining_?float(length_-remaining_+1)/length_:0;
        for(unsigned c=0;c<channels_;++c) {
            float x=in[f*channels_+c];if(!std::isfinite(x)) x=0;
            float a=Sample(banks_[current_],x,c);
            out[f*channels_+c]=remaining_?a+(Sample(banks_[1-current_],x,c)-a)*t:a;
        }
        if(remaining_ && !--remaining_) current_=1-current_;
    }
}
}
