// Game-specific, read-only menu recognition and tap-to-select input.
// The engine supplies raw 240x160 RGB888 pixels on present and TouchHub
// tap gestures on VBlank; all guest control is through active-low KEYINPUT.
// No ROM offsets, guest-memory edits, polling threads, or SDL dependencies.
#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include "touch_input.h"

namespace dragonball::menu_touch {

enum class Scene : std::uint8_t { None = 0, Data = 1, Game = 2 };
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
    // Regression verified against frames of the user's 1680x720 Android
    // recording, reconstructed to 240x160. The old right x=237 probe hit
    // an orange antialiased border pixel (green ≈115), and top y=2 hit a
    // dark-red edge (red ≈199): red_frame() rejected BOTH real menus.
    // These four pixels are solid interior-red portions of the border.
    if (!red_frame(p,w,3,70) || !red_frame(p,w,238,70) ||
        !red_frame(p,w,30,3) || !red_frame(p,w,30,157)) return {};
    // The previous DATA predicate required blue > red+13. The actual
    // purple title at y=15 is dark magenta (roughly 106,22,70): blue
    // exceeds green but NOT red. GAME title is brown/gold (~150,70,11).
    // Use component relationships anchored to the *real* samples, with
    // tolerance for capture scaling/gradients; require 3/4 matches.
    constexpr unsigned xx[4]={80,95,125,150};
    constexpr unsigned yy[4]={15,15,15,15};
    unsigned violet=0, cream=0;
    for(int k=0;k<4;k++) {
        const std::size_t i=(std::size_t(yy[k])*w+xx[k])*3;
        const int r=p[i],g=p[i+1],b=p[i+2];
        violet += r>=65 && r<=165 && g<115 && b>=45 && b>=g+20;
        cream += r>=125 && g>=40 && b<60 &&
                 r>=g+35 && g>=b+18;
    }
    Scene scene=violet>=3 ? Scene::Data : cream>=3 ? Scene::Game : Scene::None;
    if(scene==Scene::None) return {};
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
    return -1;
}

struct Plan {
    std::array<std::uint16_t,32> frames{};
    unsigned count=0,pos=0;
    Scene scene=Scene::None;
    void clear(){count=pos=0;scene=Scene::None;}
    void press(unsigned bit) {
        if(count+4>frames.size()) return;
        const auto held=std::uint16_t(0x03FFu & ~(1u<<bit));
        // Two guest VBlanks pressed, two released; do not rely on
        // host-frame timing or OS touch repeat to navigate menus.
        frames[count++]=held;
        frames[count++]=held;
        frames[count++]=0x03FFu;
        frames[count++]=0x03FFu;
    }
};
inline Plan pending{};

inline std::uint16_t menu_input_frame(const gbarecomp::TouchFrameInfo* f) {
    if(!f) return 0x03FFu;
    const Observation o=unpack(observed.load(std::memory_order_relaxed));
    if(pending.pos<pending.count) {
        if(o.scene!=pending.scene) {
            pending.clear(); // menu disappeared: never leak an A into play
        } else {
            const std::uint16_t key=pending.frames[pending.pos++];
            if(pending.pos==pending.count) pending.clear();
            return key;
        }
    }
    // Do not generate A/B or navigation until the highlight is visible:
    // transition/fade frames can briefly retain the menu banner.
    if(o.scene==Scene::None || o.selected<0) return 0x03FFu;
    // On a phone, SDL FINGERDOWN is routed as TouchEvent::Down; no gesture
    // has to complete before selecting a visible menu item. Previously this
    // code only consumed GestureKind::Tap on FINGERUP, making a tap dependent
    // on recognition/release timing and causing some Android taps to be lost.
    // Prefer the raw Down stream. Gestures are a fallback for clients that
    // supply a synthetic Tap without corresponding finger events.
    const bool has_events = f->event_count != 0;
    const std::size_t n = has_events ? f->event_count : f->gesture_count;
    for(std::size_t i=0;i<n;i++) {
        float view_x=0.0f, view_y=0.0f;
        bool inside=false;
        if(has_events) {
            const auto& e=f->events[i];
            if(e.phase!=gbarecomp::TouchPhase::Down) continue;
            view_x=e.view_x;
            view_y=e.view_y;
            inside=e.in_view;
        } else {
            const auto& g=f->gestures[i];
            if(g.kind!=gbarecomp::GestureKind::Tap ||
               g.fingers!=1) continue;
            view_x=g.view_x;
            view_y=g.view_y;
            inside=g.start_in_view;
        }
        if(!inside) continue;
        const float x=view_x-static_cast<float>(f->extra_left);
        const float y=view_y-static_cast<float>(f->extra_top);
        if(x<0||y<0||x>=240||y>=160) continue;
        pending.clear();
        pending.scene=o.scene;
        // The game's RETURN label occupies the upper-right of both menus.
        if(x>=181&&x<=223&&y>=29&&y<=45) {
            pending.press(1); // B = return
        } else {
            const int target=hit_target(o.scene,x,y);
            if(target>=0) {
                if(o.scene==Scene::Data) {
                    for(int d=o.selected;d<target;d++)pending.press(7);
                    for(int d=o.selected;d>target;d--)pending.press(6);
                } else {
                    const int oldrow=o.selected/2,newrow=target/2;
                    const int oldcol=o.selected%2,newcol=target%2;
                    if(oldrow<newrow)pending.press(7);
                    else if(oldrow>newrow)pending.press(6);
                    if(oldcol<newcol)pending.press(4);
                    else if(oldcol>newcol)pending.press(5);
                }
            }
            // On the menu background a simple tap advances current choice.
            pending.press(0); // A = confirm
        }
        if(pending.count) {
            const auto key=pending.frames[pending.pos++];
            if(pending.pos==pending.count)pending.clear();
            return key;
        }
    }
    return 0x03FFu;
}
} // namespace dragonball::menu_touch
