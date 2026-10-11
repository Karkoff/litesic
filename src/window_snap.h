#pragma once
#include <windows.h>
#include <cstdlib>
#include <initializer_list>
#include <algorithm>
#include <cwchar>

// Fixed anchors, recomputed from the current sizes (including DPI changes).
namespace WindowSnap {
enum class Side { None, Bottom, Top, Right, Left };
enum class Anchor { Start, Center, End };
struct Dock { Side side=Side::None; Anchor anchor=Anchor::Start; };
// Keep a docked panel reachable when the owner moves beyond a monitor edge.
// If the panel exceeds the work area, retain access to its top/left controls.
inline bool KeepVisible(RECT& panel,const RECT& work) {
    int w=panel.right-panel.left,h=panel.bottom-panel.top;
    int x=std::max<int>(work.left,std::min<int>(panel.left,work.right-w));
    int y=std::max<int>(work.top,std::min<int>(panel.top,work.bottom-h));
    bool moved=x!=panel.left || y!=panel.top;
    panel={x,y,x+w,y+h};return moved;
}
inline int Aligned(int start,int end,int size,Anchor anchor) {
    return anchor==Anchor::Start?start:anchor==Anchor::End?end-size:start+(end-start-size)/2;
}
inline void Place(RECT& panel,const RECT& main,Dock dock) {
    int w=panel.right-panel.left,h=panel.bottom-panel.top;
    int x=panel.left,y=panel.top;
    if(dock.side==Side::Bottom || dock.side==Side::Top) {
        x=Aligned(main.left,main.right,w,dock.anchor);
        y=dock.side==Side::Bottom?main.bottom:main.top-h;
    } else if(dock.side==Side::Right || dock.side==Side::Left) {
        y=Aligned(main.top,main.bottom,h,dock.anchor);
        x=dock.side==Side::Right?main.right:main.left-w;
    }
    panel={x,y,x+w,y+h};
}
inline void PlaceVisible(RECT& panel,const RECT& main,Dock& dock,const RECT& work,const RECT* obstacle=nullptr) {
    auto free=[&](const RECT& r){RECT overlap{};return !obstacle || !IntersectRect(&overlap,&r,obstacle);};
    Place(panel,main,dock);RECT fitted=panel;
    if(!KeepVisible(fitted,work)&&free(panel))return;
    // Prefer another complete edge over obscuring the owner after clamping.
    for(auto side:{Side::Right,Side::Left,Side::Bottom,Side::Top}) {
        RECT candidate=panel;Place(candidate,main,{side,dock.anchor});RECT clamped=candidate;
        if(!KeepVisible(clamped,work)&&free(candidate)){panel=candidate;dock.side=side;return;}
    }
    panel=fitted;dock={};
}
struct PanelSearch { HWND owner,panel;RECT* rect;bool found; };
inline BOOL CALLBACK FindOtherPanel(HWND h,LPARAM p) {
        auto& s=*reinterpret_cast<PanelSearch*>(p);wchar_t cls[80]{};GetClassNameW(h,cls,80);
        if(h!=s.panel && GetWindow(h,GW_OWNER)==s.owner && IsWindowVisible(h)
           && (!std::wcscmp(cls,L"LiteSic.EqualizerWnd")||!std::wcscmp(cls,L"LiteSic.PlaylistWnd"))) {
            s.found=GetWindowRect(h,s.rect)!=FALSE;return FALSE;
        }return TRUE;
}
inline bool OtherPanel(HWND owner,HWND panel,RECT& rect) {
    PanelSearch search{owner,panel,&rect,false};
    EnumThreadWindows(GetCurrentThreadId(),FindOtherPanel,reinterpret_cast<LPARAM>(&search));return search.found;
}
inline Dock Snap(RECT& panel,const RECT& main,int tolerance) {
    Dock dock; int best=tolerance+1;
    auto choose=[&](Side side,int distance) {
        if(std::abs(distance)<best){best=std::abs(distance);dock.side=side;}
    };
    if(panel.left<main.right && panel.right>main.left) {
        choose(Side::Bottom,panel.top-main.bottom);choose(Side::Top,panel.bottom-main.top);
    }
    if(panel.top<main.bottom && panel.bottom>main.top) {
        choose(Side::Right,panel.left-main.right);choose(Side::Left,panel.right-main.left);
    }
    if(dock.side==Side::None)return dock;
    bool horizontal=dock.side==Side::Bottom || dock.side==Side::Top;
    int start=horizontal?main.left:main.top,end=horizontal?main.right:main.bottom;
    int size=horizontal?panel.right-panel.left:panel.bottom-panel.top;
    int position=horizontal?panel.left:panel.top;best=MAXLONG;
    for(auto anchor:{Anchor::Start,Anchor::Center,Anchor::End}) {
        int distance=std::abs(position-Aligned(start,end,size,anchor));
        if(distance<best){best=distance;dock.anchor=anchor;}
    }
    Place(panel,main,dock);return dock;
}
}
