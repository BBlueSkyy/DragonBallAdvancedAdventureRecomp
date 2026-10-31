#!/usr/bin/env python3
"""ROM-private GBA 4bpp sprite tile replacement -> IPS data-only patch.

Never writes a ROM, never bundles art, and rejects unmatched/unverified tiles.
See docs/SPRITE_OVERRIDES.md before using on the BDVE retail USA game.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

USA_SHA1 = "1338584d8cfa57402603197e65d2e2ff0184d24f"
USA_SIZE = 16 * 1024 * 1024
TILE_BYTES = 32
IPS_LIMIT = 0x1000000


def encode_4bpp(width: int, height: int, pixels: bytes) -> bytes:
    """Pack an *indexed* image into row-major GBA 8x8 tiles, 4bpp."""
    if width < 8 or height < 8 or width % 8 or height % 8:
        raise ValueError("indexed image dimensions must be positive multiples of 8")
    if len(pixels) != width * height:
        raise ValueError("indexed image pixel length does not match dimensions")
    if any(value > 15 for value in pixels):
        raise ValueError("indexed PNG must use only palette entries 0..15")
    out = bytearray()
    for ty in range(0, height, 8):
        for tx in range(0, width, 8):
            for y in range(8):
                row = (ty + y) * width + tx
                for x in range(0, 8, 2):
                    a, b = pixels[row + x], pixels[row + x + 1]
                    out.append(a | (b << 4))
    return bytes(out)


def png_tiles(input_path: Path) -> bytes:
    try:
        from PIL import Image
    except ImportError as exc:
        raise ValueError("PNG conversion needs Pillow; raw .4bpp tiles do not") from exc
    with Image.open(input_path) as image:
        if image.mode != "P":
            raise ValueError("PNG must be palette-indexed (mode P), not RGBA/RGB")
        transparency = image.info.get("transparency")
        if isinstance(transparency, int) and transparency != 0:
            raise ValueError("transparent palette entry must be index 0")
        if isinstance(transparency, bytes) and any(
            alpha != 255 and index != 0 for index, alpha in enumerate(transparency)
        ):
            raise ValueError("only palette entry 0 may be transparent")
        return encode_4bpp(image.width, image.height, image.tobytes())


def ips_from_diff(offset: int, original: bytes, replacement: bytes) -> bytes:
    """Only emit changed spans; IPS records use a 24-bit ROM-relative offset."""
    if len(original) != len(replacement):
        raise ValueError("data-only replacement must preserve byte length")
    if offset < 0xC0 or offset + len(original) > IPS_LIMIT:
        raise ValueError("ROM-relative tile range outside safe IPS bounds")
    patch = bytearray(b"PATCH")
    i = 0
    while i < len(original):
        if original[i] == replacement[i]:
            i += 1
            continue
        start = i
        while i < len(original) and original[i] != replacement[i] and i - start < 0xFFFF:
            i += 1
        absolute = offset + start
        patch.extend(absolute.to_bytes(3, "big"))
        patch.extend((i - start).to_bytes(2, "big"))
        patch.extend(replacement[start:i])
    patch.extend(b"EOF")
    return bytes(patch)


def patch_4bpp(rom: bytes, source: bytes, replacement: bytes,
               expected_sha1: str = USA_SHA1, offset: int | None = None):
    if len(rom) != USA_SIZE or rom[0xAC:0xB0] != b"BDVE":
        raise ValueError("only the verified USA BDVE 16 MiB cartridge is supported")
    actual_sha1 = hashlib.sha1(rom).hexdigest()
    if actual_sha1 != expected_sha1.lower():
        raise ValueError("source ROM SHA-1 mismatch; refuse an unknown game/revision")
    if not source or len(source) % TILE_BYTES or len(source) < 2 * TILE_BYTES:
        raise ValueError("source must contain at least two complete 4bpp tiles")
    if len(replacement) != len(source):
        raise ValueError("replacement must contain exactly the same number of tiles")
    if source == replacement:
        raise ValueError("replacement has no different pixel data")
    if offset is None:
        first = rom.find(source, 0xC0)
        if first < 0:
            raise ValueError("source tiles not found as contiguous raw ROM data; "
                             "possibly compressed/streamed; do not guess offsets")
        second = rom.find(source, first + 1)
        if second >= 0:
            raise ValueError("source tiles match multiple ROM offsets; supply a "
                             "verified --offset or use a longer source tile span")
        offset = first
    if offset < 0xC0 or offset + len(source) > len(rom):
        raise ValueError("invalid source range (including ROM header)")
    if offset % 4:
        raise ValueError("source must start at a 4-byte aligned ROM offset")
    if rom[offset:offset + len(source)] != source:
        raise ValueError("source tile bytes do not match ROM at selected offset")
    return ips_from_diff(offset, source, replacement), {
        "format": "GBA 4bpp raw tiles; IPS patch",
        "game_code": "BDVE",
        "source_sha1": actual_sha1,
        "offset_hex": f"0x{offset:06X}",
        "tile_count": len(source) // TILE_BYTES,
        "patch_sha256": "",  # Filled by caller after constructing patch
        "note": "data-only; source ROM not modified; runtime validation required",
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    conv = commands.add_parser("tiles", help="convert palette-indexed PNG to GBA 4bpp tiles")
    conv.add_argument("--image", type=Path, required=True)
    conv.add_argument("--out", type=Path, required=True)
    patch = commands.add_parser("patch", help="create ROM-free IPS from verified tile data")
    patch.add_argument("--rom", type=Path, required=True, help="private BDVE original ROM")
    patch.add_argument("--original", type=Path, required=True, help="original 4bpp tiles")
    patch.add_argument("--replacement", type=Path, required=True, help="new 4bpp tiles")
    patch.add_argument("--out", type=Path, required=True, help="generated IPS patch")
    patch.add_argument("--offset", type=lambda s: int(s, 0),
                       help="verified ROM offset when the source match is ambiguous")
    args = parser.parse_args(argv)
    try:
        if args.command == "tiles":
            raw = png_tiles(args.image)
            args.out.parent.mkdir(parents=True, exist_ok=True)
            args.out.write_bytes(raw)
            print(f"Packed {len(raw) // TILE_BYTES} indexed 4bpp tiles")
        else:
            rom = args.rom.read_bytes()
            source = args.original.read_bytes()
            replacement = args.replacement.read_bytes()
            data, report = patch_4bpp(rom, source, replacement,
                                      offset=args.offset)
            report["patch_sha256"] = hashlib.sha256(data).hexdigest()
            args.out.parent.mkdir(parents=True, exist_ok=True)
            args.out.write_bytes(data)
            args.out.with_suffix(args.out.suffix + ".json").write_text(
                json.dumps(report, indent=2) + "\n", encoding="utf-8")
            print(f"Generated {args.out} ({report['tile_count']} tiles at "
                  f"{report['offset_hex']}); source ROM unchanged")
        return 0
    except (OSError, ValueError) as exc:
        print(f"sprite override rejected: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
