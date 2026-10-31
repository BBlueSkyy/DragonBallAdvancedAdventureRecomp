#include <string>
#include <vector>
#include "mobile_platform.h"
#include "runtime.h"
#include "game_launcher_boot.h"
#include "dragonball_special_meter.h"
#include "dragonball_menu_touch.h"

#ifndef GBARECOMP_BUILTIN_NAME
#define GBARECOMP_BUILTIN_NAME "Dragon Ball: Advanced Adventure"
#endif
#ifndef GBARECOMP_BUILTIN_SHA1
#define GBARECOMP_BUILTIN_SHA1 ""
#endif
#ifndef GBARECOMP_BUILTIN_REGION
#define GBARECOMP_BUILTIN_REGION ""
#endif
#ifndef GBARECOMP_BUILTIN_CRC32
#define GBARECOMP_BUILTIN_CRC32 0
#endif
#ifndef GBARECOMP_DEFAULT_GAME_CONFIG
#define GBARECOMP_DEFAULT_GAME_CONFIG "variants/dragon_ball_advanced_adventure/game.toml"
#endif

static int game_main(int argc, char** argv) {
  std::vector<std::string> args(argv, argv + argc);
  gbarecomp::MobileProcessOptions mobile{};
  mobile.game_config = GBARECOMP_DEFAULT_GAME_CONFIG;
  mobile.program_name = "./DragonBallAdvancedAdventureRecomp";
  gbarecomp::mobile_prepare_process(args, mobile);

  gbarecomp::RunOptions options{};
  options.builtin_game_name = GBARECOMP_BUILTIN_NAME;
  options.builtin_rom_sha1 = GBARECOMP_BUILTIN_SHA1;
  options.builtin_rom_crc32 = GBARECOMP_BUILTIN_CRC32;
  options.launcher_region = GBARECOMP_BUILTIN_REGION;
  options.launcher_game_config = GBARECOMP_DEFAULT_GAME_CONFIG;
  options.launcher_save_path = "saves/dragon_ball_advanced_adventure.sav";
  // Opt-in, ROM-private sprite data patches: enabled only when the player
  // explicitly picks a verified IPS/IPS32/BPS patch in the existing launcher.
  // The unmodified BDVE cartridge remains the verified source; the original
  // file is never written. No ROM, artwork or modified executable is bundled.
  options.launcher_enable_rom_patches = true;
  options.launcher_rom_patch_note =
      "Optional graphics-only IPS patch for the verified USA BDVE ROM. "
      "Use only data-only sprite tile patches made from your own cartridge; "
      "the original ROM remains unchanged.";
  options.launcher_expose_gyro = false;
#if defined(__ANDROID__)
  options.ui_touch_friendly = true;
  // Four independent buttons: A = jump, B = punch, L = Power Pole,
  // R = Ki/Kamehameha. Read-only guest image probe controls blue glow.
  options.mobile_action_hud = true;
  options.mobile_special_meter_probe = dragonball::special_meter_fill;
  // Tap PRESS START and DATA/GAME/PAUSE choices; explicit BACK is
  // context-gated. OPTION/COLLECTION preserve their original GBA pages.
  // Gameplay is unchanged: unrecognized scenes synthesize no keys.
  options.mobile_menu_frame_probe = dragonball::menu_touch::observe_menu_frame;
  options.input_frame = dragonball::menu_touch::menu_input_frame;
  // Menu navigation owns both short taps and long holds. Unclaimed
  // LongPress is an engine shortcut that opens runtime settings after
  // ~650 ms, stealing touch focus from the GBA menu. The game policy
  // already acts on the initial TouchEvent::Down, so a hold must not
  // trigger a separate host action or an extra A press.
  // Runtime settings remain available through Android Back / the
  // engine's explicit menu paths, rather than holding game-screen pixels.
  options.touch_gesture_claims |=
      gbarecomp::kTouchClaimTap | gbarecomp::kTouchClaimLongPress;
#endif
  if (game_launcher_preboot(args, options)) return 0;
  std::vector<char*> native_args;
  native_args.reserve(args.size());
  for (auto& a : args) native_args.push_back(a.data());
  return gbarecomp::run_game(static_cast<int>(native_args.size()), native_args.data(), options);
}

#if defined(__ANDROID__)
extern "C" int SDL_main(int argc, char** argv) {
  return gbarecomp::mobile_run_with_stack(game_main, argc, argv);
}
#else
int main(int argc, char** argv) { return game_main(argc, argv); }
#endif
