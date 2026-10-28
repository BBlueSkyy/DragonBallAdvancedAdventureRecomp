#include "../src/dragonball_menu_touch.h"
#include "recorded_menu_frames.h"
#include <cstdio>
using namespace dragonball::menu_touch;
int main(){int errors=0;auto check=[&](bool ok,const char* msg){if(!ok){++errors;std::fprintf(stderr,"FAIL: %s\n",msg);}};
 for(auto which:{0,1,2}) {
  auto im=which==0?recorded::frame(recorded::Data):which==1?recorded::frame(recorded::Game):recorded::frame(recorded::Pause);
  auto o=classify(im.data(),240,160);
  check(int(o.scene)==which+1,"recorded DATA/GAME/PAUSE title must reach input policy");
  check(o.selected==(which==2?1:0),"recorded selected choice agrees with screenshot");
 }
 auto title=recorded::frame(recorded::Title);check(classify(title.data(),240,160).scene==Scene::None,"title screen is not an interactive menu");
 // A missing navigation acknowledgement must never confirm the wrong slot.
 observed.store(pack({Scene::Data,0}));pending.clear();
 gbarecomp::TouchEvent down{};down.phase=gbarecomp::TouchPhase::Down;down.in_view=true;down.view_x=100;down.view_y=95;
 gbarecomp::TouchFrameInfo f{};f.events=&down;f.event_count=1;
 menu_input_frame(&f);f.events=nullptr;f.event_count=0;
 bool confirmed=false;for(int i=0;i<32;++i)confirmed|=!(menu_input_frame(&f)&1);
 check(!confirmed,"unacknowledged DOWN must not be followed by A on DATA1");
 std::printf("menu_delivery_test: %d failures\n",errors);return errors?1:0;
}
