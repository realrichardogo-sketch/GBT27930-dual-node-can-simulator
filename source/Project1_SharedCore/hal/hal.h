/*
 * hal/hal.h - GBT HAL contract, API version 1 (C99).
 *
 * Source: STEP 07-A Revision 3.2 §4.3.1-4.3.3, approved as ND-1 / B1-05.
 * STEP 07-A.1 compatibility changes (no change to the API surface):
 *   (1) gbt_can_frame_t, gbt_output_cmd_t and gbt_output_fb_t are defined in
 *       common/gbt_types.h (identical fields) and are visible here through
 *       that include, so the protocol core can use them without hal.h.
 *   (2) The caller of claim/release and of the runtime functions is the
 *       application/integration layer. Revision 3.2 named gbt_node_init /
 *       gbt_node_deinit as the callers of claim/release; under the STEP 07-A.1
 *       rule the protocol core never calls the HAL, so the integration layer
 *       claims the instance when it attaches a protocol node and releases it
 *       when it detaches the node. (Responsibility shift pending Owner review.)
 *
 * Lifecycle (Revision 3.2 §4.3.3):
 *   UNINIT --gbt_port_<p>_init--> READY --bind--> BOUND --claim--> CLAIMED
 *   CLAIMED --release--> BOUND --unbind--> READY --gbt_port_<p>_deinit--> UNINIT
 *   Any other transition returns GBT_HAL_E_STATE (GBT_HAL_E_NOT_READY when the
 *   instance is not initialised, or for claim when no sink is bound).
 *
 * Rules:
 *   - Exactly one concrete HAL implementation is linked per executable; each
 *     port completes struct gbt_hal in its own header. A test executable may
 *     hold several instances of the same (fake) implementation.
 *   - Only the application/integration layer calls gbt_hal_* functions; the
 *     protocol core (protocol/) never includes this header.
 *   - All gbt_hal_* functions: main context only, non-blocking, bounded.
 *   - Sink callbacks run only inside gbt_hal_output_apply/_feedback, in the
 *     main context, never from an ISR. They must not call any gbt_hal_* or
 *     gbt_node_* function, must not block, and must return in bounded time.
 *   - sink.ctx and the backend behind it must stay valid from bind until
 *     unbind returns.
 *   - Naming: gbt_hal_ / GBT_HAL_ prefixes avoid STM32Cube HAL_OK etc.
 */
#ifndef GBT_HAL_H
#define GBT_HAL_H

#include <stdint.h>
#include "common/gbt_types.h"   /* gbt_can_frame_t, gbt_output_cmd_t, gbt_output_fb_t */

#define GBT_HAL_API_VERSION 1u

/* Incomplete here; completed by exactly one port header per executable. */
typedef struct gbt_hal gbt_hal_t;

typedef enum {
    GBT_HAL_OK = 0,
    GBT_HAL_E_PARAM,     /* NULL pointer or invalid argument (invalid frame) */
    GBT_HAL_E_FULL,      /* TX: controller TX resources full; frame NOT accepted; retry later */
    GBT_HAL_E_EMPTY,     /* RX: no frame available */
    GBT_HAL_E_BUS_OFF,   /* controller bus-off; frame NOT accepted */
    GBT_HAL_E_NOT_READY, /* instance not initialised, or no sink bound */
    GBT_HAL_E_STATE,     /* call not permitted in the current lifecycle state */
    GBT_HAL_E_IO         /* any other failure; a sink also returns this to refuse a command */
} gbt_hal_status_t;

#define GBT_CAN_STATE_UNKNOWN        0u
#define GBT_CAN_STATE_ERROR_ACTIVE   1u
#define GBT_CAN_STATE_ERROR_WARNING  2u
#define GBT_CAN_STATE_ERROR_PASSIVE  3u
#define GBT_CAN_STATE_BUS_OFF        4u

typedef struct {
    uint8_t  state;              /* GBT_CAN_STATE_* */
    uint8_t  tec;
    uint8_t  rec;
    uint8_t  tec_rec_valid;      /* 0 if the port cannot read TEC/REC */
    uint32_t rx_overflow_sw;     /* saturating at 0xFFFFFFFF */
    uint32_t rx_overflow_hw;     /* saturating */
    uint32_t rx_invalid;         /* saturating */
    uint32_t tx_error;           /* saturating */
} gbt_can_status_t;

/* Generic output sink: the ONLY path from a HAL instance to the physical output. */
typedef struct {
    gbt_hal_status_t (*apply)(void *ctx, const gbt_output_cmd_t *cmd);
    gbt_hal_status_t (*feedback)(void *ctx, gbt_output_fb_t *fb);
    void *ctx;
} gbt_output_sink_t;

/* ---- Lifecycle ---- */
/* READY -> BOUND. The struct is copied. E_PARAM if apply or feedback is NULL. */
gbt_hal_status_t gbt_hal_bind_output_sink(gbt_hal_t *h, const gbt_output_sink_t *sink);
/* BOUND -> READY. E_STATE while CLAIMED. */
gbt_hal_status_t gbt_hal_unbind_output_sink(gbt_hal_t *h);
/* BOUND -> CLAIMED; called by the integration layer when it attaches a node. */
gbt_hal_status_t gbt_hal_claim(gbt_hal_t *h);
/* CLAIMED -> BOUND; called by the integration layer when it detaches the node. */
gbt_hal_status_t gbt_hal_release(gbt_hal_t *h);

/* ---- Runtime: main context only, non-blocking, bounded ---- */

/*
 * TX: hand one frame to the controller.
 *   GBT_HAL_OK      accepted (copied); order preserved among accepted frames
 *   GBT_HAL_E_FULL  NOT accepted; caller keeps the frame and retries it first
 *   other errors    NOT accepted (see the TX ownership contract, ARCHITECTURE.md §5)
 */
gbt_hal_status_t gbt_hal_can_send(gbt_hal_t *h, const gbt_can_frame_t *f);
/* RX: oldest frame from the port's bounded RX queue; GBT_HAL_E_EMPTY if none. */
gbt_hal_status_t gbt_hal_can_receive(gbt_hal_t *h, gbt_can_frame_t *out);
/* Monotonic millisecond clock; wraps at 2^32; 0 if uninitialised. */
uint32_t         gbt_hal_now_ms(gbt_hal_t *h);
/*
 * Deliver a software output command to the bound sink.
 *   E_PARAM if an argument is NULL; E_STATE if the instance is not CLAIMED;
 *   E_NOT_READY if no sink is bound; otherwise exactly the status returned by
 *   sink.apply. On GBT_HAL_OK only, the port records the command's seq as
 *   accepted and updates its timing marker (if it has one).
 */
gbt_hal_status_t gbt_hal_output_apply(gbt_hal_t *h, const gbt_output_cmd_t *cmd);
gbt_hal_status_t gbt_hal_output_feedback(gbt_hal_t *h, gbt_output_fb_t *fb);
gbt_hal_status_t gbt_hal_can_get_status(gbt_hal_t *h, gbt_can_status_t *s);
void             gbt_hal_event(gbt_hal_t *h, uint16_t code, int32_t a, int32_t b);

#endif /* GBT_HAL_H */
