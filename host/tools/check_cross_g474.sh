#!/bin/sh
# tools/check_cross_g474.sh <arm-none-eabi-gcc> <repo_root> <g474_project> <out_dir>
# STEP 07-C1.2-B1: compile-only check of the G474 FDCAN adapter against the
# REAL STM32G4 HAL headers of the read-only reference project (HAL 1.2.4),
# Cortex-M4F, strict C99, -Os and -O0. Also compiles the SPSC queue and
# tests/stm32/check_g4_constants.c (proves the host fake's constants equal the
# real ones), keeps disassembly, and checks that STM32 HAL headers are used only
# in ports/stm32/ and tests/support/fake_stm32g4/. Exit 77 if the project is absent.
# GCC evidence only - not an ARMCC (Keil) build.
set -u
CC="$1"; R="$2"; G="$3"; OUT="$4"
[ -d "$G/Drivers/STM32G4xx_HAL_Driver/Inc" ] || { echo "G474 reference project not found: $G (NOT RUN)"; exit 77; }
BIN="$(dirname "$CC")"
OD="$BIN/arm-none-eabi-objdump"; [ -x "$OD" ] || OD=arm-none-eabi-objdump
SZ="$BIN/arm-none-eabi-size"; [ -x "$SZ" ] || SZ=arm-none-eabi-size
F="-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion -ffunction-sections -fdata-sections"
CPU="-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"
DEF="-DSTM32G474xx -DUSE_HAL_DRIVER"
INC="-I$R -isystem $G/Core/Inc -isystem $G/Drivers/STM32G4xx_HAL_Driver/Inc -isystem $G/Drivers/CMSIS/Device/ST/STM32G4xx/Include -isystem $G/Drivers/CMSIS/Include"
fail=0
for opt in Os O0; do
    d="$OUT/m4-$opt"; mkdir -p "$d"
    for src in ports/stm32/g474/port_g474.c ports/common/gbt_spsc.c tests/stm32/check_g4_constants.c; do
        o="$d/$(basename "$src" .c).o"
        # shellcheck disable=SC2086
        "$CC" $CPU $F -$opt $DEF $INC -c "$R/$src" -o "$o" || { echo "FAIL $src -$opt"; fail=1; }
    done
    [ -f "$d/port_g474.o" ] && "$OD" -d "$d/port_g474.o" > "$d/port_g474.dis"
    [ -f "$d/gbt_spsc.o" ] && "$OD" -d "$d/gbt_spsc.o" > "$d/gbt_spsc.dis"
    [ -f "$d/port_g474.o" ] && "$SZ" "$d/port_g474.o" "$d/gbt_spsc.o" > "$d/size.txt"
done
# STM32 HAL / CMSIS headers only in the adapter and the host fake
bad=$(grep -rln -E '#include *[<"](stm32g4xx|stm32f1xx|core_cm[0-9]|cmsis_)' "$R/protocol" "$R/common" "$R/hal" "$R/app" "$R/ports/common" "$R/ports/linux" 2>/dev/null)
[ -n "$bad" ] && { echo "FAIL STM32 header outside ports/stm32: $bad"; fail=1; }
[ "$fail" -eq 0 ] && echo "cross C99 G474 adapter vs real HAL 1.2.4 headers (M4F, -Os/-O0): PASS"
exit "$fail"
