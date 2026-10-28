# Android direct touch for Dragon Ball menu screens

**Scope:** `DATA MENU` (three save slots) and `GAME MENU` (STORY, ONE-ON-ONE, LINK MODE, OPTION) shown in the user's October 27 video. No game control, guest memory or emulated timing is modified.

## Implementation

- Game-owned `src/dragonball_menu_touch.h` reads the raw, unfiltered **240×160 RGB888** output from an optional engine present callback. It verifies four **real, stable red-border samples** (left, right, top, bottom) and samples the unobstructed header background at y=15 to distinguish violet DATA MENU from cream GAME MENU. Neither gameplay nor transitions are treated as a menu.
- The selected DATA row is identified by **bright red label characters** in the inner text rectangle; shared gold borders are not counted. GAME MENU selection is identified by reddish-brown lettering as opposed to the inactive pale violet letters; the search rectangles stay inside the letter area rather than counting the orange frame, and ambiguous highlights are rejected. An ambiguous selection is never used to choose a different row.
- `TouchHub` recognizes short one-finger taps only. The guest-frame input policy uses GBA **KEYINPUT** at VBlank to synthesize the required Up/Down/Left/Right direction(s), separated by released frames, then **A** to confirm. Tapping the upper-right **RETURN** label sends **B**. A tap in blank space while a selection is known confirms the highlighted option.
- If the detected scene disappears during the sequence, any remaining synthetic keys are dropped immediately, preventing a queued A from becoming a gameplay attack. Touching the on-screen A/B/L/R or D-pad remains controlled by the engine's existing pad routing. The three-finger shortcut and Android Back remain available for runtime settings; single-finger LongPress is now claimed by the Dragon Ball touch policy, preventing the host menu from stealing focus while holding the game screen.
- No ROM offsets, HLE, direct save-slot writes, UI OCR, guest-memory patches or background threads. A tiny atomic scene observation is reset to None on other screens. This feature is Android opt-in for this title only.
- Neither the HALT/IRQ/SWI fix on main nor its performance-critical shards are altered.

## Tests and validation

A standalone ROM-free test `tests/menu_touch_test.cpp` supplies synthetic RGB888 banner/selection frames and verifies hitboxes, selected slot and option classification, taps to other items, frame-by-frame press/release, Return and cancellation on scene change.

Compile manually from the game root after the usual `gbarecomp` checkout:

```
c++ -std=c++20 -Igbarecomp/src/runtime tests/menu_touch_test.cpp -o /tmp/menu_touch_test
/tmp/menu_touch_test
```

The observed RGB thresholds and button positions were checked against the user's attached 1680×720 Android recording, projected back to the native 240×160 game image. **A concrete bug was found and fixed:** old border probes `(5,6)` and `(230,154)` sampled black corner ornaments in the real frames. Both menus therefore failed identification; four new red samples and title samples at y=15 recognize stable DATA and GAME menu frames. Letter-only GAME selection rectangles also fix a misclassification of selected STORY as OPTION. **These samples are reference observations, not proof of a compiled Android result.**

Device acceptance: from GAME MENU tap STORY and OPTION; in DATA MENU tap an unselected DATA slot and verify it really navigates there, tap a selected slot and the RETURN label; try blank-space advance and gestures over existing virtual buttons. Then enter gameplay: taps in the stage should **not** trigger menu confirmation or random movement, and HALT-fixed performance should remain fluid. Check touch after a save/load and after returning to either menu.

**Validation achieved:** independently sampled video frames confirm the old border test fails (false/false), the new four border samples pass (true/true/true/true) on DATA and GAME screens, and the corrected GAME label score selects STORY instead of OPTION. Code-level regression tests now exercise black corners, each menu choice, blank-space advance and cancellation. **Pending:** ROM-free C++ test execution, Android APK compile and Moto G100 visual/functional validation. Avoid merging this Draft until those pass.
## Follow-up: Android taps still not responding (27 Oct 2026)

