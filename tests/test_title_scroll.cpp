#include "title_scroll.h"
#include "check.h"
static void pauses_and_bidirectional_motion(){
    CHECK(TitleScrollOffset(0,4000)==0);CHECK(TitleScrollOffset(100,1000)==0);
    CHECK(std::abs(TitleScrollOffset(100,2750)-50)<.001);
    CHECK(TitleScrollOffset(100,4500)==100);
    CHECK(std::abs(TitleScrollOffset(100,6750)-50)<.001);
    CHECK(TitleScrollOffset(100,8000)==0);
    CHECK(TitleScrollOffset(100,1501)<.001);
    CHECK(TitleScrollOffset(100,3999)>99.999);
    for(int t=0;t<16000;++t){float value=TitleScrollOffset(100,t);CHECK(value>=0&&value<=100);}
}
int main(){int cases=0;CASE(pauses_and_bidirectional_motion);std::printf("%d title scroll cases\n",cases);}
