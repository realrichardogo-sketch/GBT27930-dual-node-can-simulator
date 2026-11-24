/*
 * protocol/gbt_timer.h - wrap-safe millisecond timer primitives (M2, B1-08).
 *
 * Scope: elapsed arithmetic (uint32_t)(now - start), arm, disarm, armed query.
 * Expiry: gbt_timer_expired() implements S-5 (PROVISIONAL, register entry
 * P-S5-01): a timer is expired when elapsed >= duration. Timeout-table rows
 * (TO-xx durations and their start/stop events) belong to the state machines.
 *
 * Elapsed values are correct across the 2^32 ms wrap as long as the real
 * elapsed time is below 2^32 ms (~49.7 days).
 */
#ifndef GBT_TIMER_H
#define GBT_TIMER_H

#include <stdint.h>

typedef struct {
    uint32_t start_ms;
    uint32_t dur_ms;   /* stored for later timeout-table use; not evaluated here */
    uint8_t  armed;
} gbt_timer_t;

uint32_t gbt_time_elapsed(uint32_t now_ms, uint32_t start_ms);
void     gbt_timer_arm(gbt_timer_t *t, uint32_t now_ms, uint32_t dur_ms);
void     gbt_timer_disarm(gbt_timer_t *t);
uint8_t  gbt_timer_is_armed(const gbt_timer_t *t);
/* Elapsed time since arm; 0 when the timer is disarmed or t is NULL. */
uint32_t gbt_timer_elapsed(const gbt_timer_t *t, uint32_t now_ms);
/* 1 if armed and elapsed >= dur_ms (S-5, PROVISIONAL P-S5-01), else 0. */
uint8_t  gbt_timer_expired(const gbt_timer_t *t, uint32_t now_ms);

#endif /* GBT_TIMER_H */
