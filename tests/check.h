#pragma once
#include <cstdio>
#include <cstdlib>
// Explicit opt-out for audits where window image capture is not authorized.
inline bool CaptureAllowed() {
    const bool allowed=std::getenv("LITESIC_NO_CAPTURE")==nullptr;
    static bool reported=false;
    if(!allowed && !reported){std::puts("PENDING window pixel/capture assertions: LITESIC_NO_CAPTURE");reported=true;}
    return allowed;
}
#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); std::exit(1); } } while(0)
#define CASE(name) do { name(); ++cases; std::printf("PASS %s\n",#name); } while(0)
