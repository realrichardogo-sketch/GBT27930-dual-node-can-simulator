/* app/battery_model.c - see battery_model.h */
#include "app/battery_model.h"

#include <stddef.h>
#include <string.h>

void battery_model_init(battery_model_t *b, uint16_t soc_permille, uint16_t target_permille,
                        uint16_t permille_per_s, uint32_t voltage_mv, uint32_t current_ma, uint8_t mode)
{
    memset(b, 0, sizeof *b);
    b->soc_permille = soc_permille;
    b->target_permille = target_permille;
    b->permille_per_s = permille_per_s;
    b->voltage_mv = voltage_mv;
    b->current_ma = current_ma;
    b->mode = mode;
}

uint8_t battery_model_step(battery_model_t *b, uint32_t now_ms, uint8_t charging, gbt_vehicle_demand_t *d)
{
    if (b->started == 0U) {
        b->started = 1U;
        b->last_ms = now_ms;
    }
    if (charging != 0U) {
        b->acc_ms += (uint32_t)(now_ms - b->last_ms);
        while (b->acc_ms >= 1000U) {
            b->acc_ms -= 1000U;
            b->soc_permille = (uint16_t)((b->soc_permille + b->permille_per_s > 1000U)
                                             ? 1000U : b->soc_permille + b->permille_per_s);
        }
    }
    b->last_ms = now_ms;
    d->voltage_mv = b->voltage_mv;
    d->current_ma = b->current_ma;
    d->mode = b->mode;
    d->soc_permille = b->soc_permille;
    return (b->soc_permille >= b->target_permille) ? 1U : 0U;
}
