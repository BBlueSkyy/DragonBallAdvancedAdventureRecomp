#include "../src/dragonball_special_meter.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>

// ROM-free and BIOS-free test. This exercises the title-specific visual
// readiness probe independently of the GBARecomp emulator and Android SDK.
int main() {
    std::array<std::uint8_t, 240 * 160 * 3> frame{};
    const auto set_rgb = [&](int x, int y, int r, int g, int b) {
        auto* p = frame.data() + (y * 240 + x) * 3;
        p[0] = static_cast<std::uint8_t>(r);
        p[1] = static_cast<std::uint8_t>(g);
        p[2] = static_cast<std::uint8_t>(b);
    };
    // y151 dark outer frame, y146..148 colored segments.
    for (int x = 25; x <= 42; ++x) for (int y = 146; y <= 148; ++y)
        set_rgb(x, y, 245, 245, 245); // empty/white unfilled segment
    assert(dragonball::special_meter_fill(frame.data(), 240, 160) == 0.0f);
    for (int x = 25; x <= 39; ++x) for (int y = 146; y <= 148; ++y)
        set_rgb(x, y, 255, y == 146 ? 157 : 255, 0);
    const float partial = dragonball::special_meter_fill(frame.data(), 240, 160);
    assert(partial > 0.80f && partial < 0.96f);
    for (int x = 40; x <= 42; ++x) for (int y = 146; y <= 148; ++y)
        set_rgb(x, y, 255, 225, 0);
    assert(dragonball::special_meter_fill(frame.data(), 240, 160) == 1.0f);
    assert(dragonball::special_meter_fill(frame.data(), 480, 320) == -1.0f);
    for (int x = 25; x <= 42; ++x) set_rgb(x, 151, 210, 210, 210);
    assert(dragonball::special_meter_fill(frame.data(), 240, 160) == -1.0f);
    std::puts("mobile special gauge: empty, partial, full, size and invalid frame OK");
}
