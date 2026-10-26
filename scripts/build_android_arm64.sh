#!/usr/bin/env bash
# Private host build: ROM stays outside the repo; generated code is local and ignored.
set -euo pipefail
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
  echo "Usage: bash scripts/build_android_arm64.sh /absolute/path/to/USA.gba [/absolute/path/to/gba_bios.bin]" >&2
  exit 2
fi
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ROM="$(realpath "$1")"
BIOS="${2:-}"
if [ -n "$BIOS" ]; then BIOS="$(realpath "$BIOS")"; fi
cd "$ROOT"
if [ -n "$BIOS" ]; then
  python3 scripts/prepare_rom.py "$ROM" --bios "$BIOS"
else
  python3 scripts/prepare_rom.py "$ROM"
fi
if [ ! -f gbarecomp/CMakeLists.txt ] || [ ! -f recomp-ui/recomp_ui.cmake ]; then
  echo "Missing engine sources. Clone gbarecomp and recomp-ui as described in README.md." >&2
  exit 1
fi
cmake -S gbarecomp -B gbarecomp/build-native -G Ninja
cmake --build gbarecomp/build-native --target gba_recompile
mkdir -p variants/dragon_ball_advanced_adventure/generated
gbarecomp/build-native/gba_recompile \
  --rom "$ROM" \
  --config variants/dragon_ball_advanced_adventure/game.toml \
  --out variants/dragon_ball_advanced_adventure/generated
if ! compgen -G "variants/dragon_ball_advanced_adventure/generated/*.cpp" > /dev/null; then
  echo "Recompiler did not produce C++ shards; inspect its output." >&2
  exit 1
fi
cd android
if [ -x ./gradlew ]; then GRADLE=./gradlew; else GRADLE=gradle; fi
"$GRADLE" :app:assembleDebug -PgbaAbis=arm64-v8a -PgbaNativeJobs=4
echo "Check android/app/build/outputs/apk/debug/app-debug.apk"
echo "An APK alone does not demonstrate gameplay compatibility."
