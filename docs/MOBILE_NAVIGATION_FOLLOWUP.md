# Mobile navigation follow-up

Continues game PR8 from ecd52db5 and engine PR11 from bc32727a. This is a
qualified delivery stage, not full direct-touch support for every game screen.

## Evidence and changes

The supplied `1002505937.mp4` is 1680x720, 50.73s. Native samples use viewport
(300,0,1080,720), 4.5x, at pixel centres. Samples every 3s identify DATA1,
STORY, OPTION (including return from COLLECTION), COLLECTION, GAME/OPTION
highlight, PAUSE/Back to game and PAUSE/Back to map. Intro, transition,
conversation, gameplay and the No/Yes warning remain unrecognized, as intended.
The video does not show map destination selection or a usable map state.

PRESS START uses the existing supplied PNG at native (76,112)..(164,128).
A tap there emits START bit3, not A. Title/OPTION/COLLECTION reuse the existing
contrast-normalized glyph-mask mechanism and 97.5% shape agreement, without
changing RGB thresholds for existing DATA/GAME/PAUSE. Narrow fixture patches
carry sample provenance; no ROM/BIOS is included. Like the previous stage,
presented captures are not claimed to be raw private-game framebuffer dumps.

The host now draws one grey vector PLAY control and one hooked BACK, removing
SELECT and its padded hitbox. Physical SELECT and generic titles are unchanged.
PLAY/BACK Down+Up survive arrival entirely between VBlanks through explicit
TouchHub control ownership. They cannot become a second gesture confirmation
or LongPress settings shortcut. START works independently of scene recognition,
including alongside a held diagonal/action. BACK emits B only for recognized
DATA/GAME/PAUSE/OPTION/COLLECTION and is inert on title/unknown/gameplay.

A fixed FIFO of four requests preserves distinct rapid taps, with two pressed
and two released guest frames per command. Excess queued requests are dropped.
BACK is tied to the screen observed at Down and cancelled if it changes. Host
focus/lifecycle cancellation clears queued policy requests too. No guest memory
or flags are edited; original menu plans retain observed-selection acknowledgement.

OPTION and COLLECTION gained their visible RETURN hit regions, sending B.
Their internal rows/grid, save-erasure action and collection paging are not
claimed as direct-touch supported by this patch. Their original GBA pages and
traditional controls remain available. No runtime settings UI replaces them.

## Essential verification

The same six local targets pass: menu_touch_test, menu_delivery_test,
menu_runtime_delivery_test, host_touch_coordinates_tests, touch_input_tests,
presentation_layout_tests. No full unrelated suite or remote build is required.

New red cases failed on the previous implementation: direct PRESS START and
video OPTION/COLLECTION RETURN were discarded; HUD short Down+Up disappeared.
A review also reproduced compact-portrait overlapping actions and rapid commands
being merged/dropped. Tests failed first, then passed with the fixes. Another
red case proved queued BACK surviving host suspension; explicit title cancellation
now prevents that stale command.

The integration exercises actual SDL FINGERDOWN/UP, TouchHub, runtime VBlank,
GbaIo KEYINPUT and authored ARM `LDRH r1,[r0]` reading 0x04000130. Cases include
all prior menu targets, PRESS START, PLAY in recognized/unknown scenes, BACK's
context guard, long hold, consecutive PLAY/BACK/BACK taps, scene-change and
suspend/resume cancellation, and simultaneous DOWN+RIGHT+punch with PLAY.
The final integration run observed 1,294 VBlank callbacks and 85 Down/85 Up
events with zero failures. Resolutions: 2520x1080,1920x1080,1080x2400; engine geometry additionally covers
1080x1200 at16px/mm. Visual inspection uses the actual SDL vector renderer.

The authored navigation model is not the private game's ARM menu handler.
Existing DATA/GAME/PAUSE navigation feedback remains model-driven. These tests
prove delivery and bounded input semantics, not every private-game transition,
raw-frame recognition, Android GLES rendering or device performance. No Moto
G100 FPS claim is made. Added navigation uses fixed storage and vector spans;
no new per-frame heap allocations, polling threads, shaders or framebuffer copies.
HALT, IRQ/SWI, snapshots v5, DMA, clocks, O1 shards and build flags are unchanged.

## Remaining work

- Direct No/Yes confirmation: the video identifies the warning after Back to map,
  but does not qualify both changed selections and the cancel/return behaviour.
  No blind A/B fallback is added. The original virtual/physical controls remain.
- Map: missing visible destination layout, availability and navigation/entry
  transitions. Gameplay pixels must not be guessed into map hits or attacks.
- Additional menus/collection grid: qualify their real selection/return rules
  before enabling synthetic directions and confirmation.
- Styled runtime OPTION: the original OPTION page is now visible (COLLECTION,
  ERASE SAVE DATA?). A verified additional host entry, capability categories and
  game-style resource/layout implementation are still pending. Preserve these
  original options; a purple runtime recolor or fictional controls is not a substitute.
- Actual private-game/on-device qualification remains distinct from local tests.
  No new phone logging procedure is requested.

Both exact engine pins are synchronized for a future manually requested Android
build. No merge, shard regeneration, remote builds or Actions monitoring.
