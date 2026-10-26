#!/usr/bin/env bash
# Install Android game-port dependencies without touching your GBARecomp fork.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
ENGINE_URL="https://github.com/BBlueSkyy/gbarecomp.git"
UI_URL="https://github.com/mstan/recomp-ui.git"
if [ ! -d gbarecomp ]; then
  git clone --recurse-submodules "$ENGINE_URL" gbarecomp
elif [ -d gbarecomp/.git ]; then
  origin="$(git -C gbarecomp remote get-url origin)"
  case "$origin" in
    https://github.com/BBlueSkyy/gbarecomp|https://github.com/BBlueSkyy/gbarecomp.git|git@github.com:BBlueSkyy/gbarecomp.git)
      git -C gbarecomp submodule update --init --recursive ;;
    *) echo "gbarecomp exists with unexpected origin: $origin; refusing to change it" >&2; exit 1 ;;
  esac
else
  echo "gbarecomp exists but is not a Git checkout" >&2; exit 1
fi
if [ ! -d recomp-ui ]; then
  git clone --recurse-submodules "$UI_URL" recomp-ui
elif [ -d recomp-ui/.git ]; then
  git -C recomp-ui submodule update --init --recursive
else
  echo "recomp-ui exists but is not a Git checkout" >&2; exit 1
fi
printf 'GBARecomp engine commit: '; git -C gbarecomp rev-parse --short HEAD
printf 'recomp-ui commit: '; git -C recomp-ui rev-parse --short HEAD
printf 'Next: run scripts/prepare_rom.py with your private USA ROM.\n'
