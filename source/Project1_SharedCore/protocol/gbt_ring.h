/*
 * protocol/gbt_ring.h - fixed-capacity FIFO, caller-owned storage (C99).
 *
 * Adapted from OLD hal/ring_buffer.[ch] (K-9b, approved as B1-04):
 *   ring_buffer.c SHA-256 9b2bbafdd7f8a3dc15c3b94df3ee196e9afe7e67a3f971e906ebcc334e0acd41
 *   ring_buffer.h SHA-256 ca18ed64be69bf5efc21a0f85963d0f0ee3915bf8546e9fc7949f3e77a526321
 * Changes: gbt_ prefix, include path, added gbt_ring_peek().
 *
 * SINGLE-CONTEXT ONLY. push and pop both modify `count`, so this ring must
 * never be shared between an ISR and the main loop. The ISR/main SPSC ring is
 * a separate component gated by G-SPSC (STEP 07-C) and is not implemented here.
 */
#ifndef GBT_RING_H
#define GBT_RING_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    GBT_RING_OK = 0,
    GBT_RING_E_PARAM,
    GBT_RING_E_NOT_INIT,
    GBT_RING_E_FULL,
    GBT_RING_E_EMPTY
} gbt_ring_status_t;

typedef struct {
    uint8_t *storage;
    size_t   capacity;
    size_t   element_size;
    size_t   head;
    size_t   tail;
    size_t   count;
} gbt_ring_t;

gbt_ring_status_t gbt_ring_init(gbt_ring_t *r, void *storage, size_t capacity, size_t element_size);
gbt_ring_status_t gbt_ring_push(gbt_ring_t *r, const void *element);   /* never overwrites */
gbt_ring_status_t gbt_ring_pop(gbt_ring_t *r, void *element);
gbt_ring_status_t gbt_ring_peek(const gbt_ring_t *r, void *element);   /* copy head, keep it */
size_t            gbt_ring_count(const gbt_ring_t *r);
size_t            gbt_ring_capacity(const gbt_ring_t *r);

#endif /* GBT_RING_H */
