/*
 * ports/common/gbt_spsc.h - ISR-safe single-producer / single-consumer CAN
 * frame queue (G-SPSC, STEP 07-C1.1). Pure C99; no HAL, protocol, RTOS or
 * compiler-specific dependency. Design: docs/STEP07C1_SPSC_DESIGN.md.
 *
 * Roles (exactly one of each per queue):
 *   producer : CAN RX interrupt  - gbt_spsc_push() only
 *   consumer : main loop         - gbt_spsc_pop(), gbt_spsc_count() only
 *   any      : gbt_spsc_dropped() (read-only)
 *
 * Capacity: GBT_SPSC_CAPACITY (16) frames, ALL 16 slots usable. head and tail
 * are free-running 32-bit counters; fill level = head - tail (0..16, modulo
 * 2^32); slot index = counter & (CAPACITY - 1).
 *
 * Ownership: head and dropped are written only by the producer; tail is
 * written only by the consumer; a slot is written only by the producer while
 * it is free and read only by the consumer while it is published.
 *
 * Ordering: every access to shared state (indices, counter and slot fields)
 * goes through a volatile-qualified lvalue, so the compiler keeps these
 * accesses in program order (C99 5.1.2.3). The hardware side is covered by
 * the platform assumption documented in the design note (single core, no
 * data cache; same-core program-order observation), with an optional port
 * hook GBT_SPSC_HW_BARRIER() that is empty unless the build defines it.
 *
 * Preconditions: gbt_spsc_init() runs before the producer interrupt is
 * enabled and never while it is enabled (there is no reset function). Index
 * words must be naturally aligned 32-bit objects (guaranteed for this struct
 * by the C ABI of the supported targets), so each index load/store is a
 * single access.
 */
#ifndef GBT_SPSC_H
#define GBT_SPSC_H

#include <stdint.h>
#include "common/gbt_types.h"   /* gbt_can_frame_t */

#define GBT_SPSC_CAPACITY 16u    /* power of two; all slots usable */

typedef enum {
    GBT_SPSC_OK = 0,
    GBT_SPSC_E_PARAM,
    GBT_SPSC_E_FULL,             /* push: frame NOT stored, dropped counted */
    GBT_SPSC_E_EMPTY             /* pop: no frame */
} gbt_spsc_status_t;

typedef struct {
    volatile gbt_can_frame_t slot[GBT_SPSC_CAPACITY];
    volatile uint32_t        head;     /* producer-owned: frames published */
    volatile uint32_t        tail;     /* consumer-owned: frames consumed */
    volatile uint32_t        dropped;  /* producer-owned: frames rejected (full), saturating */
} gbt_spsc_t;

/* Initialise (empty, dropped 0). Interrupt producing into q must be disabled. */
gbt_spsc_status_t gbt_spsc_init(gbt_spsc_t *q);
/* Producer only. Copies the frame (id, dlc, flags, all 8 data bytes). */
gbt_spsc_status_t gbt_spsc_push(gbt_spsc_t *q, const gbt_can_frame_t *f);
/* Consumer only. Oldest frame into *out. */
gbt_spsc_status_t gbt_spsc_pop(gbt_spsc_t *q, gbt_can_frame_t *out);
/* Consumer only. Frames waiting (0..16); may grow concurrently, never shrink. */
uint32_t          gbt_spsc_count(const gbt_spsc_t *q);
/* Any context. Frames dropped because the queue was full (saturating). */
uint32_t          gbt_spsc_dropped(const gbt_spsc_t *q);

#endif /* GBT_SPSC_H */
