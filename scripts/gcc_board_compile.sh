#!/bin/sh
# scripts/gcc_board_compile.sh <source_root> <out_dir>
# Board source regression (GCC, compile-only; NOT an ARMCC build): compiles EVERY .c file listed
# in each Keil .uvprojx (application, SharedCore and STM32 HAL drivers) with arm-none-eabi-gcc
# against the project's own include paths and defines, at the Keil optimisation level of the
# board (G474 -O0, F103 -O3).
#   application + SharedCore: -std=c99 -Wall -Wextra -Werror (SharedCore also -Wconversion)
#   HAL / CMSIS driver files: compiled, warnings logged but not fatal (vendor code, unchanged)
# Also lists the g_hw_diag_* objects of main.o. Exit 0 only if every file compiled.
set -u
SRC="$1"; OUT="$2"; mkdir -p "$OUT"; fail=0
board() { # name cpu defs opt uvprojx
    name="$1"; cpu="$2"; defs="$3"; opt="$4"; proj="$5"; pd=$(dirname "$proj"); d="$OUT/$name"; mkdir -p "$d"
    inc=""
    # first NON-EMPTY <IncludePath> = C/C++ include paths (an empty element precedes it)
    for i in $(grep -o '<IncludePath>[^<][^<]*</IncludePath>' "$proj" | head -1 | sed 's/<[^>]*>//g' | tr ';' ' ' | tr '\\' '/'); do
        case "$i" in *SharedCore*) inc="$inc -I$pd/$i";; *) inc="$inc -isystem $pd/$i";; esac
    done
    ok=0; bad=0; hw=0
    for s in $(grep -o '<FilePath>[^<]*\.c</FilePath>' "$proj" | sed 's/<[^>]*>//g' | tr '\\' '/'); do
        o="$d/$(basename "$s" .c).o"
        case "$s" in
            *Drivers/*) fl="-std=c99 -Wall";;
            *SharedCore*) fl="-std=c99 -Wall -Wextra -Wconversion -Werror";;
            *) fl="-std=c99 -Wall -Wextra -Werror";;
        esac
        # shellcheck disable=SC2086
        if arm-none-eabi-gcc $cpu $defs $opt $fl -ffunction-sections -fdata-sections $inc -c "$pd/$s" -o "$o" 2> "$o.log"; then
            ok=$((ok + 1))
            case "$s" in *Drivers/*) [ -s "$o.log" ] && hw=$((hw + 1));; esac
        else
            bad=$((bad + 1)); fail=1; echo "FAIL $name $s"; cat "$o.log"
        fi
    done
    echo "$name: $ok compiled, $bad failed, $hw HAL/CMSIS files with (non-fatal) warnings; flags $opt"
    if [ -f "$d/main.o" ]; then
        echo "$name: main.o g_hw_diag_* objects: $(arm-none-eabi-nm "$d/main.o" | grep -c ' g_hw_diag_')"
        arm-none-eabi-nm -S --size-sort "$d/main.o" | grep ' g_hw_diag_' > "$d/g_hw_diag_symbols.txt"
    fi
}
board g474 "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard" "-DUSE_NUCLEO_64 -DUSE_HAL_DRIVER -DSTM32G474xx" "-O0" \
      "$SRC/Project1_G474/MDK-ARM/Project1_G474.uvprojx"
board f103 "-mcpu=cortex-m3 -mthumb" "-DUSE_HAL_DRIVER -DSTM32F103xE" "-O3" \
      "$SRC/Project1_F103B_Receiver_CubeMX/MDK-ARM/Project1_F103.uvprojx"
[ "$fail" -eq 0 ] && echo "BOARD SOURCE COMPILE (GCC, not ARMCC): PASS" || echo "BOARD SOURCE COMPILE (GCC, not ARMCC): FAIL"
exit "$fail"
