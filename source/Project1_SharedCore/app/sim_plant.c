/* app/sim_plant.c - see sim_plant.h */
#include "app/sim_plant.h"

#include <stddef.h>
#include <string.h>

static gbt_hal_status_t plant_apply(void *ctx, const gbt_output_cmd_t *cmd)
{
    output_if_backend_t *p = (output_if_backend_t *)ctx;
    p->apply_calls++;
    if (p->reply != GBT_HAL_OK) {
        return p->reply;
    }
    p->last = *cmd;
    p->accepted++;
    p->last_accept_ms = *p->clock_ms;
    if (cmd->current_ma > p->max_current_ma) {
        p->max_current_ma = cmd->current_ma;
    }
    if (cmd->enabled == 0U && p->disable_seen == 0U) {
        p->disable_seen = 1U;
        p->first_disable_ms = *p->clock_ms;
    }
    return GBT_HAL_OK;
}

static gbt_hal_status_t plant_feedback(void *ctx, gbt_output_fb_t *fb)
{
    output_if_backend_t *p = (output_if_backend_t *)ctx;
    fb->valid = 1U;
    fb->voltage_mv = (p->last.enabled != 0U) ? p->last.voltage_mv : 0U;
    fb->current_ma = (p->last.enabled != 0U) ? p->last.current_ma : 0U;
    fb->t_ms = *p->clock_ms;
    return GBT_HAL_OK;
}

void sim_plant_init(output_if_backend_t *p, const uint32_t *clock_ms)
{
    memset(p, 0, sizeof *p);
    p->clock_ms = clock_ms;
    p->reply = GBT_HAL_OK;
}

void sim_plant_arm_disable_capture(output_if_backend_t *p)
{
    p->disable_seen = 0U;
    p->first_disable_ms = 0U;
}

gbt_hal_status_t output_if_make_sink(output_if_backend_t *backend, gbt_output_sink_t *sink)
{
    if (backend == NULL || sink == NULL) {
        return GBT_HAL_E_PARAM;
    }
    sink->apply = plant_apply;
    sink->feedback = plant_feedback;
    sink->ctx = backend;
    return GBT_HAL_OK;
}
