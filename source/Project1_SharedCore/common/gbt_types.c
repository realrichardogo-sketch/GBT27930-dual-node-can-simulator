/* common/gbt_types.c - definitions for common/gbt_types.h (C99). */
#include "common/gbt_types.h"

#include <stddef.h>

uint8_t gbt_can_frame_is_valid(const gbt_can_frame_t *f)
{
    if (f == NULL) {
        return 0U;
    }
    if (f->dlc > GBT_CAN_MAX_DLC || (f->flags & (uint8_t)~GBT_CAN_FLAG_MASK) != 0U) {
        return 0U;
    }
    if ((f->flags & GBT_CAN_FLAG_EXT) != 0U) {
        return (f->id <= GBT_CAN_MAX_EXT_ID) ? 1U : 0U;
    }
    return (f->id <= GBT_CAN_MAX_STD_ID) ? 1U : 0U;
}
