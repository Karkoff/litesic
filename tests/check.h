#pragma once
#include <cstdio>
#include <cstdlib>
#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#condition); std::exit(1); } } while(0)
#define CASE(name) do { name(); ++cases; std::printf("PASS %s\n",#name); } while(0)
