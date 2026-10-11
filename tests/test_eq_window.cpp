#include "mainwnd.h"
#include "eqwnd.h"
#include "eq_state.h"
#include "audio.h"
#include "ui_theme.h"
#include "check.h"
#include <vector>
#include <string>
#include <cmath>
static HWND mainWindow,eqWindow;
static BOOL CALLBACK Find(HWND h,LPARAM){wchar_t c[80];GetClassNameW(h,c,80);if(!wcscmp(c,L"LiteSic.EqualizerWnd")&&GetWindow(h,GW_OWNER)==mainWindow)eqWindow=h;return TRUE;}
static LPARAM Point(int x,int y){return MAKELPARAM(MainWnd_DPIScale(x),MainWnd_DPIScale(y));}
static void Click(int x,int y){SendMessageW(eqWindow,WM_LBUTTONDOWN,MK_LBUTTON,Point(x,y));SendMessageW(eqWindow,WM_LBUTTONUP,0,Point(x,y));}
static void Screenshot(unsigned theme){
    RECT r{};GetClientRect(eqWindow,&r);HDC screen=GetDC(nullptr),dc=CreateCompatibleDC(screen);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=r.right;info.bmiHeader.biHeight=-r.bottom;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;void* pixels=nullptr;
    HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);CHECK(bitmap);auto old=SelectObject(dc,bitmap);
    CHECK(PrintWindow(eqWindow,dc,0));GdiFlush();CHECK(GetPixel(dc,2,MainWnd_DPIScale(35))==Ui::NativeColor(Ui::Background));
    BITMAPFILEHEADER header{};header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+r.right*r.bottom*4;
    CreateDirectoryW(L"build\\m6",nullptr);std::wstring name=L"build\\m6\\eq-theme-"+std::to_wstring(theme)+L"-"+std::to_wstring(sizeof(void*)*8)+
#ifdef DEBUG
    L"-debug.bmp";
#else
    L"-release.bmp";
#endif
    FILE* file=_wfopen(name.c_str(),L"wb");CHECK(file);fwrite(&header,sizeof(header),1,file);fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,file);fwrite(pixels,r.right*r.bottom*4,1,file);fclose(file);
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(nullptr,screen);
}
static double Render(){std::vector<float> pcm(9600);CHECK(Audio_TestRead(pcm.data(),4800));double sum=0;for(unsigned i=4800;i<9600;++i)sum+=pcm[i]*pcm[i];return std::sqrt(sum/4800);}
static void controls_publish_dsp_and_hidden_processing(){Eq::Configure(nullptr);CHECK(Audio_TestInit(48000));CHECK(Audio_Load(L"tests\\eq_fixtures\\eq-tone-2.wav"));Audio_Play();double dry=Render();
    Click(45,54);CHECK(Eq::State().enabled);
    SendMessageW(eqWindow,WM_LBUTTONDOWN,MK_LBUTTON,Point(245,161));
    SendMessageW(eqWindow,WM_MOUSEMOVE,MK_LBUTTON,Point(245,143));SendMessageW(eqWindow,WM_LBUTTONUP,0,Point(245,143));
    CHECK(Eq::State().gains[4]>6&&Eq::State().gains[4]<6.3&&Eq::State().preset==-1);
    double boost=Render();CHECK(boost/dry>1.9&&boost/dry<2.2);SendMessageW(eqWindow,WM_CLOSE,0,0);CHECK(!IsWindowVisible(eqWindow));CHECK(Eq::State().enabled);
    CHECK(Render()/dry>1.9);EqWnd_Toggle(mainWindow);CHECK(Eq::State().gains[4]>6);
    Click(370,54);CHECK(Eq::State().preset==0&&Eq::State().gains[4]==0);Render();CHECK(std::abs(Render()/dry-1)<.01);
    Click(60,161);SendMessageW(eqWindow,WM_KEYDOWN,VK_UP,0);CHECK(Eq::State().preamp==.5f);
    SendMessageW(eqWindow,WM_KEYDOWN,VK_HOME,0);CHECK(Eq::State().preamp==12);
    SendMessageW(eqWindow,WM_KEYDOWN,VK_END,0);CHECK(Eq::State().preamp==-12);Audio_Shutdown();}
