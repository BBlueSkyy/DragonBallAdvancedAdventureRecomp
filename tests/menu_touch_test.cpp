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
    pixel(f,5,6,253,0,0);pixel(f,230,154,253,0,0);
    constexpr unsigned xx[4]={76,80,154,160}, yy[4]={28,29,27,25};
    for(unsigned k=0;k<4;k++)
        if(scene==Scene::Data)pixel(f,xx[k],yy[k],206,180,239);
        else pixel(f,xx[k],yy[k],255,238,173);
    if(scene==Scene::Data) {
        constexpr unsigned y[3]={53,84,117};
        for(unsigned i=0;i<3;i++)
            if(int(i)==selected)
                for(unsigned j=y[i];j<y[i]+5;j++)
                    for(unsigned x=54;x<74;x++)pixel(f,x,j,230,44,26);
    }else if(scene==Scene::Game) {
        constexpr unsigned x[4]={52,143,52,143}, y[4]={64,64,106,106};
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
int main() {
    constexpr auto A=std::uint16_t(0x3fe),B=std::uint16_t(0x3fd);
    constexpr auto DOWN=std::uint16_t(0x37f);
    constexpr auto RIGHT=std::uint16_t(0x3ef);
    constexpr auto neutral=std::uint16_t(0x3ff);

    const Frame data=menu(Scene::Data,0);
    assert(classify(data.data(),W,H).scene==Scene::Data);
    assert(classify(data.data(),W,H).selected==0);
    assert(classify(data.data(),W,159).scene==Scene::None);
    auto no_border=data;pixel(no_border,5,6,0,0,0);
    assert(classify(no_border.data(),W,H).scene==Scene::None);
    const Frame other_data=menu(Scene::Data,2);
    assert(classify(other_data.data(),W,H).selected==2);
    const Frame game=menu(Scene::Game,0);
    assert(classify(game.data(),W,H).scene==Scene::Game);
    assert(classify(game.data(),W,H).selected==0);
    assert(classify(menu(Scene::Game,3).data(),W,H).selected==3);
    assert(hit_target(Scene::Data,99,97)==1);
    assert(hit_target(Scene::Game,155,113)==3);

    observe_menu_frame(nullptr,0,0);
    assert(touch(99,96)==neutral); // no accidental gameplay A
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
    assert(touch(78,70)==A); // tap currently highlighted STORY
    std::puts("menu touch: DATA rows, GAME tiles, pulse/release, return, scene guard OK");
}
