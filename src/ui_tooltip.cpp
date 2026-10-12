#include "ui_tooltip.h"
#include "i18n.h"
#include "language_selection.h"
#include <commctrl.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

namespace Ui::Tooltips {
namespace {
constexpr UINT_PTR Subclass=0x4c535454, Timer=0x4c535454;
constexpr UINT Delay=650, Poll=150, Lifetime=8000;
struct Context {
    HWND surface=nullptr,popup=nullptr;
    Hit hit; Describe describe; Scale scale;
    Target target; std::wstring text;
    Canvas canvas;
    IDWriteFactory* factory=nullptr;
    IDWriteTextFormat* format=nullptr;
    IDWriteTextLayout* layout=nullptr;
    unsigned language=0,theme=0;
    float fontScale=0;
    bool shown=false,expired=false;
    DWORD displayed=0;
    ~Context(){canvas.Reset();if(layout)layout->Release();if(format)format->Release();if(factory)factory->Release();}
};
std::map<HWND,std::unique_ptr<Context>> contexts;
Context* Get(HWND hwnd){auto i=contexts.find(hwnd);return i==contexts.end()?nullptr:i->second.get();}
void Cancel(Context& c){KillTimer(c.surface,Timer);if(c.popup)ShowWindow(c.popup,SW_HIDE);c.shown=false;c.target={};c.text.clear();c.expired=false;}
bool Valid(Context& c){
    if(!IsWindowVisible(c.surface)||IsIconic(GetAncestor(c.surface,GA_ROOT))||GetCapture())return false;
    HWND foreground=GetForegroundWindow();
    if(foreground!=GetAncestor(c.surface,GA_ROOT))return false;
    POINT p{};GetCursorPos(&p);HWND under=WindowFromPoint(p);
    if(under!=c.surface)return false;
    ScreenToClient(c.surface,&p);return c.hit(p).id==c.target.id;
}
bool Measure(Context& c,float width,float& w,float& h){
    float scale=c.scale();
    if(!c.factory&&FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),reinterpret_cast<IUnknown**>(&c.factory))))return false;
    if(!c.format||c.language!=I18n::Generation()||c.theme!=ThemeGeneration()||c.fontScale!=scale){
        if(c.format){c.format->Release();c.format=nullptr;}
        const auto& type=ActiveTheme().typography;
        if(FAILED(c.factory->CreateTextFormat(I18n::ScriptFont()?I18n::ScriptFont():type.body,nullptr,
           DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,DWRITE_FONT_STRETCH_NORMAL,type.sizes[0],I18n::LocaleName().c_str(),&c.format)))return false;
        c.format->SetReadingDirection(I18n::IsRtl()?DWRITE_READING_DIRECTION_RIGHT_TO_LEFT:DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
        c.format->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
        c.language=I18n::Generation();c.theme=ThemeGeneration();c.fontScale=scale;
    }
    if(c.layout){c.layout->Release();c.layout=nullptr;}
    if(FAILED(c.factory->CreateTextLayout(c.text.c_str(),static_cast<UINT32>(c.text.size()),c.format,width,10000,&c.layout)))return false;
    DWRITE_TEXT_METRICS m{};HRESULT result=c.layout->GetMetrics(&m);
    w=std::ceil(m.widthIncludingTrailingWhitespace);h=std::ceil(m.height);c.layout->SetMaxWidth(std::max(1.f,w));return SUCCEEDED(result);
}
LRESULT CALLBACK PopupProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    auto c=reinterpret_cast<Context*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(msg==WM_NCCREATE){c=static_cast<Context*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(c));}
    if(msg==WM_NCHITTEST)return HTTRANSPARENT;
    if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_GETTEXT && c){int n=std::min<int>(int(wp)-1,int(c->text.size()));if(n>=0){std::copy_n(c->text.c_str(),n,reinterpret_cast<wchar_t*>(lp));reinterpret_cast<wchar_t*>(lp)[n]=0;return n;}return 0;}
    if(msg==WM_GETTEXTLENGTH&&c)return c->text.size();
    if((msg==WM_PAINT||msg==WM_PRINTCLIENT||msg==WM_PRINT)&&c){
        PAINTSTRUCT ps{};HDC dc=msg==WM_PAINT?BeginPaint(hwnd,&ps):reinterpret_cast<HDC>(wp);RECT r{};GetClientRect(hwnd,&r);float scale=c->scale();
        if(c->canvas.Begin(dc,r,scale)){
            c->canvas.Fill({0,0,r.right/scale,r.bottom/scale},Surface);
            c->canvas.Stroke({.5f,.5f,r.right/scale-1,r.bottom/scale-1},Bronze);
            if(c->layout)c->canvas.TextLayout(10,7,c->layout,Text);
            c->canvas.End();
        }else{
            HBRUSH b=CreateSolidBrush(NativeColor(Surface));FillRect(dc,&r,b);DeleteObject(b);
            b=CreateSolidBrush(NativeColor(Bronze));FrameRect(dc,&r,b);DeleteObject(b);
            float size=ActiveTheme().typography.sizes[0];
            HFONT font=CreateFontW(-int(std::ceil(size*scale)),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,I18n::ScriptFont()?I18n::ScriptFont():ActiveTheme().typography.body);
            auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,NativeColor(Text));
            InflateRect(&r,-int(10*scale),-int(7*scale));DrawTextW(dc,c->text.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX|(I18n::IsRtl()?DT_RTLREADING|DT_RIGHT:0));SelectObject(dc,old);DeleteObject(font);
        }
        if(msg==WM_PAINT)EndPaint(hwnd,&ps);
        return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
