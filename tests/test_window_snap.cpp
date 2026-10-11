#include "window_snap.h"
#include "check.h"
using namespace WindowSnap;
static void fixed_anchors_and_follow() {
    RECT main{100,100,500,300};
    for(auto side:{Side::Bottom,Side::Top,Side::Right,Side::Left})
        for(auto anchor:{Anchor::Start,Anchor::Center,Anchor::End}) {
            RECT panel{0,0,200,80};Dock expected{side,anchor};Place(panel,main,expected);
            RECT exact=panel;OffsetRect(&panel,7,9);
            Dock actual=Snap(panel,main,24);
            CHECK(actual.side==side && actual.anchor==anchor);
            CHECK(EqualRect(&panel,&exact));
            RECT moved{300,200,800,500};Place(panel,moved,actual);
            if(side==Side::Bottom||side==Side::Top) {
                CHECK(panel.left==Aligned(moved.left,moved.right,200,anchor));
                CHECK(side==Side::Bottom?panel.top==moved.bottom:panel.bottom==moved.top);
            } else {
                CHECK(panel.top==Aligned(moved.top,moved.bottom,80,anchor));
                CHECK(side==Side::Right?panel.left==moved.right:panel.right==moved.left);
            }
        }
}
static void tolerance_and_detach() {
    RECT main{-500,-300,-100,-100};
    RECT panel{-490,-76,-290,4};CHECK(Snap(panel,main,24).side==Side::Bottom);
    panel={-490,-75,-290,5};RECT original=panel;
    CHECK(Snap(panel,main,24).side==Side::None);CHECK(EqualRect(&panel,&original));
    panel={-90,-90,110,-10};CHECK(Snap(panel,main,24).side==Side::None);
    panel={-480,-65,-280,15};CHECK(Snap(panel,main,36).side==Side::Bottom);
}
static void work_area_edges_and_oversized_panel(){
    RECT work{-1920,-200,0,880};
    for(RECT panel:{RECT{-2200,-300,-1800,0},RECT{-100,800,300,1100}}){
        CHECK(KeepVisible(panel,work));CHECK(panel.left>=work.left&&panel.right<=work.right);
        CHECK(panel.top>=work.top&&panel.bottom<=work.bottom);CHECK(!KeepVisible(panel,work));
    }
    RECT large{-100,0,2900,1600};CHECK(KeepVisible(large,work));
    CHECK(large.left==work.left&&large.top==work.top);
    RECT main{-400,0,0,200},panel{0,0,600,300};Dock dock{Side::Right,Anchor::Start};
    PlaceVisible(panel,main,dock,work);RECT overlap{};
    CHECK(dock.side==Side::Left&&panel.right==main.left);
    CHECK(!IntersectRect(&overlap,&panel,&main));
}
int main(){int cases=0;CASE(fixed_anchors_and_follow);CASE(tolerance_and_detach);CASE(work_area_edges_and_oversized_panel);std::printf("%d window snap cases\n",cases);}
