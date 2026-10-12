#include "ui_accessibility.h"
#include <oleacc.h>
#include "ui_scale.h"
#include "ui_tooltip.h"
#include "i18n.h"
#include "language_selection.h"
#include "eqwnd.h"
#include "skin.h"
#include "mainwnd.h"
#include "ui_draw.h"
#include "eq_state.h"
#include "audio.h"
#include "plwnd.h"
#include <windowsx.h>
#include <cmath>
#include <algorithm>

namespace {
HWND window=nullptr,mainWindow=nullptr;
auto& dock=Skin::Attachment(Skin::Equalizer);
float scale=1,monitorDpi=1;
Ui::Canvas canvas;
int hover=-1,pressed=-1,focus=0;
float dragY=0,dragGain=0;
int S(float v){return int(std::lround(v*scale));}
float Gain(int id){return id==3?Eq::State().preamp:Eq::State().gains[id-4];}
bool Enabled(int id){return id<4 || Eq::Available(id-4,Audio_GetSampleRate());}
void SavePosition(){
    if(!window)return;
    RECT panel{};if(GetWindowRect(window,&panel))
        Ui::SaveEqualizerPosition({panel.left,panel.top,static_cast<int>(dock.side),static_cast<int>(dock.anchor),Skin::Parent(Skin::Equalizer)});
}
bool RestorePosition(RECT& panel,RECT& parent,const RECT& work){
    Ui::PanelPosition saved;if(!Ui::LoadEqualizerPosition(saved))return false;
    dock={static_cast<WindowSnap::Side>(saved.side),static_cast<WindowSnap::Anchor>(saved.anchor)};
    Skin::RestoreParent(Skin::Equalizer,saved.parent);
    panel={saved.left,saved.top,saved.left+S(460),saved.top+S(Ui::EqHeight)};
    if(dock.side==WindowSnap::Side::None){
        MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
        WindowSnap::KeepVisible(panel,GetMonitorInfoW(MonitorFromRect(&panel,MONITOR_DEFAULTTONEAREST),&monitor)?monitor.rcWork:work);
    }else if(saved.parent==Skin::Player){
        WindowSnap::Place(panel,parent,dock);
        RECT group{},peer{},fitted{};UnionRect(&group,&parent,&panel);
        if(PlWnd_DockedRect(parent,peer))UnionRect(&group,&group,&peer);
        fitted=group;WindowSnap::KeepVisible(fitted,work);
        OffsetRect(&parent,fitted.left-group.left,fitted.top-group.top);
        OffsetRect(&panel,fitted.left-group.left,fitted.top-group.top);
        SetWindowPos(mainWindow,nullptr,parent.left,parent.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
    return true;
}
void Hide(HWND hwnd){SavePosition();Eq::Save();ShowWindow(hwnd,SW_HIDE);Skin::Reconcile();Ui::SaveAppearanceValue(L"EqualizerVisible",0);InvalidateRect(mainWindow,nullptr,FALSE);}
Ui::Rect Button(int id){return {76.f+id*128,192,120,18};}
int Hit(float x,float y){
    for(int id=0;id<3;++id)if(Button(id).Contains(x,y))return id;
    if(y>=56&&y<142)for(int i=0;i<11;++i)if(std::abs(x-(60+i*37))<16)return i+3;
    return -1;
}
const char* presetIds[]={"preset.flat","preset.rock","preset.pop","preset.jazz","preset.classical","preset.bass","preset.voice"};
void PresetMenu(HWND hwnd){
    HMENU menu=CreatePopupMenu();for(unsigned i=0;i<Eq::PresetCount();++i)
        AppendMenuW(menu,MF_STRING|(Eq::State().preset==int(i)?MF_CHECKED:0),100+i,I18n::Text(presetIds[i]).c_str());
    POINT p{S(204),S(210)};ClientToScreen(hwnd,&p);
    int id=Ui::TrackThemedMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY,p.x,p.y,hwnd,scale);DestroyMenu(menu);
    if(id>=100 && id<100+int(Eq::PresetCount())){Eq::ApplyPreset(id-100);Eq::Save();}
}
void Activate(HWND hwnd,int id){
    if(id==0){Eq::SetEnabled(!Eq::State().enabled);Eq::Save();}
    if(id==1)PresetMenu(hwnd);
    if(id==2){Eq::ApplyPreset(0);Eq::Save();}
    InvalidateRect(hwnd,nullptr,FALSE);
}
std::wstring Tooltip(int id){
    if(id==0)return I18n::Text(Eq::State().enabled?"tooltip.eq_off":"tooltip.eq_on");
    if(id==1)return I18n::Text("tooltip.presets");
    if(id==2)return I18n::Text("tooltip.reset");
    if(id==14)return I18n::Text("tooltip.hide_eq");
    if(id==15)return I18n::Text("equalizer.help");
    if(id<3||id>13)return {};
    wchar_t gain[32]{};swprintf(gain,32,L"%+.1f",double(Gain(id)));
    if(id==3)return I18n::Format("tooltip.preamp",{{"gain",gain}});
    return I18n::Format(Enabled(id)?"tooltip.band":"tooltip.band_unavailable",{{"frequency",std::to_wstring(int(Eq::Frequencies[id-4]))},{"gain",gain}});
}
void AttachTooltips(HWND hwnd){
    Ui::Tooltips::Attach(hwnd,[](POINT p){
        float x=p.x/scale,y=p.y/scale;int id=Hit(x,y);Ui::Rect r{};
        if(id>=0&&id<3)r=Button(id);
        else if(id>=3)r={44.f+(id-3)*37,56,32,86};
        else if((r={428,0,32,32}).Contains(x,y))id=14;
        else if((r={12,164,436,20}).Contains(x,y))id=15;
        return Ui::Tooltips::Target{id<0?0:id+1,{S(r.x),S(r.y),S(r.x+r.w),S(r.y+r.h)}};
    },[](int id){return Tooltip(id-1);},[]{return scale;});
}
LRESULT CALLBACK Proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    LRESULT result=0;if(Ui::ThemeMenuMessage(message,wp,lp,result))return result;
    switch(message){
    case WM_CREATE:monitorDpi=Ui::WindowDpi(hwnd);scale=Ui::FitScale(monitorDpi*MainWnd_UserScaleMultiplier(),460,int(Ui::EqHeight),Ui::MonitorWork(hwnd));
        SetWindowPos(hwnd,nullptr,0,0,S(460),S(Ui::EqHeight),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);AttachTooltips(hwnd);
        Ui::Accessibility::Attach(hwnd,[hwnd]{std::vector<Ui::Accessibility::Control> controls;
            for(int id=0;id<=14;++id){Ui::Rect r=id<3?Button(id):id==14?Ui::Rect{428,0,32,32}:Ui::Rect{44.f+(id-3)*37,56,32,86};
                LONG flags=STATE_SYSTEM_FOCUSABLE;if(!Enabled(id==14?0:id))flags|=STATE_SYSTEM_UNAVAILABLE;
                if(id==0&&Eq::State().enabled)flags|=STATE_SYSTEM_CHECKED;
                if(GetFocus()==hwnd&&focus==id)flags|=STATE_SYSTEM_FOCUSED;
                auto text=Tooltip(id);controls.push_back({id+1,{S(r.x),S(r.y),S(r.x+r.w),S(r.y+r.h)},text,text,id>=3&&id<=13?std::to_wstring(Gain(id)):L"",id>=3&&id<=13?ROLE_SYSTEM_SLIDER:id==0?ROLE_SYSTEM_CHECKBUTTON:ROLE_SYSTEM_PUSHBUTTON,flags});
            }auto help=I18n::Text("equalizer.help");controls.push_back({16,{S(12),S(164),S(448),S(184)},help,help,L"",ROLE_SYSTEM_STATICTEXT,0});return controls;
        },[hwnd](int id){if(id==15)Hide(hwnd);else if(id<=3)Activate(hwnd,id-1);},[hwnd](int id){focus=id-1;Ui::Accessibility::Focus(hwnd,id);InvalidateRect(hwnd,nullptr,FALSE);},[hwnd](int id,double value){if(id<4||id>14||value<-12||value>12)return false;Eq::SetGain(id-5,float(value));Eq::Save();InvalidateRect(hwnd,nullptr,FALSE);return true;});return 0;
    case WM_ENTERSIZEMOVE:Skin::BeginMove(Skin::Equalizer);return 0;
    case WM_MOVING:Skin::Moving(Skin::Equalizer,*reinterpret_cast<RECT*>(lp),S(24));return TRUE;
    case WM_EXITSIZEMOVE:Skin::EndMove(Skin::Equalizer,S(24));SavePosition();return 0;
    case WM_WINDOWPOSCHANGING:Skin::Constrain(Skin::Equalizer,*reinterpret_cast<WINDOWPOS*>(lp));break;
    case WM_WINDOWPOSCHANGED:Skin::Changed(Skin::Equalizer);break;
    case WM_DISPLAYCHANGE:Skin::Recover();return 0;
    case WM_ERASEBKGND:return 1;
    case I18n::WM_LANGUAGE_CHANGED:
        SetWindowTextW(hwnd,I18n::Format("window.caption",{{"title",I18n::Text("equalizer.title")}}).c_str());
        InvalidateRect(hwnd,nullptr,FALSE);return 0;
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
        if(pressed>=3&&GetCapture()==hwnd)Eq::SetGain(pressed-4,dragGain+(dragY-y)*24/60);
        InvalidateRect(hwnd,nullptr,FALSE);return 0;}
    case WM_MOUSELEAVE:hover=-1;InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_LBUTTONUP:{int id=pressed;pressed=-1;if(GetCapture()==hwnd)ReleaseCapture();
        if(GET_X_LPARAM(lp)>=S(428)&&GET_Y_LPARAM(lp)<S(32))Hide(hwnd);
        else if(id>=0&&id<3&&Hit(GET_X_LPARAM(lp)/scale,GET_Y_LPARAM(lp)/scale)==id)Activate(hwnd,id);
        else if(id>=3)Eq::Save();
        InvalidateRect(hwnd,nullptr,FALSE);return 0;}
    case WM_CAPTURECHANGED:case WM_CANCELMODE:pressed=-1;Eq::Save();InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_APPCOMMAND:return SendMessageW(mainWindow,WM_APPCOMMAND,wp,lp);
    case WM_KEYDOWN:
        {MSG key{hwnd,WM_KEYDOWN,wp,lp,0,{0,0}};if(MainWnd_TranslateShortcut(&key))return 0;}
        if(wp==VK_ESCAPE){Hide(hwnd);return 0;}
        if(wp==VK_TAB){int step=GetKeyState(VK_SHIFT)<0?-1:1;do{focus=(focus+step+14)%14;}while(!Enabled(focus));Ui::Accessibility::Focus(hwnd,focus+1);}
        else if(focus<3&&(wp==VK_SPACE||wp==VK_RETURN))Activate(hwnd,focus);
        else if(focus>=3&&Enabled(focus)){
            float value=Gain(focus);if(wp==VK_UP||wp==VK_RIGHT)value+=.5f;
            else if(wp==VK_DOWN||wp==VK_LEFT)value-=.5f;
            else if(wp==VK_HOME)value=12;else if(wp==VK_END)value=-12;else if(wp==VK_SPACE)value=0;
            Eq::SetGain(focus-4,value);Eq::Save();}
        InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_SETFOCUS:case WM_KILLFOCUS:InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_CLOSE:Hide(hwnd);return 0;
    case WM_DPICHANGED:{if(!lp)return 0;monitorDpi=LOWORD(wp)/96.f;scale=Ui::FitScale(monitorDpi*MainWnd_UserScaleMultiplier(),460,int(Ui::EqHeight),Ui::MonitorWork(*reinterpret_cast<RECT*>(lp)));auto r=reinterpret_cast<RECT*>(lp);
        SetWindowPos(hwnd,nullptr,r->left,r->top,S(460),S(Ui::EqHeight),SWP_NOZORDER|SWP_NOACTIVATE);EqWnd_MainMoved();return 0;}
    case WM_PAINT:{using namespace Ui;PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
        if(canvas.Begin(dc,r,scale)){
            canvas.Fill({0,0,460,EqHeight},Background);canvas.TitleBar(460,I18n::Text("equalizer.title").c_str(),false);
            const auto& state=Eq::State();bool hasFocus=GetFocus()==hwnd;
            canvas.Fill(Button(0),state.enabled?SelectedButtonFill():pressed==0?Border:hover==0?Hover:Surface,3);
            canvas.Stroke(Button(0),hasFocus&&focus==0?Accent:Bronze,3);
            canvas.TextAt(Button(0),state.enabled?I18n::Text("equalizer.on").c_str():I18n::Text("equalizer.off").c_str(),state.enabled?SelectedButtonText:Text,Font::Small,true);
            std::wstring name=I18n::Format("equalizer.preset",{{"name",I18n::Text(state.preset<0?"equalizer.custom":presetIds[state.preset])}});
            for(int id:{1,2}){auto b=Button(id);canvas.Fill(b,pressed==id?Border:hover==id?Hover:Surface,3);canvas.Stroke(b,hasFocus&&focus==id?Accent:Bronze,3);canvas.TextAt(b,id==1?name:I18n::Text("equalizer.reset").c_str(),Text,Font::Small,true);}
            canvas.TextAt({8,190,60,22},L"EQ",Muted,Font::Small);
            canvas.TextAt({12,164,436,20},I18n::Text("equalizer.help").c_str(),Muted,Font::Small,true);
            canvas.Fill({8,40,444,104},Surface,3);
            for(int j=0;j<3;++j){float y=70+j*30;canvas.Line(44,y,446,y,Border);canvas.TextAt({9,y-8,36,16},j==0?L"+12":j==1?L"0":L"−12",Muted,Font::Small);}
            const std::wstring labels[]={I18n::Text("equalizer.preamp").c_str(),L"60",L"170",L"310",L"600",L"1k",L"3k",L"6k",L"12k",L"14k",L"16k"};
            for(int i=0;i<11;++i){int id=i+3;float x=60+i*37,y=100-Gain(id)*60/24;bool enabled=Enabled(id);
                if(i>1)canvas.Line(x-37,100-Gain(id-1)*60/24,x,y,Accent,1.5f);
                canvas.Line(x,70,x,130,enabled?palette.sliderTrack:Border,3);
                canvas.Circle(x,y,pressed==id?6:5,enabled?(hover==id?Accent:palette.sliderFill):Muted,Border);
                if(hasFocus&&focus==id)canvas.Stroke({x-13,60,26,77},Accent,3);
                wchar_t gain[32];swprintf(gain,32,L"%+.1f",double(Gain(id)));
                canvas.TextAt({x-18,42,36,18},enabled?gain:I18n::Text("equalizer.unavailable").c_str(),enabled?Text:Muted,Font::Small,true);
                canvas.TextAt({x-18,145,36,18},labels[i],enabled?Text:Muted,Font::Small,true);
            }
            if(!canvas.End())InvalidateRect(hwnd,nullptr,FALSE);
        }else{SetBkColor(dc,NativeColor(Surface));SetTextColor(dc,NativeColor(Text));DrawTextW(dc,I18n::Text("errors.eq_render").c_str(),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
        EndPaint(hwnd,&ps);return 0;}
    case WM_DESTROY:SavePosition();Eq::Save();Skin::Unbind(Skin::Equalizer);window=nullptr;canvas.Reset();return 0;
    }return DefWindowProcW(hwnd,message,wp,lp);
}
}
void EqWnd_Toggle(HWND main){
    bool restored=false;
    if(!window){mainWindow=main;dock={};Skin::RestoreParent(Skin::Equalizer,Skin::Player);WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);cls.lpfnWndProc=Proc;cls.hInstance=GetModuleHandleW(nullptr);
        cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.lpszClassName=L"LiteSic.EqualizerWnd";
        if(!RegisterClassExW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
        monitorDpi=Ui::WindowDpi(main);scale=MainWnd_DPIScaleFloat(1);RECT parent{};GetWindowRect(main,&parent);MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
        GetMonitorInfoW(MonitorFromWindow(main,MONITOR_DEFAULTTONEAREST),&monitor);
        RECT panel{0,0,S(460),S(Ui::EqHeight)};
        restored=RestorePosition(panel,parent,monitor.rcWork);
        // Prefer the right edge, then another complete side that fits.
        if(!restored)for(auto side:{WindowSnap::Side::Right,WindowSnap::Side::Left,WindowSnap::Side::Bottom,WindowSnap::Side::Top}){
            WindowSnap::Place(panel,parent,{side,WindowSnap::Anchor::Start});
            if(panel.left>=monitor.rcWork.left&&panel.right<=monitor.rcWork.right&&
                panel.top>=monitor.rcWork.top&&panel.bottom<=monitor.rcWork.bottom){dock={side,WindowSnap::Anchor::Start};break;}
        }
        int x=panel.left,y=panel.top;
        if(!restored&&dock.side==WindowSnap::Side::None){
            x=std::max<int>(monitor.rcWork.left,std::min<int>(parent.right,monitor.rcWork.right-S(460)));
            y=std::max<int>(monitor.rcWork.top,std::min<int>(parent.top,monitor.rcWork.bottom-S(Ui::EqHeight)));
        }
        window=CreateWindowExW(0,cls.lpszClassName,I18n::Format("window.caption",{{"title",I18n::Text("equalizer.title")}}).c_str(),WS_POPUP,x,y,S(460),S(Ui::EqHeight),main,nullptr,cls.hInstance,nullptr);
    }if(window){MainWnd_ApplyTopmost(main);Skin::Bind(Skin::Equalizer,window);SavePosition();ShowWindow(window,IsWindowVisible(window)?SW_HIDE:SW_SHOWNOACTIVATE);EqWnd_MainMoved(restored);Ui::SaveAppearanceValue(L"EqualizerVisible",IsWindowVisible(window)!=FALSE);InvalidateRect(window,nullptr,FALSE);InvalidateRect(mainWindow,nullptr,FALSE);}
}
bool EqWnd_IsVisible(){return window&&IsWindowVisible(window);}
void EqWnd_ScaleChanged(){
    scale=Ui::FitScale(monitorDpi*MainWnd_UserScaleMultiplier(),460,int(Ui::EqHeight),Ui::MonitorWork(window));
    if(window){
        SetWindowPos(window,nullptr,0,0,S(460),S(Ui::EqHeight),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        // Resizing alone invalidates only exposed strips, leaving old-scale pixels.
        InvalidateRect(window,nullptr,FALSE);
    }
    EqWnd_MainMoved();
}
bool EqWnd_DockedRect(const RECT& owner,RECT& panel){return Skin::DockedRect(Skin::Equalizer,owner,panel);}
void EqWnd_MainMoved(bool preserveDock){if(preserveDock)Skin::Reconcile();else Skin::Recover();}
void EqWnd_Shutdown(){if(window)DestroyWindow(window);mainWindow=nullptr;dock={};Eq::Save();}
void EqWnd_Restore(HWND main){
    bool visible=Ui::LoadAppearance().equalizerVisible;
    if(visible||Skin::Parent(Skin::Playlist)==Skin::Equalizer){
        EqWnd_Toggle(main);if(!visible)Hide(window);
    }
    Skin::Reconcile();
}
