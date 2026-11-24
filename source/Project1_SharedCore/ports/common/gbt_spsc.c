/* ports/common/gbt_spsc.c - see gbt_spsc.h and docs/STEP07C1_SPSC_DESIGN.md. */
#include "ports/common/gbt_spsc.h"

#include <stddef.h>

/*
 * Optional hardware ordering hook between the slot accesses and the index
 * publication. Empty by default: on the supported single-core Cortex-M3/M4
 * targets without data cache, producer (ISR) and consumer (thread) run on the
 * same core, which observes its own memory accesses in program order, so
 * only compiler ordering is required, and that is provided by the volatile
 * accesses below. A port for a multi-core, cached or DMA-shared target must
 * define this (e.g. to a data memory barrier) on the compiler command line.
 */
#ifndef GBT_SPSC_HW_BARRIER
#define GBT_SPSC_HW_BARRIER() ((void)0)
#endif

#define GBT_SPSC_MASK (GBT_SPSC_CAPACITY - 1u)

gbt_spsc_status_t gbt_spsc_init(gbt_spsc_t *q)
{
    unsigned i;
    unsigned b;
    if (q == NULL) {
        return GBT_SPSC_E_PARAM;
    }
    for (i = 0U; i < GBT_SPSC_CAPACITY; i++) {
        q->slot[i].id = 0U;
        q->slot[i].dlc = 0U;
        q->slot[i].flags = 0U;
        for (b = 0U; b < 8U; b++) {
            q->slot[i].data[b] = 0U;
        }
    }
    q->dropped = 0U;
    q->tail = 0U;
    q->head = 0U;
    return GBT_SPSC_OK;
}

gbt_spsc_status_t gbt_spsc_push(gbt_spsc_t *q, const gbt_can_frame_t *f)
{
    uint32_t head;
    uint32_t tail;
    uint32_t d;
    unsigned b;
    volatile gbt_can_frame_t *s;

    if (q == NULL || f == NULL) {
        return GBT_SPSC_E_PARAM;
    }
    head = q->head;                         /* own index */
    tail = q->tail;                         /* consumer index: one 32-bit load */
    if ((uint32_t)(head - tail) >= GBT_SPSC_CAPACITY) {
        d = q->dropped;
        if (d != 0xFFFFFFFFu) {
            q->dropped = d + 1U;            /* producer is the only writer */
        }
        return GBT_SPSC_E_FULL;
    }
    s = &q->slot[head & GBT_SPSC_MASK];     /* free slot: consumer does not read it */
    s->id = f->id;
    s->dlc = f->dlc;
    s->flags = f->flags;
    for (b = 0U; b < 8U; b++) {
        s->data[b] = f->data[b];
    }
    GBT_SPSC_HW_BARRIER();
    q->head = head + 1U;                    /* publish: last access */
    return GBT_SPSC_OK;
}

gbt_spsc_status_t gbt_spsc_pop(gbt_spsc_t *q, gbt_can_frame_t *out)
{
    uint32_t head;
    uint32_t tail;
    unsigned b;
    const volatile gbt_can_frame_t *s;

    if (q == NULL || out == NULL) {
        return GBT_SPSC_E_PARAM;
    }
    tail = q->tail;                         /* own index */
    head = q->head;                         /* producer index: one 32-bit load */
    if (head == tail) {
        return GBT_SPSC_E_EMPTY;
    }
    s = &q->slot[tail & GBT_SPSC_MASK];     /* published slot: producer does not write it */
    out->id = s->id;
    out->dlc = s->dlc;
    out->flags = s->flags;
    for (b = 0U; b < 8U; b++) {
        out->data[b] = s->data[b];
    }
    GBT_SPSC_HW_BARRIER();
    q->tail = tail + 1U;                    /* release the slot: last access */
    return GBT_SPSC_OK;
}

uint32_t gbt_spsc_count(const gbt_spsc_t *q)
{
    uint32_t tail;
    uint32_t head;
    if (q == NULL) {
        return 0U;
    }
    tail = q->tail;
    head = q->head;
    return (uint32_t)(head - tail);
}

uint32_t gbt_spsc_dropped(const gbt_spsc_t *q)
{
    return (q == NULL) ? 0U : q->dropped;
}
