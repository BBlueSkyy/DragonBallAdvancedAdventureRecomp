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
  options.launcher_expose_gyro = false;
#if defined(__ANDROID__)
  options.ui_touch_friendly = true;
  // Four independent buttons: A = jump, B = punch, L = Power Pole,
  // R = Ki/Kamehameha. Read-only guest image probe controls blue glow.
  options.mobile_action_hud = true;
  options.mobile_special_meter_probe = dragonball::special_meter_fill;
  // Tap a visible DATA MENU row or GAME MENU tile to navigate and confirm.
  // Gameplay is unchanged: unrecognized scenes synthesize no keys.
  options.mobile_menu_frame_probe = dragonball::menu_touch::observe_menu_frame;
  options.input_frame = dragonball::menu_touch::menu_input_frame;
  options.touch_gesture_claims |= gbarecomp::kTouchClaimTap;
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
