# Dragon Ball: Advanced Adventure — HD remaster (presentation stage 1)

This is the first **real host-resolution art replacement path** for the Android
ARM64 port. It is not an upscaling shader. A verified static area of the
original 240x160 image can be covered by new, independently authored artwork
whose source resolution is at least **2x** the GBA area. The game remains
native and the original PPU frame stays byte-for-byte unchanged.

## Implemented

- The game opts into an optional \`remaster/hdpack.json\`.
- The engine parses/validates the manifest once and loads at most eight
  full-opaque 24-bit BMP textures (2x+ native source, max 2048x2048 each,
  16 MiB maximum file). SDL2 does not need SDL_image or new dependencies.
- Each entry specifies an exact native GBA rectangle and a 16-digit
  FNV-1a hash of the *unfiltered RGB888 pixels* visible there in the
  reference frame. Only when the *current* raw frame matches the hash does
  the renderer cover that area with the HD texture. The image is scaled
  smoothly using SDL linear filtering at output display resolution; it is
  never downsampled into the native framebuffer.
- Region bounds, file names and path traversal, manifest length, total
  pixels hashed, image size, overlapping regions, the checksum format and
  altered/unknown frame signatures are all rejected. Any failed image load
  prevents the entire pack from activating.
- On Android, the engine resolves the relative pack filename in SDL's
  app-private **gbarecomp/hd-packs preference directory**, e.g. the
  \`SDL_GetPrefPath("gbarecomp","hd-packs")\` directory. It logs the path
  once when missing. This avoids APK read-only paths. On desktop, the path
  is relative to the executable directory.
- All textures are created at session setup, not in the emulation loop.
  If the pack is absent, there is no HD matching, rendering or allocation
  in the per-frame path. Original touch mapping, native rendering, clocks,
  IRQ/SWI/HALT, DMA, snapshots and shaders remain untouched.
- Only the **Android** output path currently draws these HD panels.

## Preparing a private pack

First capture an **unaltered native 240x160** frame (PNG) from your own
game session. It must be the *raw GBA source image*, not a phone screenshot,
not a JPEG and not a shader/scanline/color-graded capture. Locate a
**stable, fixed-location HUD rectangle** in that frame and measure its
actual native \`x,y,w,h\`. Different animation frames or a changed HUD
may require different scene signatures. The values below are placeholders,
**not game-verified coordinates**.

Draw original, new art for that exact rectangle (HD PNG, at least 2x its
native width and height). It must cover the full rectangle and be opaque.
For this first stage, transparent partial sprite overlays are deliberately
disabled because they risk double-drawing the original and violating PPU
layer priorities.

\`\`\`sh
python3 -m pip install Pillow
python3 scripts/build_hd_pack.py \
  --frame /private/native-gba-frame.png \
  --art /private/my-redrawn-hud.png \
  --x YOUR_VERIFIED_X --y YOUR_VERIFIED_Y \
  --w YOUR_VERIFIED_W --h YOUR_VERIFIED_H \
  --out /private/remaster
\`\`\`

The script writes only \`panel.bmp\` and \`hdpack.json\`. It does not
copy the source GBA screenshot, ROM, BIOS or existing game artwork.
Both output files stay private. Place them together under the app's
\`remaster/\` directory inside the SDL preference root named above; the
engine logs the expected complete \`hdpack.json\` path to help with
Android debug builds. Never commit original game assets.

Once a verified match is active, the overlay replaces only that small
static region with smooth HD art. During other scenes it automatically
falls back to original pixels. Turning the pack off is as simple as
removing/renaming \`hdpack.json\`. No update to game saves is required.

### Example manifest shape (illustrative, hash and dimensions not from game)

\`\`\`json
{
  "version": 1,
  "regions": [{
    "image": "panel.bmp",
    "x": 8,
    "y": 8,
    "w": 24,
    "h": 16,
    "fingerprint": "0123456789abcdef"
  }]
}
\`\`\`

The example signature is deliberately not associated with any commercial
GBA ROM/frame; it will **not** activate a visible overlay.

## Native verification

Engine ROM-free C++ test:

\`\`\`sh
g++ -std=c++17 -Isrc/runtime tests/mod_runtime/hd_remaster_test.cpp -o /tmp/hdremaster
/tmp/hdremaster
\`\`\`

Game ROM-free Python tests:

\`\`\`sh
python3 -m unittest discover -s tests -p 'hd_pack_test.py' -v
\`\`\`

After generating an APK from this branch, test with/without the same
private HUD pack: idle and moving frames, menu transitions, save/load,
portrait changes, input/touch and fixed 60 Hz output. Confirm no remaster
panel appears when its fingerprint does not match. Track FPS and GPU
memory on the Moto G100 before enabling additional panels.

## Explicitly NOT implemented yet

- Mapping Goku's OAM entries, animation atlas or palette banks.
- Replacing individual **moving OBJ sprites** with smooth, larger images.
- Rebuilding scrolling BG layers or their independent parallax/camera.
- Handling OBJ/BG priority, alpha/window/blending and per-scanline occlusion.
- Redrawing or bundling the actual Goku, enemies, Kamehameha or stages.
- HD support on desktop or an in-game art-pack selection menu.

These need **verified guest graphical identities and layer ownership**,
not a screen-space rectangle or screenshot classifier. An OAM-derived
replacement without correct priority would float over foregrounds and
misrepresent the game. The next step is a read-only OBJ/BG attribution
probe, followed by a separate renderer respecting priority and masking.
Do not claim this MVP recreates the concept art.

Separate PRs for compact touch UI (#9 game / #12 engine) and the ROM-side
IPS sprite patches (#10 game) remain independent; nothing is merged here.
