/* protocol/gbt_ring.c - see gbt_ring.h (adapted from OLD hal/ring_buffer.c, K-9b). */
#include "protocol/gbt_ring.h"

#include <string.h>

static int gbt_ring_is_ready(const gbt_ring_t *r)
{
    if (r == NULL) {
        return 0;
    }
    if (r->storage == NULL || r->capacity == 0U || r->element_size == 0U) {
        return 0;
    }
    if (r->head >= r->capacity || r->tail >= r->capacity || r->count > r->capacity) {
        return 0;
    }
    return 1;
}

gbt_ring_status_t gbt_ring_init(gbt_ring_t *r, void *storage, size_t capacity, size_t element_size)
{
    if (r == NULL || storage == NULL || capacity == 0U || element_size == 0U) {
        return GBT_RING_E_PARAM;
    }
    if (capacity > SIZE_MAX / element_size) {   /* offset calculation overflow guard */
        return GBT_RING_E_PARAM;
    }
    r->storage = (uint8_t *)storage;
    r->capacity = capacity;
    r->element_size = element_size;
    r->head = 0U;
    r->tail = 0U;
    r->count = 0U;
    return GBT_RING_OK;
}

gbt_ring_status_t gbt_ring_push(gbt_ring_t *r, const void *element)
{
    if (r == NULL || element == NULL) {
        return GBT_RING_E_PARAM;
    }
    if (!gbt_ring_is_ready(r)) {
        return GBT_RING_E_NOT_INIT;
    }
    if (r->count == r->capacity) {
        return GBT_RING_E_FULL;
    }
    memcpy(&r->storage[r->head * r->element_size], element, r->element_size);
    r->head++;
    if (r->head == r->capacity) {
        r->head = 0U;
    }
    r->count++;
    return GBT_RING_OK;
}

gbt_ring_status_t gbt_ring_peek(const gbt_ring_t *r, void *element)
{
    if (r == NULL || element == NULL) {
        return GBT_RING_E_PARAM;
    }
    if (!gbt_ring_is_ready(r)) {
        return GBT_RING_E_NOT_INIT;
    }
    if (r->count == 0U) {
        return GBT_RING_E_EMPTY;
    }
    memcpy(element, &r->storage[r->tail * r->element_size], r->element_size);
    return GBT_RING_OK;
}

gbt_ring_status_t gbt_ring_pop(gbt_ring_t *r, void *element)
{
    gbt_ring_status_t st = gbt_ring_peek(r, element);
    if (st != GBT_RING_OK) {
        return st;
    }
    r->tail++;
    if (r->tail == r->capacity) {
        r->tail = 0U;
    }
    r->count--;
    return GBT_RING_OK;
}

size_t gbt_ring_count(const gbt_ring_t *r)
{
    return gbt_ring_is_ready(r) ? r->count : 0U;
}

size_t gbt_ring_capacity(const gbt_ring_t *r)
{
    return gbt_ring_is_ready(r) ? r->capacity : 0U;
}
