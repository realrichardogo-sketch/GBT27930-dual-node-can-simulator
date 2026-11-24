/*
 * protocol/gbt_charger_sm.c - charger (SECC) state machine, Lean subset.
 * Transitions TCxx: docs/sources/01.2_STATE_MACHINES.md §1.2.
 * Lean cuts (K-1): no CTS, no parameter-mismatch stops (TC12/TC16), no
 * resume (TC50), no reconnection (C-RC), no BSM-abnormal stop.
 * Non-blocking: every handler runs to completion; waits are timers.
 */
#include "protocol/gbt_node_internal.h"
#include "protocol/gbt_codec.h"

#include <stddef.h>
#include <string.h>

#define PGN_BRM 0x000200u
#define PGN_BCP 0x000600u
#define PGN_BCS 0x001100u
#define FEEDBACK_MAX_AGE_MS 100U          /* SAF-5 [DOC]: fresh <= 100 ms */

uint8_t gbt_csm_accept_tp(uint32_t pgn, uint16_t len)
{
    return (uint8_t)(((pgn == PGN_BRM && len == GBT_LEN_BRM) || (pgn == PGN_BCP && len == GBT_LEN_BCP)
                      || (pgn == PGN_BCS && len == GBT_LEN_BCS)) ? 1U : 0U);
}

static uint16_t mv_to_0p1v(uint32_t mv)
{
    uint32_t r = mv / 100U;
    return (uint16_t)((r > 0xFFFFU) ? 0xFFFFU : r);
}

static uint16_t ma_to_current_raw(uint32_t ma)
{
    uint32_t d = ma / 100U;               /* magnitude in 0.1 A */
    if (d > GBT_CURRENT_RAW_AT_0A) {
        d = GBT_CURRENT_RAW_AT_0A;
    }
    return (uint16_t)(GBT_CURRENT_RAW_AT_0A - d);   /* negative = charging (§5.4) */
}

void gbt_csm_start(gbt_node_t *n)
{
    gbt_node_set_step(n, GBT_STEP_C02);   /* TC03: aux power on, CHM (first within 1 s) */
    gbt_node_tx_enable(n, GBT_MSG_CHM);
    gbt_node_timer_start(n, GBT_TO(1U));  /* TO-01: BHM 10 s (PROT-12 charger, K-1.c) */
}

/* Charger-initiated stop from C10/C10P (TC30 / TC49): inhibit first, then CST. */
void gbt_csm_stop(gbt_node_t *n, uint8_t safe_reason)
{
    if (n->step != GBT_STEP_C10 && n->step != GBT_STEP_C10P) {
        return;
    }
    gbt_safety_disable(n, safe_reason);
    memset(&n->s.stop_msg, 0, sizeof n->s.stop_msg);
    switch (safe_reason) {
    case GBT_SAFE_REASON_REQUEST:
        n->s.stop_msg.reason[1] = GBT_CODE_01;      /* SPN3521 bits 3-4 manual stop */
        break;
    case GBT_SAFE_REASON_VOLTAGE_MIN:
        n->s.stop_msg.reason[2] = GBT_CODE_01;      /* fault stop */
        n->s.stop_msg.error[1] = GBT_CODE_01;       /* SPN3523 bits 3-4 voltage abnormal (S-11 PROVISIONAL) */
        break;
    default:                                        /* ERR-6, overflow, output failure */
        n->s.stop_msg.reason[2] = GBT_CODE_01;      /* SPN3521 bits 5-6 fault stop (S-1 PROVISIONAL) */
        n->s.stop_msg.fault[5] = GBT_CODE_01;       /* SPN3522 bits 11-12 other fault ([DOC] ERR-6) */
        break;
    }
    gbt_node_tx_disable(n, GBT_MSG_CCS);            /* CCS stops when CST is sent (Table B.1) */
    gbt_node_tx_enable(n, GBT_MSG_CST);
    gbt_node_timer_stop(n, GBT_TO(14U));
    gbt_node_timer_stop(n, GBT_TO(15U));
    gbt_node_timer_stop(n, GBT_TO(16U));
    gbt_node_timer_start(n, GBT_TO(18U));           /* TO-18: BST 5 s from first CST */
    gbt_node_timer_start(n, GBT_TO(19U));           /* TO-19: BSD 10 s from first CST */
    n->s.stop_self = 1U;
    gbt_node_set_step(n, GBT_STEP_C11);
}

