# Android direct touch for Dragon Ball menu screens

**Scope:** `DATA MENU` (three save slots) and `GAME MENU` (STORY, ONE-ON-ONE, LINK MODE, OPTION) shown in the user's October 27 video. No game control, guest memory or emulated timing is modified.

## Implementation

- Game-owned `src/dragonball_menu_touch.h` reads the raw, unfiltered **240×160 RGB888** output from an optional engine present callback. It verifies the red menu border and uses four non-text header samples to distinguish the violet DATA MENU from the cream GAME MENU. Neither gameplay nor transitions are treated as a menu.
- The selected DATA row is identified by **bright red label characters** in the inner text rectangle; shared gold borders are not counted. GAME MENU selection is identified by reddish-brown lettering as opposed to the inactive pale violet letters. An ambiguous selection is never used to choose a different row.
- `TouchHub` recognizes short one-finger taps only. The guest-frame input policy uses GBA **KEYINPUT** at VBlank to synthesize the required Up/Down/Left/Right direction(s), separated by released frames, then **A** to confirm. Tapping the upper-right **RETURN** label sends **B**. A tap in blank space while a selection is known confirms the highlighted option.
- If the detected scene disappears during the sequence, any remaining synthetic keys are dropped immediately, preventing a queued A from becoming a gameplay attack. Touching the on-screen A/B/L/R or D-pad remains controlled by the engine's existing pad routing. Three-finger menu and unclaimed long press remain host-owned.
- No ROM offsets, HLE, direct save-slot writes, UI OCR, guest-memory patches or background threads. A tiny atomic scene observation is reset to None on other screens. This feature is Android opt-in for this title only.
- Neither the HALT/IRQ/SWI fix on main nor its performance-critical shards are altered.

## Tests and validation

A standalone ROM-free test `tests/menu_touch_test.cpp` supplies synthetic RGB888 banner/selection frames and verifies hitboxes, selected slot and option classification, taps to other items, frame-by-frame press/release, Return and cancellation on scene change.

Compile manually from the game root after the usual `gbarecomp` checkout:

```
c++ -std=c++20 -Igbarecomp/src/runtime tests/menu_touch_test.cpp -o /tmp/menu_touch_test
/tmp/menu_touch_test
```

The observed RGB thresholds and button positions were checked against samples of the user's attached 1680×720 Android gameplay recording, projected back to the native 240×160 game image. **These samples are reference observations, not proof of a compiled Android result.**

Device acceptance: from GAME MENU tap STORY and OPTION; in DATA MENU tap an unselected DATA slot and verify it really navigates there, tap a selected slot and the RETURN label; try blank-space advance and gestures over existing virtual buttons. Then enter gameplay: taps in the stage should **not** trigger menu confirmation or random movement, and HALT-fixed performance should remain fluid. Check touch after a save/load and after returning to either menu.

**Pending:** ROM-free C++ test execution, Android APK compile and Moto G100 visual/functional validation. Avoid merging this Draft until those pass.