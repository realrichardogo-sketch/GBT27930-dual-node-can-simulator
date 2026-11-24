#!/bin/sh
# tools/check_cross_f103.sh <arm-none-eabi-gcc> <repo_root> <f103_project> <out_dir>
# STEP 07-C2.1: compile-only check of the F103 bxCAN adapter against the REAL STM32F1 HAL
# 1.1.10 headers of the F103 project (read-only), Cortex-M3, strict C99, -O0 and -O3 (the
# Keil target uses -O3). Also compiles the SPSC queue, the battery model and
# tests/stm32/check_f1_constants.c (fake == real constants), keeps disassembly and sizes, and
# checks that STM32 HAL headers appear only in ports/stm32/. Exit 77 if the project is absent.
# GCC evidence only - not an ARMCC (Keil) build.
set -u
CC="$1"; R="$2"; G="$3"; OUT="$4"
[ -d "$G/Drivers/STM32F1xx_HAL_Driver/Inc" ] || { echo "F103 project not found: $G (NOT RUN)"; exit 77; }
BIN="$(dirname "$CC")"
OD="$BIN/arm-none-eabi-objdump"; [ -x "$OD" ] || OD=arm-none-eabi-objdump
SZ="$BIN/arm-none-eabi-size"; [ -x "$SZ" ] || SZ=arm-none-eabi-size
F="-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion -ffunction-sections -fdata-sections"
CPU="-mcpu=cortex-m3 -mthumb -mfloat-abi=soft"
DEF="-DSTM32F103xE -DUSE_HAL_DRIVER"
INC="-I$R -isystem $G/Core/Inc -isystem $G/Drivers/STM32F1xx_HAL_Driver/Inc -isystem $G/Drivers/CMSIS/Device/ST/STM32F1xx/Include -isystem $G/Drivers/CMSIS/Include"
fail=0
for opt in O0 O3; do
    d="$OUT/m3-$opt"; mkdir -p "$d"
    for src in ports/stm32/f103/port_f103.c ports/common/gbt_spsc.c app/battery_model.c tests/stm32/check_f1_constants.c; do
        o="$d/$(basename "$src" .c).o"
        # shellcheck disable=SC2086
        "$CC" $CPU $F -$opt $DEF $INC -c "$R/$src" -o "$o" || { echo "FAIL $src -$opt"; fail=1; }
    done
    for o in port_f103 gbt_spsc; do
        [ -f "$d/$o.o" ] && "$OD" -d "$d/$o.o" > "$d/$o.dis"
    done
    [ -f "$d/port_f103.o" ] && "$SZ" "$d/port_f103.o" "$d/gbt_spsc.o" "$d/battery_model.o" > "$d/size.txt"
done
bad=$(grep -rln -E '#include *[<"](stm32g4xx|stm32f1xx|core_cm[0-9]|cmsis_)' "$R/protocol" "$R/common" "$R/hal" "$R/ports/common" "$R/ports/linux" "$R/app"/*.c "$R/app"/*.h 2>/dev/null)
[ -n "$bad" ] && { echo "FAIL STM32 header outside ports/stm32: $bad"; fail=1; }
[ "$fail" -eq 0 ] && echo "cross C99 F103 adapter vs real F1 HAL 1.1.10 headers (M3, -O0/-O3): PASS"
exit "$fail"