static void enter_charging(gbt_node_t *n)
{
    gbt_node_tx_disable(n, GBT_MSG_CRO);            /* TC24: CRO stops on BCL (Annex A, S-7) */
    gbt_node_timer_stop(n, GBT_TO(12U));
    gbt_node_tx_enable(n, GBT_MSG_CCS);
    gbt_node_timer_start(n, GBT_TO(14U));           /* TO-14 BCL 1 s */
    gbt_node_timer_start(n, GBT_TO(15U));           /* TO-15 BCS 5 s (start at C10 entry, PROVISIONAL) */
    n->s.ccs_permit = GBT_CODE_01;
    gbt_node_set_step(n, GBT_STEP_C10);
}

static void on_bcl(gbt_node_t *n, const gbt_can_frame_t *f)
{
    gbt_bcl_t bcl;
    gbt_codec_status_t st = gbt_decode_bcl(f, &bcl);

    if (st == GBT_CODEC_E_RANGE) {                  /* T-3a: ERR-6 */
        n->diag.rx_invalid++;
        gbt_csm_stop(n, GBT_SAFE_REASON_CURRENT);
        return;
    }
    if (st != GBT_CODEC_OK) {                       /* T-3d: ZERO_CURRENT, no TO-14 refresh (ERR-3) */
        n->diag.rx_invalid++;
        gbt_safety_zero_current(n);
        return;
    }
    gbt_node_timer_start(n, GBT_TO(14U));           /* each valid BCL restarts TO-14 */
    n->s.bcl = bcl;
    if (gbt_safety_voltage_below_min(n, &bcl) != 0U) {   /* T-3c: PROT-7 -> CST */
        gbt_csm_stop(n, GBT_SAFE_REASON_VOLTAGE_MIN);
        return;
    }
    if (n->step == GBT_STEP_C10) {
        gbt_safety_apply_bcl(n, &bcl);
    }
}

static void on_bst(gbt_node_t *n)
{
    if (n->step == GBT_STEP_C10 || n->step == GBT_STEP_C10P) {     /* TC30, T-1 */
        gbt_safety_disable(n, GBT_SAFE_REASON_VEHICLE_STOP);
        memset(&n->s.stop_msg, 0, sizeof n->s.stop_msg);
        n->s.stop_msg.reason[3] = GBT_CODE_01;      /* SPN3521 bits 7-8 vehicle stop */
        gbt_node_tx_disable(n, GBT_MSG_CCS);
        gbt_node_tx_enable(n, GBT_MSG_CST);
        gbt_node_timer_stop(n, GBT_TO(14U));
        gbt_node_timer_stop(n, GBT_TO(15U));
        gbt_node_timer_stop(n, GBT_TO(16U));
        gbt_node_timer_start(n, GBT_TO(19U));
        n->s.stop_self = 0U;
        gbt_node_set_step(n, GBT_STEP_C12);         /* TC33: not charger-initiated */
    } else if (n->step == GBT_STEP_C11) {           /* TC31 */
        gbt_node_timer_stop(n, GBT_TO(18U));
        gbt_node_set_step(n, GBT_STEP_C12);
    } else if (n->step != GBT_STEP_C12 && n->step != GBT_STEP_END) {
        gbt_safety_disable(n, GBT_SAFE_REASON_VEHICLE_STOP);       /* D-04; sequence OPEN (OQ-S10) */
    }
}

