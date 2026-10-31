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
  app-private **files/remaster/** directory. SDL2 Android ignores the
  org/app arguments to SDL_GetPrefPath: it returns the app's internal
  storage directory directly, typically
  /data/user/0/org.dbadvancedadventure.recomp/files/.
  The engine logs the full optional pack path once when missing.
  On desktop, the path is relative to the executable directory.
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
remaster/ directory inside the app-private files/ directory,
typically /data/user/0/org.dbadvancedadventure.recomp/files/remaster/.
The engine logs the full path. Android 12 file managers cannot write here:
use a debug APK and adb run-as, or a future in-game importer. Never commit original game assets.

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


### Deploying a pack to the Android debug APK

The package ID is `org.dbadvancedadventure.recomp`. SDL2 Android resolves
`remaster/hdpack.json` relative to the app-private `files/` directory.
It does **not** create `files/gbarecomp/hd-packs/`.

```sh
adb shell run-as org.dbadvancedadventure.recomp mkdir -p files/remaster
adb exec-out run-as org.dbadvancedadventure.recomp sh -c 'cat > files/remaster/hdpack.json' < /private/remaster/hdpack.json
adb exec-out run-as org.dbadvancedadventure.recomp sh -c 'cat > files/remaster/panel.bmp' < /private/remaster/panel.bmp
```

Requires an installed debuggable APK and ADB. Verify the exact missing
manifest location with `adb logcat` before copying. The HD pack is loaded
once per game session: force-close and reopen after installing a pack.
Only a byte-matching, unfiltered native RGB888 source frame from this
port will activate a region; phone screenshots do not qualify.

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

## Stage 2 started: read-only Goku/scene attribution

The HD engine now has an **explicitly gated** OBJ/BG snapshot probe. No probe
runs during normal gameplay. To activate it for a single 1-minute gameplay
diagnostic, use ADB with this **debug** Android APK installed:

```sh
adb shell run-as org.dbadvancedadventure.recomp mkdir -p files/remaster
adb shell run-as org.dbadvancedadventure.recomp sh -c 'echo 1 > files/remaster/capture.flag'
```

**Force-close and restart the app** after enabling the flag. Navigate from
PRESS START into a level, move Goku, jump, punch, use the Power Pole and
Kamehameha, then visit a different background/area. The probe records OAM
and BG identities at the real **scanline 0 boundary**, every 30 frames,
capped at 120 samples. Normal session rendering remains native.

Recover the report from the app-private directory with:

```sh
adb exec-out run-as org.dbadvancedadventure.recomp cat files/remaster/obj_attribution.jsonl > obj_attribution.jsonl
```

You can also analyze the captured file locally, without uploading the game ROM:

```sh
python3 scripts/analyze_hd_attribution.py --input obj_attribution.jsonl --out hd_candidates.json
```

This prints motion-based OAM slot candidates and changing BG scroll/map
signatures. **Do not treat a high ranking as proof that a slot is Goku**;
compare frames with actual gameplay actions, then use verified sprite IDs
and layers to implement the HD object compositor.

The report contains **no original tile or palette pixel bytes**, only
per-object tile/palette FNV64 signatures and native OAM attributes,
position, priority, mode and 4 BG control/scroll/screen-block hashes.
Each line after the header is one independent sample. Correlating
the changing object hashes and positions over multiple animation actions
will let the next implementation identify the Goku animation groups and
their foreground/background ordering **without guessing**. BG screen-block
hashes are only reference hints, not complete decoded scenery.

Remove the flag when done:

```sh
adb shell run-as org.dbadvancedadventure.recomp rm files/remaster/capture.flag
```

The probe does not write to GBA memory, interfere with snapshots, replace
artwork or generate an HD atlas. **This is evidence collection, not yet
Goku or scenery HD rendering**. Running with capture.flag enabled performs
limited hash work; verify FPS and remove the flag after collecting the data.
Normal unflagged gameplay skips the observer entirely.

### Identifying stable art signatures (additional offline analysis)

In addition to the existing `analyze_hd_attribution.py` motion-based OAM
ranking, the new **complementary** analyzer groups the same private metadata
by `(OBJ tile hash, palette hash, native width/height, priority)`.
This helps distinguish reused OAM slots from actual graphic data changes.
It does not assign character names and does not extract or create images.

```sh
python3 scripts/analyze_hd_obj_capture.py \
  --input obj_attribution.jsonl \
  --out hd_obj_candidates.json
```

Compare `obj_candidates` with the existing motion ranking and recorded
actions before labeling the Goku animation. Background map hashes are
also grouped, but they are not complete source scenery. Actual HD art
replacement remains pending source identification, transparency and
correct per-layer occlusion. No runtime/PPU timing changes.

## Explicitly NOT implemented yet

- Verified identification of Goku's individual OAM animation groups (stage-2 probe has been added, device attribution pending).
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
