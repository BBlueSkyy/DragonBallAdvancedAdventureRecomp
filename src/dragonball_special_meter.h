#pragma once
#include <cstdint>

// Visual read-only HUD probe for the verified USA game's first story stage.
// The special gauge is drawn along the lower-left of the native 240x160 GBA
// image. This is an intentionally conservative **calibration candidate**:
// no raw guest-memory address is guessed or written. Return -1 when the
// pattern is unrecognized, rather than pretending the special is charged.
// Calibrate on a real screenshot of both empty and full meter before claiming
// production readiness detection.
namespace dragonball {
inline float special_meter_fill(const std::uint8_t* rgb,
                                unsigned width, unsigned height) {
    if (!rgb || width != 240 || height != 160) return -1.0f;
    // Center of the colored horizontal Ki gauge, skipping left kanji and
    // right frame. Coordinates are native guest pixels, *not* Android pixels.
    constexpr int x_begin = 25, x_end = 42;
    constexpr int ys[3] = {142, 143, 144};
    unsigned filled = 0, samples = 0;
    for (int x = x_begin; x <= x_end; ++x) {
        int vote = 0;
        for (int y : ys) {
            const auto* p = rgb + (static_cast<unsigned>(y) * width + x) * 3u;
            const int r = p[0], g = p[1], b = p[2];
            if (r >= 155 && g >= 105 && r >= g + 16 && g >= b + 50)
                ++vote;
        }
        if (vote >= 2) ++filled;
        ++samples;
    }
    // Detect a dark horizontal lower boundary beneath the gauge. A yellow
    // scenery patch alone must not automatically trigger an active special.
    unsigned dark = 0;
    for (int x = x_begin; x <= x_end; x += 3) {
        constexpr int y = 148;
        const auto* p = rgb + (y * width + x) * 3u;
        if (p[0] < 100 && p[1] < 100 && p[2] < 100) ++dark;
    }
    if (dark < 3) return -1.0f;
    return samples ? static_cast<float>(filled) / samples : -1.0f;
}
} // namespace dragonball