void gbt_csm_on_msg(gbt_node_t *n, gbt_msg_t msg, const gbt_can_frame_t *f)
{
    switch (msg) {
    case GBT_MSG_BHM:
        if (n->step == GBT_STEP_C02) {              /* TC04 */
            gbt_bhm_t bhm;
            (void)gbt_decode_bhm(f, &bhm);
            gbt_node_timer_stop(n, GBT_TO(1U));
            n->s.selfcheck_pending = 1U;
            n->s.selfcheck_start_ms = n->now_ms;
            gbt_node_set_step(n, GBT_STEP_C03);
        }
        break;
    case GBT_MSG_BRO:
        if (n->step == GBT_STEP_C07 || n->step == GBT_STEP_C08) {
            gbt_bro_t bro;
            if (gbt_decode_bro(f, &bro) != GBT_CODEC_OK) {
                n->diag.rx_invalid++;
                break;
            }
            gbt_node_timer_stop(n, GBT_TO(7U));     /* any BRO satisfies TO-07 */
            if (n->step == GBT_STEP_C07 && bro.ready == 0xAAU) {   /* TC19 */
                gbt_node_timer_stop(n, GBT_TO(8U));
                gbt_node_tx_disable(n, GBT_MSG_CML);
                n->s.cro_ready = 0x00U;
                gbt_node_tx_enable(n, GBT_MSG_CRO);
                gbt_node_timer_start(n, GBT_TO(9U));
                gbt_node_set_step(n, GBT_STEP_C08);
            } else if (n->step == GBT_STEP_C08 && bro.ready == 0xAAU) {
                gbt_node_timer_start(n, GBT_TO(9U));                /* TC20 */
            }
        }
        break;
    case GBT_MSG_BCL:
        if (n->step == GBT_STEP_C09) {              /* TC24 */
            enter_charging(n);
            on_bcl(n, f);
        } else if (n->step == GBT_STEP_C10 || n->step == GBT_STEP_C10P) {
            on_bcl(n, f);
        }
        break;
    case GBT_MSG_BSM:
        if (n->step == GBT_STEP_C10 || n->step == GBT_STEP_C10P) {
            gbt_bsm_t bsm;
            if (gbt_decode_bsm(f, &bsm) != GBT_CODEC_OK) {
                n->diag.rx_invalid++;
                break;
            }
            n->s.bsm_seen = 1U;
            gbt_node_timer_start(n, GBT_TO(16U));   /* TO-16 armed after the first BSM */
            if (n->step == GBT_STEP_C10 && bsm.status[GBT_BSM_IDX_CHARGE_PERMIT] == GBT_CODE_00) {
                gbt_safety_disable(n, GBT_SAFE_REASON_PERMIT_OFF);   /* TC29, D-08 */
                n->s.ccs_permit = GBT_CODE_00;
                gbt_node_set_step(n, GBT_STEP_C10P);
            }
        }
        break;
    case GBT_MSG_BST:
        on_bst(n);
        break;
    case GBT_MSG_BSD:
        if (n->step == GBT_STEP_C12) {              /* TC34 */
            gbt_node_tx_disable(n, GBT_MSG_CST);
            gbt_node_timer_stop(n, GBT_TO(19U));
            gbt_node_tx_enable(n, GBT_MSG_CSD);
            gbt_node_set_step(n, GBT_STEP_END);
        }
        break;
    default:
        n->diag.rx_ignored++;
        break;
    }
}

