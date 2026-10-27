#pragma once
#include <cstdint>

// Read-only visual Ki gauge for Dragon Ball: Advanced Adventure (USA).
// Operates on the unfiltered, native 240x160 RGB888 guest framebuffer.
// Never reads guessed emulated RAM or changes game inputs/energy.
namespace dragonball {
inline float special_meter_fill(const std::uint8_t* rgb,
                                unsigned width, unsigned height) {
    if (!rgb || width != 240 || height != 160) return -1.0f;
    const auto pixel = [rgb, width](int x, int y) {
        return rgb + (static_cast<unsigned>(y) * width +
                      static_cast<unsigned>(x)) * 3u;
    };
    // Measured from two *actual* Moto G100 screenshots:
    // the orange/yellow charge interior occupies x24..39, y146..149.
    // x40 is the SILVER EDGE and x41..43 is the WHITE CAP.
    // The previous x25..42 probe included the cap and could NEVER reach
    // the >=96% threshold on a full bar (at most 15/18).
    // Verify the stationary frame before treating random scenery as charge.
    int bottom_black = 0;
    for (int x : {26, 29, 32, 35, 38}) {
        const auto* p = pixel(x, 151);
        if (p[0] < 75 && p[1] < 75 && p[2] < 75) ++bottom_black;
    }
    const auto* cap = pixel(41, 147);
    const auto* edge = pixel(40, 148);
    const bool cap_white = cap[0] > 190 && cap[1] > 190 && cap[2] > 190;
    const bool edge_gray = edge[0] >= 90 && edge[0] <= 215 &&
                           (int(edge[0]) - int(edge[1]) < 24) &&
                           (int(edge[1]) - int(edge[2]) < 24);
    if (bottom_black < 4 || !cap_white || !edge_gray) return -1.0f;

    int filled_columns = 0;
    for (int x = 24; x <= 39; ++x) {
        int chroma = 0;
        // The actual gauge uses vivid yellow in the middle and orange
        // at its edges; count both without classifying the white cap as Ki.
        for (int y : {146, 147, 148, 149}) {
            const auto* p = pixel(x, y);
            const int r=p[0], g=p[1], b=p[2];
            if (r >= 190 && g >= 100 && b <= 100 &&
                r >= g - 10 && g > b + 65) ++chroma;
        }
        if (chroma >= 2) ++filled_columns;
    }
    return static_cast<float>(filled_columns) / 16.0f;
}
}  // namespace dragonball
