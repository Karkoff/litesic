#include "skin.h"
#include <array>

namespace Skin {
namespace {
WindowSnap::Graph graph;
std::array<HWND,3> windows{};
std::array<bool,3> dragging{},moveLoop{};
std::array<POINT,3> cursors{};
std::array<RECT,3> origins{};
bool applying=false;
bool Exists(int i){return i>=0&&i<3&&windows[i]&&IsWindow(windows[i]);}
std::array<RECT,3> Rects(){std::array<RECT,3> r{};for(int i=0;i<3;++i)if(Exists(i))GetWindowRect(windows[i],&r[i]);return r;}
bool Work(const RECT& r,RECT& work){MONITORINFO m{};m.cbSize=sizeof(m);if(!GetMonitorInfoW(MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST),&m))return false;work=m.rcWork;return true;}
BOOL CALLBACK CollectWork(HMONITOR monitor,HDC,LPRECT,LPARAM data){
    MONITORINFO info{};info.cbSize=sizeof(info);
    if(GetMonitorInfoW(monitor,&info))reinterpret_cast<std::vector<RECT>*>(data)->push_back(info.rcWork);
    return TRUE;
}
std::vector<RECT> WorkAreas(){std::vector<RECT> areas;EnumDisplayMonitors(nullptr,nullptr,CollectWork,reinterpret_cast<LPARAM>(&areas));return areas;}
void FollowLayout(int root,std::array<RECT,3>& rects){
    std::array<bool,3> visible{};
    for(int i=0;i<3;++i)visible[i]=Exists(i)&&(i==Player||IsWindowVisible(windows[i]));
    graph.FollowVisible(root,rects,visible,WorkAreas());
}
void Apply(const std::array<RECT,3>& rects){
    if(applying)return;
    applying=true;
    for(int i=0;i<3;++i)if(Exists(i)&&!IsIconic(windows[i])){
        RECT old{};GetWindowRect(windows[i],&old);
        if(old.left!=rects[i].left||old.top!=rects[i].top)
            SetWindowPos(windows[i],nullptr,rects[i].left,rects[i].top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    }
    applying=false;
}
void SnapToPeers(Panel id,RECT& proposed,int tolerance,bool remember){
    const auto rects=Rects();int target=-1,best=MAXLONG;WindowSnap::Dock chosen;RECT fitted=proposed;
    for(int peer=0;peer<3;++peer){
        if(peer==id||!Exists(peer)||(peer!=Player&&!IsWindowVisible(windows[peer]))||graph.Descendant(peer,id))continue;
        RECT candidate=proposed;auto dock=WindowSnap::Snap(candidate,rects[peer],tolerance);
        if(dock.side==WindowSnap::Side::None)continue;
        bool overlap=false;RECT intersect{};
        for(int other=0;other<3;++other)if(other!=id&&other!=peer&&Exists(other)&&IsWindowVisible(windows[other])&&!graph.Descendant(other,id))
            overlap|=IntersectRect(&intersect,&candidate,&rects[other])!=FALSE;
        if(overlap&&remember&&id!=Player){
            RECT work{};
            for(int other=0;other<3;++other)if(other!=id&&other!=peer&&Exists(other)&&IsWindowVisible(windows[other])&&Work(candidate,work))
                WindowSnap::PlaceVisible(candidate,rects[peer],dock,work,&rects[other]);
            overlap=false;
            for(int other=0;other<3;++other)if(other!=id&&other!=peer&&Exists(other)&&IsWindowVisible(windows[other]))
                overlap|=IntersectRect(&intersect,&candidate,&rects[other])!=FALSE;
        }
        int distance=std::abs(candidate.left-proposed.left)+std::abs(candidate.top-proposed.top);
        if(!overlap&&distance<best){best=distance;target=peer;chosen=dock;fitted=candidate;}
    }
    proposed=fitted;
    if(!remember)return;
    graph.dock[id]={};
    if(target<0)return;
    if(id==Player){
        // Player remains the root: attach the independent target component
        // in the opposite direction at the same physical shared edge.
        RECT peer=rects[target];auto reverse=WindowSnap::Snap(peer,proposed,0);
        if(reverse.side!=WindowSnap::Side::None)graph.Attach(target,id,reverse);
    }else graph.Attach(id,target,chosen);
}
}
void Bind(Panel id,HWND hwnd){
    if(id==Player&&windows[Player]!=hwnd){graph={};windows={};dragging={};moveLoop={};}
    windows[id]=hwnd;
}
void Unbind(Panel id){windows[id]=nullptr;dragging[id]=moveLoop[id]=false;}
WindowSnap::Dock& Attachment(Panel id){return graph.dock[id];}
int Parent(Panel id){return graph.parent[id];}
void RestoreParent(Panel id,int parent){
    if(parent<0||parent>=3||parent==id||graph.Descendant(parent,id)){
        graph.parent[id]=Player;graph.dock[id]={};
    }else graph.parent[id]=parent;
}
bool ParentRect(Panel id,RECT& rect){int p=graph.parent[id];return p>=0&&p<3&&Exists(p)&&GetWindowRect(windows[p],&rect);}
void BeginMove(Panel id){moveLoop[id]=true;GetCursorPos(&cursors[id]);GetWindowRect(windows[id],&origins[id]);}
void Moving(Panel id,RECT& proposed,int tolerance){
    dragging[id]=true;graph.dock[id]={};POINT cursor{};GetCursorPos(&cursor);
    if(moveLoop[id]&&(cursor.x!=cursors[id].x||cursor.y!=cursors[id].y))
        OffsetRect(&proposed,origins[id].left+cursor.x-cursors[id].x-proposed.left,origins[id].top+cursor.y-cursors[id].y-proposed.top);
    SnapToPeers(id,proposed,tolerance,false);
}
void EndMove(Panel id,int tolerance){
    moveLoop[id]=false;
    if(dragging[id]){RECT r{};GetWindowRect(windows[id],&r);SnapToPeers(id,r,tolerance,true);
        applying=true;SetWindowPos(windows[id],nullptr,r.left,r.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);applying=false;}
    dragging[id]=false;Reconcile();
}
void Constrain(Panel id,WINDOWPOS& p){
    if(applying||!Exists(id)||IsIconic(windows[id])||((p.flags&(SWP_NOMOVE|SWP_NOSIZE))==(SWP_NOMOVE|SWP_NOSIZE)))return;
    auto rects=Rects();RECT old=rects[id];int x=p.flags&SWP_NOMOVE?old.left:p.x,y=p.flags&SWP_NOMOVE?old.top:p.y;
    rects[id]={x,y,x+(p.flags&SWP_NOSIZE?old.right-old.left:p.cx),y+(p.flags&SWP_NOSIZE?old.bottom-old.top:p.cy)};
    FollowLayout(id,rects);RECT group=rects[id],work{};bool attached=false;
    const auto areas=WorkAreas();bool covered=WindowSnap::Covered(rects[id],areas);
    for(int i=0;i<3;++i)if(i!=id&&Exists(i)&&IsWindowVisible(windows[i])&&graph.Descendant(i,id)){
        UnionRect(&group,&group,&rects[i]);attached=true;covered&=WindowSnap::Covered(rects[i],areas);
    }
    if(attached&&!covered&&Work(rects[id],work)){
        RECT fitted=group;WindowSnap::KeepVisible(fitted,work);
        p.x=x+fitted.left-group.left;p.y=y+fitted.top-group.top;p.flags&=~SWP_NOMOVE;
    }
}
void Changed(Panel id){
    if(applying||!Exists(id)||IsIconic(windows[id]))return;
    auto r=Rects();FollowLayout(id,r);Apply(r);
}
void Reconcile(){
    if(applying||!Exists(Player)||IsIconic(windows[Player]))return;
    auto r=Rects();
    for(int i=0;i<3;++i)if(Exists(i)&&(graph.dock[i].side==WindowSnap::Side::None||!Exists(graph.parent[i])))FollowLayout(i,r);
    RECT overlap{},work{};
    if(Exists(Playlist)&&Exists(Equalizer)&&IsWindowVisible(windows[Playlist])&&IsWindowVisible(windows[Equalizer])&&
       graph.dock[Playlist].side!=WindowSnap::Side::None&&graph.dock[Equalizer].side!=WindowSnap::Side::None&&
       graph.parent[Playlist]==graph.parent[Equalizer]&&IntersectRect(&overlap,&r[Playlist],&r[Equalizer])&&Work(r[Player],work))
        WindowSnap::PlaceVisible(r[Playlist],r[graph.parent[Playlist]],graph.dock[Playlist],work,&r[Equalizer]);
    Apply(r);
    // Constrain the complete formation without changing its attachment sides.
    RECT p{};GetWindowRect(windows[Player],&p);WINDOWPOS pos{windows[Player],nullptr,p.left,p.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE};
    Constrain(Player,pos);
    if(pos.x!=p.left||pos.y!=p.top){SetWindowPos(windows[Player],nullptr,pos.x,pos.y,0,0,pos.flags);Changed(Player);}
}
void Recover(){
    if(applying)return;
    Reconcile();auto r=Rects();const auto areas=WorkAreas();
    for(int i=0;i<3;++i)if(Exists(i)&&!IsIconic(windows[i])){
        RECT work{};if(!WindowSnap::Covered(r[i],areas)&&Work(r[i],work)&&WindowSnap::KeepVisible(r[i],work)){
            // A formation larger than the available area cannot retain every
            // edge. Detach only panels that must move to remain accessible.
            if(i!=Player)graph.dock[i]={};
        }
    }
    Apply(r);
}
bool DockedRect(Panel id,const RECT& owner,RECT& rect){
    if(!Exists(id)||dragging[id]||!IsWindowVisible(windows[id])||!graph.Descendant(id,Player))return false;
    auto r=Rects();r[Player]=owner;FollowLayout(Player,r);rect=r[id];return true;
}
}
