#!/usr/bin/env python3
"""Analyze owner-triggered, ROM-free HD OBJ/BG attribution JSONL.

This ranks moving OAM slots for investigation. It never labels a slot "Goku"
without owner-provided footage or a validated in-game state comparison.
"""
from __future__ import annotations
import argparse
from collections import defaultdict
import json
from pathlib import Path

FORMAT = "gba-hd-attribution-v1"
MAX_BYTES = 8 * 1024 * 1024
MAX_FRAMES = 120


def analyze(lines: list[str]) -> dict:
    if not lines:
        raise ValueError("empty attribution file")
    header = json.loads(lines[0])
    if not isinstance(header, dict) or header.get("format") != FORMAT:
        raise ValueError("unknown attribution format")
    if not 1 <= len(lines) - 1 <= MAX_FRAMES:
        raise ValueError("expected between 1 and 120 captured frames")
    objects = defaultdict(list)
    backgrounds = defaultdict(list)
    last_frame = -1
    for line in lines[1:]:
        sample = json.loads(line)
        if not isinstance(sample, dict):
            raise ValueError("invalid sample")
        frame = sample.get("frame")
        if not isinstance(frame, int) or frame <= last_frame:
            raise ValueError("frame numbers must increase")
        last_frame = frame
        bg, objs = sample.get("bg"), sample.get("objs")
        if not isinstance(bg, list) or len(bg) != 4 or not isinstance(objs, list):
            raise ValueError("invalid BG or OBJ payload")
        if len(objs) > 128:
            raise ValueError("invalid OBJ count")
        for item in objs:
            if not isinstance(item, list) or len(item) != 14:
                raise ValueError("OBJ must have 14 fields")
            idx, a0, a1, a2, x, y, w, h, tile, priority, bank, mode, thash, phash = item
            if (not isinstance(idx, int) or not 0 <= idx < 128 or
                not all(isinstance(v, int) for v in [a0, a1, a2, x, y, w, h, tile, priority, bank, mode]) or
                not isinstance(thash, str) or len(thash) != 16 or
                not isinstance(phash, str) or len(phash) != 16):
                raise ValueError("invalid OBJ fields")
            int(thash, 16)
            int(phash, 16)
            # Preserve palette-bank grouping, but don't claim that OAM slots
            # uniquely identify a character across frames or scenes.
            objects[(idx, bank)].append((frame, x, y, w, h, thash, phash, tile, priority))
        for item in bg:
            if not isinstance(item, list) or len(item) != 5:
                raise ValueError("BG must have 5 fields")
            idx, control, hofs, vofs, mhash = item
            if not all(isinstance(v, int) for v in [idx, control, hofs, vofs]) or not 0 <= idx < 4:
                raise ValueError("invalid BG fields")
            if not isinstance(mhash, str) or len(mhash) != 16:
                raise ValueError("invalid BG map fingerprint")
            int(mhash, 16)
            backgrounds[idx].append((control, hofs, vofs, mhash))
    motion = []
    for (idx, bank), obs in objects.items():
        if len(obs) < 3:
            continue
        xs = [e[1] for e in obs]
        ys = [e[2] for e in obs]
        spread = max(xs)-min(xs)+max(ys)-min(ys)
        hashes = len({e[5] for e in obs})
        palette_count = len({e[6] for e in obs})
        score = int(spread >= 8)*4 + int(hashes >= 2)*4 + min(len(obs), 40)/10
        # This only sorts for manual attribution; *not* a player classifier.
        if score < 4:
            continue
        motion.append({
            "oam_slot": idx, "palette_bank": bank, "samples": len(obs),
            "x_span": [min(xs), max(xs)], "y_span": [min(ys), max(ys)],
            "tile_patterns": hashes, "palette_patterns": palette_count,
            "typical_size": [obs[0][3], obs[0][4]],
            "priority_values": sorted({e[8] for e in obs}),
            "candidate_score": round(score, 1)
        })
    motion.sort(key=lambda x: (-x["candidate_score"], -x["samples"], x["oam_slot"]))
    layers = []
    for idx, obs in sorted(backgrounds.items()):
        layers.append({
            "bg_index": idx,
            "scroll_x_span": [min(e[1] for e in obs), max(e[1] for e in obs)],
            "scroll_y_span": [min(e[2] for e in obs), max(e[2] for e in obs)],
            "screen_block_patterns": len({e[3] for e in obs}),
            "control_patterns": len({e[0] for e in obs}),
        })
    return {
        "format": "gba-hd-candidates-v1",
        "captured_frames": len(lines)-1,
        "last_guest_frame": last_frame,
        "object_tracks_seen": len(objects),
        "motion_candidates": motion[:40],
        "bg_layers": layers,
        "caution": "Candidate ranks do NOT prove character identity or BG priority. Correlate with gameplay footage before any HD replacement."
    }


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--input", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    args = p.parse_args()
    if args.input.stat().st_size > MAX_BYTES:
        raise SystemExit("Attribution file exceeds 8 MiB")
    result = analyze(args.input.read_text(encoding="utf-8").splitlines())
    args.out.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Captured {result['captured_frames']} frames; "
          f"{len(result['motion_candidates'])} moving candidates -> {args.out}")


if __name__ == "__main__":
    main()
