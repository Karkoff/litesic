#pragma once
#include <algorithm>
#include <cmath>

// DIP coordinates; pauses and eased reversals keep both ends readable.
inline float TitleScrollOffset(float overflow,double elapsedMs) {
    if(overflow<=0 || !std::isfinite(overflow) || !std::isfinite(elapsedMs))return 0;
    const double pause=1500,travel=std::max(1000.0,double(overflow)/40*1000);
    double t=std::fmod(std::max(0.0,elapsedMs),2*(pause+travel));
    if(t<pause)return 0;
    if(t<pause+travel)return float(overflow*(.5-.5*std::cos(3.141592653589793*(t-pause)/travel)));
    if(t<2*pause+travel)return overflow;
    return float(overflow*(.5+.5*std::cos(3.141592653589793*(t-2*pause-travel)/travel)));
}
