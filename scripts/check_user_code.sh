#!/bin/bash
# scripts/check_user_code.sh <original_main.c> <modified_main.c> <label>
# Every added/changed line of the modified CubeMX file must lie inside a USER CODE BEGIN/END
# block, no original line outside a USER CODE block may be removed, the marker sequence must be
# identical, and the line-ending style must be unchanged.
set -u
O="$1"; N="$2"; L="$3"; T="${TMPDIR:-/tmp}/ucc.$$"
rc=0
echo "== $L: BEGIN=$(grep -c 'USER CODE BEGIN' "$N") END=$(grep -c 'USER CODE END' "$N") (original BEGIN=$(grep -c 'USER CODE BEGIN' "$O") END=$(grep -c 'USER CODE END' "$O"))"
diff --unchanged-line-format= --old-line-format= --new-line-format='%dn
' "$O" "$N" > "$T.add"
diff --unchanged-line-format= --old-line-format='%dn
' --new-line-format= "$O" "$N" > "$T.del"
out=$(awk 'NR==FNR{c[$1]=1;next} /USER CODE BEGIN/{b=1} {if(c[FNR] && !b) print "  ADDED OUTSIDE USER CODE line " FNR ": " $0} /USER CODE END/{b=0}' "$T.add" "$N")
[ -n "$out" ] && { echo "$out"; rc=1; }
out=$(awk 'NR==FNR{c[$1]=1;next} /USER CODE BEGIN/{b=1} {if(c[FNR] && !b) print "  REMOVED OUTSIDE USER CODE line " FNR ": " $0} /USER CODE END/{b=0}' "$T.del" "$O")
[ -n "$out" ] && { echo "$out"; rc=1; }
echo "  lines added/changed: $(wc -l < "$T.add"); original lines removed: $(wc -l < "$T.del")"
if cmp -s <(grep -o 'USER CODE [A-Z]* [A-Za-z0-9_ ]*' "$O") <(grep -o 'USER CODE [A-Z]* [A-Za-z0-9_ ]*' "$N"); then
    echo "  marker sequence identical"; else echo "  MARKER SEQUENCE CHANGED"; rc=1; fi
oc=$(grep -c "$(printf '\r')\$" "$O"); ol=$(grep -vc "$(printf '\r')\$" "$O")
nc=$(grep -c "$(printf '\r')\$" "$N"); nl=$(grep -vc "$(printf '\r')\$" "$N")
echo "  line endings: original CRLF=$oc LF-only=$ol; modified CRLF=$nc LF-only=$nl"
if [ "$oc" -gt 0 ] && [ "$nl" -ne 0 ]; then echo "  MIXED LINE ENDINGS"; rc=1; fi
[ "$rc" -eq 0 ] && echo "  RESULT: PASS" || echo "  RESULT: FAIL"
rm -f "$T.add" "$T.del"
exit "$rc"
