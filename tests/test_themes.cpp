#include "ui_theme.h"
#include "ui_draw.h"
#include "mainwnd.h"
#include "plwnd.h"
#include "playback.h"
#include "audio.h"
#include "m3u.h"
#include "check.h"
#include <set>
#include <string>
#include <psapi.h>
#include <cstring>

static std::string Variant() {
    std::string value=std::to_string(sizeof(void*)*8);
#ifdef DEBUG
    return value+"-debug";
#else
    return value+"-release";
#endif
}
static std::string Ascii(const wchar_t* value) {
    std::string out;while(*value) {CHECK(*value<128);out+=static_cast<char>(*value++);}return out;
}

static HWND mainWindow,playlist;
static BOOL CALLBACK FindList(HWND hwnd,LPARAM) {
    wchar_t name[80]{};GetClassNameW(hwnd,name,80);
    if(!wcscmp(name,L"LiteSic.PlaylistWnd")) playlist=hwnd;
    return TRUE;
}
static void catalog_and_contrast() {
    CHECK(Ui::ThemeCount()==36);std::set<std::wstring> ids;
    FILE* catalog=std::fopen(("build/themes-catalog-"+Variant()+".json").c_str(),"w");CHECK(catalog);std::fputs("[\n",catalog);
    for(size_t i=0;i<36;++i) {
        auto& t=Ui::ThemeAt(i);auto& p=t.palette;
        CHECK(*t.id && *t.name && *t.category);CHECK(ids.insert(t.id).second);
        CHECK(Ui::SelectTheme(t.id,false));
        CHECK(Ui::Contrast(Ui::SelectedButtonFill(),Ui::SelectedButtonText)>=4.5);
        uint32_t roles[]={p.background,p.surface,p.border,p.bronze,p.green,p.accent,p.text,p.muted,p.danger,
            p.display,p.active,p.hover,p.pressed,p.selection,p.sliderTrack,p.sliderFill,p.fft,p.osc,p.warning};
        for(auto c:roles) CHECK(c<=0xFFFFFF);
        std::fprintf(catalog,"%s{\"id\":\"%s\",\"body\":\"%s\",\"digital\":\"%s\",\"italic_heading\":%s,\"palette\":{",
            i?",\n":"",Ascii(t.id).c_str(),Ascii(t.typography.body).c_str(),Ascii(t.typography.digital).c_str(),t.typography.italicHeading?"true":"false");
        const char* names[]={"background","surface","border","bronze","green","accent","text","muted","danger","display","active","hover",
            "pressed","selection","sliderTrack","sliderFill","fft","osc","warning"};
        for(size_t r=0;r<19;++r) std::fprintf(catalog,"%s\"%s\":\"#%06X\"",r?",":"",names[r],roles[r]);
        std::fputs("}}",catalog);
        for(auto bg:{p.background,p.surface,p.border,p.active,p.hover,p.selection,p.pressed}) CHECK(Ui::Contrast(p.text,bg)>=4.5);
        for(auto bg:{p.background,p.surface,p.display}) CHECK(Ui::Contrast(p.muted,bg)>=4.5);
        for(auto bg:{p.surface,p.display,p.active}) {CHECK(Ui::Contrast(p.green,bg)>=4.5);CHECK(Ui::Contrast(p.accent,bg)>=3);}
        CHECK(Ui::Contrast(p.fft,p.display)>=3);CHECK(Ui::Contrast(p.osc,p.display)>=3);
        CHECK(*t.typography.body && *t.typography.digital);
        for(int f=0;f<4;++f) {CHECK(t.typography.sizes[f]>0 && t.typography.sizes[f]<=27);CHECK(t.typography.weights[f]>=400);}
    }
    CHECK(Ui::ThemeAt(0).palette.background==0x3B2820);
    CHECK(Ui::ThemeAt(0).palette.green==0x00E676);
    std::fputs("\n]\n",catalog);std::fclose(catalog);
    CHECK(Ui::SelectTheme(Ui::ThemeAt(0).id,false));
}
static void typography_fits_fixed_controls() {
    IDWriteFactory* factory=nullptr;CHECK(SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(&factory))));
    for(size_t i=0;i<36;++i) {
        const auto& type=Ui::ThemeAt(i).typography;
        struct Sample {const wchar_t* text;int font;float width,height;};
        for(auto sample : {Sample{L"+ Arquivos",0,100,28},Sample{L"Balanço",0,55,28},Sample{L"Volume",0,52,28},
                           Sample{L"99:59",3,108,44},Sample{L"PLAYLIST",2,376,32},
                           Sample{L"EQ OFF",1,120,18},Sample{L"Preset: Clássica",1,120,18},Sample{L"Redefinir",1,120,18}}) {
            IDWriteTextFormat* format=nullptr;IDWriteTextLayout* layout=nullptr;
            CHECK(SUCCEEDED(factory->CreateTextFormat(sample.font==3?type.digital:type.body,nullptr,
                static_cast<DWRITE_FONT_WEIGHT>(type.weights[sample.font]),sample.font==2 && type.italicHeading?DWRITE_FONT_STYLE_ITALIC:DWRITE_FONT_STYLE_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,type.sizes[sample.font],L"pt-BR",&format)));
            CHECK(SUCCEEDED(factory->CreateTextLayout(sample.text,static_cast<UINT32>(wcslen(sample.text)),format,1000,1000,&layout)));
            DWRITE_TEXT_METRICS metrics{};CHECK(SUCCEEDED(layout->GetMetrics(&metrics)));
            CHECK(metrics.width<=sample.width && metrics.height<=sample.height);layout->Release();format->Release();
        }
    }
    factory->Release();
}
static void selection_persistence_and_fallback() {
    std::wstring variant=std::to_wstring(sizeof(void*)*8);
#ifdef DEBUG
    variant+=L"-debug";
#else
    variant+=L"-release";
#endif
    auto path=FullPath(L"build\\theme-tests-"+variant+L".ini");DeleteFileW(path.c_str());
    Ui::ThemeConfigure(path.c_str());CHECK(!wcscmp(Ui::ActiveTheme().id,L"original"));
    CHECK(Ui::SelectTheme(L"cyberpunk"));
    Ui::ThemeConfigure(nullptr);CHECK(!wcscmp(Ui::ActiveTheme().id,L"original"));
    Ui::ThemeConfigure(path.c_str());CHECK(!wcscmp(Ui::ActiveTheme().id,L"cyberpunk"));
    CHECK(Ui::RestoreTheme());Ui::ThemeConfigure(path.c_str());CHECK(!wcscmp(Ui::ActiveTheme().id,L"original"));
    CHECK(WritePrivateProfileStringW(L"Appearance",L"Theme",L"missing-future-theme",path.c_str()));
    Ui::ThemeConfigure(path.c_str());CHECK(!wcscmp(Ui::ActiveTheme().id,L"original"));
    CHECK(Ui::SelectTheme(L"nonexistent"));CHECK(!wcscmp(Ui::ActiveTheme().id,L"original"));
    Ui::ThemeConfigure(nullptr);DeleteFileW(path.c_str());
}
static COLORREF PaintedPixel(HWND hwnd,int x,int y) {
    RECT r{};GetClientRect(hwnd,&r);HDC dc=CreateCompatibleDC(nullptr);CHECK(dc);
    HDC screen=GetDC(nullptr);HBITMAP bitmap=CreateCompatibleBitmap(screen,r.right,r.bottom);ReleaseDC(nullptr,screen);CHECK(bitmap);
    auto old=SelectObject(dc,bitmap);CHECK(PrintWindow(hwnd,dc,0));GdiFlush();
    COLORREF color=GetPixel(dc,x,y);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return color;
}
static void all_windows_and_state_survive_switches() {
    CHECK(Audio_TestInit(48000));PlayerPlayback().Clear();
    PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));
    PlayerPlaylist().Add(FullPath(L"tests\\fixtures\\native-44100.wav"));
    CHECK(PlayerPlayback().PlayIndex(0));Audio_SeekSeconds(.5);Audio_SetVolume(.4f);Audio_SetPan(-.2f);
    PlWnd_Refresh();
    int scale=MainWnd_DPIScale(100);
    LPARAM point=MAKELPARAM(70*scale/100,120*scale/100);
    SendMessageW(playlist,WM_LBUTTONDOWN,MK_LBUTTON,point);SendMessageW(playlist,WM_LBUTTONUP,0,point);
    HWND search=GetDlgItem(playlist,520);CHECK(search);SetWindowTextW(search,L"native");
    auto current=PlayerPlaylist().At(0)->id,second=PlayerPlaylist().At(1)->id;
    RECT beforeMain{},beforeList{};GetWindowRect(mainWindow,&beforeMain);GetWindowRect(playlist,&beforeList);
    for(size_t i=0;i<36;++i) {
        CHECK(Ui::SelectTheme(Ui::ThemeAt(i).id,false));
        if(CaptureAllowed()) {
            CHECK(PaintedPixel(mainWindow,2,MainWnd_DPIScale(35))==Ui::NativeColor(Ui::Background));
            CHECK(PaintedPixel(playlist,2,MainWnd_DPIScale(35))==Ui::NativeColor(Ui::Background));
        } else { UpdateWindow(mainWindow);UpdateWindow(playlist); }
        CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);CHECK(Audio_GetPositionSeconds()==.5);
        CHECK(PlayerPlaylist().Count()==2 && PlayerPlaylist().At(0)->id==current && PlayerPlaylist().At(1)->id==second);
        CHECK(PlayerPlaylist().GetCurrentIndex()==0);
        wchar_t query[32]{};GetWindowTextW(search,query,32);CHECK(!wcscmp(query,L"native"));
        RECT a{},b{};GetWindowRect(mainWindow,&a);GetWindowRect(playlist,&b);
        CHECK(!std::memcmp(&a,&beforeMain,sizeof(a)) && !std::memcmp(&b,&beforeList,sizeof(b)));
    }
    // Delete after all switches proves the second-row selection was preserved.
    SendMessageW(playlist,WM_KEYDOWN,VK_DELETE,0);CHECK(PlayerPlaylist().Count()==1);
    CHECK(PlayerPlaylist().At(0)->id==current);CHECK(Audio_GetState()==AUDIO_STATE_PLAYING);
    SetWindowTextW(search,L"");
    CHECK(Ui::RestoreTheme());
}
static void resource_recovery_and_repeat_switching() {
    HDC dc=CreateCompatibleDC(nullptr);CHECK(dc);HDC screen=GetDC(nullptr);
    HBITMAP bitmap=CreateCompatibleBitmap(screen,460,290);ReleaseDC(nullptr,screen);CHECK(bitmap);
    auto old=SelectObject(dc,bitmap);
    {
        Ui::Canvas canvas;
        for(int i=0;i<180;++i) {
            Ui::SelectTheme(Ui::ThemeAt(i%36).id,false);CHECK(canvas.Begin(dc,{0,0,460,290},1));
            canvas.Fill({0,0,460,290},Ui::Background);canvas.TextAt({8,8,200,28},L"Música — 00:23",Ui::Text);
            canvas.Button({8,40,100,28},L"Reproduzir",true);CHECK(canvas.End());
            if(i%12==0) canvas.DiscardTarget();
        }
    }
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);Ui::RestoreTheme();
}
static void selector_categories_restore_and_new_window() {
    Ui::SelectTheme(L"forest",false);Ui::ShowThemes(mainWindow);
    HWND selector=FindWindowW(L"LiteSic.ThemesWnd",nullptr);CHECK(selector);
    HWND combo=GetDlgItem(selector,701),list=GetDlgItem(selector,702);CHECK(combo && list);
    CHECK(SendMessageW(list,LB_GETCOUNT,0,0)==36);
    SendMessageW(combo,CB_SETCURSEL,6,0);
    SendMessageW(selector,WM_COMMAND,MAKEWPARAM(701,CBN_SELCHANGE),reinterpret_cast<LPARAM>(combo));
    CHECK(SendMessageW(list,LB_GETCOUNT,0,0)==2);
    SendMessageW(list,LB_SETCURSEL,1,0);
    SendMessageW(selector,WM_COMMAND,MAKEWPARAM(702,LBN_SELCHANGE),reinterpret_cast<LPARAM>(list));
    CHECK(!wcscmp(Ui::ActiveTheme().id,L"vintage"));
    if(CaptureAllowed())CHECK(PaintedPixel(mainWindow,2,MainWnd_DPIScale(35))==Ui::NativeColor(Ui::Background));
    SendMessageW(GetDlgItem(selector,703),BM_CLICK,0,0);CHECK(!wcscmp(Ui::ActiveTheme().id,L"original"));
    CHECK(SendMessageW(list,LB_GETCOUNT,0,0)==2); // Applying/restoring does not change the chosen category.
    SendMessageW(combo,CB_SETCURSEL,0,0);
    SendMessageW(selector,WM_COMMAND,MAKEWPARAM(701,CBN_SELCHANGE),reinterpret_cast<LPARAM>(combo));
    CHECK(SendMessageW(list,LB_GETCOUNT,0,0)==36);
    SendMessageW(list,LB_SETTOPINDEX,12,0);SendMessageW(list,LB_SETCURSEL,14,0);
    auto top=SendMessageW(list,LB_GETTOPINDEX,0,0);
    SendMessageW(selector,WM_COMMAND,MAKEWPARAM(702,LBN_SELCHANGE),reinterpret_cast<LPARAM>(list));
    CHECK(SendMessageW(list,LB_GETTOPINDEX,0,0)==top&&SendMessageW(list,LB_GETCURSEL,0,0)==14);
    CHECK(GetWindowLongPtrW(selector,GWL_STYLE)&WS_CLIPCHILDREN);
    auto point=[](int x,int y){return MAKELPARAM(MainWnd_DPIScale(x),MainWnd_DPIScale(y));};
    SendMessageW(selector,WM_LBUTTONDOWN,MK_LBUTTON,point(503,200));
    SendMessageW(selector,WM_MOUSEMOVE,MK_LBUTTON,point(503,360));SendMessageW(selector,WM_LBUTTONUP,0,point(503,360));
    CHECK(SendMessageW(list,LB_GETTOPINDEX,0,0)>=20);
    MSG escape{};escape.hwnd=list;escape.message=WM_KEYDOWN;escape.wParam=VK_ESCAPE;
    CHECK(Ui::ThemeDialogMessage(&escape));CHECK(!IsWindow(selector));
}
static void sustained_switch_memory() {
    std::string file="build/themes-memory-"+std::to_string(sizeof(void*)*8);
#ifdef DEBUG
    file+="-debug.jsonl";
#else
    file+="-release.jsonl";
#endif
    FILE* log=std::fopen(file.c_str(),"w");CHECK(log);
    SIZE_T first=0,last=0;DWORD firstGdi=0,firstUser=0;
    for(int batch=0;batch<=10;++batch) {
        for(int i=0;i<180;++i) {
            Ui::SelectTheme(Ui::ThemeAt(i%36).id,false);
            if(CaptureAllowed()) {
                CHECK(PaintedPixel(mainWindow,2,MainWnd_DPIScale(35))==Ui::NativeColor(Ui::Background));
                CHECK(PaintedPixel(playlist,2,MainWnd_DPIScale(35))==Ui::NativeColor(Ui::Background));
            } else { UpdateWindow(mainWindow);UpdateWindow(playlist); }
        }
        PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);
        CHECK(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory)));
        auto gdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS),user=GetGuiResources(GetCurrentProcess(),GR_USEROBJECTS);
        DWORD handles=0;CHECK(GetProcessHandleCount(GetCurrentProcess(),&handles));
        std::fprintf(log,"{\"switches\":%d,\"private_bytes\":%llu,\"working_set\":%llu,\"gdi\":%lu,\"user\":%lu,\"handles\":%lu}\n",
            (batch+1)*180,static_cast<unsigned long long>(memory.PrivateUsage),static_cast<unsigned long long>(memory.WorkingSetSize),gdi,user,handles);
        if(!batch) {first=memory.PrivateUsage;firstGdi=gdi;firstUser=user;}
        last=memory.PrivateUsage;CHECK(gdi<=firstGdi+2 && user<=firstUser+2);
    }
    std::fclose(log);CHECK(last<=first+8*1024*1024);Ui::RestoreTheme();
}
int main() {
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    CHECK(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));
    mainWindow=MainWnd_Create(GetModuleHandleW(nullptr),SW_SHOWNOACTIVATE);CHECK(mainWindow);
    CHECK(PlWnd_Create(GetModuleHandleW(nullptr),mainWindow));EnumThreadWindows(GetCurrentThreadId(),FindList,0);CHECK(playlist);
    Audio_Shutdown();int cases=0;
    CASE(catalog_and_contrast);CASE(typography_fits_fixed_controls);CASE(selection_persistence_and_fallback);
    CASE(all_windows_and_state_survive_switches);CASE(resource_recovery_and_repeat_switching);
    CASE(selector_categories_restore_and_new_window);
    CASE(sustained_switch_memory);
    DestroyWindow(mainWindow);CoUninitialize();std::printf("%d theme cases\n",cases);
}
