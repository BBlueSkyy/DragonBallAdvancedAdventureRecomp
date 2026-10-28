> Previous delivery-stage report. See [the navigation follow-up](MOBILE_NAVIGATION_FOLLOWUP.md) for current PRESS START, HUD and video findings.

# Menu touch delivery, staged continuation of PR8

Base game `ce33ba4a807e8486810a734ba97ba2c781ad79ff`, companion engine PR11
base `dfe4cddffd6b11188c16f1401f5295f48fa8e73d`. Existing HUD, LongPress ownership,
HALT/IRQ/SWI continuations, snapshots v5, DMA wake cost and -O1 shards are preserved.

## Reproduced failures

The five attachments in this turn are PNGs. There is no MP4 attached here; the
prior PR discussion already documents its long-hold runtime-menu behavior.
The current source already claims LongPress, and this change retains that claim.

Native-coordinate samples from the supplied DATA and GAME captures do not pass
the old background gate: the four perimeter samples are black and title y15 is
red, not the expected violet/gold fill. Screenshots are presented pixels, not a
private raw-PPU dump; this distinction matters. The old policy test used invented
background values and passed while the recorded captures failed. The new red
regression reports seven failures: DATA/GAME/PAUSE scene/selection and blind A
following an unacknowledged direction.

Instead of another RGB threshold adjustment, identify the actual title glyph
raster: DATA MENU and GAME MENU at x80,y22,w80,h11, PAUSE at x108,y48,w29,h7.
Contrast normalization separates font shape from background hue. At least 97.5%
of bits must match the recorded title, with a flat/dark-contrast guard. DATA/GAME
selected-label confidence is retained; PAUSE uses its captured bright selected
text. Unrecognized/ambiguous frames emit no keys. The native callback examines
bounded patches (under 6200 pixel samples), without copying the framebuffer,
adding per-frame heap storage, or introducing any shader/guest polling.

`tests/recorded_menu_frames.h` stores only narrow recorded raster patches and
SHA256 provenance. Sampling uses the supplied 2520x1080 image's centered native
viewport (450,0,1620,1080), at pixel centres and scale6.75. These are captured
presentation fixtures, not ROM/BIOS bytes. Glyph arrays derive from these same
patches; changed-selection fixtures for every real game state are not available.

## Input contract

A visible DATA1/2/3, STORY/ONE-ON-ONE/LINK MODE/OPTION, RETURN, or one of the two
PAUSE rows starts one plan on a raw Down. A direction is held for two guest
VBlanks and released for two. Before any further direction or A, the policy
must observe the expected selected item. If navigation never acknowledges it,
the plan cancels after30 VBlanks and never confirms the wrong choice. The final
A/B is one two-frame hold plus release; the matching Up/LongPress does not make
another confirmation. Background pixels no longer blanket-confirm A.

The real SDL short-tap case (Down+Up between two VBlanks) is tested, not just
release-generated Tap. Scene disappearance/ambiguous selection, explicit Cancel,
host Back/three-finger-menu gestures, and physical key input cancel pending
synthesis. Traditional KEYINPUT remains the host's half of the active-low AND;
the policy does not rewrite guest RAM, selected indices or BIOS flags.

PAUSE hit regions come from the supplied screenshot: text rows y56..64 and
65..74, x88..169. Selecting Back to game/map still invokes the game's normal
UP/DOWN/A path; this is not a direct runtime map transition.

## Essential validation

- `menu_delivery_test`: recorded DATA/GAME/PAUSE classifications and selections,
  title-screen rejection, no A when DOWN is not consumed. Seven assertions fail
  against the old policy and pass now.
- `menu_touch_test`: every start-to-target pair (9 DATA,16 GAME,4 PAUSE), including
  all four directions, acknowledged confirmation, RETURN, timeout, ambiguity,
  physical input, Cancel and long hold/release. Assertions remain enabled in
  Release. Cancel was separately reproduced failing before its guard.
