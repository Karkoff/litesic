#include "eqwnd.h"
#include "window_snap.h"
#include "mainwnd.h"
#include "ui_draw.h"
#include "eq_state.h"
#include "audio.h"
#include <windowsx.h>
#include <cmath>
#include <algorithm>

namespace {
HWND window=nullptr,mainWindow=nullptr;
WindowSnap::Dock dock;
bool moving=false,sizeMoveLoop=false;
POINT moveCursor{}; RECT moveOrigin{};
float scale=1;
Ui::Canvas canvas;
int hover=-1,pressed=-1,focus=0;
float dragY=0,dragGain=0;
int S(float v){return int(std::lround(v*scale));}
float Gain(int id){return id==3?Eq::State().preamp:Eq::State().gains[id-4];}
bool Enabled(int id){return id<4 || Eq::Available(id-4,Audio_GetSampleRate());}
void Hide(HWND hwnd){Eq::Save();ShowWindow(hwnd,SW_HIDE);Ui::SaveAppearanceValue(L"EqualizerVisible",0);}
int Hit(float x,float y){
    if(y>=40&&y<68){if(x>=12&&x<100)return 0;if(x>=112&&x<300)return 1;if(x>=312&&x<448)return 2;}
    if(y>=112&&y<208)for(int i=0;i<11;++i)if(std::abs(x-(60+i*37))<16)return i+3;
    return -1;
}
void PresetMenu(HWND hwnd){
    HMENU menu=CreatePopupMenu();for(unsigned i=0;i<Eq::PresetCount();++i)
        AppendMenuW(menu,MF_STRING|(Eq::State().preset==int(i)?MF_CHECKED:0),100+i,Eq::PresetAt(i).name);
    POINT p{S(112),S(68)};ClientToScreen(hwnd,&p);
    int id=Ui::TrackThemedMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,hwnd,scale);DestroyMenu(menu);
    if(id>=100 && id<100+int(Eq::PresetCount())){Eq::ApplyPreset(id-100);Eq::Save();}
}
void Activate(HWND hwnd,int id){
    if(id==0){Eq::SetEnabled(!Eq::State().enabled);Eq::Save();}
    if(id==1)PresetMenu(hwnd);
    if(id==2){Eq::ApplyPreset(0);Eq::Save();}
    InvalidateRect(hwnd,nullptr,FALSE);
}
LRESULT CALLBACK Proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    LRESULT result=0;if(Ui::ThemeMenuMessage(message,wp,lp,result))return result;
    switch(message){
    case WM_ENTERSIZEMOVE:
        sizeMoveLoop=true;GetCursorPos(&moveCursor);GetWindowRect(hwnd,&moveOrigin);return 0;
    case WM_MOVING:{
        moving=true;dock={};auto proposed=reinterpret_cast<RECT*>(lp);
        POINT cursor{};GetCursorPos(&cursor);
        if(sizeMoveLoop&&(cursor.x!=moveCursor.x||cursor.y!=moveCursor.y))
            OffsetRect(proposed,moveOrigin.left+cursor.x-moveCursor.x-proposed->left,
                moveOrigin.top+cursor.y-moveCursor.y-proposed->top);
        RECT parent{};GetWindowRect(mainWindow,&parent);
        WindowSnap::Snap(*proposed,parent,S(24));return TRUE;}
    case WM_EXITSIZEMOVE:{
        sizeMoveLoop=false;
        if(moving){RECT panel{},parent{};GetWindowRect(hwnd,&panel);GetWindowRect(mainWindow,&parent);
            dock=WindowSnap::Snap(panel,parent,S(24));
            SetWindowPos(hwnd,nullptr,panel.left,panel.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
            moving=false;}
        EqWnd_MainMoved();return 0;}
    case WM_ERASEBKGND:return 1;
    case Ui::WM_THEME_CHANGED:InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_NCHITTEST:{POINT p{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(hwnd,&p);
        if(p.y<S(32)&&p.x<S(428))return HTCAPTION;
        return HTCLIENT;}
    case WM_LBUTTONDOWN:{float x=GET_X_LPARAM(lp)/scale,y=GET_Y_LPARAM(lp)/scale;
        SetFocus(hwnd);pressed=Hit(x,y);if(pressed>=0&&Enabled(pressed)){focus=pressed;SetCapture(hwnd);
            if(pressed>=3){dragY=y;dragGain=Gain(pressed);}}else pressed=-1;
        InvalidateRect(hwnd,nullptr,FALSE);return 0;}
    case WM_MOUSEMOVE:{float x=GET_X_LPARAM(lp)/scale,y=GET_Y_LPARAM(lp)/scale;
        hover=Hit(x,y);TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);
        if(pressed>=3&&GetCapture()==hwnd)Eq::SetGain(pressed-4,dragGain+(dragY-y)*24/70);
        InvalidateRect(hwnd,nullptr,FALSE);return 0;}
    case WM_MOUSELEAVE:hover=-1;InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_LBUTTONUP:{int id=pressed;pressed=-1;if(GetCapture()==hwnd)ReleaseCapture();
        if(GET_X_LPARAM(lp)>=S(428)&&GET_Y_LPARAM(lp)<S(32))Hide(hwnd);
        else if(id>=0&&id<3&&Hit(GET_X_LPARAM(lp)/scale,GET_Y_LPARAM(lp)/scale)==id)Activate(hwnd,id);
        else if(id>=3)Eq::Save();
        InvalidateRect(hwnd,nullptr,FALSE);return 0;}
    case WM_CAPTURECHANGED:case WM_CANCELMODE:pressed=-1;Eq::Save();InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_KEYDOWN:
        if(wp==VK_ESCAPE){Hide(hwnd);return 0;}
        if(wp==VK_TAB){int step=GetKeyState(VK_SHIFT)<0?-1:1;do{focus=(focus+step+14)%14;}while(!Enabled(focus));}
        else if(focus<3&&(wp==VK_SPACE||wp==VK_RETURN))Activate(hwnd,focus);
        else if(focus>=3&&Enabled(focus)){
            float value=Gain(focus);if(wp==VK_UP||wp==VK_RIGHT)value+=.5f;
            else if(wp==VK_DOWN||wp==VK_LEFT)value-=.5f;
            else if(wp==VK_HOME)value=12;else if(wp==VK_END)value=-12;else if(wp==VK_SPACE)value=0;
            Eq::SetGain(focus-4,value);Eq::Save();}
        InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_SETFOCUS:case WM_KILLFOCUS:InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_CLOSE:Hide(hwnd);return 0;
    case WM_DPICHANGED:{scale=LOWORD(wp)/96.f*MainWnd_UserScaleMultiplier();auto r=reinterpret_cast<RECT*>(lp);
        SetWindowPos(hwnd,nullptr,r->left,r->top,S(460),S(244),SWP_NOZORDER|SWP_NOACTIVATE);EqWnd_MainMoved();return 0;}
    case WM_PAINT:{using namespace Ui;PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
        if(canvas.Begin(dc,r,scale)){
            canvas.Fill({0,0,460,244},Background);canvas.TitleBar(460,L"LITESIC · EQUALIZADOR",false);
            const auto& state=Eq::State();bool hasFocus=GetFocus()==hwnd;
            canvas.Button({12,40,88,28},state.enabled?L"EQ ON":L"EQ OFF",state.enabled,hover==0,pressed==0,true,hasFocus&&focus==0);
            std::wstring name=L"Preset: ";name+=state.preset<0?L"Custom":Eq::PresetAt(state.preset).name;
            canvas.Button({112,40,188,28},name,false,hover==1,pressed==1,true,hasFocus&&focus==1);
            canvas.Button({312,40,136,28},L"Redefinir",false,hover==2,pressed==2,true,hasFocus&&focus==2);
            canvas.TextAt({12,72,436,20},L"±12 dB · ↑/↓ ajusta · Tab navega · Espaço zera",Muted,Font::Small,true);
            canvas.Fill({8,96,444,114},Surface,3);
            for(int j=0;j<3;++j){float y=126+j*35;canvas.Line(44,y,446,y,Border);canvas.TextAt({9,y-8,36,16},j==0?L"+12":j==1?L"0":L"−12",Muted,Font::Small);}
            const wchar_t* labels[]={L"PRE",L"60",L"170",L"310",L"600",L"1k",L"3k",L"6k",L"12k",L"14k",L"16k"};
            for(int i=0;i<11;++i){int id=i+3;float x=60+i*37,y=161-Gain(id)*70/24;bool enabled=Enabled(id);
                if(i>1)canvas.Line(x-37,161-Gain(id-1)*70/24,x,y,Accent,1.5f);
                canvas.Line(x,126,x,196,enabled?palette.sliderTrack:Border,3);
                canvas.Circle(x,y,pressed==id?6:5,enabled?(hover==id?Accent:palette.sliderFill):Muted,Border);
                if(hasFocus&&focus==id)canvas.Stroke({x-13,116,26,87},Accent,3);
                wchar_t gain[32];swprintf(gain,32,L"%+.1f",double(Gain(id)));
                canvas.TextAt({x-18,98,36,18},enabled?gain:L"N/A",enabled?Text:Muted,Font::Small,true);
                canvas.TextAt({x-18,211,36,18},labels[i],enabled?Text:Muted,Font::Small,true);
            }
            bool boost=state.preamp>0;for(float gain:state.gains)boost=boost||gain>0;
            canvas.TextAt({8,228,444,16},boost?L"Ganhos positivos podem clipar: reduza o PRE.":L"Bandas N/A: indisponíveis nesta taxa; valores preservados.",boost?palette.warning:Muted,Font::Small,true);
            if(!canvas.End())InvalidateRect(hwnd,nullptr,FALSE);
        }else{SetBkColor(dc,NativeColor(Surface));SetTextColor(dc,NativeColor(Text));DrawTextW(dc,L"Equalizador: renderização indisponível",-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
        EndPaint(hwnd,&ps);return 0;}
    case WM_DESTROY:Eq::Save();window=nullptr;canvas.DiscardTarget();return 0;
    }return DefWindowProcW(hwnd,message,wp,lp);
}
}
void EqWnd_Toggle(HWND main){
    if(!window){mainWindow=main;dock={};moving=false;sizeMoveLoop=false;WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);cls.lpfnWndProc=Proc;cls.hInstance=GetModuleHandleW(nullptr);
        cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.lpszClassName=L"LiteSic.EqualizerWnd";
        if(!RegisterClassExW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
        scale=MainWnd_DPIScaleFloat(1);RECT parent{};GetWindowRect(main,&parent);MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
        GetMonitorInfoW(MonitorFromWindow(main,MONITOR_DEFAULTTONEAREST),&monitor);
        RECT panel{0,0,S(460),S(244)};
        // Prefer the right edge, then another complete side that fits.
        for(auto side:{WindowSnap::Side::Right,WindowSnap::Side::Left,WindowSnap::Side::Bottom,WindowSnap::Side::Top}){
            WindowSnap::Place(panel,parent,{side,WindowSnap::Anchor::Start});
            if(panel.left>=monitor.rcWork.left&&panel.right<=monitor.rcWork.right&&
                panel.top>=monitor.rcWork.top&&panel.bottom<=monitor.rcWork.bottom){dock={side,WindowSnap::Anchor::Start};break;}
        }
        int x=panel.left,y=panel.top;
        if(dock.side==WindowSnap::Side::None){
            x=std::max<int>(monitor.rcWork.left,std::min<int>(parent.right,monitor.rcWork.right-S(460)));
            y=std::max<int>(monitor.rcWork.top,std::min<int>(parent.top,monitor.rcWork.bottom-S(244)));
        }
        window=CreateWindowExW(0,cls.lpszClassName,L"LiteSic — Equalizador",WS_POPUP,x,y,S(460),S(244),main,nullptr,cls.hInstance,nullptr);
    }if(window){ShowWindow(window,IsWindowVisible(window)?SW_HIDE:SW_SHOWNOACTIVATE);Ui::SaveAppearanceValue(L"EqualizerVisible",IsWindowVisible(window)!=FALSE);InvalidateRect(window,nullptr,FALSE);}
}
void EqWnd_ScaleChanged(){scale=MainWnd_DPIScaleFloat(1);if(window)SetWindowPos(window,nullptr,0,0,S(460),S(244),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);EqWnd_MainMoved();}
void EqWnd_MainMoved(){
    if(!window||!mainWindow||moving||dock.side==WindowSnap::Side::None||IsIconic(mainWindow))return;
    RECT parent{},panel{};GetWindowRect(mainWindow,&parent);GetWindowRect(window,&panel);
    RECT aligned=panel;WindowSnap::Place(aligned,parent,dock);
    if(aligned.left!=panel.left||aligned.top!=panel.top)
        SetWindowPos(window,nullptr,aligned.left,aligned.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
void EqWnd_Shutdown(){if(window)DestroyWindow(window);mainWindow=nullptr;dock={};Eq::Save();}
void EqWnd_Restore(HWND main){if(Ui::LoadAppearance().equalizerVisible)EqWnd_Toggle(main);}
