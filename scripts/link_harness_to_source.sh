#!/bin/sh
# scripts/link_harness_to_source.sh - STEP 07-C2 source-of-truth wiring (run once, idempotent).
#
# Replaces every SharedCore file mirrored in the host harness host/ (list: MIRROR_FILES.txt in
# source/Project1_SharedCore) by a RELATIVE symlink to source/Project1_SharedCore/<same path>.
# All existing CMake targets and check scripts of the harness then compile the actual hardware
# SharedCore. Before replacing a regular file it must be byte-identical to the source file
# (proves the harness copy was the same baseline); otherwise the script stops.
# STEP 07 release layout: harness moved from regression/gbt27930-sim-lean/ to host/ (one level
# below the workspace root instead of two). Re-run after copying the tree through Windows, which
# turns the links into regular copies.
set -eu
WS=$(cd "$(dirname "$0")/.." && pwd)
SC="$WS/source/Project1_SharedCore"
H="$WS/host"
n=0
while IFS= read -r f; do
    [ -n "$f" ] || continue
    src="$SC/$f"; dst="$H/$f"
    [ -f "$src" ] || { echo "missing in source: $f"; exit 1; }
    if [ -L "$dst" ]; then
        :
    elif [ -f "$dst" ]; then
        cmp -s "$dst" "$src" || { echo "harness copy differs from source (not linked): $f"; exit 1; }
        depth=$(printf '%s' "$f" | tr -cd '/' | wc -c)
        up=".."; i=0; while [ "$i" -lt "$depth" ]; do up="$up/.."; i=$((i + 1)); done
        ln -sf "$up/source/Project1_SharedCore/$f" "$dst"   # replaces the identical regular file
    else
        echo "missing in harness: $f"; exit 1
    fi
    [ "$(readlink -f "$dst")" = "$(readlink -f "$src")" ] || { echo "bad link: $f"; exit 1; }
    n=$((n + 1))
done < "$SC/MIRROR_FILES.txt"
echo "linked $n harness files to source/Project1_SharedCore"
