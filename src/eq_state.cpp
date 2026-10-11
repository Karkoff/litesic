#include "eq_state.h"
#include "audio.h"
#include <windows.h>
#include <string>
#include <cwchar>
#include <cmath>
namespace Eq {
namespace {
Parameters state;
std::wstring path;
const Preset presets[]={
 {L"Flat",0,{0,0,0,0,0,0,0,0,0,0}},
 {L"Rock",-4,{4,3,1,-2,-1,2,3,2,1,1}},
 {L"Pop",-3,{-1,1,3,3,1,-1,-1,1,2,2}},
 {L"Jazz",-3,{3,2,0,1,-1,-1,1,2,2,1}},
 {L"Clássica",-3,{2,1,0,0,-1,-1,0,2,2,1}},
 {L"Graves",-5,{5,4,2,0,0,0,0,0,0,0}},
 {L"Voz",-3,{-3,-2,-1,1,3,3,1,-1,-2,-2}}
};
void Publish() { Audio_UpdateEq(state); }
float Read(const wchar_t* key,float fallback) {
    wchar_t text[80]{};GetPrivateProfileStringW(L"Equalizer",key,L"",text,80,path.c_str());
    wchar_t* end=nullptr;double v=std::wcstod(text,&end);
    return end!=text && !*end && std::isfinite(v) && v>=-12 && v<=12?float(v):fallback;
}
}
unsigned PresetCount(){return sizeof(presets)/sizeof(presets[0]);}
const Preset& PresetAt(unsigned i){return presets[i<PresetCount()?i:0];}
const Parameters& State(){return state;}
void Configure(const wchar_t* ini) {
    path=ini?ini:L"";state={};
    if(!path.empty()) {
        state.enabled=Read(L"Enabled",0)==1;state.preamp=Read(L"Preamp",0);
        for(unsigned i=0;i<Bands;++i) state.gains[i]=Read((L"Band"+std::to_wstring(i)).c_str(),0);
        wchar_t selected[80]{};GetPrivateProfileStringW(L"Equalizer",L"Preset",L"",selected,80,path.c_str());
        state.preset=-1;
        for(unsigned i=0;i<PresetCount();++i) {
            bool match=state.preamp==presets[i].preamp;
            for(unsigned b=0;b<Bands;++b) match=match && state.gains[b]==presets[i].gains[b];
            if(match && (!*selected || !wcscmp(selected,presets[i].name))) { state.preset=int(i);break; }
        }
    }
    Publish();
}
bool Save() {
    if(path.empty()) return true;
    // One section update preserves Appearance and any future configuration sections.
    std::wstring data;
    auto add=[&](std::wstring key,std::wstring value){data+=key+L"="+value;data.push_back(0);};
    add(L"Enabled",state.enabled?L"1":L"0");add(L"Preamp",std::to_wstring(state.preamp));
    add(L"Preset",state.preset<0?L"Custom":PresetAt(state.preset).name);
    for(unsigned b=0;b<Bands;++b) add(L"Band"+std::to_wstring(b),std::to_wstring(state.gains[b]));
    data.push_back(0);return WritePrivateProfileSectionW(L"Equalizer",data.c_str(),path.c_str())!=FALSE;
}
void SetEnabled(bool v){state.enabled=v;Publish();}
void SetGain(int b,float v){if(b==-1) state.preamp=SafeGain(v);else if(b>=0&&b<int(Bands)) state.gains[b]=SafeGain(v);else return;state.preset=-1;Publish();}
void ApplyPreset(unsigned i){if(i>=PresetCount())return;state.preamp=presets[i].preamp;for(unsigned b=0;b<Bands;++b)state.gains[b]=presets[i].gains[b];state.preset=int(i);Publish();}
}