void gbt_csm_on_tp(gbt_node_t *n, uint32_t pgn, const uint8_t *buf, uint16_t len)
{
    if (pgn == PGN_BRM && n->step == GBT_STEP_C04) {           /* TC08 */
        gbt_brm_t brm;
        if (gbt_decode_brm_payload(buf, len, &brm) != GBT_CODEC_OK) {
            n->diag.rx_invalid++;
            return;
        }
        gbt_node_timer_stop(n, GBT_TO(3U));
        n->s.crm_result = 0xAAU;
        gbt_node_timer_start(n, GBT_TO(5U));                   /* TO-05: BCP 5 s */
        gbt_node_set_step(n, GBT_STEP_C05);
    } else if (pgn == PGN_BCP && n->step == GBT_STEP_C05) {    /* TC10 + TC13 */
        n->s.bcp_valid = (gbt_decode_bcp_payload(buf, len, &n->s.bcp) == GBT_CODEC_OK) ? 1U : 0U;
        gbt_node_timer_stop(n, GBT_TO(5U));
        gbt_node_tx_disable(n, GBT_MSG_CRM);
        gbt_node_tx_enable(n, GBT_MSG_CML);
        gbt_node_timer_start(n, GBT_TO(7U));                   /* TO-07 BRO 5 s */
        gbt_node_timer_start(n, GBT_TO(8U));                   /* TO-08 BRO AA 60 s */
        gbt_node_set_step(n, GBT_STEP_C07);
    } else if (pgn == PGN_BCS && (n->step == GBT_STEP_C10 || n->step == GBT_STEP_C10P)) {
        gbt_bcs_t bcs;
        gbt_codec_status_t st = gbt_decode_bcs_payload(buf, len, &bcs);
        if (st == GBT_CODEC_E_RANGE) {                          /* T-3b: ERR-6 */
            n->diag.rx_invalid++;
            gbt_csm_stop(n, GBT_SAFE_REASON_CURRENT);
            return;
        }
        if (st == GBT_CODEC_OK) {
            gbt_node_timer_start(n, GBT_TO(15U));
        }
    }
}

