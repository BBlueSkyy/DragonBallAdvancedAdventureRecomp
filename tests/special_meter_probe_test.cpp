#include "../src/dragonball_special_meter.h"
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>

int main() {
    std::array<std::uint8_t, 240u * 160u * 3u> frame{};
    const auto set_rgb = [&](int x,int y,int r,int g,int b) {
        auto* p=frame.data()+(y*240+x)*3;
        p[0]=static_cast<std::uint8_t>(r);
        p[1]=static_cast<std::uint8_t>(g);
        p[2]=static_cast<std::uint8_t>(b);
    };
    // Exact fixed cap and border measured on the Moto G100 screenshot:
    // silver x40, white x41 and black bottom y151.
    for (int y=146;y<=149;++y) {
        set_rgb(40,y,173,173,173);
        set_rgb(41,y,255,255,255);
        for(int x=24;x<=39;++x)set_rgb(x,y,0,0,0);
    }
    for(int x=24;x<=43;++x)set_rgb(x,151,0,0,0);
    const auto measure=[&]{return dragonball::special_meter_fill(frame.data(),240,160);};
    assert(measure()==0.0f); // real empty black interior
    for(int x=24;x<=35;++x)for(int y=146;y<=149;++y)
        set_rgb(x,y,255,y==146 || y==149?148:255,0);
    assert(measure()==0.75f); // partial Ki
    for(int x=36;x<=39;++x)for(int y=146;y<=149;++y)
        set_rgb(x,y,255,y==146 || y==149?148:255,0);
    assert(measure()==1.0f); // full bar, white cap EXCLUDED
    assert(dragonball::special_meter_fill(frame.data(),480,320)<0);
    set_rgb(41,147,0,0,0); // missing frame must not trigger glow
    assert(measure()<0);
    set_rgb(41,147,255,255,255);
    for(int x=26;x<=38;x+=3)set_rgb(x,151,255,255,255);
    assert(measure()<0);
    std::puts("PASS: zero/75%/100%, white cap excluded, invalid size/cap/border");
    return 0;
}
