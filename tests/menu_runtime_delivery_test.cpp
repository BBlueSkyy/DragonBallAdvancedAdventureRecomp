// SDL software backend -> real TouchHub -> actual VBlank callback -> GbaIo ->
// authored ARM LDRH consumer. This does not execute the private game's menu code.
#include "../src/dragonball_menu_touch.h"
#include "recorded_menu_frames.h"
#include "host_window.h"
#include "runtime.h"
#include "runtime_arm.h"
#include "runtime_bus_bridge.h"
#include "runtime_context.h"
#include "gba_bus.h"
#include "gba_ppu.h"
#include "interpreter.h"
#include "arm_decode.h"
#include <SDL.h>
#include <cstdio>
#include <functional>
void runtime_set_vblank_input_hook(std::function<void()>);
#include <memory>
using namespace dragonball::menu_touch;
struct DispatchEntry {uint32_t pc;uint8_t thumb,resume;void(*fn)();};
extern "C" const DispatchEntry kDispatchTable[]={{0,0,0,nullptr}};
extern "C" const unsigned kDispatchTableLen=0;
extern "C" const DispatchEntry kBiosDispatchTable[]={{0,0,0,nullptr}};
extern "C" const unsigned kBiosDispatchTableLen=0;
int main(){
 SDL_setenv("SDL_VIDEODRIVER","dummy",1);SDL_setenv("SDL_AUDIODRIVER","dummy",1);
 SDL_setenv("SDL_RENDER_DRIVER","software",1);SDL_setenv("GBARECOMP_TOUCH_CONTROLS","1",1);
 int failures=0;auto check=[&](bool ok,const char* msg){if(!ok){++failures;std::fprintf(stderr,"FAIL: %s (%s)\n",msg,SDL_GetError());}};
 auto bus=std::make_unique<gba::GbaBus>();auto ppu=std::make_unique<gba::GbaPpu>();
 bus->set_rom(nullptr,0);bus->io().set_bus(bus.get());bus->io().set_ppu(ppu.get());
 gbarecomp::set_active_bus(bus.get());gbarecomp::set_active_ppu(ppu.get());runtime_init(bus.get());
 gbarecomp::runtime_restore_timing_context({});g_cpu={};g_cpu.cpsr=0x9f;
 gbarecomp::HostWindow win;check(win.open(4,240,160,"menu-delivery",nullptr,false,false),"open actual SDL host backend");
 if(!win.is_open())return 1;
 auto& hub=gbarecomp::TouchHub::instance();hub.reset_for_tests();
 gbarecomp::RunOptions opts{};opts.input_frame=menu_input_frame;opts.mobile_menu_frame_probe=observe_menu_frame;
 opts.touch_gesture_claims=gbarecomp::kTouchClaimTap|gbarecomp::kTouchClaimLongPress;
 win.configure_touch(true,opts.touch_gesture_claims,1,false);win.configure_mobile_action_hud(true,nullptr);
 win.set_mobile_menu_frame_probe(opts.mobile_menu_frame_probe);
 unsigned vblanks=0,down_events=0,up_events=0,a_edges=0,b_edges=0;uint16_t previous=0;
 Observation state{};
 // Authored guest instruction reads the emulated register, not the policy's
 // return value. The test navigation model consumes its active-low key edges.
 auto guest_read=[&](){armv4t::CPUState cpu{};cpu.R[0]=0x04000130;cpu.R[15]=0x03000000;cpu.cpsr.mode=0x1f;
  auto insn=armv4t::ArmDecoder::decode(0xe1d010b0,0x03000000);uint32_t cycles=0;
  const auto result=armv4t::Interpreter::step(cpu,*bus,insn,&cycles);check(result!=armv4t::Interpreter::Result::NotImplemented,"execute ARM KEYINPUT read");return uint16_t(cpu.R[1]);};
 runtime_set_vblank_input_hook([&](){
  ++vblanks;auto batch=hub.drain(ppu->frame_count(),gbarecomp::touch_clock_ms());const auto p=hub.presentation();
  gbarecomp::TouchFrameInfo info{};info.frame_count=ppu->frame_count();info.host_ms=batch.now_ms;
  info.host_keyinput=bus->io().host_keyinput();info.view_width=p.view_width;info.view_height=p.view_height;
  info.game_rect=p.layout;info.drawable_width=p.drawable_width;info.drawable_height=p.drawable_height;
  info.events=batch.events.data();info.event_count=batch.events.size();info.gestures=batch.gestures.data();info.gesture_count=batch.gestures.size();
  info.points=batch.points.data();info.point_count=batch.points.size();
  for(auto e:batch.events){down_events+=e.phase==gbarecomp::TouchPhase::Down;up_events+=e.phase==gbarecomp::TouchPhase::Up;}
  bus->io().set_synthesized_keyinput(opts.input_frame(&info));
  const auto read=guest_read();check(read==bus->io().composed_keyinput(),"ARM sees composed KEYINPUT, including traditional controls");
  const uint16_t held=uint16_t(~read)&0x3ff,edge=held&~previous;previous=held;
  a_edges+=bool(edge&1);b_edges+=bool(edge&2);
  if(state.scene==Scene::Game){if(edge&(1<<7))state.selected|=2;if(edge&(1<<6))state.selected&=1;if(edge&(1<<4))state.selected|=1;if(edge&(1<<5))state.selected&=2;}
  else if(state.scene!=Scene::None){if(edge&(1<<7))++state.selected;if(edge&(1<<6))--state.selected;}
  observed.store(pack(state));
 });
 auto next_frame=[&](){const auto before=vblanks;while(vblanks==before)runtime_tick(ppu->cycles_until_next_event());};
 auto event=[&](Uint32 type,float x,float y,int id=19){const auto p=hub.presentation();float dx,dy;
  gbarecomp::logical_point_to_presentation(p.layout,p.view_width,p.view_height,x,y,&dx,&dy);
  SDL_Event e{};e.type=type;e.tfinger.type=type;e.tfinger.touchId=17;e.tfinger.fingerId=id;
  e.tfinger.x=dx/p.drawable_width;e.tfinger.y=dy/p.drawable_height;
  check(SDL_PushEvent(&e)==1,"queue SDL finger event");auto out=win.pump();bus->io().set_keyinput(out.keyinput);
 };
 auto* window=SDL_GetWindowFromID(1);check(window!=nullptr,"SDL test window exists");
 for(auto size:{std::pair{2520,1080},std::pair{1920,1080},std::pair{1080,2400}}){
  SDL_SetWindowSize(window,size.first,size.second);
  for(int scene=1;scene<=3;++scene){int count=scene==1?3:scene==2?4:2;
   for(int target=0;target<count;++target){
    auto im=scene==1?recorded::frame(recorded::Data):scene==2?recorded::frame(recorded::Game):recorded::frame(recorded::Pause);
    pending.clear();win.present(im.data());state=unpack(observed.load());previous=0;const auto firstA=a_edges,firstD=down_events,firstU=up_events;
    const float x=scene==2?(target%2?165:78):scene==3?120:100;
    const float y=scene==1?60+32*target:scene==2?(target/2?110:70):target?68:60;
    // Down and Up both arrive before one VBlank: the actual short-tap case.
    event(SDL_FINGERDOWN,x,y);event(SDL_FINGERUP,x,y);
    for(int f=0;f<24;++f)next_frame();
    check(down_events==firstD+1&&up_events==firstU+1,"both SDL edges arrive through TouchHub");
    check(state.selected==target&&a_edges==firstA+1,"selected target receives exactly one guest A edge");
   }
  }
 }
 // RETURN sends B, not an A fallback; then a prolonged hold acts only once.
 auto im=recorded::frame(recorded::Game);win.present(im.data());state=unpack(observed.load());pending.clear();previous=0;
 const auto oldB=b_edges;event(SDL_FINGERDOWN,200,36);event(SDL_FINGERUP,200,36);for(int i=0;i<8;++i)next_frame();check(b_edges==oldB+1,"RETURN consumed once by ARM input reader");
 win.present(im.data());state=unpack(observed.load());pending.clear();previous=0;auto firstA=a_edges;
 event(SDL_FINGERDOWN,78,70);for(int i=0;i<60;++i)next_frame();
 hub.advance(gbarecomp::touch_clock_ms()+1000);check(hub.take_host_actions().empty(),"claimed LongPress produces no host settings shortcut");
 event(SDL_FINGERUP,78,70);for(int i=0;i<8;++i)next_frame();check(a_edges==firstA+1,"long hold/release cannot double confirm");
 state={};observed.store(0);pending.clear();firstA=a_edges;event(SDL_FINGERDOWN,100,95);event(SDL_FINGERUP,100,95);for(int i=0;i<8;++i)next_frame();check(a_edges==firstA,"no synthesized menu action in gameplay");
 runtime_set_vblank_input_hook(nullptr);runtime_shutdown();gbarecomp::set_active_bus(nullptr);gbarecomp::set_active_ppu(nullptr);win.close();
 std::printf("SDL/TouchHub/VBlank/KEYINPUT/ARM: %u callbacks, %u Downs, %u Ups; %d failures\n",vblanks,down_events,up_events,failures);return failures?1:0;
}
