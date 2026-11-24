/* protocol/gbt_timer.c - see gbt_timer.h */
#include "protocol/gbt_timer.h"

#include <stddef.h>

uint32_t gbt_time_elapsed(uint32_t now_ms, uint32_t start_ms)
{
    return (uint32_t)(now_ms - start_ms);   /* modular arithmetic, defined for unsigned */
}

void gbt_timer_arm(gbt_timer_t *t, uint32_t now_ms, uint32_t dur_ms)
{
    if (t == NULL) {
        return;
    }
    t->start_ms = now_ms;
    t->dur_ms = dur_ms;
    t->armed = 1U;
}

void gbt_timer_disarm(gbt_timer_t *t)
{
    if (t == NULL) {
        return;
    }
    t->armed = 0U;
}

uint8_t gbt_timer_is_armed(const gbt_timer_t *t)
{
    return (t != NULL && t->armed != 0U) ? 1U : 0U;
}

uint32_t gbt_timer_elapsed(const gbt_timer_t *t, uint32_t now_ms)
{
    if (gbt_timer_is_armed(t) == 0U) {
        return 0U;
    }
    return gbt_time_elapsed(now_ms, t->start_ms);
}

uint8_t gbt_timer_expired(const gbt_timer_t *t, uint32_t now_ms)
{
    if (gbt_timer_is_armed(t) == 0U) {
        return 0U;
    }
    return (gbt_time_elapsed(now_ms, t->start_ms) >= t->dur_ms) ? 1U : 0U;   /* P-S5-01 */
}
