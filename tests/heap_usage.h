#pragma once
#include <windows.h>
#include "check.h"

// PrivateUsage includes committed free pages, stacks and other process state.
// Count allocated blocks instead when testing C++ catalog ownership. Walk every
// process heap because the CRT need not use GetProcessHeap(). No allocations or
// logging occur while a heap is locked.
inline SIZE_T LiveHeapBytes() {
    HANDLE heaps[64]{};
    DWORD count=GetProcessHeaps(64,heaps);
    CHECK(count>0&&count<=64);
    SIZE_T bytes=0;
    for(DWORD i=0;i<count;++i) {
        CHECK(HeapLock(heaps[i]));
        PROCESS_HEAP_ENTRY entry{};
        while(HeapWalk(heaps[i],&entry))
            if(entry.wFlags&PROCESS_HEAP_ENTRY_BUSY)bytes+=entry.cbData;
        DWORD error=GetLastError();
        BOOL unlocked=HeapUnlock(heaps[i]);
        CHECK(unlocked);CHECK(error==ERROR_NO_MORE_ITEMS);
    }
    return bytes;
}

inline void heap_accounting_detects_retention() {
    constexpr SIZE_T payload=1024*1024,limit=512*1024;
    const auto before=LiveHeapBytes();
    void* allocation=HeapAlloc(GetProcessHeap(),0,payload);
    CHECK(allocation);
    const auto retained=LiveHeapBytes();
    CHECK(retained>=before+payload);CHECK(retained>before+limit);
    CHECK(HeapFree(GetProcessHeap(),0,allocation));
    CHECK(LiveHeapBytes()<=before+limit);
}
