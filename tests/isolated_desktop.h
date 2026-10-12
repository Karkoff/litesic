#pragma once
#include <windows.h>
#include <cwchar>
#include "check.h"

// Window-state tests must not share foreground permission, hotkeys or input
// services with the user's desktop. Never switch the visible input desktop.
// Run the fixture on a fresh Win32 thread so all its windows are released when
// it exits, before the launching thread closes the desktop handle.
inline int OnIsolatedDesktop(int (*run)(int,char**),int argc,char** argv) {
    wchar_t name[80]{};
    swprintf(name,80,L"LiteSic.Test.%lu",GetCurrentProcessId());
    HDESK desktop=CreateDesktopW(name,nullptr,nullptr,0,GENERIC_ALL,nullptr);
    CHECK(desktop);
    struct Invocation {
        HDESK desktop;int (*run)(int,char**);int argc;char** argv;
        static DWORD WINAPI Entry(void* data) {
            auto& invocation=*static_cast<Invocation*>(data);
            CHECK(SetThreadDesktop(invocation.desktop));
            return static_cast<DWORD>(invocation.run(invocation.argc,invocation.argv));
        }
    };
    Invocation invocation{desktop,run,argc,argv};
    HANDLE thread=CreateThread(nullptr,0,Invocation::Entry,&invocation,0,nullptr);
    CHECK(thread);CHECK(WaitForSingleObject(thread,INFINITE)==WAIT_OBJECT_0);
    DWORD result=1;CHECK(GetExitCodeThread(thread,&result));
    CHECK(CloseHandle(thread));CHECK(CloseDesktop(desktop));
    return static_cast<int>(result);
}
