/* protocol/gbt_outbox.c - see gbt_outbox.h */
#include "protocol/gbt_outbox.h"

gbt_ring_status_t gbt_outbox_init(gbt_outbox_t *ob)
{
    if (ob == NULL) {
        return GBT_RING_E_PARAM;
    }
    return gbt_ring_init(&ob->ring, ob->storage, GBT_OUTBOX_LEN, sizeof(gbt_can_frame_t));
}

gbt_ring_status_t gbt_outbox_push(gbt_outbox_t *ob, const gbt_can_frame_t *f)
{
    if (ob == NULL) {
        return GBT_RING_E_PARAM;
    }
    return gbt_ring_push(&ob->ring, f);
}

gbt_ring_status_t gbt_outbox_peek(const gbt_outbox_t *ob, gbt_can_frame_t *f)
{
    if (ob == NULL) {
        return GBT_RING_E_PARAM;
    }
    return gbt_ring_peek(&ob->ring, f);
}

gbt_ring_status_t gbt_outbox_pop(gbt_outbox_t *ob, gbt_can_frame_t *f)
{
    if (ob == NULL) {
        return GBT_RING_E_PARAM;
    }
    return gbt_ring_pop(&ob->ring, f);
}

size_t gbt_outbox_count(const gbt_outbox_t *ob)
{
    return (ob == NULL) ? 0U : gbt_ring_count(&ob->ring);
}
