/*
 * protocol/gbt_outbox.h - protocol node TX outbox (single context).
 *
 * B1-06 / S-9 (PROVISIONAL): 16 usable entries. This is NOT the ISR/main SPSC
 * ring (ND-11, gated by G-SPSC in STEP 07-C).
 * The drain policy (ND-3) and TX gate (ND-5) are batch-2 items and are not
 * implemented here; this module only stores frames in FIFO order.
 *
 * A gbt_outbox_t must not be copied or moved after gbt_outbox_init(): the
 * ring refers to the outbox's own storage array.
 */
#ifndef GBT_OUTBOX_H
#define GBT_OUTBOX_H

#include <stddef.h>
#include "common/gbt_types.h"
#include "protocol/gbt_ring.h"

#define GBT_OUTBOX_LEN 16u   /* S-9, PROVISIONAL */

typedef struct {
    gbt_ring_t      ring;
    gbt_can_frame_t storage[GBT_OUTBOX_LEN];
} gbt_outbox_t;

gbt_ring_status_t gbt_outbox_init(gbt_outbox_t *ob);
gbt_ring_status_t gbt_outbox_push(gbt_outbox_t *ob, const gbt_can_frame_t *f);  /* E_FULL when full */
gbt_ring_status_t gbt_outbox_peek(const gbt_outbox_t *ob, gbt_can_frame_t *f);
gbt_ring_status_t gbt_outbox_pop(gbt_outbox_t *ob, gbt_can_frame_t *f);
size_t            gbt_outbox_count(const gbt_outbox_t *ob);

#endif /* GBT_OUTBOX_H */
