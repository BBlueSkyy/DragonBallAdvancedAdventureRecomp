// Game-specific, read-only menu recognition and tap-to-select input.
// The engine supplies raw 240x160 RGB888 pixels on present and TouchHub
// finger events on VBlank; all guest control is through active-low KEYINPUT.
// No ROM offsets, guest-memory edits, polling threads, or SDL dependencies.
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "touch_input.h"
#include "dragonball_menu_glyphs.h"

namespace dragonball::menu_touch {

enum class Scene : std::uint8_t { None = 0, Data = 1, Game = 2, Pause = 3 };
struct Observation {
    Scene scene = Scene::None;
    int selected = -1;
};

inline bool red_frame(const std::uint8_t* px, unsigned w, unsigned x, unsigned y) {
    const std::size_t i = (std::size_t(y)*w+x)*3;
    return px[i] > 200 && px[i+1] < 85 && px[i+2] < 90;
}
inline unsigned score_red_ink(const std::uint8_t* px, unsigned w,
                              unsigned x0,unsigned y0,unsigned x1,unsigned y1) {
    unsigned n=0;
    for(unsigned y=y0;y<y1;y++)
        for(unsigned x=x0;x<x1;x++) {
            const std::size_t k=(std::size_t(y)*w+x)*3;
            const unsigned r=px[k], g=px[k+1], b=px[k+2];
            // Highlighted menu labels use red/brown characters. Inactive
            // labels are pale violet. Ignore the blue body and gold border.
            if(r>58 && r>g*1.4f && r>b*1.5f && b<110) ++n;
        }
    return n;
}

inline Observation classify(const std::uint8_t* p, unsigned w, unsigned h) {
    if (!p || w!=240 || h!=160) return {};
    // Recognize the actual title glyph raster, rather than isolated RGB
    // points on its changing border/background. Coordinates/shapes are from
    // the supplied native-coordinate captures; no guest memory is inspected.
    const bool data=glyph::matches(p,w,80,22,80,11,false,glyph::Data);
    const bool game=glyph::matches(p,w,80,22,80,11,false,glyph::Game);
    const bool pause=glyph::matches(p,w,108,48,29,7,true,glyph::Pause);
    if(unsigned(data)+unsigned(game)+unsigned(pause)!=1) return {};
    Scene scene=data?Scene::Data:game?Scene::Game:Scene::Pause;
    if(pause) {
        unsigned lo=255,hi=0;
        for(unsigned y=57;y<72;++y)for(unsigned x=96;x<157;++x){const auto* q=p+3*(y*w+x);const auto v=std::min({q[0],q[1],q[2]});lo=std::min(lo,unsigned(v));hi=std::max(hi,unsigned(v));}
        if(hi-lo<32)return {scene,-1};
        unsigned score[2]={};const auto cut=lo+(hi-lo)*3/4;
        for(unsigned k=0;k<2;++k)for(unsigned y=57+8*k;y<64+8*k;++y)for(unsigned x=96;x<157;++x){const auto* q=p+3*(y*w+x);score[k]+=std::min({q[0],q[1],q[2]})>cut;}
        const auto best=score[1]>score[0]?1u:0u;
        return {scene,score[best]>=16 && score[best]>=score[1-best]+12?int(best):-1};
    }
    // DATA labels have a gold/brown border, also present on inactive
    // rows. Count only the bright-red selected label ink inside its text,
    // never the frame/border (verified on both menu frames in the video).
    static constexpr unsigned data_box[3][4]={
        {52,55,90,68},{52,86,90,100},{52,119,90,133}
    };
    static constexpr unsigned game_box[4][4]={
        // Restrict to *letter interiors*. The old boxes counted red/gold
        // border pixels and misidentified STORY as OPTION in the video.
        {48,66,99,76},{135,66,198,76},
        {48,109,105,118},{135,109,196,118}
    };
    int best=-1; unsigned best_score=0,runner_up_score=0;
    const unsigned n=scene==Scene::Data?3:4;
    for(unsigned k=0;k<n;k++) {
        const unsigned* r=scene==Scene::Data?data_box[k]:game_box[k];
        unsigned score=0;
        if(scene==Scene::Data) {
            for(unsigned y=r[1];y<r[3];++y)
                for(unsigned x=r[0];x<r[2];++x) {
                    const auto j=(std::size_t(y)*w+x)*3;
                    const unsigned red=p[j],green=p[j+1],blue=p[j+2];
                    if(red>175 && green<110 && blue<90 &&
                       red>green*1.8f) ++score;
                }
        } else score=score_red_ink(p,w,r[0],r[1],r[2],r[3]);
        if(score>best_score){runner_up_score=best_score;best_score=score;best=int(k);}
        else if(score>runner_up_score)runner_up_score=score;
    }
    // Avoid steering towards an invented selection on fades/animations.
    if(best_score<(scene==Scene::Data?30u:24u) ||
       best_score<runner_up_score+12u) best=-1;
    return {scene,best};
}

// Packed observed scene/selection; 0 means not a visible recognized menu.
// The callback is invoked at presentation; input policy reads at VBlank.
inline std::atomic<unsigned> observed{0};
inline unsigned pack(Observation o) {
    return (unsigned(o.scene)<<8) | unsigned(o.selected+1);
}
inline Observation unpack(unsigned p) {
    return {Scene((p>>8)&0xffu),int(p&0xffu)-1};
}
inline void observe_menu_frame(const std::uint8_t* rgb,unsigned w,unsigned h) {
    observed.store(pack(classify(rgb,w,h)),std::memory_order_relaxed);
}
inline int hit_target(Scene scene,float x,float y) {
    if(x<0||x>=240||y<0||y>=160) return -1;
    if(scene==Scene::Data) {
        if(x>=40&&x<=202) {
            if(y>=47&&y<79)return 0;
            if(y>=80&&y<113)return 1;
            if(y>=114&&y<148)return 2;
        }
    } else if(scene==Scene::Game) {
        if(y>=53&&y<88) {
            if(x>=28&&x<120)return 0;   // STORY
            if(x>=123&&x<=217)return 1; // ONE-ON-ONE
        }
        if(y>=92&&y<130) {
            if(x>=28&&x<120)return 2;   // LINK MODE
            if(x>=123&&x<=217)return 3; // OPTION
        }
    }
    else if(scene==Scene::Pause && x>=88 && x<170) {
        if(y>=56&&y<65)return 0; // Back to game
        if(y>=65&&y<75)return 1; // Back to map
    }
    return -1;
}

struct Plan {
    std::array<std::uint16_t,4> frames{};
    unsigned count=0,pos=0,waited=0;
    Scene scene=Scene::None;
    int target=-1,expected=-1;
    bool waiting=false,finishing=false;
    void clear(){*this=Plan{};}
    void press(unsigned bit) {
        count=4;pos=0;
        frames={std::uint16_t(0x03ffu&~(1u<<bit)),std::uint16_t(0x03ffu&~(1u<<bit)),0x03ffu,0x03ffu};
    }
};
inline Plan pending{};
inline std::uint16_t advance_plan(Observation o) {
    if(pending.scene!=o.scene || o.selected<0){pending.clear();return 0x03ff;}
    if(pending.pos<pending.count) return pending.frames[pending.pos++];
    if(pending.finishing){pending.clear();return 0x03ff;}
    if(pending.waiting) {
        if(o.selected!=pending.expected) {
            if(++pending.waited>=30)pending.clear();
            return 0x03ff; // no blind A if a direction was never consumed
        }
        pending.waiting=false;pending.waited=0;
    }
    if(pending.target==-2) {pending.press(1);pending.finishing=true;}
    else if(pending.target==o.selected) {pending.press(0);pending.finishing=true;}
    else {
        unsigned bit;int expected=o.selected;
        if(o.scene==Scene::Game) {
            if(o.selected/2<pending.target/2){bit=7;expected+=2;}
            else if(o.selected/2>pending.target/2){bit=6;expected-=2;}
            else if(o.selected%2<pending.target%2){bit=4;++expected;}
            else {bit=5;--expected;}
        } else if(o.selected<pending.target){bit=7;++expected;}
        else {bit=6;--expected;}
        pending.press(bit);pending.expected=expected;pending.waiting=true;
    }
    return pending.frames[pending.pos++];
}
inline std::uint16_t menu_input_frame(const gbarecomp::TouchFrameInfo* f) {
    if(!f)return 0x03ff;
    const auto o=unpack(observed.load(std::memory_order_relaxed));
    for(size_t i=0;i<f->event_count;++i)if(f->events[i].phase==gbarecomp::TouchPhase::Cancel){pending.clear();return 0x03ff;}
    for(size_t i=0;i<f->gesture_count;++i){const auto kind=f->gestures[i].kind;
        if(kind==gbarecomp::GestureKind::Cancel||kind==gbarecomp::GestureKind::Back||kind==gbarecomp::GestureKind::ThreeFingerTap){pending.clear();return 0x03ff;}}

    // Traditional input wins; never combine a held physical A/direction with
    // an automated confirmation for a different selection.
    if(f->host_keyinput!=0x03ff){pending.clear();return 0x03ff;}
    if(pending.scene!=Scene::None)return advance_plan(o);
    if(o.scene==Scene::None||o.selected<0)return 0x03ff;
    const bool raw=f->event_count!=0;
    for(size_t i=0;i<(raw?f->event_count:f->gesture_count);++i){
        float vx,vy;bool inside;
        if(raw){const auto& e=f->events[i];if(e.phase!=gbarecomp::TouchPhase::Down)continue;vx=e.view_x;vy=e.view_y;inside=e.in_view;}
        else {const auto& g=f->gestures[i];if(g.kind!=gbarecomp::GestureKind::Tap||g.fingers!=1)continue;vx=g.view_x;vy=g.view_y;inside=g.start_in_view;}
        if(!inside)continue;
        const float x=vx-f->extra_left,y=vy-f->extra_top;
        if(x<0||y<0||x>=240||y>=160)continue;
        int target=hit_target(o.scene,x,y);
        if(o.scene!=Scene::Pause && x>=181&&x<=223&&y>=29&&y<=45)target=-2;
        else if(target<0)continue; // no blanket A on unrelated/background pixels
        pending.clear();pending.scene=o.scene;pending.target=target;
        return advance_plan(o);
    }
    return 0x03ff;
}
} // namespace dragonball::menu_touch
