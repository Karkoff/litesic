// External MSAA client: every exposed control, live languages, hidden/reopened panels.
// Accept only the PID of an isolated test process supplied by the runner.
#include <windows.h>
#include <oleacc.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <cwchar>
static DWORD pid;
static HWND player,eq,playlist;
static bool ok=true;
static void Require(bool condition,const char* label){if(!condition){printf("FAIL %s\n",label);ok=false;}}
static BOOL CALLBACK Find(HWND hwnd,LPARAM){DWORD owner=0;GetWindowThreadProcessId(hwnd,&owner);if(owner==pid){wchar_t cls[80]{};GetClassNameW(hwnd,cls,80);if(!wcscmp(cls,L"LiteSic.MainWnd"))player=hwnd;if(!wcscmp(cls,L"LiteSic.EqualizerWnd"))eq=hwnd;if(!wcscmp(cls,L"LiteSic.PlaylistWnd"))playlist=hwnd;}return TRUE;}
static VARIANT Child(int id){VARIANT c{};c.vt=VT_I4;c.lVal=id;return c;}
static IAccessible* Provider(HWND hwnd){IAccessible* p=nullptr;Require(SUCCEEDED(AccessibleObjectFromWindow(hwnd,OBJID_CLIENT,IID_IAccessible,reinterpret_cast<void**>(&p))),"obtain provider");return p;}
static std::wstring Name(IAccessible* p,int id){BSTR b=nullptr;Require(p&&p->get_accName(Child(id),&b)==S_OK&&b&&*b,"nonempty accessible name");std::wstring result=b?b:L"";SysFreeString(b);return result;}
static void Utf8(const std::wstring& text){int n=WideCharToMultiByte(CP_UTF8,0,text.c_str(),int(text.size()),nullptr,0,nullptr,nullptr);std::string bytes(n,'\0');WideCharToMultiByte(CP_UTF8,0,text.c_str(),int(text.size()),bytes.data(),n,nullptr,nullptr);printf("%s",bytes.c_str());}
static int Inspect(HWND hwnd,const char* surface,const char* language){
    auto p=Provider(hwnd);if(!p)return 0;IEnumVARIANT* list=nullptr;Require(SUCCEEDED(p->QueryInterface(IID_IEnumVARIANT,reinterpret_cast<void**>(&list))),"enumerate provider");if(!list){p->Release();return 0;}
    int count=0;VARIANT c{};ULONG fetched=0;
    while(list->Next(1,&c,&fetched)==S_OK){++count;auto name=Name(p,c.lVal);VARIANT role{},state{};LONG x=0,y=0,w=0,h=0;
        Require(p->get_accRole(c,&role)==S_OK&&role.vt==VT_I4&&role.lVal>0,"accessible role");
        Require(p->get_accState(c,&state)==S_OK&&state.vt==VT_I4,"accessible state");
        Require(p->accLocation(&x,&y,&w,&h,c)==S_OK&&w>0&&h>0,"control bounds");
        printf("CONTROL language=%s surface=%s id=%ld role=%ld state=%ld bounds=%ld,%ld,%ld,%ld name=",language,surface,c.lVal,role.lVal,state.lVal,x,y,w,h);Utf8(name);printf("\n");VariantClear(&c);
    }
    LONG declared=0;Require(p->get_accChildCount(&declared)==S_OK&&declared==count&&count>0,"full enumeration count");list->Release();p->Release();return count;
}
int main(int argc,char** argv){if(argc<2||argc>3)return 2;pid=DWORD(strtoul(argv[1],nullptr,10));CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);EnumWindows(Find,0);if(!player||!eq||!playlist)return 3;
    if(argc==3&&!strcmp(argv[2],"--focus")){auto p=Provider(player);VARIANT focus{},state{};Require(p&&p->get_accFocus(&focus)==S_OK&&focus.vt==VT_I4&&focus.lVal>0,"keyboard focus exposed");if(p&&focus.vt==VT_I4){Require(p->get_accState(focus,&state)==S_OK&&(state.lVal&STATE_SYSTEM_FOCUSED),"focused state exposed");printf("FOCUS child=%ld state=%ld name=",focus.lVal,state.lVal);Utf8(Name(p,focus.lVal));printf("\n");}if(p)p->Release();CoUninitialize();return ok?0:1;}
    const char* languages[]={"pt-BR","en-US","es-ES","fr-FR","de-DE","it-IT","pt-PT","nl-NL","pl-PL","ru-RU","ja-JP","zh-CN","zh-TW","ko-KR","tr-TR","ar-SA","hi-IN","sv-SE"};
    int total=0;
    for(int i=0;i<18;++i){SendMessageW(player,WM_COMMAND,921+i,0);total+=Inspect(player,"player",languages[i]);total+=Inspect(eq,"eq",languages[i]);total+=Inspect(playlist,"playlist",languages[i]);
        auto native=Provider(GetDlgItem(playlist,520));auto canvas=Provider(playlist);if(native&&canvas){auto n=Name(native,0),c=Name(canvas,520);Require(!n.empty()&&!c.empty(),"localized native search");printf("SEARCH language=%s name=",languages[i]);Utf8(n);printf("\n");}if(native)native->Release();if(canvas)canvas->Release();}
    SendMessageW(player,WM_COMMAND,922,0); // English; exact expected names below.
    auto p=Provider(player);auto e=Provider(eq);auto l=Provider(playlist);
    if(p&&e&&l){auto en=Name(p,5);SendMessageW(player,WM_COMMAND,921,0);Require(Name(p,5)!=en,"name changes immediately with language");SendMessageW(player,WM_COMMAND,922,0);
        for(auto pair:std::vector<std::pair<int,IAccessible*>>{{12,e},{11,l}}){Require(p->accDoDefaultAction(Child(pair.first))==S_OK,"hide panel action");VARIANT state{};Require(pair.second->get_accState(Child(0),&state)==S_OK&&(state.lVal&STATE_SYSTEM_INVISIBLE),"hidden panel MSAA state");Require(p->accDoDefaultAction(Child(pair.first))==S_OK,"reopen panel action");Require(pair.second->get_accState(Child(0),&state)==S_OK&&!(state.lVal&STATE_SYSTEM_INVISIBLE),"reopened panel MSAA state");}
        VARIANT band=Child(9);BSTR gain=SysAllocString(L"3");Require(e->put_accValue(band,gain)==S_OK,"correct EQ band setter");SysFreeString(gain);BSTR observed=nullptr;Require(e->get_accValue(band,&observed)==S_OK&&observed&&wcstod(observed,nullptr)==3,"correct EQ band value");SysFreeString(observed);gain=SysAllocString(L"0");e->put_accValue(band,gain);SysFreeString(gain);
    }
    if(p)p->Release();
    if(e)e->Release();
    if(l)l->Release();
    CoUninitialize();printf("MSAA_MATRIX languages=18 controls=%d live_names_and_panel_lifecycle=%s\n",total,ok?"PASS":"FAIL");return ok?0:1;
}
