#include "ui_draw.h"
#include "mainwnd.h"
#include <algorithm>
#include <vector>
#include <windowsx.h>
#include <commctrl.h>

namespace Ui {
namespace {
HWND window=nullptr,list=nullptr,categories=nullptr;
HFONT font=nullptr,small=nullptr;
HBRUSH background=nullptr;
float scale=1;
Canvas canvas;
bool scrollDragging=false;
std::vector<size_t> visible;
const wchar_t* groups[]={L"Todos",L"Essenciais",L"Sazonais",L"Comemorativos",L"Tecnológicos",L"Naturais",L"Clássicos"};
int S(int value) { return static_cast<int>(value*scale+.5f); }
void Rebuild() {
    if(background) DeleteObject(background);
    background=CreateSolidBrush(NativeColor(Surface));
    HFONT oldFont=font,oldSmall=small;
    font=CreateFontW(-S(12),0,0,0,400,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,DEFAULT_PITCH,ActiveTheme().typography.body);
    small=CreateFontW(-S(10),0,0,0,400,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    for(int id : {701,702,703,704,705,706}) SendDlgItemMessageW(window,id,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);
    if(oldFont) DeleteObject(oldFont);
    if(oldSmall) DeleteObject(oldSmall);
    SendMessageW(list,LB_SETITEMHEIGHT,0,S(42));
    InvalidateRect(window,nullptr,TRUE);InvalidateRect(list,nullptr,TRUE);
}
void Populate() {
    int group=static_cast<int>(SendMessageW(categories,CB_GETCURSEL,0,0));
    SetWindowTextW(GetDlgItem(window,706),groups[std::max(0,group)]);
    visible.clear();SendMessageW(list,LB_RESETCONTENT,0,0);
    for(size_t i=0;i<ThemeCount();++i) if(group<=0 || !wcscmp(groups[group],ThemeAt(i).category)) {
        int row=static_cast<int>(visible.size());visible.push_back(i);
        SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(ThemeAt(i).name));
        if(!wcscmp(ActiveTheme().id,ThemeAt(i).id)) SendMessageW(list,LB_SETCURSEL,row,0);
    }
    InvalidateRect(window,nullptr,FALSE);
}
void Apply(size_t index) {
    if(!SelectTheme(ThemeAt(index).id))
        MessageBoxW(window,L"Tema aplicado, mas não foi possível salvar a preferência.",L"LiteSic",MB_OK|MB_ICONWARNING);
}
int Rows() { return 7; }
void ScrollTo(int y) {
    int count=static_cast<int>(visible.size());
    int top=std::clamp((y-S(80))*std::max(0,count-Rows())/S(294),0,std::max(0,count-Rows()));
    SendMessageW(list,LB_SETTOPINDEX,top,0);InvalidateRect(window,nullptr,FALSE);
}
LRESULT CALLBACK ButtonProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR) {
    if(msg==WM_MOUSEMOVE) {
        TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE,hwnd,0};TrackMouseEvent(&track);InvalidateRect(hwnd,nullptr,FALSE);
    }
    if(msg==WM_MOUSELEAVE) InvalidateRect(hwnd,nullptr,FALSE);
    if(msg==WM_NCDESTROY) RemoveWindowSubclass(hwnd,ButtonProc,1);
    return DefSubclassProc(hwnd,msg,wp,lp);
}
LRESULT CALLBACK ListProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR) {
    if(msg==WM_NCDESTROY) RemoveWindowSubclass(hwnd,ListProc,1);
    LRESULT result=DefSubclassProc(hwnd,msg,wp,lp);
    if(msg==WM_MOUSEWHEEL || msg==WM_KEYDOWN || msg==WM_LBUTTONUP) InvalidateRect(window,nullptr,FALSE);
    return result;
}
LRESULT CALLBACK Proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    LRESULT menuResult{};if(ThemeMenuMessage(msg,wp,lp,menuResult)) return menuResult;
    switch(msg) {
    case WM_CREATE:
        window=hwnd;
        categories=CreateWindowExW(0,L"COMBOBOX",L"",WS_CHILD|CBS_DROPDOWNLIST,
            S(12),S(12),S(260),S(220),hwnd,reinterpret_cast<HMENU>(701),GetModuleHandleW(nullptr),nullptr);
        for(auto group:groups) SendMessageW(categories,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(group));
        SendMessageW(categories,CB_SETCURSEL,0,0);
        list=CreateWindowExW(0,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS,
            S(12),S(80),S(482),S(294),hwnd,reinterpret_cast<HMENU>(702),GetModuleHandleW(nullptr),nullptr);
        CreateWindowExW(0,L"BUTTON",L"Restaurar Original",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            S(12),S(386),S(180),S(30),hwnd,reinterpret_cast<HMENU>(703),GetModuleHandleW(nullptr),nullptr);
        CreateWindowExW(0,L"BUTTON",L"Fechar",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            S(398),S(386),S(110),S(30),hwnd,reinterpret_cast<HMENU>(704),GetModuleHandleW(nullptr),nullptr);
        CreateWindowExW(0,L"BUTTON",L"Todos",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            S(12),S(44),S(260),S(28),hwnd,reinterpret_cast<HMENU>(706),GetModuleHandleW(nullptr),nullptr);
        CreateWindowExW(0,L"BUTTON",L"×",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,
            S(488),S(4),S(28),S(24),hwnd,reinterpret_cast<HMENU>(705),GetModuleHandleW(nullptr),nullptr);
        SetWindowSubclass(list,ListProc,1,0);
        for(int id:{703,704,705,706}) SetWindowSubclass(GetDlgItem(hwnd,id),ButtonProc,1,0);
        Rebuild();Populate();return 0;
    case WM_THEME_CHANGED:
        Rebuild();Populate();return 0;
    case WM_COMMAND:
        if(LOWORD(wp)==701 && HIWORD(wp)==CBN_SELCHANGE) Populate();
        if(LOWORD(wp)==702 && HIWORD(wp)==LBN_SELCHANGE) {
            int row=static_cast<int>(SendMessageW(list,LB_GETCURSEL,0,0));
            if(row>=0 && static_cast<size_t>(row)<visible.size()) Apply(visible[row]);
        }
        if(LOWORD(wp)==703) {SendMessageW(categories,CB_SETCURSEL,0,0);Apply(0);Populate();}
        if(LOWORD(wp)==704 || LOWORD(wp)==705) DestroyWindow(hwnd);
        if(LOWORD(wp)==706) {
            HMENU menu=CreatePopupMenu();
            int group=static_cast<int>(SendMessageW(categories,CB_GETCURSEL,0,0));
            for(int i=0;i<7;++i) AppendMenuW(menu,MF_STRING|(group==i?MF_CHECKED:0),710+i,groups[i]);
            RECT r{};GetWindowRect(GetDlgItem(hwnd,706),&r);
            int result=TrackThemedMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,r.left,r.bottom,hwnd,scale);
            DestroyMenu(menu);
            if(result>=710 && result<=716) {SendMessageW(categories,CB_SETCURSEL,result-710,0);Populate();}
        }
        return 0;
    case WM_MEASUREITEM: reinterpret_cast<MEASUREITEMSTRUCT*>(lp)->itemHeight=S(42);return TRUE;
    case WM_DRAWITEM: {
        auto draw=reinterpret_cast<DRAWITEMSTRUCT*>(lp);
        if(draw->CtlType==ODT_BUTTON) {
            RECT r=draw->rcItem;HDC dc=draw->hDC;
            POINT cursor{};GetCursorPos(&cursor);ScreenToClient(draw->hwndItem,&cursor);
            HBRUSH brush=CreateSolidBrush(NativeColor(draw->itemState&ODS_SELECTED?palette.pressed:PtInRect(&r,cursor)?Hover:Surface));
            FillRect(dc,&r,brush);DeleteObject(brush);
            brush=CreateSolidBrush(NativeColor(draw->itemState&ODS_FOCUS?Green:Border));FrameRect(dc,&r,brush);DeleteObject(brush);
            auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,NativeColor(Text));
            wchar_t text[128]{};GetWindowTextW(draw->hwndItem,text,128);
            std::wstring label=text;if(draw->CtlID==706) label+=L"  ▾";
            DrawTextW(dc,label.c_str(),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);SelectObject(dc,old);
            return TRUE;
        }
        if(draw->CtlID!=702 || draw->itemID>=visible.size()) return TRUE;
        const auto& theme=ThemeAt(visible[draw->itemID]);
        auto p=theme.palette;HDC dc=draw->hDC;RECT r=draw->rcItem;
        auto fill=[&](RECT area,uint32_t color) {HBRUSH b=CreateSolidBrush(NativeColor(color));FillRect(dc,&area,b);DeleteObject(b);};
        fill(r,p.surface);SetBkMode(dc,TRANSPARENT);
        bool active=!wcscmp(theme.id,ActiveTheme().id);
        RECT title{r.left+S(10),r.top+S(2),r.right-S(180),r.top+S(24)};
        auto old=SelectObject(dc,font);SetTextColor(dc,NativeColor(p.text));
        std::wstring caption=(active?L"✓ ":L"")+std::wstring(theme.name);
        DrawTextW(dc,caption.c_str(),-1,&title,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|DT_NOPREFIX);
        SelectObject(dc,small);SetTextColor(dc,NativeColor(p.muted));
        title.top=r.top+S(23);title.bottom=r.bottom-S(2);
        DrawTextW(dc,theme.category,-1,&title,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
        int x=r.right-S(164);
        for(auto color : {p.background,p.border,p.green,p.accent,p.text}) {
            fill({x,r.top+S(10),x+S(26),r.top+S(32)},color);x+=S(30);
        }
        SelectObject(dc,old);
        if(active || (draw->itemState&ODS_FOCUS)) {
            HBRUSH b=CreateSolidBrush(NativeColor(active?p.green:p.accent));FrameRect(dc,&r,b);DeleteObject(b);
        }
        return TRUE;
    }
    case WM_CTLCOLORLISTBOX: case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wp),NativeColor(Text));SetBkColor(reinterpret_cast<HDC>(wp),NativeColor(Surface));
        return reinterpret_cast<LRESULT>(background);
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps{};HDC dc=BeginPaint(hwnd,&ps);RECT r{};GetClientRect(hwnd,&r);
        // Keep the custom frame and scroll control usable if Direct2D fails.
        HBRUSH brush=CreateSolidBrush(NativeColor(Background));FillRect(dc,&r,brush);DeleteObject(brush);
        RECT title{0,0,r.right,S(32)};brush=CreateSolidBrush(NativeColor(Border));FillRect(dc,&title,brush);DeleteObject(brush);
        auto oldFont=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,NativeColor(Text));title.left=S(12);title.right-=S(40);
        DrawTextW(dc,L"LITESIC · TEMAS",-1,&title,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);SelectObject(dc,oldFont);
        RECT rail{S(498),S(80),S(508),S(374)};brush=CreateSolidBrush(NativeColor(palette.sliderTrack));FillRect(dc,&rail,brush);DeleteObject(brush);
        int count=static_cast<int>(visible.size()),top=static_cast<int>(SendMessageW(list,LB_GETTOPINDEX,0,0));
        if(count>Rows()) {int h=S(294)*Rows()/count;rail.top+=(S(294)-h)*top/(count-Rows());rail.bottom=rail.top+h;
            brush=CreateSolidBrush(NativeColor(Bronze));FillRect(dc,&rail,brush);DeleteObject(brush);}
        if(canvas.Begin(dc,r,scale)) {
            canvas.Fill({0,0,r.right/scale,r.bottom/scale},Background);
            canvas.Fill({0,0,r.right/scale,32},Border);
            canvas.TextAt({12,0,460,32},L"LITESIC · TEMAS",Text,Font::Heading);
            canvas.Fill({498,80,10,294},palette.sliderTrack,3);
            if(count>Rows()) {float h=294.f*Rows()/count;canvas.Fill({498,80+(294-h)*top/(count-Rows()),10,h},Bronze,3);}
            canvas.End();
        }
        EndPaint(hwnd,&ps);return 0;
    }
    case WM_NCHITTEST: {
        POINT point{GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(hwnd,&point);
        if(point.y<S(32) && point.x<S(488)) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_LBUTTONDOWN:
        if(GET_X_LPARAM(lp)>=S(496) && GET_Y_LPARAM(lp)>=S(80) && GET_Y_LPARAM(lp)<S(374)) {
            scrollDragging=true;SetCapture(hwnd);ScrollTo(GET_Y_LPARAM(lp));
        } return 0;
    case WM_MOUSEMOVE: if(scrollDragging) ScrollTo(GET_Y_LPARAM(lp));return 0;
    case WM_LBUTTONUP: scrollDragging=false;if(GetCapture()==hwnd) ReleaseCapture();return 0;
    case WM_CAPTURECHANGED: scrollDragging=false;return 0;
    case WM_KEYDOWN: if(wp==VK_ESCAPE) DestroyWindow(hwnd);return 0;
    case WM_CLOSE: DestroyWindow(hwnd);return 0;
    case WM_DESTROY:
        canvas.DiscardTarget();DeleteObject(font);DeleteObject(small);DeleteObject(background);font=small=nullptr;background=nullptr;
        window=list=categories=nullptr;return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
}
void ShowThemes(HWND owner) {
    if(window) {ShowWindow(window,SW_SHOW);SetForegroundWindow(window);return;}
    scale=MainWnd_DPIScale(100)/100.f;
    WNDCLASSW cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpfnWndProc=Proc;
    cls.lpszClassName=L"LiteSic.ThemesWnd";cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    if(!RegisterClassW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return;
    RECT parent{};GetWindowRect(owner,&parent);
    RECT r{0,0,S(520),S(430)};DWORD style=WS_POPUP;
    AdjustWindowRectEx(&r,style,FALSE,WS_EX_TOOLWINDOW);
    CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"LiteSic — Aparência — Temas",style|WS_VISIBLE,
        parent.left,parent.top,r.right-r.left,r.bottom-r.top,owner,nullptr,cls.hInstance,nullptr);
}
bool ThemeDialogMessage(MSG* message) {
    if(!window) return false;
    if(message->message==WM_KEYDOWN && message->wParam==VK_ESCAPE &&
       (message->hwnd==window || IsChild(window,message->hwnd))) {DestroyWindow(window);return true;}
    return IsDialogMessageW(window,message)!=FALSE;
}
}
