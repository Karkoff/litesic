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
static void transitive_graph_and_cycle_rejection(){
    Graph g;CHECK(g.Attach(2,0,{Side::Right,Anchor::Start}));CHECK(g.Attach(1,2,{Side::Bottom,Anchor::End}));
    CHECK(!g.Attach(2,1,{Side::Top,Anchor::Start}));CHECK(!g.Attach(1,1,{Side::Left,Anchor::Start}));
    CHECK(!g.Attach(1,3,{Side::Left,Anchor::Start}));CHECK(g.Descendant(1,0));
    std::array<RECT,3> r{{{100,100,500,300},{0,0,300,200},{0,0,200,80}}};g.Follow(0,r);
    CHECK(r[2].left==500&&r[2].top==100);CHECK(r[1].right==r[2].right&&r[1].top==r[2].bottom);
    auto original=r;OffsetRect(&r[0],-240,70);g.Follow(0,r);
    for(int i=1;i<3;++i){OffsetRect(&original[i],-240,70);CHECK(EqualRect(&r[i],&original[i]));}
}
static void graph_detachment_and_negative_topologies(){
    for(RECT work:{RECT{-1920,-200,0,880},RECT{0,0,800,600},RECT{1920,0,3840,2160}}){
        Graph g;CHECK(g.Attach(1,0,{Side::Bottom,Anchor::Start}));CHECK(g.Attach(2,1,{Side::Right,Anchor::Start}));
        std::array<RECT,3> r{{{work.left+20,work.top+10,work.left+220,work.top+110},{0,0,180,140},{0,0,100,100}}};g.Follow(0,r);
        RECT group=r[0];for(int i=1;i<3;++i)UnionRect(&group,&group,&r[i]);CHECK(!KeepVisible(group,work));
        RECT before=r[2];g.dock[1]={};OffsetRect(&r[0],50,60);g.Follow(0,r);CHECK(EqualRect(&before,&r[2]));
        OffsetRect(&r[1],-10,5);g.Follow(1,r);OffsetRect(&before,-10,5);CHECK(EqualRect(&before,&r[2]));
    }
}
static void monitor_coverage_preserves_legitimate_spanning(){
    std::vector<RECT> contiguous{{-1920,-200,0,880},{0,-200,1920,880}};
    CHECK(Covered({-200,-100,200,400},contiguous));CHECK(Covered({-1920,-200,1920,880},contiguous));
    CHECK(!Covered({-200,-201,200,400},contiguous));CHECK(!Covered({-200,-100,200,881},contiguous));
    std::vector<RECT> gap{{-1920,0,-100,1080},{0,0,1920,1080}};
    CHECK(!Covered({-200,100,200,400},gap));CHECK(Covered({-400,100,-200,400},gap));
    CHECK(!Covered({0,0,100,100},{}));
}
static void hidden_middle_compacts_all_edges_and_restores(){
    for(auto side:{Side::Bottom,Side::Top,Side::Right,Side::Left})
        for(auto anchor:{Anchor::Start,Anchor::Center,Anchor::End}) {
            Graph g;CHECK(g.Attach(2,0,{side,anchor}));CHECK(g.Attach(1,2,{side,anchor}));
            std::array<RECT,3> r{{{500,500,900,700},{0,0,300,180},{0,0,400,140}}};
            std::vector<RECT> work{{-2000,-2000,3000,3000}};
            g.FollowVisible(0,r,{{true,true,true}},work);auto expanded=r;
            g.FollowVisible(0,r,{{true,true,false}},work);
            RECT expected=expanded[1];Place(expected,r[0],g.dock[1]);CHECK(EqualRect(&expected,&r[1]));
            CHECK(g.parent[1]==2);CHECK(g.dock[1].side==side);
            OffsetRect(&r[0],30,-20);g.FollowVisible(0,r,{{true,true,false}},work);
            OffsetRect(&expected,30,-20);CHECK(EqualRect(&expected,&r[1]));
            g.FollowVisible(0,r,{{true,true,true}},work);
            for(int i=1;i<3;++i){OffsetRect(&expanded[i],30,-20);CHECK(EqualRect(&expanded[i],&r[i]));}
        }
}
static void hidden_corner_compacts_only_when_accessible(){
    Graph g;CHECK(g.Attach(2,0,{Side::Right,Anchor::Start}));CHECK(g.Attach(1,2,{Side::Bottom,Anchor::Start}));
    std::array<RECT,3> r{{{100,100,500,300},{0,0,300,200},{0,0,200,80}}};
    g.FollowVisible(0,r,{{true,true,false}},{{0,0,1000,1000}});
    CHECK(r[1].left==100&&r[1].top==300);
    // The projected location is in a monitor gap: retain the logical position.
    g.FollowVisible(0,r,{{true,true,false}},{{0,0,500,300},{500,0,1000,1000}});
    CHECK(r[1].left==500&&r[1].top==180);
    // Opposing edges would cover the player. Preserve the expanded position.
    CHECK(g.Attach(1,2,{Side::Left,Anchor::Start}));
    CHECK(g.Attach(2,0,{Side::Right,Anchor::Start}));
    r[0]={0,0,400,200};r[1]={0,0,100,100};r[2]={0,0,200,100};
    // Bottom -> Top projects above the work area, while expanded remains reachable.
    CHECK(g.Attach(2,0,{Side::Bottom,Anchor::Start}));CHECK(g.Attach(1,2,{Side::Top,Anchor::Start}));
    g.FollowVisible(0,r,{{true,true,false}},{{0,0,1000,1000}});
    CHECK(r[1].top==100);
}
int main(){int cases=0;CASE(fixed_anchors_and_follow);CASE(tolerance_and_detach);CASE(work_area_edges_and_oversized_panel);CASE(transitive_graph_and_cycle_rejection);CASE(graph_detachment_and_negative_topologies);CASE(monitor_coverage_preserves_legitimate_spanning);CASE(hidden_middle_compacts_all_edges_and_restores);CASE(hidden_corner_compacts_only_when_accessible);std::printf("%d window snap cases\n",cases);}