bool Present(Context& c,bool force=false){
    auto text=c.describe(c.target.id);if(text.empty()){Cancel(c);return false;}
    if(c.shown&&!force&&text==c.text)return true;
    c.text=std::move(text);
    RECT anchor=c.target.rect;MapWindowPoints(c.surface,nullptr,reinterpret_cast<POINT*>(&anchor),2);
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);if(!GetMonitorInfoW(MonitorFromRect(&anchor,MONITOR_DEFAULTTONEAREST),&monitor))return false;
    float scale=std::clamp(c.scale(),.5f,8.f),w=0,h=0;
    float maxWidth=std::max(20.f,std::min(360.f,(monitor.rcWork.right-monitor.rcWork.left)/scale-20));
    if(!Measure(c,maxWidth,w,h))return false;
    SIZE size{int(std::ceil((w+20)*scale)),int(std::ceil((h+14)*scale))};
    RECT place=Place(anchor,size,monitor.rcWork,int(6*scale));
    if(!c.popup){
        WNDCLASSW cls{};cls.lpfnWndProc=PopupProc;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"LiteSic.TooltipWnd";
        if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return false;
        c.popup=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT,cls.lpszClassName,nullptr,WS_POPUP,
            0,0,1,1,GetAncestor(c.surface,GA_ROOT),nullptr,cls.hInstance,&c);
        if(!c.popup)return false;
    }
    SetWindowPos(c.popup,HWND_TOP,place.left,place.top,place.right-place.left,place.bottom-place.top,SWP_NOACTIVATE|SWP_SHOWWINDOW);
    InvalidateRect(c.popup,nullptr,FALSE);c.shown=true;return true;
}
LRESULT CALLBACK SurfaceProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){
    auto& c=*reinterpret_cast<Context*>(data);
    if(msg==WM_MOUSEMOVE){
        if(GetCapture()||(wp&(MK_LBUTTON|MK_RBUTTON|MK_MBUTTON))){Cancel(c);}
        else{
            Target t=c.hit({GET_X_LPARAM(lp),GET_Y_LPARAM(lp)});
            if(t.id!=c.target.id){Cancel(c);c.target=t;if(t.id)SetTimer(hwnd,Timer,Delay,nullptr);}
            TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);
        }
    }
    if(msg==WM_TIMER&&wp==Timer){
        if(!c.target.id||!Valid(c)){Cancel(c);return 0;}
        if(c.shown&&GetTickCount()-c.displayed>=Lifetime){KillTimer(hwnd,Timer);ShowWindow(c.popup,SW_HIDE);c.shown=false;c.expired=true;return 0;}
        if(!c.expired){bool first=!c.shown;if(Present(c)){if(first)c.displayed=GetTickCount();SetTimer(hwnd,Timer,Poll,nullptr);}else Cancel(c);}
        return 0;
    }
    switch(msg){
    case WM_MOUSELEAVE:case WM_LBUTTONDOWN:case WM_RBUTTONDOWN:case WM_MBUTTONDOWN:case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDOWN:case WM_MOUSEWHEEL:case WM_KEYDOWN:case WM_KILLFOCUS:case WM_CANCELMODE:
    case WM_CAPTURECHANGED:case WM_ENTERSIZEMOVE:case WM_ENTERMENULOOP:case WM_CLOSE:case WM_SIZE:case WM_MOVE:case WM_DPICHANGED:Cancel(c);break;
    case WM_SHOWWINDOW:if(!wp){Cancel(c);for(auto& entry:contexts)if(IsChild(hwnd,entry.first))Cancel(*entry.second);}break;
    case WM_ACTIVATE:if(LOWORD(wp)==WA_INACTIVE){Cancel(c);for(auto& entry:contexts)if(IsChild(hwnd,entry.first))Cancel(*entry.second);}break;
    case WM_NCDESTROY:{Cancel(c);if(c.popup)DestroyWindow(c.popup);RemoveWindowSubclass(hwnd,SurfaceProc,Subclass);auto result=DefSubclassProc(hwnd,msg,wp,lp);contexts.erase(hwnd);return result;}
    }
    LRESULT result=DefSubclassProc(hwnd,msg,wp,lp);
    if(msg==I18n::WM_LANGUAGE_CHANGED||msg==WM_THEME_CHANGED)Refresh();
    return result;
}
}
RECT Place(RECT a,SIZE size,RECT work,int gap){
    int w=std::min<int>(size.cx,work.right-work.left),h=std::min<int>(size.cy,work.bottom-work.top);
    int x=std::clamp<int>(a.left,work.left,work.right-w),y=a.bottom+gap;
    if(y+h>work.bottom)y=a.top-gap-h;
    y=std::clamp<int>(y,work.top,work.bottom-h);return {x,y,x+w,y+h};
}
bool Attach(HWND hwnd,Hit hit,Describe describe,Scale scale){
    if(!hwnd||Get(hwnd)||GetWindowThreadProcessId(hwnd,nullptr)!=GetCurrentThreadId())return false;
    auto c=std::make_unique<Context>();c->surface=hwnd;c->hit=std::move(hit);c->describe=std::move(describe);c->scale=std::move(scale);
    if(!SetWindowSubclass(hwnd,SurfaceProc,Subclass,reinterpret_cast<DWORD_PTR>(c.get())))return false;
    contexts.emplace(hwnd,std::move(c));return true;
}
bool AttachControl(HWND hwnd,const char* key,Scale scale){
    return Attach(hwnd,[hwnd](POINT p){RECT r{};GetClientRect(hwnd,&r);return Target{PtInRect(&r,p)?1:0,r};},[key](int){return I18n::Text(key);},std::move(scale));
}
void Hide(HWND hwnd){if(auto c=Get(hwnd))Cancel(*c);}
void Refresh(){for(auto& entry:contexts){auto& c=*entry.second;if(c.shown){if(!Valid(c)||!Present(c,true))Cancel(c);}}}
Target Resolve(HWND hwnd,POINT p){auto c=Get(hwnd);return c?c->hit(p):Target{};}
std::wstring Description(HWND hwnd,int id){auto c=Get(hwnd);return c?c->describe(id):std::wstring{};}
HWND Popup(HWND hwnd){auto c=Get(hwnd);return c?c->popup:nullptr;}
size_t AttachedCount(){return contexts.size();}
}
