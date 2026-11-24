#!/bin/sh
# tools/check_cross_c99.sh <arm-none-eabi-gcc> <repo_root> <out_dir>
# Compile-only check (no link, no firmware image) of common/ and protocol/
# for Cortex-M3 (F103) and Cortex-M4F (G474) with strict C99 flags, then
# verify the resulting objects contain no writable static data.
set -u
CC="$1"; R="$2"; OUT="$3"
NM="$(dirname "$CC")/arm-none-eabi-nm"
[ -x "$NM" ] || NM=arm-none-eabi-nm
F="-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion -Os -ffunction-sections -fdata-sections"
fail=0
for cpu in "cortex-m3" "cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard"; do
    tag=$(echo "$cpu" | cut -d' ' -f1)
    mkdir -p "$OUT/$tag"
    for c in "$R"/common/*.c "$R"/protocol/*.c; do
        o="$OUT/$tag/$(basename "$c" .c).o"
        # shellcheck disable=SC2086
        "$CC" -mcpu=$cpu -mthumb $F -I"$R" -c "$c" -o "$o" || { echo "FAIL $tag $c"; fail=1; }
    done
    sh "$R/tools/check_no_writable_statics.sh" "$NM" "$OUT/$tag"/*.o || fail=1
done
[ "$fail" -eq 0 ] && echo "cross C99 compile ($CC): PASS"
exit "$fail"
