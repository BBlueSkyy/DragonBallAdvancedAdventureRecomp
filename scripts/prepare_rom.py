#!/usr/bin/env python3
"""Generate USA-only ROM metadata and GBARecomp Android files. Never copies ROM bytes."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import sys
import zlib

BIOS_SHA1 = "300c20df6731a33952ded8c436f7f186d25d3492"
GBA_SIZE = 16 * 1024 * 1024
REGIONS = {"BDVE": "usa"}
USA_CRC32 = 0x7D7306DF
USA_SHA1 = "1338584d8cfa57402603197e65d2e2ff0184d24f"
USA_MD5 = "442bab94b7b3ba7e4a8799d18de52731"
VARIANT = "dragon_ball_advanced_adventure"
ROM_FILE = "dragon_ball_advanced_adventure.gba"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path, help="Private, legally dumped .gba file")
    parser.add_argument("--bios", type=Path, default=None,
                        help="Private BIOS path for local desktop runtime (not copied)")
    parser.add_argument("--save-size", type=int, choices=(512, 8192), default=512,
                        help="USA EEPROM size candidate (512 or 8192 bytes); verify against a real save")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    rom_path = args.rom.expanduser().resolve()
    if not rom_path.is_file():
        parser.error(f"ROM not found: {rom_path}")
    data = rom_path.read_bytes()
    if len(data) != GBA_SIZE:
        parser.error(f"Unexpected ROM size {len(data)}; expected {GBA_SIZE} bytes (16 MiB)")
    if data[0xB2] != 0x96:
        parser.error("Invalid GBA fixed header byte at offset 0xB2")
    expected_check = (-0x19 - sum(data[0xA0:0xBD])) & 0xFF
    if data[0xBD] != expected_check:
        parser.error("Invalid GBA header checksum: ROM may be corrupt or modified")
    code = data[0xAC:0xB0].decode("ascii", errors="replace")
    if code not in REGIONS:
        parser.error(f"Unexpected game code {code!r}; expected one of {', '.join(REGIONS)}")
    sha = hashlib.sha1(data).hexdigest()
    md5 = hashlib.md5(data).hexdigest()
    crc = zlib.crc32(data) & 0xFFFFFFFF
    region = REGIONS[code]
    print(f"GBA game code: {code} ({region})")
    print(f"ROM SHA-1:     {sha}")
    print(f"ROM MD5:       {md5}")
    print(f"ROM CRC32:     {crc:08x}")
    print(f"Save marker EEPROM_V: {b'EEPROM_V' in data}")
    if sha != USA_SHA1 or crc != USA_CRC32 or md5 != USA_MD5:
        parser.error("This is not the supported, unmodified USA dump (BDVE / CRC32 7D7306DF). "
                     "Check your ROM revision and hashes; no code will be generated for an unknown image.")
    print("Retail USA ROM hash verified against the configured reference.")
    print("WARNING: USA EEPROM capacity remains unverified; default 512 bytes is provisional.", file=sys.stderr)
    if b"EEPROM_V" not in data:
        print("WARNING: No EEPROM_V SDK marker found; verify the actual save chip.", file=sys.stderr)
    print("IMPORTANT: file identity does not prove compatibility; guest-code execution needs testing.")
    var = root / "variants" / VARIANT
    var.mkdir(parents=True, exist_ok=True)
    (root / "android" / "app").mkdir(parents=True, exist_ok=True)

    # The first TOML is used on the host for guest code generation.
    # The second TOML is copied into Android app-private storage and uses relative paths.
    bios_host = str(args.bios.expanduser().resolve()) if args.bios else str(root / "bios" / "gba_bios.bin")
    game = f'''# Generated for a user-verified ROM; do not publish generated ROM-derived code.
[game]
name = "Dragon Ball: Advanced Adventure"
short_name = "{VARIANT}"
default_region = "{region}"

[program]
name = "Dragon Ball: Advanced Adventure ({region})"
id = "{code}"
load_address = 0x08000000
size = 0x01000000
entry_pc = 0x08000000
codegen_shards = 16
static_resume_all = true

[identity]
sha1 = "{sha}"
md5 = "{md5}"

[bios]
path = {json.dumps(bios_host)}
sha1 = "{BIOS_SHA1}"

[rom]
path = {json.dumps(str(rom_path))}
sha1 = "{sha}"

[save]
type = "eeprom"
size = {args.save_size}
path = "../../saves/{VARIANT}.sav"

[runtime]
debug_port = 19890
window_title = "DragonBallAdvancedAdventureRecomp"

[video]
screen = "raw"
view_width = 240
resize_view = false

[audio]
shadow = false
'''
    android_toml = f'''# Runtime paths resolve in the Android private-file layout.
[game]
name = "Dragon Ball: Advanced Adventure"
short_name = "{VARIANT}"
default_region = "{region}"

[program]
name = "Dragon Ball: Advanced Adventure ({region})"
id = "{code}"
load_address = 0x08000000
size = 0x01000000
entry_pc = 0x08000000

[identity]
sha1 = "{sha}"
md5 = "{md5}"

[bios]
path = "../../bios/gba_bios.bin"
sha1 = "{BIOS_SHA1}"

[rom]
path = "../../roms/{ROM_FILE}"
sha1 = "{sha}"

[save]
type = "eeprom"
size = {args.save_size}
path = "../../saves/{VARIANT}.sav"

[runtime]
debug_port = 19890
window_title = "DragonBallAdvancedAdventureRecomp"

[video]
screen = "raw"
view_width = 240
resize_view = false

[audio]
shadow = false
'''
    gradle = f'''plugins {{ id "com.android.application" }}
// Game-specific settings. Shared SDL2 Android shell is in gbarecomp/platform/android/.
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
  romLabel      : "Dragon Ball: Advanced Adventure ROM ({region})",
  romSha1       : ["{sha}"],
  romSize       : {len(data)},
  romFile       : "{ROM_FILE}",
  biosRequired  : true,
  orientation   : "landscape",
  gyro          : false,
  payloadVersion: "1",
  setupNote     : "Select your verified ROM and GBA BIOS. Experimental research build.",
  variant       : "{VARIANT}",
  gameToml      : new File(rootDir, "game_android.toml"),
  gameRoot      : gameRoot,
  engineRoot    : engineRoot,
  recompUiRoot  : recompUiRoot,
  releaseAbis   : ["arm64-v8a"],
  debugAbis     : ["arm64-v8a"],
  nativeJobs    : 4,
]
apply from: new File(engineRoot, "platform/android/gbarecomp-app.gradle")
'''
    cartridge_cmake = f'''# Locally generated from a verified cartridge dump.
set(DBAA_ROM_SHA1 "{sha}")
set(DBAA_ROM_CRC32 0x{crc:08X})
set(DBAA_REGION "{region}")
'''
    (var / "game.toml").write_text(game, encoding="utf-8")
    (var / "cartridge.cmake").write_text(cartridge_cmake, encoding="utf-8")
    (root / "android" / "game_android.toml").write_text(android_toml, encoding="utf-8")
    (root / "android" / "app" / "build.gradle").write_text(gradle, encoding="utf-8")
    identity = {
        "title": "Dragon Ball: Advanced Adventure",
        "game_code": code, "region": region, "rom_size": len(data),
        "sha1": sha, "md5": md5, "crc32": f"{crc:08x}",
        "save_type_candidate": "eeprom", "save_size_candidate": args.save_size,
        "usa_reference_hashes_match": True, "usa_save_capacity_verified": False,
        "native_gameplay_verified": False, "android_apk_built": False,
    }
    (root / "rom_identity.json").write_text(json.dumps(identity, indent=2), encoding="utf-8")
    print("Wrote game.toml, Android TOML, Gradle game config, cartridge.cmake and identity report.")
    print("No ROM or BIOS bytes were copied. No code was recompiled; no APK was built.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
