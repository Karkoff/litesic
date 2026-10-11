#pragma once
#include "eq_dsp.h"
namespace Eq {
struct Preset { const wchar_t* name; float preamp; float gains[Bands]; };
unsigned PresetCount();
const Preset& PresetAt(unsigned index);
const Parameters& State(); // UI thread only
void Configure(const wchar_t* iniPath);
void SetEnabled(bool enabled);
void SetGain(int band,float gain); // -1 = preamp
void ApplyPreset(unsigned index);
bool Save();
}
