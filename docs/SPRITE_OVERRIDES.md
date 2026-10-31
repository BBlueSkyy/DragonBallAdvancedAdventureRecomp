# Dragon Ball: Advanced Adventure — private sprite replacement (Stage 1)

This stage is **infrastructure, not a claim that the Goku sprite, HUD
portrait or Kamehameha graphics have already been replaced** on-device.

The BDVE GBA game renders sprites from OBJ tiles and OBJ palettes in VRAM.
Those tiles may be DMA-uploaded or compressed in the ROM, with animation
changes and palette banks. A screenshot alone does not identify the ROM
offset or animation atlas. Never guess an offset, rewrite an OAM table or
pretend that a host drawn overlay is a real sprite replacement.

## What is now implemented

- The existing verified **IPS/IPS32/BPS** user-patch selector in the
  recomp-ui launcher is enabled for this game. It is **off unless the user
  explicitly selects a patch**; the original private ROM is never written.
- `scripts/create_sprite_override.py` generates a **data-only IPS patch**
  from known original GBA 4bpp tiles and equal-sized replacement 4bpp tiles.
- The script verifies the **unmodified, retail USA BDVE 16 MiB ROM SHA-1**,
  byte-for-byte reference match, 4-byte alignment and unique match (or a
  manually verified offset). It refuses mismatches, changed tile lengths,
  header modifications and unknown revisions.
- A utility converts a palette-indexed PNG (P mode, palette indices
  0..15) into a tile-ordered 4bpp binary. Index 0 is transparent. Palette
  colors and banks are *not* modified: the 16 indices must have the same
  semantics as the game's existing OBJ palette.
- Test fixtures create **synthetic data only**, no ROM-derived data in Git.
- No guest recompilation, PPU hook, shader, VRAM write, OAM override,
  frame-by-frame allocation, extra GPU upload, or Android shader is needed.

## Private workflow (requires verified, *uncompressed* tile provenance)

The following commands operate on **your own local legal cartridge dump**.
Keep all source graphics, ROM and generated patch files out of Git.

1. With private analysis tools identify a real contiguous set of at least
   two 8x8 4bpp tiles in the retail ROM. Confirm the tile data is what
   Goku/HUD/FX uploads to VRAM. Determine its palette index mapping,
   dimensions, animation frame and whether it is compressed. **If the
   graphic exists only inside an LZ77 block, this stage cannot replace it.**
2. Keep a byte-for-byte copy of the **original ROM tile bytes**, for
   example `private-sprites/goku_idle_original.4bpp`. Do not derive this
   file by simply cropping a screenshot; screenshots contain composited RGB,
   not native ROM tile/palette indices.
3. Create a replacement in indexed-PNG mode with exactly the same
   dimensions and palette indices (8x8 tile grid, index 0 transparent).
   Convert it:

   ```sh
   python3 -m pip install Pillow
   python3 scripts/create_sprite_override.py tiles \
     --image private-sprites/goku_idle_new_indexed.png \
     --out private-sprites/goku_idle_new.4bpp
   ```

4. Generate an IPS patch against the original **BDVE USA** ROM (never
   uploaded or bundled):

   ```sh
   python3 scripts/create_sprite_override.py patch \
     --rom /private/DragonBallAdvancedAdventure-USA.gba \
     --original private-sprites/goku_idle_original.4bpp \
     --replacement private-sprites/goku_idle_new.4bpp \
     --out private-sprites/goku_idle.ips
   ```

   If the original data has multiple hits, the command aborts. The
   `--offset 0x...` argument is accepted only for a **separately
   verified** original location; it never bypasses the byte comparison.
   The generated `.ips.json` reports the source hash, offset, tile count
   and patch SHA-256; it contains no game pixels.

5. Start the Android launcher and select the original verified ROM and
   your private IPS graphic patch through its optional ROM patch UI.
   The launcher produces a separate cached patched ROM, retaining the
   original untouched. If the current recomp-ui checkout cannot support
   the patch UI on-device, record that blocker rather than claiming it
   works. Compare Goku animations in idle, walk, jump, attack and staff
   actions with and without patch. Check alternate palettes, layering,
   save states, collision stability and FPS.

## Technical limits and next development stage

- Recompiled guest instruction shards are tied to the verified game. Only
  **data-only, length-preserving tile changes** are supported. This cannot
  add new animation frames, increase resolution, move sprite origins or
  change hitboxes. To make truly higher-resolution sprites, a separate
  presentation-only replacement system would be needed with proven sprite
  identities and exact layer/priority tracking.
- A tile may be duplicated, encrypted, recompressed, streamed or shared
  with another character; the tool refuses non-unique uncompressed
  matches. **Do not force a patch until the source tile is identified.**
- Edited palette indices may have wrong colors on another animation or
  palette bank; palette editing is not implemented here.
- **Not included:** final Goku/HUD/Kamehameha art, discovered cartridge
  sprite address/animation maps, compressed tiles, on-device screenshots
  or FPS gains. A source PNG screenshot is not sufficient to infer them.

## ROM-free tests

```sh
python3 -m unittest discover -s tests -p 'sprite_override_test.py' -v
```

The tests check tile packing, unique/ambiguous matches, BDVE SHA-1 rejection,
IPS changed-byte application and untouched-ROM identity using synthetic
16 MiB buffers. These tests do not prove that the Dragon Ball ROM has a
contiguous uncompressed sprite sequence.

## Safeguards

Never commit or bundle the ROM, BIOS, gameplay sprite sheets, extracted raw
tiles or a generated IPS patch containing commercial graphics. The source
repo ignores `*.gba`, `*.4bpp`, `*.ips`, `*.bps` and
`private-sprites/`. This code is a user-side tool, not a ROM distributor.

Related active touch controls: engine Draft #12 and game Draft #9 are
**separate** and remain unmerged. This sprite stage does not alter them.
