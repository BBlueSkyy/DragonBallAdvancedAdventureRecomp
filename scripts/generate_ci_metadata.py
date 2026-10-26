#!/usr/bin/env python3
"""Generate ROM-free metadata for a *private* Android CI build.

This script accepts only generated sources produced from the already-verified
USA BDVE ROM. It does not generate source from a ROM or embed game assets.
The actual source provenance still needs review by the project owner.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VAR = 'dragon_ball_advanced_adventure'
VAR_DIR = ROOT / 'variants' / VAR
GEN = VAR_DIR / 'generated'
SHA = '1338584d8cfa57402603197e65d2e2ff0184d24f'
BIOS_SHA = '300c20df6731a33952ded8c436f7f186d25d3492'
ROM_FILE = 'dragon_ball_advanced_adventure.gba'

shards = sorted(GEN.glob('recompiled_*.cpp'))
required = [GEN / 'recompiled.h', GEN / 'dispatch_table.cpp']
if len(shards) != 16 or any(not f.is_file() for f in required):
    raise SystemExit(
        f'ERROR: expected 16 recompiled shards and recompiled.h/dispatch_table.cpp; '
        f'found {len(shards)} shards. No APK build will be attempted.'
    )

VAR_DIR.mkdir(parents=True, exist_ok=True)
(ROOT / 'android' / 'app').mkdir(parents=True, exist_ok=True)

# CMake checks for game.toml. Runtime actually uses the separately staged
# android/game_android.toml, so no Termux paths belong in the CI metadata.
(VAR_DIR / 'game.toml').write_text(f'''[game]
name = "Dragon Ball: Advanced Adventure"
short_name = "{VAR}"
default_region = "usa"

[program]
name = "Dragon Ball: Advanced Adventure (USA)"
id = "BDVE"
load_address = 0x08000000
size = 0x01000000
entry_pc = 0x08000000
codegen_shards = 16
static_resume_all = true

[identity]
sha1 = "{SHA}"

[bios]
path = "../../bios/gba_bios.bin"
sha1 = "{BIOS_SHA}"

[rom]
path = "../../roms/{ROM_FILE}"
sha1 = "{SHA}"

[save]
type = "eeprom"
size = 512
path = "../../saves/{VAR}.sav"
''', encoding='utf-8')

(ROOT / 'android' / 'game_android.toml').write_text(f'''[game]
name = "Dragon Ball: Advanced Adventure"
short_name = "{VAR}"
default_region = "usa"

[program]
name = "Dragon Ball: Advanced Adventure (USA)"
id = "BDVE"
load_address = 0x08000000
size = 0x01000000
entry_pc = 0x08000000

[identity]
sha1 = "{SHA}"

[bios]
path = "../../bios/gba_bios.bin"
sha1 = "{BIOS_SHA}"

[rom]
path = "../../roms/{ROM_FILE}"
sha1 = "{SHA}"

[save]
type = "eeprom"
size = 512
path = "../../saves/{VAR}.sav"

[runtime]
debug_port = 19890
window_title = "DragonBallAdvancedAdventureRecomp"

[video]
screen = "raw"
view_width = 240
resize_view = false

[audio]
shadow = false
''', encoding='utf-8')

(VAR_DIR / 'cartridge.cmake').write_text(f'''set(DBAA_ROM_SHA1 "{SHA}")
set(DBAA_ROM_CRC32 0x7D7306DF)
set(DBAA_REGION "usa")
''', encoding='utf-8')

(ROOT / 'android' / 'app' / 'build.gradle').write_text(f'''plugins {{ id "com.android.application" }}
def gameRoot = rootDir.parentFile
def pick = {{ String prop, String env, File fallback ->
  def v = providers.gradleProperty(prop).orNull ?: System.getenv(env)
  return v ? new File(v) : fallback
}}
def engineRoot = pick("gbarecompRoot", "GBARECOMP_ROOT", new File(gameRoot, "gbarecomp"))
def recompUiRoot = pick("recompUiRoot", "RECOMP_UI_ROOT", new File(gameRoot, "recomp-ui"))
ext.gbaGame = [
  applicationId : "org.dbadvancedadventure.recomp",
  versionCode   : 1,
  versionName   : "0.0.1",
  title         : "Dragon Ball: Advanced Adventure",
  romLabel      : "Dragon Ball: Advanced Adventure ROM (USA)",
  romSha1       : ["{SHA}"],
  romSize       : 16777216,
  romFile       : "{ROM_FILE}",
  biosRequired  : true,
  orientation   : "landscape",
  gyro          : false,
  payloadVersion: "1",
  setupNote     : "Select your verified USA ROM and original GBA BIOS.",
  variant       : "{VAR}",
  gameToml      : new File(rootDir, "game_android.toml"),
  gameRoot      : gameRoot,
  engineRoot    : engineRoot,
  recompUiRoot  : recompUiRoot,
  releaseAbis   : ["arm64-v8a"],
  debugAbis     : ["arm64-v8a"],
  nativeJobs    : 1,
]
apply from: new File(engineRoot, "platform/android/gbarecomp-app.gradle")
''', encoding='utf-8')
print(f'PASS: {len(shards)} generated shards present; CI metadata written. No ROM/BIOS present or staged.')
