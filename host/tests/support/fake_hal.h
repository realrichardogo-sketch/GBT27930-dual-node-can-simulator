/*
 * tests/support/fake_hal.h - Fake HAL implementing hal/hal.h (ND-1). Host only.
 *
 * Completes gbt_hal_t for executables that link the fake HAL. Several
 * instances per executable are allowed (same type). Single-threaded: no ISR.
 * Two instances attached to one gbt_sim_bus_t exchange frames.
 * Must not include app/ or protocol/ headers. Instance storage must be
 * zero-initialised before gbt_fake_hal_init().
 *
 * Fake-specific fields (test infrastructure): in_sink (detects forbidden
 * re-entry from a sink callback), accept_time_ms, event_count, last_event_code.
 */
#ifndef GBT_FAKE_HAL_H
#define GBT_FAKE_HAL_H

#include <stdint.h>
#include "hal/hal.h"
#include "ports/common/gbt_hal_lc.h"

#define GBT_FAKE_RXQ_LEN 16u

typedef struct gbt_sim_bus gbt_sim_bus_t;

struct gbt_hal {
    uint8_t           lifecycle;          /* GBT_HAL_LC_* */
    gbt_sim_bus_t    *bus;
    uint8_t           node_index;         /* 0 or 1 */
    const uint32_t   *virtual_now_ms;     /* shared virtual clock */
    gbt_can_frame_t   rxq[GBT_FAKE_RXQ_LEN];
    uint32_t          rx_head;
    uint32_t          rx_tail;
    uint32_t          rx_count;
    uint32_t          accepted_seq;
    uint32_t          accept_time_ms;     /* virtual time of the last acceptance */
    uint8_t           inject_tx_status;   /* gbt_hal_status_t as uint8_t; GBT_HAL_OK unless injected */
    uint8_t           in_sink;
    uint32_t          event_count;
    uint16_t          last_event_code;
    gbt_can_status_t  status;
    gbt_output_sink_t sink;
};

/* Optional delivery filter (fault injection): return 0 to drop the frame. */
typedef uint8_t (*gbt_sim_filter_t)(void *ctx, uint8_t from_node, const gbt_can_frame_t *f);

struct gbt_sim_bus {
    gbt_hal_t       *node[2];
    uint32_t         frames_delivered;
    uint32_t         frames_dropped;
    gbt_sim_filter_t filter;
    void            *filter_ctx;
};

gbt_hal_status_t gbt_fake_hal_init(gbt_hal_t *h, gbt_sim_bus_t *bus, uint8_t node_index,
                                   const uint32_t *virtual_now_ms);
gbt_hal_status_t gbt_fake_hal_deinit(gbt_hal_t *h);
gbt_hal_status_t gbt_fake_hal_inject_rx(gbt_hal_t *h, const gbt_can_frame_t *f);
gbt_hal_status_t gbt_fake_hal_inject_overflow(gbt_hal_t *h, uint8_t hardware);
gbt_hal_status_t gbt_fake_hal_set_tx_status(gbt_hal_t *h, gbt_hal_status_t st);

#endif /* GBT_FAKE_HAL_H */