static void all_themes_preserve_parameters(){CHECK(Audio_TestInit(48000));Eq::ApplyPreset(1);Eq::SetEnabled(true);auto before=Eq::State();
    for(unsigned i=0;i<Ui::ThemeCount();++i){CHECK(Ui::SelectTheme(Ui::ThemeAt(i).id,false));Screenshot(i);
        CHECK(Eq::State().enabled==before.enabled&&Eq::State().preamp==before.preamp);for(unsigned b=0;b<10;++b)CHECK(Eq::State().gains[b]==before.gains[b]);}
    for(int percent:{80,90,100,110,125,150}){SendMessageW(mainWindow,WM_COMMAND,percent==80?IDM_SCALE_80:percent==90?IDM_SCALE_90:percent==100?IDM_SCALE_100:percent==110?IDM_SCALE_110:percent==125?IDM_SCALE_125:IDM_SCALE_150,0);
        RECT r{};GetClientRect(eqWindow,&r);CHECK(r.right==MainWnd_DPIScale(460)&&r.bottom==MainWnd_DPIScale(244));}
    Audio_Shutdown();}
static void docking_follow_detach_and_scale(){
    SetWindowPos(mainWindow,nullptr,500,500,400,200,SWP_NOZORDER|SWP_NOACTIVATE);
    for(int side=0;side<4;++side){
        RECT parent{},panel{};GetWindowRect(mainWindow,&parent);GetWindowRect(eqWindow,&panel);
        int w=panel.right-panel.left,h=panel.bottom-panel.top;
        int x=parent.left+8,y=parent.bottom+20;
        if(side==1)y=parent.top-h-20;
        if(side==2){x=parent.right+20;y=parent.top+8;}
        if(side==3){x=parent.left-w-20;y=parent.top+8;}
        RECT proposed{x,y,x+w,y+h};
        SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&proposed));
        SetWindowPos(eqWindow,nullptr,proposed.left,proposed.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
        SetWindowPos(mainWindow,nullptr,parent.left+30,parent.top+40,420,220,SWP_NOZORDER|SWP_NOACTIVATE);
        EqWnd_ScaleChanged();GetWindowRect(mainWindow,&parent);GetWindowRect(eqWindow,&panel);
        if(side==0){CHECK(panel.top==parent.bottom);CHECK(panel.left==parent.left);}
        if(side==1){CHECK(panel.bottom==parent.top);CHECK(panel.left==parent.left);}
        if(side==2){CHECK(panel.left==parent.right);CHECK(panel.top==parent.top);}
        if(side==3){CHECK(panel.right==parent.left);CHECK(panel.top==parent.top);}
    }
    RECT detached{1600,1500,1600+MainWnd_DPIScale(460),1500+MainWnd_DPIScale(244)};
    SendMessageW(eqWindow,WM_MOVING,0,reinterpret_cast<LPARAM>(&detached));
    SetWindowPos(eqWindow,nullptr,detached.left,detached.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    SendMessageW(eqWindow,WM_EXITSIZEMOVE,0,0);
    SetWindowPos(mainWindow,nullptr,500,500,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    RECT panel{};GetWindowRect(eqWindow,&panel);CHECK(panel.left==1600&&panel.top==1500);
}
int main(){CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);Ui::ThemeConfigure(nullptr);CHECK(MainWnd_RegisterClass(GetModuleHandleW(nullptr)));
    mainWindow=MainWnd_Create(GetModuleHandleW(nullptr),SW_HIDE);CHECK(mainWindow);Audio_Shutdown();EqWnd_Toggle(mainWindow);
    EnumThreadWindows(GetCurrentThreadId(),Find,0);CHECK(eqWindow);int cases=0;CASE(controls_publish_dsp_and_hidden_processing);CASE(all_themes_preserve_parameters);CASE(docking_follow_detach_and_scale);
    DestroyWindow(mainWindow);CoUninitialize();std::printf("%d EQ window cases\n",cases);}