void gbt_csm_tick(gbt_node_t *n)
{
    if (n->out_state == GBT_NODE_OUT_FAILED && n->inhibit_latched != 0U) {
        gbt_safety_disable(n, GBT_SAFE_REASON_OUTPUT_FAIL);   /* ND-6 rule 7: retry each step */
    }
    switch (n->step) {
    case GBT_STEP_C02:
        if (gbt_node_timer_expired(n, GBT_TO(1U)) != 0U) {   /* TC05: compatibility path */
            gbt_node_timer_stop(n, GBT_TO(1U));
            n->s.selfcheck_pending = 1U;
            n->s.selfcheck_start_ms = n->now_ms;
            gbt_node_set_step(n, GBT_STEP_C03);
        }
        break;
    case GBT_STEP_C03:                                       /* self-check simulated (S-8) */
        if (gbt_time_elapsed(n->now_ms, n->s.selfcheck_start_ms) >= n->cfg.selfcheck_delay_ms) {
            n->s.selfcheck_pending = 0U;
            gbt_node_tx_disable(n, GBT_MSG_CHM);             /* TC07 */
            n->s.crm_result = 0x00U;
            gbt_node_tx_enable(n, GBT_MSG_CRM);
            gbt_node_timer_start(n, GBT_TO(3U));             /* TO-03 BRM 5 s */
            gbt_node_set_step(n, GBT_STEP_C04);
        }
        break;
    case GBT_STEP_C04:
        if (gbt_node_timer_expired(n, GBT_TO(3U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BRM);          /* TC09 */
        }
        break;
    case GBT_STEP_C05:
        if (gbt_node_timer_expired(n, GBT_TO(5U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BCP);          /* TC11 */
        }
        break;
    case GBT_STEP_C07:
        if (gbt_node_timer_expired(n, GBT_TO(7U)) != 0U || gbt_node_timer_expired(n, GBT_TO(8U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BRO);          /* TC15 / TC18 */
        }
        break;
    case GBT_STEP_C08:
        if (gbt_node_timer_expired(n, GBT_TO(9U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BRO);          /* TC21 */
        } else if (n->s.tx_on[GBT_MSG_CRO] == 1U) {         /* TC23 after >= 1 CRO 0x00 sent; ready simulated */
            gbt_node_timer_stop(n, GBT_TO(9U));
            n->s.cro_ready = 0xAAU;
            gbt_node_timer_start(n, GBT_TO(12U));            /* TO-12 first BCL 1 s */
            gbt_node_set_step(n, GBT_STEP_C09);
        }
        break;
    case GBT_STEP_C09:
        if (gbt_node_timer_expired(n, GBT_TO(12U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BCL);          /* TC25 */
        }
        break;
    case GBT_STEP_C10:
    case GBT_STEP_C10P:                                      /* order of §1.4 item 2 */
        if (gbt_node_timer_expired(n, GBT_TO(14U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BCL);          /* TC26 / TC51 */
        } else if (gbt_node_timer_expired(n, GBT_TO(15U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BCS);          /* TC27 / TC52 */
        } else if (n->s.bsm_seen != 0U && gbt_node_timer_expired(n, GBT_TO(16U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BSM);          /* TC28 / TC53 */
        }
        break;
    case GBT_STEP_C11:
        if (gbt_node_timer_expired(n, GBT_TO(18U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BST);          /* TC32 */
        }
        break;
    case GBT_STEP_C12:
        if (gbt_node_timer_expired(n, GBT_TO(19U)) != 0U) {
            gbt_node_enter_timeout(n, GBT_CEM_BSD);          /* TC35 */
        }
        break;
    default:
        break;
    }
}

uint8_t gbt_csm_build(gbt_node_t *n, gbt_msg_t msg, gbt_can_frame_t *f)
{
    const gbt_charger_params_t *c = n->cfg.charger;
    gbt_codec_status_t st = GBT_CODEC_E_PARAM;

    switch (msg) {
    case GBT_MSG_CHM: {
        gbt_chm_t m;
        memcpy(m.version, gbt_chm_version_v1_1, 3U);
        st = gbt_encode_chm(&m, f);
        break;
    }
    case GBT_MSG_CRM: {
        gbt_crm_t m;
        memset(&m, 0, sizeof m);
        m.result = n->s.crm_result;
        m.charger_number = c->charger_number;
        st = gbt_encode_crm(&m, f);
        break;
    }
    case GBT_MSG_CML: {
        gbt_cml_t m;
        m.max_voltage_raw = mv_to_0p1v(c->max_voltage_mv);
        m.min_voltage_raw = mv_to_0p1v(c->min_voltage_mv);
        m.max_current_raw = ma_to_current_raw(c->max_current_ma);
        m.min_current_raw = ma_to_current_raw(c->min_current_ma);
        st = gbt_encode_cml(&m, f);
        break;
    }
    case GBT_MSG_CRO: {
        gbt_cro_t m;
        m.ready = n->s.cro_ready;
        st = gbt_encode_cro(&m, f);
        break;
    }
    case GBT_MSG_CCS: {
        gbt_ccs_t m;
        uint8_t fresh = (n->feedback.valid != 0U
                         && gbt_time_elapsed(n->now_ms, n->feedback.t_ms) <= FEEDBACK_MAX_AGE_MS) ? 1U : 0U;
        m.voltage_raw = (fresh != 0U) ? mv_to_0p1v(n->feedback.voltage_mv) : 0U;          /* SAF-5 */
        m.current_raw = (fresh != 0U) ? ma_to_current_raw(n->feedback.current_ma) : (uint16_t)GBT_CURRENT_RAW_AT_0A;
        m.charge_time_min = 0U;
        m.permit = n->s.ccs_permit;
        st = gbt_encode_ccs(&m, f);
        break;
    }
    case GBT_MSG_CST:
        st = gbt_encode_cst(&n->s.stop_msg, f);
        break;
    case GBT_MSG_CSD: {
        gbt_csd_t m;
        m.charge_time_min = 0U;
        m.energy_raw = 0U;
        m.charger_number_raw = c->charger_number;   /* P-K5-02 offset 0 */
        st = gbt_encode_csd(&m, f);
        break;
    }
    case GBT_MSG_CEM: {
        gbt_cem_t m;
        memcpy(m.code, n->s.err_codes, sizeof m.code);
        st = gbt_encode_cem(&m, f);
        memcpy(n->err_fields, f->data, sizeof n->err_fields);
        break;
    }
    default:
        break;
    }
    return (st == GBT_CODEC_OK) ? 1U : 0U;
}
