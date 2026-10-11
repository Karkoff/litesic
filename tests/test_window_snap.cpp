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
int main(){int cases=0;CASE(fixed_anchors_and_follow);CASE(tolerance_and_detach);std::printf("%d window snap cases\n",cases);}
