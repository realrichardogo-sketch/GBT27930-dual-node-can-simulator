/*
 * protocol/gbt_vehicle_sm.c - vehicle (EVCC) state machine, Lean subset.
 * Transitions TVxx: docs/sources/01.2_STATE_MACHINES.md §2.2.
 * Lean cuts (K-1): no CRM-without-CHM path (TV06), no parameter-mismatch stop
 * (TV14), no reconnection (V-RC/V-CE), no BMV/BMT/BSP, no time sync.
 * CCS supervision (K-2, PROVISIONAL): one timer, armed at CRO AA (TO-13) and
 * re-armed on each valid CCS (TO-17); TV41 is evaluated before TV24.
 */
#include "protocol/gbt_node_internal.h"
#include "protocol/gbt_codec.h"

#include <stddef.h>
#include <string.h>

#define BST_FRAMES_CHARGER_STOP 5U   /* TM-06: 5-10 BST frames before BSD (PROJECT_DEFINED within range) */

static uint16_t clamp16(uint32_t v)
{
    return (uint16_t)((v > 0xFFFFU) ? 0xFFFFU : v);
}

static uint16_t ma_to_current_raw(uint32_t ma)
{
    uint32_t d = ma / 100U;
    if (d > GBT_CURRENT_RAW_AT_0A) {
        d = GBT_CURRENT_RAW_AT_0A;
    }
    return (uint16_t)(GBT_CURRENT_RAW_AT_0A - d);
}

void gbt_vsm_start(gbt_node_t *n)
{
    gbt_node_set_step(n, GBT_STEP_V02);   /* TV03: wait CHM (no wait timer drawn, RQ-05) */
}

static void stop_charging_messages(gbt_node_t *n)
{
    gbt_node_tx_disable(n, GBT_MSG_BCL);
    gbt_node_tx_disable(n, GBT_MSG_BCS);
    gbt_node_tx_disable(n, GBT_MSG_BSM);
    gbt_node_timer_stop(n, GBT_TO(13U));
    gbt_node_timer_stop(n, GBT_TO(17U));
}

/* Vehicle-initiated stop (TV24 vehicle end condition / TV42). */
void gbt_vsm_stop(gbt_node_t *n, uint8_t reason)
{
    if (n->step != GBT_STEP_V11) {
        return;
    }
    memset(&n->s.stop_msg, 0, sizeof n->s.stop_msg);
    if (reason == GBT_STOP_REASON_SOC_TARGET) {
        n->s.stop_msg.reason[0] = GBT_CODE_01;      /* SPN3511 bits 1-2 required SOC reached */
    } else {
        n->s.stop_msg.fault[7] = GBT_CODE_01;       /* SPN3512 bits 15-16 other reason ([DOC] ERR-6) */
    }
    stop_charging_messages(n);
    gbt_node_tx_enable(n, GBT_MSG_BST);
    gbt_node_timer_start(n, GBT_TO(20U));           /* TO-20 CST 5 s from first BST */
    gbt_node_timer_start(n, GBT_TO(21U));           /* TO-21 CSD 10 s from first BST */
    n->s.stop_self = 1U;
    n->s.stop_frames = 0U;
    gbt_node_set_step(n, GBT_STEP_V12);
}

