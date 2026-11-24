/*
 * common/gbt_types.h - platform-independent shared types (C99).
 *
 * Included by protocol/, hal/, ports/, app/ and tests/. Includes only C99
 * standard headers. Contains no platform, OS, HAL, CMSIS or SocketCAN types.
 * Status: STEP 07-A.1 interface proposal (pending Owner architecture review).
 */
#ifndef GBT_TYPES_H
#define GBT_TYPES_H

#include <stdint.h>

/* ---- Classical CAN frame ------------------------------------------------ */

#define GBT_CAN_FLAG_STD    0x00u        /* 11-bit identifier (no flag bits set) */
#define GBT_CAN_FLAG_EXT    0x01u        /* 29-bit identifier (CAN 2.0B extended) */
#define GBT_CAN_FLAG_MASK   0x01u        /* all defined flag bits; no RTR, no FD */

#define GBT_CAN_MAX_DLC     8u           /* Classical CAN */
#define GBT_CAN_MAX_STD_ID  0x7FFu
#define GBT_CAN_MAX_EXT_ID  0x1FFFFFFFu

typedef struct {
    uint32_t id;                         /* <= 0x1FFFFFFF if EXT, <= 0x7FF otherwise */
    uint8_t  dlc;                        /* 0..8; number of valid bytes in data[] */
    uint8_t  flags;                      /* GBT_CAN_FLAG_* */
    uint8_t  data[GBT_CAN_MAX_DLC];      /* bytes at index >= dlc are not transmitted */
} gbt_can_frame_t;

/* Returns 1 if f is non-NULL and satisfies the Classical CAN constraints
 * above, else 0. Ordinary C99 function (defined in common/gbt_types.c). */
uint8_t gbt_can_frame_is_valid(const gbt_can_frame_t *f);

/* ---- Software output command (charger) ----------------------------------- */
/*
 * Produced by the protocol (gbt_node_get_output) and delivered by the
 * integration layer through the ND-1 HAL output sink (gbt_hal_output_apply).
 * Field set identical to Revision 3.2 §4.3.1; defined here so the protocol can
 * use it without including hal/hal.h. It is a SOFTWARE command; physical power
 * output belongs to the output backend (Project 2). Magnitudes are
 * non-negative (D-02).
 */
typedef struct {
    uint32_t seq;                        /* incremented by protocol on every change */
    uint8_t  enabled;                    /* 0 = disabled (safe) */
    uint32_t voltage_mv;
    uint32_t current_ma;
} gbt_output_cmd_t;

/* Measured output feedback supplied by the application (PROT-4, SAF-5). */
typedef struct {
    uint8_t  valid;
    uint32_t voltage_mv;
    uint32_t current_ma;
    uint32_t t_ms;                       /* same millisecond time base as now_ms */
} gbt_output_fb_t;

/* ---- Generic result codes shared by app/integration helpers -------------- */
typedef enum {
    GBT_OK = 0,
    GBT_E_PARAM,                         /* NULL pointer or invalid argument */
    GBT_E_STATE,                         /* not permitted in the current state */
    GBT_E_FULL,                          /* bounded queue full */
    GBT_E_EMPTY,                         /* nothing available */
    GBT_E_RANGE                          /* value outside the defined range */
} gbt_result_t;

#endif /* GBT_TYPES_H */
