# Dragon Ball: Advanced Adventure — Android ARM64 recompilation starter

**Status:** technical starter only. This is **not a playable port or an APK**. No cartridge ROM, BIOS, game assets, or generated ROM-derived C++ is included.

## Experimental sprite replacements (ROM-private)

This fork now has an **opt-in, validated IPS sprite-tile patch workflow**:
see [Sprite override documentation](docs/SPRITE_OVERRIDES.md).
A local utility converts palette-indexed PNG graphics to GBA 4bpp tiles
and emits an IPS patch **only after** checking the original BDVE ROM
identity, exact source bytes and tile sizes. The user selects the patch in
the existing launcher; no ROM or proprietary sprite graphics are bundled.

**Current limit:** no actual Goku/HUD/FX ROM tile identity has yet been
confirmed and no sprite replacement has been validated on-device. Native
graphics mapping requires private cartridge analysis; compressed tiles
are explicitly unsupported by this first stage.

## Intent

- Target Android ARM64 from day one, using gbarecomp's shared Android SDL shell.
- Recompile GBA ARM/Thumb instructions into host C++ and link with the GBA hardware runtime; not an ordinary GBA emulator wrapper.
- Support **only the verified retail USA cartridge (BDVE)**; reject other regions, patches and unknown revisions before generating sources.
- Start with the faithful 240x160 viewport. Gameplay validation comes before optional widescreen.
- Preserve touchscreen controls, Android picker, SDL2 audio, cartridge saves and Bluetooth gamepads through the framework's Android shell.

## Prerequisites

A desktop or CI host capable of running CMake, Ninja, Python 3, Java 17, Gradle, Android SDK/NDK, and the gbarecomp source recompiler. The engine's Android configuration requires NDK 27.1.12297006 and API 35 tooling. These are reference versions, not yet validated for this specific game.

The USA ROM and original GBA BIOS are supplied privately by the user and are **never** put into this archive or public source control.

## 1. Provide and identify a cartridge dump

```
python3 scripts/prepare_rom.py /private/path/to/DragonBallAdvancedAdventure.gba
```

The script validates the GBA header, game code **BDVE**, CRC32 **7D7306DF**, SHA-1 **1338584d8cfa57402603197e65d2e2ff0184d24f**, and MD5 **442bab94b7b3ba7e4a8799d18de52731**. This identifies the reference USA ROM, rather than allowing an incompatible revision. The EEPROM *capacity* is **not yet confirmed** for USA, so the starter uses 512 bytes only as a hypothesis; validate using a known-good `.sav` before claiming save compatibility.

Generated files:
- `variants/dragon_ball_advanced_adventure/game.toml`
- `android/game_android.toml`
- `android/app/build.gradle`
- `rom_identity.json`

The generated configurations contain absolute paths on your build machine. Those files and `rom_identity.json` are gitignored. ROM, BIOS and saves must remain private.

## 2. Fetch the host runtime and UI sources

From the project root (or run `bash scripts/setup_sources.sh`):

```
git clone --recurse-submodules https://github.com/BBlueSkyy/gbarecomp.git gbarecomp
git clone --recurse-submodules https://github.com/mstan/recomp-ui.git recomp-ui
```

The Android engine checkout should come from **BBlueSkyy/gbarecomp**. For reproducible releases, **pin and record specific commits** once an initial working revision is identified. The shared engine owns the Android SDL2 shell; we intentionally avoid cloning Android Java code.

## 3. Generate the guest-code corpus on the host

```
cmake -S gbarecomp -B gbarecomp/build-native -G Ninja
cmake --build gbarecomp/build-native --target gba_recompile
```

Then run:

```
gbarecomp/build-native/gba_recompile --rom /private/path/to/DragonBallAdvancedAdventure.gba --config variants/dragon_ball_advanced_adventure/game.toml --out variants/dragon_ball_advanced_adventure/generated
```

