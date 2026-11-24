/*
 * app/battery_model.h - Lean battery model (Lean Plan: linear SOC ramp with a
 * fixed voltage and current demand). Project simulation values, not protocol.
 */
#ifndef GBT_BATTERY_MODEL_H
#define GBT_BATTERY_MODEL_H

#include <stdint.h>
#include "protocol/gbt_node.h"

typedef struct {
    uint16_t soc_permille;
    uint16_t target_permille;
    uint16_t permille_per_s;
    uint32_t voltage_mv;
    uint32_t current_ma;
    uint8_t  mode;               /* SPN3074 */
    uint32_t acc_ms;
    uint32_t last_ms;
    uint8_t  started;
} battery_model_t;

void battery_model_init(battery_model_t *b, uint16_t soc_permille, uint16_t target_permille,
                        uint16_t permille_per_s, uint32_t voltage_mv, uint32_t current_ma, uint8_t mode);
/* Advance while charging; fills *d. Returns 1 once the target SOC is reached. */
uint8_t battery_model_step(battery_model_t *b, uint32_t now_ms, uint8_t charging, gbt_vehicle_demand_t *d);

#endif /* GBT_BATTERY_MODEL_H */
