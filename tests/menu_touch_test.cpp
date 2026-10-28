// Fast policy checks; actual SDL/TouchHub/VBlank/ARM read coverage lives in
// menu_runtime_delivery_test.cpp. Recorded title/selection provenance is in
// recorded_menu_frames.h; no ROM, BIOS or invented game-state addresses.
#include "../src/dragonball_menu_touch.h"
#include "recorded_menu_frames.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdio>
using namespace dragonball::menu_touch;
constexpr uint16_t neutral=0x3ff,A=0x3fe,B=0x3fd,DOWN=0x37f,UP=0x3bf,RIGHT=0x3ef,LEFT=0x3df;
uint16_t frame(Observation o,const gbarecomp::TouchEvent* e=nullptr,uint16_t host=neutral){observed.store(pack(o));gbarecomp::TouchFrameInfo f{};f.events=e;f.event_count=e?1:0;f.host_keyinput=host;return menu_input_frame(&f);}
gbarecomp::TouchEvent down(float x,float y){gbarecomp::TouchEvent e{};e.phase=gbarecomp::TouchPhase::Down;e.in_view=true;e.view_x=x;e.view_y=y;return e;}
int main(){
 for(auto scene:{Scene::Data,Scene::Game,Scene::Pause}) {
  const auto im=scene==Scene::Data?recorded::frame(recorded::Data):scene==Scene::Game?recorded::frame(recorded::Game):recorded::frame(recorded::Pause);
  auto o=classify(im.data(),240,160);assert(o.scene==scene);assert(o.selected==(scene==Scene::Pause?1:0));
 }
 for(auto scene:{Scene::Data,Scene::Game,Scene::Pause}) {
  const int count=scene==Scene::Data?3:scene==Scene::Game?4:2;
  for(int start=0;start<count;++start)for(int target=0;target<count;++target){
   pending.clear();Observation o{scene,start};auto e=down(scene==Scene::Game?(target%2?165:78):120,scene==Scene::Data?60+32*target:scene==Scene::Game?(target/2?110:70):(target?68:60));
   auto k=frame(o,&e);unsigned guard=0;
   while(o.selected!=target && ++guard<4){
    assert(k==DOWN||k==UP||k==RIGHT||k==LEFT);assert(frame(o)==k);assert(frame(o)==neutral);assert(frame(o)==neutral);assert(frame(o)==neutral);
    o.selected+=(k==DOWN?scene==Scene::Game?2:1:k==UP?scene==Scene::Game?-2:-1:k==RIGHT?1:-1);k=frame(o);
   }
   assert(guard<4);assert(k==A);assert(frame(o)==A);for(int i=0;i<3;++i)assert(frame(o)==neutral);
  }
 }
 pending.clear();Observation o{Scene::Game,3};auto ret=down(200,36);assert(frame(o,&ret)==B);assert(frame(o)==B);assert(frame({})==neutral);
 for(int target=0;target<2;++target){pending.clear();Observation p{Scene::Pause,1};auto e=down(120,target?68:60);auto k=frame(p,&e);if(target==0){assert(k==UP);assert(frame(p)==UP);assert(frame(p)==neutral);assert(frame(p)==neutral);p.selected=0;k=frame(p);}assert(k==A);assert(frame({})==neutral);}
 pending.clear();auto e=down(100,95);assert(frame({Scene::Data,0},&e)==DOWN);for(int i=0;i<40;++i)assert(frame({Scene::Data,0})!=A);assert(pending.scene==Scene::None);
 assert(frame({Scene::Data,0},&e)==DOWN);assert(frame({Scene::Data,-1})==neutral);assert(pending.scene==Scene::None);
 assert(frame({Scene::Data,0},&e,LEFT)==neutral);assert(frame({},&e)==neutral);
 auto blank=down(20,125);assert(frame({Scene::Game,0},&blank)==neutral);
 auto held=down(78,70);assert(frame({Scene::Game,0},&held)==A);assert(frame({Scene::Game,0})==A);for(int i=0;i<80;++i){held.phase=gbarecomp::TouchPhase::Move;assert(frame({Scene::Game,0},&held)==neutral);}held.phase=gbarecomp::TouchPhase::Up;assert(frame({Scene::Game,0},&held)==neutral);
 pending.clear();auto start=down(100,95);assert(frame({Scene::Data,0},&start)==DOWN);
 auto cancel=start;cancel.phase=gbarecomp::TouchPhase::Cancel;assert(frame({Scene::Data,0},&cancel)==neutral);assert(pending.scene==Scene::None);
 std::puts("menu touch: recorded DATA/GAME/PAUSE, acknowledged selection, timeout/fade/physical cancellation, held touch PASS");
}
