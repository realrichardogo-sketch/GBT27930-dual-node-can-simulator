#!/bin/sh
# scripts/run_host_checks.sh [build_dir]
# Runs the existing WSL/Linux checks with one command. Nothing new is tested here;
# this only chains the commands documented in docs/VERIFICATION.md:
#   1. shared-core links of host/ resolve to source/Project1_SharedCore (no stale copies)
#   2. CMake configure + build of host/ (strict C99 flags, -Werror)
#   3. ctest (linux_sil_vcan reports NOT RUN = exit 77 without vcan0; never counted as PASS)
#   4. GCC compile-only of every .c of both Keil projects (NOT an ARMCC build)
#   5. every file and include directory referenced by both .uvprojx exists
# All outputs go to build_dir (default: <workspace>/build/host_checks). Exit 0 only if all pass.
set -u
WS=$(cd "$(dirname "$0")/.." && pwd)
B="${1:-$WS/build/host_checks}"
mkdir -p "$B"
rc=0
step() { echo "== $1: $2"; }

bad=0
while IFS= read -r f; do
    [ -n "$f" ] || continue
    [ -L "$WS/host/$f" ] && [ "$(readlink -f "$WS/host/$f")" = "$(readlink -f "$WS/source/Project1_SharedCore/$f")" ] || {
        echo "  not linked to source: host/$f (run scripts/link_harness_to_source.sh)"; bad=1; }
done < "$WS/source/Project1_SharedCore/MIRROR_FILES.txt"
[ "$bad" -eq 0 ] && step 1 "shared-core links PASS" || { step 1 "shared-core links FAIL"; rc=1; }

if cmake -S "$WS/host" -B "$B/host" -DCMAKE_BUILD_TYPE=Debug > "$B/configure.log" 2>&1 \
   && cmake --build "$B/host" -j8 > "$B/build.log" 2>&1; then
    step 2 "host build PASS ($(grep -c -i 'warning' "$B/build.log") warning lines)"
    if (cd "$B/host" && ctest --output-on-failure > "$B/ctest.log" 2>&1); then
        res="PASS"
    else
        res="FAIL"; rc=1
    fi
    # ctest prints "100% tests passed" even when a test was skipped; count explicitly.
    step 3 "ctest $res: $(grep -c ' Passed ' "$B/ctest.log") passed, $(grep -c '\*\*\*Failed\|\*\*\*Timeout\|\*\*\*Exception' "$B/ctest.log") failed, $(grep -c '\*\*\*Skipped\|Not Run' "$B/ctest.log") not run, of $(grep -c 'Test *#' "$B/ctest.log")"
    grep -E '\*\*\*Skipped|\*\*\*Not Run' "$B/ctest.log" | sed 's/^ */   NOT RUN: /'
else
    step 2 "host build FAIL (see $B/build.log)"; rc=1
fi

if sh "$WS/scripts/gcc_board_compile.sh" "$WS/source" "$B/board" > "$B/board_compile.log" 2>&1; then
    step 4 "GCC board compile-only PASS (not ARMCC)"
else
    step 4 "GCC board compile-only FAIL (see $B/board_compile.log)"; rc=1
fi

if sh "$WS/scripts/check_keil_refs.sh" "$WS/source/Project1_G474/MDK-ARM/Project1_G474.uvprojx" \
      "$WS/source/Project1_F103B_Receiver_CubeMX/MDK-ARM/Project1_F103.uvprojx" > "$B/keil_refs.log" 2>&1; then
    step 5 "Keil project references PASS"
else
    step 5 "Keil project references FAIL (see $B/keil_refs.log)"; rc=1
fi
echo "logs: $B"
exit "$rc"
