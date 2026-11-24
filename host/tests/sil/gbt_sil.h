/*
 * tests/sil/gbt_sil.h - dual-context SIL infrastructure (M12, B1-15).
 *
 * Infrastructure only: two gbt_node_t storage instances, two independent
 * fake-HAL instances, one simulated bus, one shared 1 ms virtual clock and a
 * deterministic, swappable per-tick execution order.
 *
 * NOT included (batch 2 / gated): protocol scenarios, full-session execution,
 * state machines, safety behaviour, transport protocol. gbt_node_step() is not
 * called; each tick invokes a test-supplied step callback per node instead.
 */
#ifndef GBT_SIL_H
#define GBT_SIL_H

#include <stdint.h>
#include "protocol/gbt_node.h"
#include "tests/support/fake_hal.h"

typedef void (*gbt_sil_step_fn)(void *ctx, uint8_t node_index, uint32_t now_ms);

typedef struct {
    uint32_t        now_ms;     /* shared virtual clock, wraps at 2^32 */
    gbt_sim_bus_t   bus;
    gbt_hal_t       hal[2];
    gbt_node_t      node[2];    /* storage only in Batch 1 */
    uint8_t         order[2];   /* node indices in per-tick execution order */
    gbt_sil_step_fn step;
    void           *step_ctx;
    uint32_t        ticks;
} gbt_sil_t;

/* *sil must be zero-initialised storage (static or = {0}). */
gbt_hal_status_t gbt_sil_init(gbt_sil_t *sil, uint32_t start_ms, gbt_sil_step_fn step, void *ctx);
gbt_hal_status_t gbt_sil_deinit(gbt_sil_t *sil);
/* first = node index executed first in each tick (0 or 1). */
gbt_hal_status_t gbt_sil_set_order(gbt_sil_t *sil, uint8_t first);
/* Advance the clock by exactly 1 ms, then run both nodes in the set order. */
void             gbt_sil_tick(gbt_sil_t *sil);
void             gbt_sil_run(gbt_sil_t *sil, uint32_t ticks);

#endif /* GBT_SIL_H */
