/*
 * protocol/gbt_safety.c - the only module that changes the software output
 * command (charger). Every change gets a new seq and puts the output state in
 * PENDING, which holds the TX queue until the integration layer has applied
 * the command through the HAL sink and confirmed it (ARCHITECTURE.md §4.3).
 *
 *   DISABLE_OUTPUT: enabled 0, 0 mV, 0 mA, inhibit latched for the session.
 *   ZERO_CURRENT  : current 0, enabled and voltage unchanged, no latch (SAF-3).
 *   PROT-7 setpoint (§11.3.1, PT L415-429; magnitudes per D-06):
 *     CC mode: voltage limit = demand, capped at the charger maximum.
 *     CV mode: voltage = demand (capped at the charger maximum as a local limit).
 *     current: demand > max -> max; demand < min -> min; otherwise demand.
 *     Local clamps (SAF-2): BCP SPN2819 max voltage, BCP SPN2817 max current
 *     (P-K5-01 PROVISIONAL).
 *   OPEN: meaning of a zero-current demand (OQ-12(a)); here 0 A is commanded
 *   as 0 A (not raised to the minimum) - see docs/PROVISIONAL_PARAMETERS.md.
 */
#include "protocol/gbt_node_internal.h"
#include "protocol/gbt_codec.h"

#include <stddef.h>

static void new_command(gbt_node_t *n, uint8_t enabled, uint32_t mv, uint32_t ma)
{
    if (n->out_cmd.enabled == enabled && n->out_cmd.voltage_mv == mv && n->out_cmd.current_ma == ma
        && n->out_state != GBT_NODE_OUT_FAILED) {
        return;                                     /* unchanged: no new seq */
    }
    n->out_cmd.seq++;
    n->out_cmd.enabled = enabled;
    n->out_cmd.voltage_mv = mv;
    n->out_cmd.current_ma = ma;
    n->out_state = GBT_NODE_OUT_PENDING;            /* TX gate closed until confirmed */
}

void gbt_safety_disable(gbt_node_t *n, uint8_t reason)
{
    (void)reason;
    n->inhibit_latched = 1U;
    new_command(n, 0U, 0U, 0U);
}

void gbt_safety_zero_current(gbt_node_t *n)
{
    if (n->out_cmd.current_ma != 0U) {
        new_command(n, n->out_cmd.enabled, n->out_cmd.voltage_mv, 0U);
    }
}

uint8_t gbt_safety_voltage_below_min(const gbt_node_t *n, const gbt_bcl_t *bcl)
{
    return (gbt_voltage_0p1v_raw_to_mv(bcl->voltage_demand_raw) < n->cfg.charger->min_voltage_mv)
               ? 1U : 0U;
}

void gbt_safety_apply_bcl(gbt_node_t *n, const gbt_bcl_t *bcl)
{
    const gbt_charger_params_t *c = n->cfg.charger;
    uint32_t mv;
    uint32_t ma = 0U;
    uint32_t max_mv = c->max_voltage_mv;
    uint32_t max_ma = c->max_current_ma;

    if (n->inhibit_latched != 0U) {
        return;                                     /* latched for the session */
    }
    if (gbt_current_raw_to_magnitude_ma(bcl->current_demand_raw, &ma) != GBT_CODEC_OK) {
        return;                                     /* caller handles T-3a before */
    }
    if (n->s.bcp_valid != 0U) {                     /* SAF-2 local clamps */
        uint32_t bcp_ma = 0U;
        uint32_t bcp_mv = gbt_voltage_0p1v_raw_to_mv(n->s.bcp.max_voltage_raw);
        if (bcp_mv < max_mv) {
            max_mv = bcp_mv;
        }
        if (gbt_current_raw_to_magnitude_ma(n->s.bcp.max_current_raw, &bcp_ma) == GBT_CODEC_OK
            && bcp_ma < max_ma) {
            max_ma = bcp_ma;
        }
    }
    mv = gbt_voltage_0p1v_raw_to_mv(bcl->voltage_demand_raw);
    if (mv > max_mv) {
        mv = max_mv;                                /* §11.3.1 CC; local limit in CV */
    }
    if (ma > max_ma) {
        ma = max_ma;
    } else if (ma != 0U && ma < c->min_current_ma) {
        ma = c->min_current_ma;                     /* below minimum -> minimum capability */
    }
    new_command(n, 1U, mv, ma);
}
