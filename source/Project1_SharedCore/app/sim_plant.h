/*
 * app/sim_plant.h - simulated output backend for SIL (implements app/output_if.h).
 * Accepts commands (or refuses them on request), records acceptance times for
 * the 10 ms software-command metric, and reports ideal feedback.
 */
#ifndef GBT_SIM_PLANT_H
#define GBT_SIM_PLANT_H

#include <stdint.h>
#include "app/output_if.h"

struct output_if_backend {
    const uint32_t  *clock_ms;         /* shared virtual clock */
    gbt_hal_status_t reply;            /* GBT_HAL_OK unless a test injects a refusal */
    gbt_output_cmd_t last;             /* last accepted command */
    uint32_t         apply_calls;
    uint32_t         accepted;
    uint32_t         last_accept_ms;
    uint32_t         first_disable_ms; /* first acceptance of a disabled command after arm */
    uint8_t          disable_seen;
    uint32_t         max_current_ma;   /* highest accepted current */
};

void sim_plant_init(output_if_backend_t *p, const uint32_t *clock_ms);
/* Re-arm the "first disabled acceptance" capture (start of a timing measurement). */
void sim_plant_arm_disable_capture(output_if_backend_t *p);

#endif /* GBT_SIM_PLANT_H */