void gbt_vsm_on_msg(gbt_node_t *n, gbt_msg_t msg, const gbt_can_frame_t *f)
{
    switch (msg) {
    case GBT_MSG_CHM:
        if (n->step == GBT_STEP_V02) {              /* TV05 */
            gbt_node_tx_enable(n, GBT_MSG_BHM);
            gbt_node_timer_start(n, GBT_TO(2U));    /* TO-02 CRM 00 30 s */
            gbt_node_set_step(n, GBT_STEP_V03);
        }
        break;
    case GBT_MSG_CRM: {
        gbt_crm_t crm;
        if (gbt_decode_crm(f, &crm) != GBT_CODEC_OK) {
            n->diag.rx_invalid++;
            break;
        }
        if (n->step == GBT_STEP_V03 && crm.result == 0x00U) {          /* TV07 */
            gbt_node_tx_disable(n, GBT_MSG_BHM);
            gbt_node_timer_stop(n, GBT_TO(2U));
            gbt_node_tx_enable(n, GBT_MSG_BRM);
            gbt_node_timer_start(n, GBT_TO(4U));    /* TO-04 CRM AA 5 s */
            gbt_node_set_step(n, GBT_STEP_V05);
        } else if (n->step == GBT_STEP_V05 && crm.result == 0xAAU) {   /* TV09 */
            gbt_node_tx_disable(n, GBT_MSG_BRM);
            gbt_node_timer_stop(n, GBT_TO(4U));
            gbt_node_tx_enable(n, GBT_MSG_BCP);
            gbt_node_timer_start(n, GBT_TO(6U));    /* TO-06 CML 5 s */
            gbt_node_set_step(n, GBT_STEP_V06);
        }
        break;
    }
    case GBT_MSG_CML:
        if (n->step == GBT_STEP_V06) {              /* TV12, TV16, TV17 */
            if (gbt_decode_cml(f, &n->s.cml) != GBT_CODEC_OK) {
                n->diag.rx_invalid++;
                break;
            }
            n->s.cml_valid = 1U;
            gbt_node_timer_stop(n, GBT_TO(6U));
            gbt_node_tx_disable(n, GBT_MSG_BCP);
            n->s.bro_ready = 0x00U;                 /* TV17: BRO 0x00 first */
            gbt_node_tx_enable(n, GBT_MSG_BRO);
            gbt_node_set_step(n, GBT_STEP_V09);
        }
        break;
    case GBT_MSG_CRO:
        if (n->step == GBT_STEP_V09 && n->s.bro_ready == 0xAAU) {
            gbt_cro_t cro;
            if (gbt_decode_cro(f, &cro) != GBT_CODEC_OK) {
                n->diag.rx_invalid++;
                break;
            }
            gbt_node_timer_stop(n, GBT_TO(10U));
            if (cro.ready == 0xAAU) {               /* TV18 */
                gbt_node_timer_stop(n, GBT_TO(11U));
                gbt_node_tx_disable(n, GBT_MSG_BRO);
                gbt_node_tx_enable(n, GBT_MSG_BCL);
                gbt_node_tx_enable(n, GBT_MSG_BCS);
                gbt_node_timer_start(n, GBT_TO(13U));   /* TO-13 first CCS 1 s */
                gbt_node_set_step(n, GBT_STEP_V10);
            }
        }
        break;
    case GBT_MSG_CCS:
        if (n->step == GBT_STEP_V10 || n->step == GBT_STEP_V11) {
            gbt_ccs_t ccs;
            gbt_codec_status_t st = gbt_decode_ccs(f, &ccs);
            if (st == GBT_CODEC_E_RANGE) {          /* vehicle ERR-6 */
                n->diag.rx_invalid++;
                if (n->step == GBT_STEP_V11) {
                    gbt_vsm_stop(n, GBT_STOP_REASON_FAULT);
                }
                break;
            }
            if (st != GBT_CODEC_OK) {
                n->diag.rx_invalid++;               /* invalid data never refreshes a wait (ERR-3) */
                break;
            }
            gbt_node_timer_stop(n, GBT_TO(13U));
            gbt_node_timer_start(n, GBT_TO(17U));   /* TO-17 from last valid CCS (K-2 PROVISIONAL) */
            if (n->step == GBT_STEP_V10) {          /* TV22 */
                gbt_node_tx_enable(n, GBT_MSG_BSM);
                gbt_node_set_step(n, GBT_STEP_V11);
            }
        }
        break;
    case GBT_MSG_CST:
        if (n->step == GBT_STEP_V11) {              /* TV24: charger stop */
            memset(&n->s.stop_msg, 0, sizeof n->s.stop_msg);
            n->s.stop_msg.reason[3] = GBT_CODE_01;  /* SPN3511 bits 7-8 charger stop */
            stop_charging_messages(n);
            gbt_node_tx_enable(n, GBT_MSG_BST);
            gbt_node_timer_start(n, GBT_TO(21U));
            n->s.stop_self = 0U;
            n->s.stop_frames = 0U;
            gbt_node_set_step(n, GBT_STEP_V12);
        } else if (n->step == GBT_STEP_V12 && n->s.stop_self != 0U) {   /* TV25 */
            gbt_node_tx_disable(n, GBT_MSG_BST);
            gbt_node_timer_stop(n, GBT_TO(20U));
            gbt_node_tx_enable(n, GBT_MSG_BSD);
            gbt_node_set_step(n, GBT_STEP_V13);
        }
        break;
    case GBT_MSG_CSD:
        if (n->step == GBT_STEP_V13) {              /* TV28 */
            gbt_node_tx_disable(n, GBT_MSG_BSD);
            gbt_node_timer_stop(n, GBT_TO(21U));
            gbt_node_set_step(n, GBT_STEP_END);
        }
        break;
    default:
        n->diag.rx_ignored++;
        break;
    }
}