After the color/selection classifier fix, the owner reported that tapping the
game image still did not advance. Comparing the TouchHub Android event route
with `menu_input_frame` revealed that the policy **only consumed the
post-release `GestureKind::Tap`**. SDL touch is first delivered as a raw
`TouchEvent::Down`, then `Move`/`Up` and possibly a synthesized gesture.
The callback now consumes **the initial Down event** when a confirmed DATA/GAME
menu is visible, avoiding dependence on release timing or gesture recognition.
If a testing client provides only an explicit Tap gesture and no raw events,
the prior gesture path remains supported. An Up or Move is never a new
confirmation. One press still emits a bounded 2-frame held / 2-frame released
KEYINPUT sequence, cancelled on a scene transition. The guest, IRQ/SWI,
HALT timing and touch-pad hitboxes are unchanged.

`tests/menu_touch_test.cpp` now includes raw Down, ignored Up/Move, another
menu item (Down→Right→A), hidden-menu and out-of-view guards, and the earlier
gesture-only tests. **These are source-level additions, not a completed
Android test run.** The menu-probe pixels were checked using captured video
frames, but the SDK event-to-guest path and actual touch response are still
pending real-device confirmation. No claim of confirmed fix until the next
APK is tested on Moto G100.


## Actual capture-color regression after on-device failure

The real video evidence exposes a **reproducible root cause** for the still
inoperative menu touches, beyond Down-vs-Tap dispatch. The classifier checked
solid-red pixels at (237,70) and (30,2), but their observed values in the
reconstructed 240x160 recording are approximately (255,115,85) and
(199,19,18); they fail the required red > 200 / green < 85. The old
DATA-title predicate also demanded blue > red+13, while the actual dark
magenta title measures ~ (106,22,70). Thus a genuine DATA frame was
misidentified as Scene::None and `menu_input_frame` suppressed *all*
synthesized presses. The GAME title was valid but failed the border gate.

The repaired classification checks (3,70), (238,70), (30,3),
(30,157), away from the antialiased border positions. DATA-title samples
are recognized as dark red/magenta (blue exceeds green, not red); GAME
uses contrasting brown/gold. The previously failing fixture in
`tests/menu_touch_test.cpp` now reproduces recorded border/header RGB
rather than hand-picked pure red and pale violet values.

Independent verification against **five different frames** extracted from
the user's video: DATA frames 4, 5 and 9 recognized as (Data, highlighted 0);
GAME frames 11 and 12 recognized as (Game, highlighted 0).
All five failed the previous four-point border gate. The classifier retains
the selected-label confidence check and explicit absence guard, so touches
do not synthesize A during gameplay or fades.

This validates menu recognition against the *rescaled recording*, not the
original native 240x160 PPU buffer. Further guest/host timing and Android
touch-input verification still require a newly built APK on the Moto G100.
No change to HALT, IRQ/SWI, DMA, guest shards, shaders or button positioning.


## 27 October device-video finding: holding opens runtime settings

The 16.6-second Moto G100 recording shows the game's DATA MENU alternating
with the full-screen purple **Game Boy Advance runtime settings** UI (Graphics,
Audio, System, Display, Controls). This is not a Dragon Ball submenu.
The engine maps an **unclaimed single-finger LongPress** to opening the
runtime UI after about 650ms; `src/main.cpp` previously claimed only
`kTouchClaimTap`, so a held finger stole input focus from the emulated GBA.

**Fix:** game `RunOptions::touch_gesture_claims` now includes
`kTouchClaimLongPress` as well as Tap, without changing engine
`TouchHub`, guest interrupts, HALT, DMA or shards. The menu policy already
uses raw `TouchPhase::Down` to act on the first contact; it ignores
`Move`, `Up` and LongPress as extra selections. A ROM-free policy test
simulates a Down followed by 60 Move frames, a LongPress and Up and asserts
no second synthesized key press. Android Back / the three-finger host
shortcut can still open settings deliberately.

**Important limitation:** the recording proves the accidental settings
shortcut. It does not independently prove the earlier failed *short taps*
are all caused by it. An approximate reconstruction of the DATA MENU
frame also passes current red-border and purple-title detection, so an
actual Android build is still needed to verify raw Down delivery through
`TouchHub`, the VBlank policy and active-low `KEYINPUT`. No remote build
has been started and no on-device fix can be claimed yet.
