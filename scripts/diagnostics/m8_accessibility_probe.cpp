#include <windows.h>
#include <oleacc.h>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
static DWORD process=0;static HWND player=nullptr,eq=nullptr;
static BOOL CALLBACK Find(HWND hwnd,LPARAM){DWORD pid=0;GetWindowThreadProcessId(hwnd,&pid);if(pid==process){wchar_t name[80]{};GetClassNameW(hwnd,name,80);if(!wcscmp(name,L"LiteSic.MainWnd"))player=hwnd;if(!wcscmp(name,L"LiteSic.EqualizerWnd"))eq=hwnd;}return TRUE;}
int main(int argc,char** argv){if(argc!=2)return 2;process=DWORD(strtoul(argv[1],nullptr,10));CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);EnumWindows(Find,0);if(!player||!eq)return 3;
    IAccessible* provider=nullptr;if(FAILED(AccessibleObjectFromWindow(eq,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&provider))))return 4;
    IEnumVARIANT* children=nullptr;if(FAILED(provider->QueryInterface(IID_IEnumVARIANT,reinterpret_cast<void**>(&children))))return 5;
    VARIANT child{};ULONG fetched=0;bool checked=false;
    while(children->Next(1,&child,&fetched)==S_OK){BSTR name=nullptr;provider->get_accName(child,&name);
        if(name&&wcsstr(name,L"1000 Hz")){LONG band=child.lVal;SysFreeString(name);BSTR gain=SysAllocString(L"3");HRESULT result=provider->put_accValue(child,gain);SysFreeString(gain);if(FAILED(result))return 6;BSTR value=nullptr;provider->get_accValue(child,&value);checked=value&&wcstod(value,nullptr)==3;SysFreeString(value);provider->get_accName(child,&name);printf("CROSS_PROCESS_EQ child_id=%ld requested_band=1000_Hz gain=3 observed_value=%s\n",band,checked?"PASS":"FAIL");}
        if(name)SysFreeString(name);
        VariantClear(&child);
    }
    children->Release();provider->Release();CoUninitialize();return checked?0:7;
}
