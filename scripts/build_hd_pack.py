#!/usr/bin/env python3
"""Build a private static HUD HD pack from an EXACT native GBA frame and original HD art.

Only a *fixed* scene-verified region is supported; this does not replace
moving OBJs, backgrounds that scroll, game code or sprite animations.
Requires Pillow. Generated BMPs are processed by SDL2 at host resolution.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

GBA_WIDTH, GBA_HEIGHT = 240, 160
FNV_OFFSET = 14695981039346656037
FNV_PRIME = 1099511628211
MASK = (1 << 64) - 1


def fingerprint(frame_rgb: bytes, x: int, y: int, w: int, h: int) -> str:
    if len(frame_rgb) != 240 * 160 * 3:
        raise ValueError("frame must be unscaled 240x160 RGB888")
    if x < 0 or y < 0 or w <= 0 or h <= 0 or x + w > 240 or y + h > 160:
        raise ValueError("invalid native source region")
    value = FNV_OFFSET
    for row in range(y, y + h):
        start = (row * 240 + x) * 3
        for channel in frame_rgb[start:start + w * 3]:
            value = ((value ^ channel) * FNV_PRIME) & MASK
    return f"{value:016x}"


def make_pack(frame_path: Path, art_path: Path, output_dir: Path,
              x: int, y: int, w: int, h: int) -> dict:
    from PIL import Image
    with Image.open(frame_path) as original:
        if original.size != (GBA_WIDTH, GBA_HEIGHT):
            raise ValueError("source must be an exact 240x160 screenshot without UI/filter")
        # No nearest resize or color correction; native RGB888 byte-for-byte.
        frame = original.convert("RGB").tobytes()
    hash_value = fingerprint(frame, x, y, w, h)
    with Image.open(art_path) as art:
        if art.width < 2*w or art.height < 2*h or max(art.size) > 2048:
            raise ValueError("HD artwork must be 2x native or larger, <=2048px")
        rgba = art.convert("RGBA")
        if rgba.getchannel("A").getextrema() != (255,255):
            raise ValueError("stage-1 replacement must be a fully opaque rectangle")
        # 24-bit BMP: SDL2's built-in BMP loader needs no SDL_image/libpng.
        final_art = rgba.convert("RGB")
        output_dir.mkdir(parents=True, exist_ok=True)
        final_art.save(output_dir / "panel.bmp", format="BMP")
    asset = output_dir / "panel.bmp"
    if asset.stat().st_size > 16 * 1024 * 1024:
        asset.unlink()
        raise ValueError("HD panel exceeds 16 MiB limit")
    manifest = {"version": 1, "regions": [{
        "image": "panel.bmp", "x": x, "y": y, "w": w, "h": h,
        "fingerprint": hash_value,
    }]}
    (output_dir / "hdpack.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Stage-1 HD pack: {output_dir}, source RGB hash {hash_value}, "
          f"artwork SHA256 {hashlib.sha256(asset.read_bytes()).hexdigest()}")
    return manifest


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--frame", type=Path, required=True,
                   help="unaltered 240x160 RGB GBA screenshot, not a phone screenshot")
    p.add_argument("--art", type=Path, required=True,
                   help="your HD artwork PNG for the source rectangle (fully opaque)")
    p.add_argument("--out", type=Path, required=True,
                   help="private directory where hdpack.json and panel.bmp are written")
    for key in ("x", "y", "w", "h"):
        p.add_argument("--" + key, type=int, required=True)
    args = p.parse_args()
    try:
        make_pack(args.frame,args.art,args.out,args.x,args.y,args.w,args.h)
        return 0
    except (OSError, ValueError, ImportError) as exc:
        print(f"HD pack rejected: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
