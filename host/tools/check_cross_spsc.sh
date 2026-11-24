#!/bin/sh
# tools/check_cross_spsc.sh <arm-none-eabi-gcc> <repo_root> <out_dir>
# STEP 07-C1.1: compile-only check of ports/common/gbt_spsc.c for Cortex-M3
# (F103) and Cortex-M4F (G474) with the strict C99 flags at -Os and -O2, no
# writable static data, and disassembly kept for the ordering review
# (docs/STEP07C1_SPSC_DESIGN.md §6). This is GCC evidence only, not ARMCC.
set -u
CC="$1"; R="$2"; OUT="$3"
BIN="$(dirname "$CC")"
NM="$BIN/arm-none-eabi-nm";      [ -x "$NM" ] || NM=arm-none-eabi-nm
OD="$BIN/arm-none-eabi-objdump"; [ -x "$OD" ] || OD=arm-none-eabi-objdump
F="-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion -ffunction-sections -fdata-sections"
fail=0
for cpu in "cortex-m3" "cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard"; do
    tag=$(echo "$cpu" | cut -d' ' -f1)
    for opt in Os O2; do
        d="$OUT/$tag-$opt"
        mkdir -p "$d"
        # shellcheck disable=SC2086
        "$CC" -mcpu=$cpu -mthumb $F -$opt -I"$R" -c "$R/ports/common/gbt_spsc.c" -o "$d/gbt_spsc.o" \
            || { echo "FAIL $tag -$opt"; fail=1; continue; }
        "$OD" -d "$d/gbt_spsc.o" > "$d/gbt_spsc.dis" || fail=1
        sh "$R/tools/check_no_writable_statics.sh" "$NM" "$d/gbt_spsc.o" > /dev/null || { echo "FAIL statics $tag -$opt"; fail=1; }
    done
done
[ "$fail" -eq 0 ] && echo "cross C99 gbt_spsc (M3/M4F, -Os/-O2): PASS"
exit "$fail"
