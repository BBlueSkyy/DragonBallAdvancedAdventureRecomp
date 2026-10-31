#!/usr/bin/env python3
"""Summarize opt-in GBA OBJ/BG attribution into candidate HD art identities.

Reads metadata-only JSONL emitted by the engine's HdObjProbe. This tool does
not read/export sprite pixels, infer characters, change the ROM, or install art.
Candidates require a *matching hashed tile and palette*. OAM indices alone
are not stable artwork identities. No game ROM or BIOS involved.

  python3 scripts/analyze_hd_obj_capture.py \
      --input /private/obj_attribution.jsonl \
      --out /private/hd_obj_candidates.json
"""
from __future__ import annotations
import argparse
from collections import defaultdict
import json
import os
from pathlib import Path
import sys

MAX_INPUT_BYTES = 10 * 1024 * 1024
MAX_SAMPLES = 120
MAX_OBJS = 128
FORMAT = "gba-hd-attribution-v1"


def parse_hash(value: object) -> str:
    if not isinstance(value, str) or len(value) != 16:
        raise ValueError("hash must be 16 hexadecimal digits")
    if any(c not in "0123456789abcdefABCDEF" for c in value):
        raise ValueError("hash contains nonhexadecimal digits")
    return value.lower()


def parse_capture(raw: str) -> dict:
    if len(raw.encode("utf-8")) > MAX_INPUT_BYTES:
        raise ValueError("capture exceeds 10 MiB safety limit")
    lines = [line for line in raw.splitlines() if line.strip()]
    if len(lines) < 2 or len(lines) > MAX_SAMPLES + 1:
        raise ValueError("requires 1 header and 1..120 frame records")
    header = json.loads(lines[0])
    if not isinstance(header, dict) or header.get("format") != FORMAT:
        raise ValueError("unsupported attribution capture header")
    objects = defaultdict(lambda: {"frames": set(), "positions": set(),
                                   "oam_indices": set(), "tiles": set()})
    backgrounds = defaultdict(lambda: {"frames": set(), "scroll": set()})
    seen_frames = set()
    for line in lines[1:]:
        frame = json.loads(line)
        if not isinstance(frame, dict) or not isinstance(frame.get("frame"), int):
            raise ValueError("missing frame number")
        frame_no = frame["frame"]
        if frame_no < 0 or frame_no in seen_frames:
            raise ValueError("duplicate or invalid frame number")
        seen_frames.add(frame_no)
        oam = frame.get("objs")
        bg = frame.get("bg")
        if not isinstance(oam, list) or len(oam) > MAX_OBJS or not isinstance(bg, list) or len(bg) != 4:
            raise ValueError("capture contains invalid OBJ/BG collections")
        seen_oam = set()
        for obj in oam:
            if not isinstance(obj, list) or len(obj) != 14:
                raise ValueError("unsupported OBJ record shape")
            index, a0, a1, a2, x, y, width, height, tile, priority, palette, mode, tilehash, palhash = obj
            values = [index, a0, a1, a2, x, y, width, height, tile, priority, palette, mode]
            if not all(isinstance(v, int) and not isinstance(v, bool) for v in values):
                raise ValueError("OBJ contains noninteger attributes")
            if not (0 <= index < 128 and 0 <= a0 <= 65535 and 0 <= a1 <= 65535 and 0 <= a2 <= 65535
                    and -256 <= x <= 511 and -256 <= y <= 255
                    and width in (8, 16, 32, 64, 128) and height in (8, 16, 32, 64, 128)
                    and 0 <= tile < 1024 and 0 <= priority < 4 and 0 <= palette < 16 and 0 <= mode < 2):
                raise ValueError("OBJ attribute outside hardware ranges")
            if index in seen_oam:
                raise ValueError("duplicate OAM index in one frame")
            seen_oam.add(index)
            tilehash = parse_hash(tilehash)
            palhash = parse_hash(palhash)
            # Identifies artwork data, not a character. A sprite may share tiles.
            key = (tilehash, palhash, width, height, priority)
            stat = objects[key]
            stat["frames"].add(frame_no)
            stat["positions"].add((x, y))
            stat["oam_indices"].add(index)
            stat["tiles"].add(tile)
        for entry in bg:
            if not isinstance(entry, list) or len(entry) != 5:
                raise ValueError("unsupported BG record shape")
            index, control, hofs, vofs, map_hash = entry
            if not (type(index) is int and 0 <= index < 4 and
                    type(control) is int and 0 <= control <= 65535 and
                    type(hofs) is int and 0 <= hofs <= 511 and
                    type(vofs) is int and 0 <= vofs <= 511):
                raise ValueError("BG data invalid")
            key = (index, parse_hash(map_hash))
            stat = backgrounds[key]
            stat["frames"].add(frame_no)
            stat["scroll"].add((hofs, vofs))
    candidates = []
    for (tilehash, palhash, width, height, priority), stat in objects.items():
        positions = sorted(stat["positions"])
        candidate = {
            "tile_hash": tilehash,
            "palette_hash": palhash,
            "width": width, "height": height, "priority": priority,
            "observed_frames": len(stat["frames"]),
            "different_positions": len(positions),
            "oam_indices": sorted(stat["oam_indices"]),
            "tile_indices": sorted(stat["tiles"]),
            "movement_observed": len(positions) >= 3,
            "positions_sample": [[x, y] for x, y in positions[:10]],
        }
        candidates.append(candidate)
    candidates.sort(key=lambda x: (-x["observed_frames"], -x["different_positions"],
                                   x["tile_hash"], x["palette_hash"]))
    for n, candidate in enumerate(candidates, 1):
        candidate["candidate_id"] = f"obj_{n:03d}"
    bgs = [{"layer": layer, "map_hash": keyhash,
            "observed_frames": len(stat["frames"]),
            "scroll_positions": len(stat["scroll"])}
           for (layer, keyhash), stat in backgrounds.items()]
    bgs.sort(key=lambda x: (-x["observed_frames"], x["layer"], x["map_hash"]))
    return {
        "format": "hd-remaster-candidates-v1",
        "observed_frames": len(seen_frames),
        "notice": ("Metadata candidates only: no inferred character names, "
                   "no bitmap extraction, no automatic HD replacement. "
                   "Match identities to gameplay frames before authoring HD assets."),
        "obj_candidates": candidates,
        "bg_candidates": bgs,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    try:
        if args.input.stat().st_size > MAX_INPUT_BYTES:
            raise ValueError("capture file exceeds size limit")
        result = parse_capture(args.input.read_text(encoding="utf-8"))
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print(f"Analyzed {result['observed_frames']} frames; "
              f"{len(result['obj_candidates'])} OBJ identities and "
              f"{len(result['bg_candidates'])} BG map states. "
              f"Report: {args.out}")
        return 0
    except (OSError, UnicodeError, ValueError, json.JSONDecodeError) as exc:
        print(f"capture analysis rejected: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