On Windows use `gba_recompile.exe`. Treat game-specific ROM-derived code as private; do not publish it without authorization from rights holders. A successful code generation and compilation is **not** evidence of playable gameplay. Inspect unresolved dispatch, RAM-copied code, and actual ARM/Thumb coverage.

## 4. Build for Android (no intermediate Windows game port)

Generate sources on the build host, configure Android SDK + NDK + JDK 17, then use a Gradle installation (or add an audited Gradle wrapper):

```
cd android
gradle :app:assembleDebug -PgbaAbis=arm64-v8a -PgbaNativeJobs=4
```

The game-specific `android/app/build.gradle` applies the engine's `platform/android/gbarecomp-app.gradle`, which builds the `main` shared library using `CMakeLists.txt`. Android setup asks the user to select their legally obtained ROM and BIOS. Do not use `-PprivateRom` / `-PprivateBios` for shareable APKs.

This is a **build recipe to validate**, not a claim that the APK builds or boots already. The compile is blocked until source generation. Compilation, runtime correctness, and successful Android boot have **not** been validated.

## 5. First acceptance route on a real device

1. ARM64 APK installs and opens shared setup, without any embedded copyrighted assets.
2. Correct ROM and BIOS pass SHA-1 verification; wrong ROM is rejected.
3. Native guest code boots to logo/menu, with structured logs for the first unsupported guest PC.
4. D-pad / A / B / L / R / Start / Select operate in the menu and first story stage.
5. Audio, DMA/IRQ timing, EEPROM saving/reloading and Android resume work.
6. Reboot and play through multiple stages; profile performance after correctness.

## Technical notes

- The actual game-specific GBA code and callback maps are unknown at this stage. Do not invent guest addresses, jump tables, `code_copy` regions or patches.
- Do not conflate a compile success with game boot. The generated source needs ROM-specific auditing.
- Widescreen 16:9 is out of scope until the faithful 240x160 output works.
- The USA save-chip capacity is not independently verified in this starter. Use an unmodified USA dump and validate the actual `.sav` behavior before adjusting `--save-size`.

## References

- https://github.com/BBlueSkyy/gbarecomp
- https://github.com/mstan/WarioWareTwistedRecomp (Android host integration reference)
- https://github.com/BBlueSkyy/gbarecomp/blob/main/platform/android/tools/README.md
- https://gbhwdb.gekkio.fi/cartridges/AGB-BDVP-0/3615retro-1.html

This starter is an independent, ROM-free setup, not a release of the original game.

## USA-specific verified identifiers

- Game Code: `BDVE` (AGB-BDVE-USA)
- Reference CRC32: `7D7306DF`
- Reference SHA-1: `1338584d8cfa57402603197e65d2e2ff0184d24f`
- Reference MD5: `442bab94b7b3ba7e4a8799d18de52731`
- Required input: your own original, unmodified 16 MiB USA cartridge dump.
- CRC32 source: https://gamehacking.org/?game=4387
- SHA-1/MD5 reference: https://gist.github.com/asahui/0d0a97b28cf779d08ae63a9f46dc53b0 (No-Intro database mirror; confirm independently where possible).

For Linux/macOS with prerequisites installed, an optional *private* automation of
identity verification, code generation and ARM64 APK building is available:

```bash
bash scripts/build_android_arm64.sh /private/path/to/DragonBallAdvancedAdventure-USA.gba /private/path/to/gba_bios.bin
```

This helper does not embed copyrighted inputs into the distributable APK.
The build will fail until a workable per-title native translation is produced.
No game start or gameplay behavior has been verified.

## Fork integration

This starter now targets the fork at **https://github.com/BBlueSkyy/gbarecomp**. The helper `scripts/setup_sources.sh` fetches this engine and upstream `recomp-ui`; it does not modify either repository or publish game data. Before creating a new game repository, keep its engine revision pinned (ideally via a Git submodule). A separate game repository has **not** been created on GitHub.
