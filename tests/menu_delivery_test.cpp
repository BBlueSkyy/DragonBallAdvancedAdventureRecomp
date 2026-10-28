#include "../src/dragonball_menu_touch.h"
#include "recorded_menu_frames.h"
#include "recorded_followup_frames.h"
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
 // Missing scene handlers currently discard these real screen interactions.
 for(auto kind:{0,1,2}) {
  auto im=kind==0?recorded::frame(recorded::Press):kind==1?recorded::frame(recorded::Option):recorded::frame(recorded::Collection);
  observe_menu_frame(im.data(),240,160);pending.clear();
  gbarecomp::TouchEvent tap{};tap.phase=gbarecomp::TouchPhase::Down;tap.in_view=true;
  tap.view_x=kind==0?120:200;tap.view_y=kind==0?120:kind==1?36:26;
  gbarecomp::TouchFrameInfo info{};info.events=&tap;info.event_count=1;
  check(menu_input_frame(&info)==(kind==0?0x3f7:0x3fd),kind==0?"PRESS START sends START, not A":"video OPTION/COLLECTION RETURN sends B");
 }
 // Distinct quick PLAY/BACK taps need distinct edges, separated by releases.
 auto nav=[&](gbarecomp::TouchControl c){gbarecomp::TouchEvent e{};e.phase=gbarecomp::TouchPhase::Down;e.control=c;gbarecomp::TouchFrameInfo f{};f.events=&e;f.event_count=1;return menu_input_frame(&f);};
 auto idle=[&](){gbarecomp::TouchFrameInfo f{};return menu_input_frame(&f);};
 observed.store(pack({Scene::Game,0}));pending.clear();
 check(nav(gbarecomp::TouchControl::Start)==0x3f7,"PLAY first press");
 check(nav(gbarecomp::TouchControl::Back)==0x3f7,"PLAY retains its second sample");
 check(idle()==0x3ff&&idle()==0x3ff,"PLAY two release samples");
 check(idle()==0x3fd&&idle()==0x3fd,"BACK queued behind PLAY is delivered");idle();idle();idle();
 pending.clear();
 check(nav(gbarecomp::TouchControl::Back)==0x3fd,"BACK first press");
 check(nav(gbarecomp::TouchControl::Back)==0x3fd,"second BACK cannot restart the active press");
 check(idle()==0x3ff&&idle()==0x3ff,"BACK keeps two released samples between distinct taps");
 check(idle()==0x3fd&&idle()==0x3fd,"second BACK gets a distinct press");idle();idle();idle();
 // A missing navigation acknowledgement must never confirm the wrong slot.
 observed.store(pack({Scene::Data,0}));pending.clear();
 gbarecomp::TouchEvent down{};down.phase=gbarecomp::TouchPhase::Down;down.in_view=true;down.view_x=100;down.view_y=95;
 gbarecomp::TouchFrameInfo f{};f.events=&down;f.event_count=1;
 menu_input_frame(&f);f.events=nullptr;f.event_count=0;
 bool confirmed=false;for(int i=0;i<32;++i)confirmed|=!(menu_input_frame(&f)&1);
 check(!confirmed,"unacknowledged DOWN must not be followed by A on DATA1");
 std::printf("menu_delivery_test: %d failures\n",errors);return errors?1:0;
}