- `menu_runtime_delivery_test`: actual SDL2.30 software event backend -> real
  HostWindow -> TouchHub -> existing runtime VBlank hook -> GbaIo KEYINPUT ->
  an authored ARM `LDRH [KEYINPUT]` consumer. Test sizes2520x1080,1920x1080,
  1080x2400; all nine target choices, short taps, RETURN, long hold and gameplay
  rejection. It reports732 callbacks,30 Downs,30 Ups and zero failures.
- Engine `host_touch_coordinates_tests`:18 real SDL edges at landscape/portrait
  sizes map once to the expected native coordinates. Existing `touch_input_tests`
  and `presentation_layout_tests` passed.

The SDL integration found an independent desktop bug: SDL's logical renderer
rewrites finger coordinates before HostWindow remaps the viewport. Portrait
DATA2 was read as native y122.22 rather than95, and other choices were outside
view. The engine now uses explicit viewports for opted-in desktop touch policies,
matching its established Android coordinate path. Android already disabled SDL
logical-size remapping; this desktop correction is not claimed as the cause on
the Moto G100. It enables meaningful coordinate qualification on the local host.

All six essential tests passed (three game tests, three engine tests). The final
three game tests passed after the Cancel guard in0.37s. SDL rendering was real
but software/dummy, runtime settings UI was not linked into this harness.
A GCC13 lambda/nullptr compile error in the base required an explicit unary-plus
conversion to the same function pointer; that one-character compatibility fix
is included in the companion engine commit so the published sources build.
No optimization flags, guest CPU timing or rendering effects were changed.

Build without private ROM, BIOS, shards or launcher UI:

```sh
cmake -S . -B build-touch -DDBAA_BUILD_TOUCH_TESTS=ON \
  -DGBARECOMP_ROOT=/path/to/companion/gbarecomp -DCMAKE_BUILD_TYPE=Release
cmake --build build-touch --target menu_touch_test menu_delivery_test \
  menu_runtime_delivery_test touch_input_tests presentation_layout_tests \
  host_touch_coordinates_tests -j3
ctest --test-dir build-touch --output-on-failure -R \
  '^(menu_touch_test|menu_delivery_test|menu_runtime_delivery_test|touch_input_tests|presentation_layout_tests|host_touch_coordinates_tests)$'
```

SDL development headers/library are needed for the SDL targets. The normal APK
build still follows the existing default CMake path and -O1 shard properties.

## What is not yet established

This is a verified local menu-delivery stage, not a definitive on-device claim.
The ARM reader/navigation model is authored, not the private game's menu handler.
It updates the observation as feedback after input; therefore it proves delivery
and the feedback contract, not pixel recognition after every real game menu
transition. Captures validate selected DATA1/STORY/Back to map only. Actual game
handlers, all changed-highlight/fade frames, the runtime-UI-open lifecycle and
physical-device performance still need qualification. An observation-generation
check is absent, so delayed/skipped presentation remains a conditional stale
observation risk. No speculative generation protocol is added here.

Map: none of the supplied captures shows the map. The repository exposes code
and the supplied HUD resources, but no accessible map asset/state trace, verified
destination coordinates, availability flags or region navigation graph. Do not
infer map hits from arbitrary gameplay pixels: they could fire attacks. Unknown
scenes intentionally remain inert.

Themed OPTION: the supplied settings capture is the purple runtime page, not
the game's original OPTION page. The original options/return boundaries and a
verified entry for an additional host layer have not been qualified. A future
layer can reuse existing HostWindow/RecompRuntimeUi bindings for supported
filters, volume, color profiles, scanlines and controls; unsupported shaders or
brightness must not be invented. No partial recolor, replacement of original
OPTION functionality, or new settings UI is included in this stage.

These map/OPTION stages remain open on the existing PRs. No merge, remote build,
Actions monitoring, new phone logs/ADB/CSV, or shard regeneration was performed.
