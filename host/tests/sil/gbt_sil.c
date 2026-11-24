/* tests/sil/gbt_sil.c - see gbt_sil.h */
#include "tests/sil/gbt_sil.h"

#include <stddef.h>

gbt_hal_status_t gbt_sil_init(gbt_sil_t *sil, uint32_t start_ms, gbt_sil_step_fn step, void *ctx)
{
    gbt_hal_status_t st;

    if (sil == NULL || step == NULL) {
        return GBT_HAL_E_PARAM;
    }
    sil->now_ms = start_ms;
    sil->step = step;
    sil->step_ctx = ctx;
    sil->order[0] = 0U;
    sil->order[1] = 1U;
    sil->ticks = 0U;
    st = gbt_fake_hal_init(&sil->hal[0], &sil->bus, 0U, &sil->now_ms);
    if (st != GBT_HAL_OK) {
        return st;
    }
    return gbt_fake_hal_init(&sil->hal[1], &sil->bus, 1U, &sil->now_ms);
}

gbt_hal_status_t gbt_sil_deinit(gbt_sil_t *sil)
{
    gbt_hal_status_t a;
    gbt_hal_status_t b;
    if (sil == NULL) {
        return GBT_HAL_E_PARAM;
    }
    a = gbt_fake_hal_deinit(&sil->hal[0]);
    b = gbt_fake_hal_deinit(&sil->hal[1]);
    return (a != GBT_HAL_OK) ? a : b;
}

gbt_hal_status_t gbt_sil_set_order(gbt_sil_t *sil, uint8_t first)
{
    if (sil == NULL || first > 1U) {
        return GBT_HAL_E_PARAM;
    }
    sil->order[0] = first;
    sil->order[1] = (uint8_t)(first ^ 1U);
    return GBT_HAL_OK;
}

void gbt_sil_tick(gbt_sil_t *sil)
{
    if (sil == NULL || sil->step == NULL) {
        return;
    }
    sil->now_ms = (uint32_t)(sil->now_ms + 1U);
    sil->ticks++;
    sil->step(sil->step_ctx, sil->order[0], sil->now_ms);
    sil->step(sil->step_ctx, sil->order[1], sil->now_ms);
}

void gbt_sil_run(gbt_sil_t *sil, uint32_t ticks)
{
    uint32_t i;
    for (i = 0U; i < ticks; i++) {
        gbt_sil_tick(sil);
    }
}
