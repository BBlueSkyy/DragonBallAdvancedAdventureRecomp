#!/usr/bin/env bash
# Start a PRIVATE game repository from a local (Termux) recompilation checkout.
# This command does not create or push the repo; it stages the expected sources.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
GEN=variants/dragon_ball_advanced_adventure/generated
python3 scripts/generate_ci_metadata.py
for f in "$GEN"/*.cpp "$GEN"/*.h; do
  [ -f "$f" ] || { echo "Missing generated file: $f" >&2; exit 1; }
  size="$(wc -c < "$f")"
  if [ "$size" -ge 95000000 ]; then
    echo "Generated shard is too large for GitHub: $f ($size bytes). Re-shard before pushing." >&2
    exit 1
  fi
done
if [ ! -d .git ]; then
  git init -b main
fi
# Do not commit ROM, BIOS, saves, object files, user Termux paths or local checkouts.
git add .gitignore .github CMakeLists.txt README.md android/build.gradle android/settings.gradle android/gradle.properties src scripts
# Only in a private GitHub repo, the generated guest corpus. Explicit force-add
# overrides the protective ignore rule; never push this repository publicly.
git add -f "$GEN"/*.cpp "$GEN"/*.h
printf '\nStaged files (review carefully before publishing to your private repo):\n'
git diff --cached --stat | tail -n 25
printf '\nIMPORTANT: generated code is ROM-derived; use a PRIVATE repository.\n'
printf 'IMPORTANT: git commit/push and GitHub repo creation are NOT done by this script.\n'
