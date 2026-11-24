#!/bin/sh
# tools/check_architecture.sh <repo_root>
# Architecture dependency and purity rules (docs/ARCHITECTURE.md §3, §6).
set -u
R="${1:-.}"
cd "$R" || exit 2
fail=0
report() { echo "ARCH FAIL: $1"; fail=1; }

# 1. protocol/ and common/ include only C99 headers, common/ and protocol/.
bad=$(grep -Hn '#include' protocol/*.c protocol/*.h common/*.c common/*.h 2>/dev/null \
      | grep -v -E '#include (<(stdint|stddef|stdbool|string)\.h>|"(common|protocol)/[a-z0-9_]+\.h")')
[ -n "$bad" ] && report "protocol/common include outside the allowed set:
$bad"

# 2. hal/hal.h includes only <stdint.h> and common/gbt_types.h.
bad=$(grep -n '#include' hal/hal.h | grep -v -E '#include (<stdint\.h>|"common/gbt_types\.h")')
[ -n "$bad" ] && report "hal/hal.h include outside the allowed set: $bad"

# 3. ports/ and tests/support/ never include app/ or protocol/ (C sources and headers;
#    documentation examples are not code - same file filter as rule 4).
bad=$(grep -rn --include='*.c' --include='*.h' -E '#include "(app|protocol)/' ports tests/support 2>/dev/null)
[ -n "$bad" ] && report "ports/tests-support include app/protocol: $bad"

# 4. nothing outside app/ (and tests/) includes app/.
bad=$(grep -rn '#include "app/' --include='*.c' --include='*.h' protocol common hal ports 2>/dev/null)
[ -n "$bad" ] && report "app/ included from a lower layer: $bad"

# 5. forbidden constructs in protocol/ and common/.
bad=$(grep -Hn -E '\b(malloc|calloc|realloc|free|printf|fprintf|float|double|_Static_assert|_Generic|_Atomic|__attribute__|inline)\b|^[[:space:]]*#[[:space:]]*(if|ifdef|elif)\b' \
      protocol/*.c protocol/*.h common/*.c common/*.h)
[ -n "$bad" ] && report "forbidden construct in protocol/common:
$bad"

# 6. no platform names in protocol/common (comments excluded).
bad=$(grep -Hn -E '\b(HAL_OK|HAL_StatusTypeDef|FDCAN_HandleTypeDef|CAN_HandleTypeDef|__NVIC|socket|ioctl)\b' \
      protocol/*.c protocol/*.h common/*.c common/*.h | grep -v -E '^[^:]+:[0-9]+:[[:space:]]*(/\*|\*)')
[ -n "$bad" ] && report "platform name in protocol/common:
$bad"

[ "$fail" -eq 0 ] && echo "architecture rules: PASS"
exit "$fail"