void gbt_vsm_tick(gbt_node_t *n)
{
    switch (n->step) {
    case GBT_STEP_V03:
        if (gbt_node_timer_expired(n, GBT_TO(2U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_BEM_CRM00);           /* TV08 */
        }
        break;
    case GBT_STEP_V05:
        if (gbt_node_timer_expired(n, GBT_TO(4U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_BEM_CRMAA);           /* TV11 */
        }
        break;
    case GBT_STEP_V06:
        if (gbt_node_timer_expired(n, GBT_TO(6U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_BEM_CML);             /* TV13 */
        }
        break;
    case GBT_STEP_V09:
        if (n->s.bro_ready == 0x00U && n->s.tx_on[GBT_MSG_BRO] == 1U) {
            n->s.bro_ready = 0xAAU;                 /* readiness reached (simulated) after >= 1 BRO 00 */
            gbt_node_timer_start(n, GBT_TO(10U));   /* TO-10 CRO 5 s from first BRO AA */
            gbt_node_timer_start(n, GBT_TO(11U));   /* TO-11 CRO AA 60 s from first BRO AA */
        }
        if (gbt_node_timer_expired(n, GBT_TO(10U)) != 0U || gbt_node_timer_expired(n, GBT_TO(11U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_BEM_CRO);             /* TV20 / TV21 */
        }
        break;
    case GBT_STEP_V10:
        if (gbt_node_timer_expired(n, GBT_TO(13U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_BEM_CCS);             /* TV23 */
        }
        break;
    case GBT_STEP_V11:
        if (gbt_node_timer_expired(n, GBT_TO(17U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_BEM_CCS);             /* TV41 (before TV24, K-2) */
        }
        break;
    case GBT_STEP_V12:
        if (n->s.stop_self != 0U) {
            if (gbt_node_timer_expired(n, GBT_TO(20U)) != 0U) {
                gbt_node_enter_timeout(n, GBT_BEM_CST);         /* TV26 */
            }
        } else if (n->s.stop_frames >= BST_FRAMES_CHARGER_STOP) {   /* TV27 */
            gbt_node_tx_disable(n, GBT_MSG_BST);
            gbt_node_tx_enable(n, GBT_MSG_BSD);
            gbt_node_set_step(n, GBT_STEP_V13);
        }
        break;
    case GBT_STEP_V13:
        if (gbt_node_timer_expired(n, GBT_TO(21U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_BEM_CSD);             /* TV29 */
        }
        break;
    default:
        break;
    }
}

uint8_t gbt_vsm_build(gbt_node_t *n, gbt_msg_t msg, gbt_can_frame_t *f)
{
    const gbt_vehicle_params_t *v = n->cfg.vehicle;
    gbt_codec_status_t st = GBT_CODEC_E_PARAM;

    switch (msg) {
    case GBT_MSG_BHM: {
        gbt_bhm_t m;
        m.insulation_voltage_raw = clamp16(v->max_charge_voltage_mv / 100U);
        st = gbt_encode_bhm(&m, f);
        break;
    }
    case GBT_MSG_BRO: {
        gbt_bro_t m;
        m.ready = n->s.bro_ready;
        st = gbt_encode_bro(&m, f);
        break;
    }
    case GBT_MSG_BCL: {
        gbt_bcl_t m;
        m.voltage_demand_raw = clamp16(n->demand.voltage_mv / 100U);
        m.current_demand_raw = ma_to_current_raw(n->demand.current_ma);
        m.mode = n->demand.mode;
        m.invalid_mask = 0U;
        st = gbt_encode_bcl(&m, f);
        break;
    }
    case GBT_MSG_BSM: {
        gbt_bsm_t m;
        memset(&m, 0, sizeof m);
        m.max_cell_v_index_raw = 0U;
        m.max_temp_raw = (uint8_t)(25 + 50);        /* 25 degC (battery model, fixed) */
        m.max_temp_point_raw = 0U;
        m.min_temp_raw = (uint8_t)(20 + 50);
        m.min_temp_point_raw = 1U;
        m.status[GBT_BSM_IDX_CHARGE_PERMIT] = GBT_CODE_01;
        st = gbt_encode_bsm(&m, f);
        break;
    }
    case GBT_MSG_BST:
        st = gbt_encode_bst(&n->s.stop_msg, f);
        if (st == GBT_CODEC_OK && n->s.stop_frames < 0xFFU) {
            n->s.stop_frames++;
        }
        break;
    case GBT_MSG_BSD: {
        gbt_bsd_t m;
        m.soc = (uint8_t)(n->demand.soc_permille / 10U);
        m.min_cell_v_raw = clamp16(v->max_cell_voltage_mv / 10U);
        m.max_cell_v_raw = clamp16(v->max_cell_voltage_mv / 10U);
        m.min_temp_raw = (uint8_t)(20 + 50);
        m.max_temp_raw = (uint8_t)(25 + 50);
        st = gbt_encode_bsd(&m, f);
        break;
    }
    case GBT_MSG_BEM: {
        gbt_bem_t m;
        memcpy(m.code, n->s.err_codes, sizeof m.code);
        st = gbt_encode_bem(&m, f);
        memcpy(n->err_fields, f->data, sizeof n->err_fields);
        break;
    }
    default:
        break;
    }
    return (st == GBT_CODEC_OK) ? 1U : 0U;
}

uint16_t gbt_vsm_tp_payload(gbt_node_t *n, gbt_msg_t msg, uint8_t *buf)
{
    const gbt_vehicle_params_t *v = n->cfg.vehicle;
    if (n->cfg.role != GBT_NODE_ROLE_VEHICLE) {
        return 0U;
    }
    if (msg == GBT_MSG_BRM) {
        gbt_brm_t m;
        memset(&m, 0, sizeof m);
        memcpy(m.version, gbt_brm_version_v1_10, 3U);
        m.battery_type = v->battery_type;
        m.rated_capacity_raw = clamp16(v->rated_capacity_mah / 100U);
        m.rated_voltage_raw = clamp16(v->rated_voltage_mv / 100U);
        return (gbt_encode_brm_payload(&m, buf, GBT_LEN_BRM) == GBT_CODEC_OK) ? (uint16_t)GBT_LEN_BRM : 0U;
    }
    if (msg == GBT_MSG_BCP) {
        gbt_bcp_t m;
        m.max_cell_v_raw = clamp16(v->max_cell_voltage_mv / 10U);
        m.max_current_raw = ma_to_current_raw(v->max_charge_current_ma);
        m.energy_raw = clamp16(v->nominal_energy_wh / 100U);
        m.max_voltage_raw = clamp16(v->max_charge_voltage_mv / 100U);
        m.max_temp_raw = (uint8_t)((v->max_cell_temp_c + 50 < 0) ? 0 : (v->max_cell_temp_c + 50 > 255) ? 255
                                                                   : v->max_cell_temp_c + 50);
        m.soc_raw = n->demand.soc_permille;
        m.present_voltage_raw = clamp16(v->rated_voltage_mv / 100U);
        return (gbt_encode_bcp_payload(&m, buf, GBT_LEN_BCP) == GBT_CODEC_OK) ? (uint16_t)GBT_LEN_BCP : 0U;
    }
    if (msg == GBT_MSG_BCS) {
        gbt_bcs_t m;
        m.voltage_raw = clamp16(n->demand.voltage_mv / 100U);
        m.current_raw = ma_to_current_raw(n->demand.current_ma);
        m.max_cell_v_raw = (uint16_t)(clamp16(v->max_cell_voltage_mv / 10U) & 0x0FFFU);
        m.max_cell_group = 0U;
        m.soc = (uint8_t)(n->demand.soc_permille / 10U);
        m.remaining_min = 0U;
        return (gbt_encode_bcs_payload(&m, buf, GBT_LEN_BCS) == GBT_CODEC_OK) ? (uint16_t)GBT_LEN_BCS : 0U;
    }
    return 0U;
}
