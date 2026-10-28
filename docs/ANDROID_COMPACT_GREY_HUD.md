# Compact neutral-grey touch controls — Android visual iteration

User-approved mockup: the final **smaller, all-grey** HUD, with a separate
rounded four-way D-pad, BACK at bottom left, a centered PLAY triangle and
four round action buttons in the right rail.

This PR pins engine Draft #12. **No changes to the game's menu policy,
HALT timing, IRQ/SWI continuations, DMA, snapshots v5 or -O1 shards.**
The engine title-specific `compute_pad_layout()` applies an 80% factor to
the existing configured scale, so rendered geometry and touch areas stay
aligned. Other GBA titles do not get this factor.

- **Punch (B):** a single monochrome closed fist, not a character portrait.
- **Kamehameha (R):** two cupped hands and a white energy sphere, drawn with
  neutral-grey overlay geometry, not a generic blue beam.
- **Power Pole (L):** small grayscale pixel-art figure in the overhead
  staff pose from the supplied gameplay reference, reconstructed in a
  lightweight 48x48 silhouette.
- **Jump (A):** small grayscale airborne Goku silhouette reconstructed
  from the supplied 192x224 screenshot. No extra jump arrow.
- **D-pad:** four independent light-grey translucent circles and white
  filled arrows; no center disc.
- **BACK / PLAY:** new positions at bottom left / centered bottom,
  using the already-merged game-owned `TouchControl` handling.
- **SELECT:** unchanged removal from the Dragon Ball HUD, while physical
  SELECT remains supported.
- **Special readiness:** only R and L gain a subtle *silver* rim at full
  charge. No neon or persistent colored action chrome.

The monochrome sprite masks live entirely in engine C++ `.rodata`.
No ROM, BIOS, commercial art resource, or image file is downloaded or
bundled. The silhouettes are simplified reconstructions of supplied image
references, not literal extracted ROM sprites.

## ROM-free smoke test and device checks

Engine: `c++ -std=c++20 -Isrc/runtime tests/runtime/mobile_action_skin_test.cpp -o /tmp/skin-test && /tmp/skin-test`

After building on Codemagic with this **branch**, verify on the actual Moto
G100: sprite readability; neutral grey on every control; BACK and PLAY
placement; four-way movement and diagonals; punch (B), jump (A),
Ki (R), Power Pole (L) and multitouch; specials glow only when ready.
Verify PRESS START, DATA/GAME/PAUSE menu taps and observe FPS against the
last working APK. This visual PR does not independently prove full
gameplay/map touch coverage or a performance improvement.

No Android build or remote workflow run was started for this change.
