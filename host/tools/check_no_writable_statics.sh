#!/bin/sh
# tools/check_no_writable_statics.sh <nm> <object>...
# Protocol objects must contain no writable static data (.data/.bss/common,
# nm types d D b B c C), so every node's state lives in caller-allocated
# gbt_node_t. Fails if nm fails or no object is given (no vacuous PASS).
set -u
NM="$1"; shift
[ "$#" -gt 0 ] || { echo "no object files given"; exit 1; }
for o in "$@"; do
    [ -f "$o" ] || { echo "missing object: $o"; exit 1; }
done
syms=$("$NM" --defined-only "$@") || { echo "nm failed"; exit 1; }
bad=$(printf '%s\n' "$syms" | awk '$2 ~ /^[dDbBcC]$/ {print}')
if [ -n "$bad" ]; then
    echo "WRITABLE STATIC DATA FOUND:"; echo "$bad"; exit 1
fi
echo "no writable static data in $# protocol object(s): PASS"
