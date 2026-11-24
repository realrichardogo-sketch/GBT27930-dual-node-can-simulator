/*
 * ports/common/gbt_hal_lc.h - HAL instance lifecycle values (ND-1; Revision 3.2 §4.3.2-4.3.3).
 * Shared by HAL implementations; must not include app/ or protocol/ headers.
 */
#ifndef GBT_HAL_LC_H
#define GBT_HAL_LC_H

#define GBT_HAL_LC_UNINIT   0u
#define GBT_HAL_LC_READY    1u
#define GBT_HAL_LC_BOUND    2u
#define GBT_HAL_LC_CLAIMED  3u

#endif /* GBT_HAL_LC_H */
