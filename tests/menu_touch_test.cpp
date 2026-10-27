// Standalone ROM-free tests of the Android Data/Game menu touch policy.
// c++ -std=c++20 -Igbarecomp/src/runtime tests/menu_touch_test.cpp -o /tmp/menu_touch_test
// /tmp/menu_touch_test
#include "../src/dragonball_menu_touch.h"
#include <array>
#include <cassert>
#include <cstdio>

using namespace dragonball::menu_touch;
constexpr unsigned W=240,H=160;
using Frame=std::array<std::uint8_t,W*H*3>;
void pixel(Frame& im,unsigned x,unsigned y,unsigned r,unsigned g,unsigned b) {
    const auto k=(std::size_t(y)*W+x)*3;
    im[k]=std::uint8_t(r);im[k+1]=std::uint8_t(g);im[k+2]=std::uint8_t(b);
}
Frame menu(Scene scene,int selected) {
    Frame f{};
    // Raw color relationships reconstructed from the owner's recorded
    // Android menus, *not* placeholder pure red/blue/gold fixtures.
    // Old border tests incorrectly sampled antialiased/dark positions.
    pixel(f,2,70,236,9,3);
    pixel(f,237,70,255,115,85); // NOT bright red under red_frame()
    pixel(f,30,2,199,19,18);    // NOT bright red under red_frame()
    pixel(f,3,70,253,0,0);
    pixel(f,238,70,250,1,0);
    pixel(f,30,3,252,3,0);
    pixel(f,30,157,253,1,0);
    constexpr unsigned xx[4]={80,95,125,150}, yy[4]={15,15,15,15};
    for(unsigned k=0;k<4;k++)
        if(scene==Scene::Data)pixel(f,xx[k],yy[k],106,22,70);
        else pixel(f,xx[k],yy[k],150,70,11);
    if(scene==Scene::Data) {
        constexpr unsigned y[3]={53,84,117};
        for(unsigned i=0;i<3;i++)
            if(int(i)==selected)
                for(unsigned j=y[i];j<y[i]+5;j++)
                    for(unsigned x=54;x<74;x++)pixel(f,x,j,230,44,26);
    }else if(scene==Scene::Game) {
        constexpr unsigned x[4]={52,143,52,143}, y[4]={67,67,110,110};
        for(unsigned i=0;i<4;i++)
            if(int(i)==selected)
                for(unsigned yy=y[i];yy<y[i]+5;yy++)
                    for(unsigned xx=x[i];xx<x[i]+21;xx++)pixel(f,xx,yy,90,18,20);
    }
    return f;
}
gbarecomp::Gesture tap(float x,float y) {
    gbarecomp::Gesture g{};
    g.kind=gbarecomp::GestureKind::Tap;
    g.start_in_view=true;
    g.view_x=x;g.view_y=y;g.fingers=1;
    return g;
}
std::uint16_t touch(float x,float y) {
    auto g=tap(x,y);
    gbarecomp::TouchFrameInfo frame{};
    frame.gestures=&g;frame.gesture_count=1;
    return menu_input_frame(&frame);
}
std::uint16_t idle() {
    gbarecomp::TouchFrameInfo f{};
    return menu_input_frame(&f);
}
std::uint16_t raw_touch(gbarecomp::TouchPhase phase,float x,float y,
                        bool in_view=true) {
    gbarecomp::TouchEvent e{};
    e.phase=phase;
    e.view_x=x;
    e.view_y=y;
    e.in_view=in_view;
    gbarecomp::TouchFrameInfo f{};
    f.events=&e;
    f.event_count=1;
    return menu_input_frame(&f);
}
int main() {
    constexpr auto A=std::uint16_t(0x3fe),B=std::uint16_t(0x3fd);
    constexpr auto DOWN=std::uint16_t(0x37f);
    constexpr auto RIGHT=std::uint16_t(0x3ef);
    constexpr auto neutral=std::uint16_t(0x3ff);

    const Frame data=menu(Scene::Data,0);
    assert(classify(data.data(),W,H).scene==Scene::Data);
    assert(classify(data.data(),W,H).selected==0);
    assert(classify(data.data(),W,159).scene==Scene::None);
    // Recorded frame reproduces the previously untested failure: old
    // x=237 and top y=2 probes are not bright-red, DATA's title is
    // red-magenta (blue<red), and the game must still recognize both.
    assert(!red_frame(data.data(),W,237,70));
    assert(!red_frame(data.data(),W,30,2));
    auto no_border=data;pixel(no_border,3,70,0,0,0);
    assert(classify(no_border.data(),W,H).scene==Scene::None);
    const Frame other_data=menu(Scene::Data,2);
    assert(classify(other_data.data(),W,H).selected==2);
    const Frame game=menu(Scene::Game,0);
    assert(classify(game.data(),W,H).scene==Scene::Game);
    assert(classify(game.data(),W,H).selected==0);
    for (int i=0;i<4;++i)
        assert(classify(menu(Scene::Game,i).data(),W,H).selected==i);
    for (int i=0;i<3;++i)
        assert(classify(menu(Scene::Data,i).data(),W,H).selected==i);
    // Regression: the old two probes are intentionally dark even though
    // both real menus have the full bright red perimeter.
    assert(!red_frame(data.data(),W,5,6));
    assert(!red_frame(data.data(),W,230,154));
    assert(hit_target(Scene::Data,99,97)==1);
    assert(hit_target(Scene::Game,155,113)==3);

    observe_menu_frame(nullptr,0,0);
    assert(touch(99,96)==neutral); // no accidental gameplay A
    const Frame fading_data=menu(Scene::Data,-1);
    observe_menu_frame(fading_data.data(),W,H);
    assert(touch(90,96)==neutral); // no arbitrary data slot during fade
    assert(touch(200,36)==neutral); // RETURN must not fire in ambiguous fade
    observe_menu_frame(data.data(),W,H);
    assert(touch(99,96)==DOWN); // tap DATA2 from selected DATA1
    assert(idle()==DOWN);
    assert(idle()==neutral);
    assert(idle()==neutral);
    assert(idle()==A);
    assert(idle()==A);
    assert(idle()==neutral);
    assert(idle()==neutral);
    assert(idle()==neutral);

    observe_menu_frame(game.data(),W,H);
    assert(touch(165,110)==DOWN);  // STORY -> OPTION: down then right
    assert(idle()==DOWN);
    assert(idle()==neutral);assert(idle()==neutral);
    assert(idle()==RIGHT);assert(idle()==RIGHT);
    assert(idle()==neutral);assert(idle()==neutral);
    assert(idle()==A);assert(idle()==A);
    assert(idle()==neutral);assert(idle()==neutral);
    observe_menu_frame(game.data(),W,H);
    assert(touch(200,36)==B); // RETURN area
    observe_menu_frame(nullptr,0,0);
    assert(idle()==neutral); // queued keys cancelled when gameplay begins
    assert(touch(90,65)==neutral);

    observe_menu_frame(game.data(),W,H);
    assert(touch(20,125)==A); // blank-space touch confirms current selection
    for(int i=0;i<3;++i)idle();
    observe_menu_frame(game.data(),W,H);
    assert(touch(78,70)==A); // tap currently highlighted STORY
    // Production Android path: Down arrives before any recognized Tap.
    observe_menu_frame(nullptr,0,0);
    assert(idle()==neutral); // flush previous sequence on scene change
    observe_menu_frame(game.data(),W,H);
    assert(raw_touch(gbarecomp::TouchPhase::Down,78,70)==A);
    assert(idle()==A);
    assert(idle()==neutral);assert(idle()==neutral);
    // The release/gesture of the same finger must NOT confirm again.
    assert(raw_touch(gbarecomp::TouchPhase::Up,78,70)==neutral);
    assert(raw_touch(gbarecomp::TouchPhase::Move,78,70)==neutral);
    // A touch to an inactive option plans the direction and confirmation.
    assert(raw_touch(gbarecomp::TouchPhase::Down,166,110)==DOWN);
    assert(idle()==DOWN);
    assert(idle()==neutral);assert(idle()==neutral);
    assert(idle()==RIGHT);assert(idle()==RIGHT);
    assert(idle()==neutral);assert(idle()==neutral);
    assert(idle()==A);assert(idle()==A);
    assert(idle()==neutral);assert(idle()==neutral);
    // A long hold is owned by this port (the runtime settings shortcut
    // must not run); the menu policy only reacts once on the original Down.
    observe_menu_frame(data.data(),W,H);
    assert(raw_touch(gbarecomp::TouchPhase::Down,98,65)==A);
    assert(idle()==A);assert(idle()==neutral);assert(idle()==neutral);
    for(int k=0;k<60;++k) {
        assert(raw_touch(gbarecomp::TouchPhase::Move,98,65)==neutral);
    }
    gbarecomp::Gesture held_gesture{};
    held_gesture.kind=gbarecomp::GestureKind::LongPress;
    held_gesture.start_in_view=true;
    held_gesture.view_x=98;held_gesture.view_y=65;held_gesture.fingers=1;
    gbarecomp::TouchFrameInfo held_frame{};
    held_frame.gestures=&held_gesture;held_frame.gesture_count=1;
    assert(menu_input_frame(&held_frame)==neutral);
    assert(raw_touch(gbarecomp::TouchPhase::Up,98,65)==neutral);

    // No touch-generated action outside the recognized menu.
    observe_menu_frame(nullptr,0,0);
    assert(raw_touch(gbarecomp::TouchPhase::Down,78,70)==neutral);
    // Out-of-view touch cannot change the in-game menu.
    observe_menu_frame(game.data(),W,H);
    assert(raw_touch(gbarecomp::TouchPhase::Down,78,70,false)==neutral);
    std::puts("menu touch: DATA/GAME, SDL Down, ignored Up/Move, VBlank pulses, return, scene guard OK");
}
