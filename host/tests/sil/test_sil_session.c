/*
 * tests/sil/test_sil_session.c - deterministic dual-node SIL (Mode A).
 * Node 0 = charger (SECC), node 1 = vehicle (EVCC); fake HAL bus; 1 ms clock.
 * Scenarios (Lean Plan): normal session + vehicle stop, charger stop, BCL
 * timeout, CCS timeout, out-of-range current, missing TP_DT, voltage demand
 * at / below the charger minimum (T-3c), RX queue overflow (T-5).
 * Timing: software output-command acceptance (sim plant) vs. trigger, target
 * <= 10 ms. This is SIL evidence only; it is not a hardware or physical-output
 * measurement. Test configuration selects ND-6 OPTION_A explicitly (ND-6 is
 * OPEN; this is a test setting, not a project decision).
 */
#include "app/battery_model.h"
#include "app/gbt_integration.h"
#include "app/sim_plant.h"
#include "tests/sil/gbt_sil.h"
#include "tests/support/test_assert.h"

#include <stdio.h>
#include <string.h>

#define ID_BCL 0x181056F4u
#define ID_CCS 0x1812F456u
#define ID_BST 0x101956F4u
#define ID_CST 0x101AF456u
#define ID_BEM 0x081E56F4u
#define ID_CEM 0x081FF456u
#define ID_TPDT_V2C 0x1CEB56F4u

typedef struct {
    gbt_sil_t            sil;
    gbt_integ_t          ig[2];
    output_if_backend_t  plant[2];
    battery_model_t      batt;
    gbt_charger_params_t cp;
    gbt_vehicle_params_t vp;
    gbt_node_config_t    cfg[2];
    uint8_t              soc_stop_sent;
    /* monitor / fault injection */
    uint8_t              drop_bcl;
    uint8_t              drop_ccs;
    uint8_t              drop_dt2_once;
    uint8_t              dt_dropped;
    uint32_t             last_bcl_ms;
    uint32_t             first_bst_ms;
    uint32_t             first_cst_ms;
    uint32_t             bst_count;
    uint32_t             cem_count;
    uint32_t             bem_count;
    uint32_t             first_bem_ms;
    uint32_t             first_low_bcl_ms;   /* T-3c: first BCL below the charger minimum on the bus */
    gbt_can_frame_t      first_cem;
    gbt_can_frame_t      first_bem;
    gbt_can_frame_t      first_cst;
    /* STEP 07-C2: independent tally of TP frames delivered on the SIL bus */
    uint32_t             bus_rts;
    uint32_t             bus_rts_brm;
    uint32_t             bus_rts_bcp;
    uint32_t             bus_rts_bcs;
    uint32_t             bus_cts;
    uint32_t             bus_dt;
    uint32_t             bus_eom;
    uint32_t             bus_abort;
    /* first session of each TP message (index 0 BRM, 1 BCP, 2 BCS), measured on the bus */
    uint8_t              first_open;      /* 0 = none, else index + 1 of the first session in progress */
    uint8_t              first_seen[3];
    uint32_t             first_dt[3];
    uint8_t              first_cts[3];
    uint8_t              first_eom[3];
    /* STEP 07-C2.4: integration event logs (0 charger, 1 vehicle) */
    gbt_evlog_t          evlog[2];
} world_t;

#define ID_TPCM_V2C 0x1CEC56F4u
#define ID_TPCM_C2V 0x1CECF456u

static void tally_tp(world_t *w, const gbt_can_frame_t *f)
{
    uint32_t pgn = (uint32_t)f->data[5] | ((uint32_t)f->data[6] << 8) | ((uint32_t)f->data[7] << 16);
    if (f->id == ID_TPDT_V2C) {
        w->bus_dt++;
        if (w->first_open != 0U) {
            w->first_dt[w->first_open - 1U]++;
        }
    } else if (f->id == ID_TPCM_V2C || f->id == ID_TPCM_C2V) {
        if (f->data[0] == 255U) {
            w->bus_abort++;
            w->first_open = 0U;
        } else if (f->id == ID_TPCM_V2C && f->data[0] == 16U) {
            uint8_t idx = (pgn == 0x000200u) ? 1U : (pgn == 0x000600u) ? 2U : (pgn == 0x001100u) ? 3U : 0U;
            w->bus_rts++;
            w->bus_rts_brm += (pgn == 0x000200u) ? 1U : 0U;
            w->bus_rts_bcp += (pgn == 0x000600u) ? 1U : 0U;
            w->bus_rts_bcs += (pgn == 0x001100u) ? 1U : 0U;
            w->first_open = 0U;
            if (idx != 0U && w->first_seen[idx - 1U] == 0U) {
                w->first_seen[idx - 1U] = 1U;
                w->first_open = idx;
            }
        } else if (f->id == ID_TPCM_C2V && f->data[0] == 17U) {
            w->bus_cts++;
            if (w->first_open != 0U) {
                w->first_cts[w->first_open - 1U]++;
            }
        } else if (f->id == ID_TPCM_C2V && f->data[0] == 19U) {
            w->bus_eom++;
            if (w->first_open != 0U) {
                w->first_eom[w->first_open - 1U]++;
                w->first_open = 0U;
            }
        }
    }
}

static world_t W;

static uint8_t monitor(void *ctx, uint8_t from, const gbt_can_frame_t *f)
{
    world_t *w = (world_t *)ctx;
    uint32_t now = w->sil.now_ms;
    (void)from;
    if (f->id == ID_BCL) {
        if (w->drop_bcl != 0U) {
            return 0U;
        }
        w->last_bcl_ms = now;
        if (w->first_low_bcl_ms == 0U && (uint32_t)(f->data[0] | (f->data[1] << 8)) < 2000U) {
            w->first_low_bcl_ms = now;               /* SPN3072 < 200.0 V (charger min) */
        }
    }
    if (f->id == ID_CCS && w->drop_ccs != 0U) {
        return 0U;
    }
    if (f->id == ID_TPDT_V2C && f->data[0] == 2U && w->drop_dt2_once != 0U && w->dt_dropped == 0U) {
        w->dt_dropped = 1U;
        return 0U;
    }
    if (f->id == ID_BST) {
        if (w->bst_count++ == 0U) {
            w->first_bst_ms = now;
        }
    }
    if (f->id == ID_CST && w->first_cst_ms == 0U) {
        w->first_cst_ms = now;
        w->first_cst = *f;
    }
    if (f->id == ID_CEM && w->cem_count++ == 0U) {
        w->first_cem = *f;
    }
    if (f->id == ID_BEM && w->bem_count++ == 0U) {
        w->first_bem = *f;
        w->first_bem_ms = now;
    }
    tally_tp(w, f);                                          /* STEP 07-C2: delivered frames only */
    return 1U;
}

static void step(void *ctx, uint8_t idx, uint32_t now)
{
    world_t *w = (world_t *)ctx;
    if (idx == 1U) {
        gbt_vehicle_demand_t d;
        gbt_node_status_t st;
        gbt_node_get_status(&w->sil.node[1], &st);
        if (battery_model_step(&w->batt, now, (uint8_t)(st.phase == GBT_NODE_PHASE_CHARGING), &d) != 0U
            && st.phase == GBT_NODE_PHASE_CHARGING && w->soc_stop_sent == 0U) {
            w->soc_stop_sent = 1U;
            gbt_node_request_stop(&w->sil.node[1], GBT_STOP_REASON_SOC_TARGET, now);   /* vehicle stop */
        }
        gbt_node_set_vehicle_demand(&w->sil.node[1], &d);
    }
    gbt_integ_step(&w->ig[idx]);
}

static void world_init_ex(world_t *w, uint16_t soc_target, uint8_t first, uint8_t connect_mask);

static void world_init(world_t *w, uint16_t soc_target, uint8_t first)
{
    world_init_ex(w, soc_target, first, 3U);
}

/* connect_mask bit 0: charger connected at t0, bit 1: vehicle connected at t0 (STEP 07-C2.4) */
static void world_init_ex(world_t *w, uint16_t soc_target, uint8_t first, uint8_t connect_mask)
{
    gbt_output_sink_t sink[2];
    unsigned i;
    gbt_vehicle_demand_t d;

    memset(w, 0, sizeof *w);
    CHECK(gbt_sil_init(&w->sil, 100000U, step, w) == GBT_HAL_OK);
    CHECK(gbt_sil_set_order(&w->sil, first) == GBT_HAL_OK);
    w->sil.bus.filter = monitor;
    w->sil.bus.filter_ctx = w;

    w->cp.max_voltage_mv = 750000U;
    w->cp.min_voltage_mv = 200000U;
    w->cp.max_current_ma = 250000U;
    w->cp.min_current_ma = 1000U;
    w->cp.charger_number = 0x01020304u;
    w->vp.battery_type = 0x03U;
    w->vp.rated_capacity_mah = 150000U;
    w->vp.rated_voltage_mv = 350000U;
    w->vp.max_cell_voltage_mv = 3650U;
    w->vp.max_charge_current_ma = 200000U;
    w->vp.nominal_energy_wh = 60000U;
    w->vp.max_charge_voltage_mv = 420000U;
    w->vp.max_cell_temp_c = 55;
    w->vp.initial_soc_permille = 500U;
    battery_model_init(&w->batt, 500U, soc_target, 10U, 400000U, 100000U, 0x02U);

    for (i = 0U; i < 2U; i++) {
        memset(&w->cfg[i], 0, sizeof w->cfg[i]);
        w->cfg[i].t_handle_ms = 1000U;                     /* K-6A PROVISIONAL */
        w->cfg[i].selfcheck_delay_ms = 0U;                 /* S-8 PROVISIONAL */
        w->cfg[i].apply_fail_tx_policy = GBT_NODE_FAILTX_OPTION_A;   /* test setting; ND-6 OPEN */
        sim_plant_init(&w->plant[i], &w->sil.now_ms);
        CHECK(output_if_make_sink(&w->plant[i], &sink[i]) == GBT_HAL_OK);
    }
    w->cfg[0].role = GBT_NODE_ROLE_CHARGER;
    w->cfg[0].charger = &w->cp;
    w->cfg[1].role = GBT_NODE_ROLE_VEHICLE;
    w->cfg[1].vehicle = &w->vp;
    for (i = 0U; i < 2U; i++) {
        CHECK(gbt_integ_attach(&w->ig[i], &w->sil.node[i], &w->sil.hal[i], &w->cfg[i], &sink[i]) == GBT_HAL_OK);
        gbt_integ_set_evlog(&w->ig[i], &w->evlog[i]);      /* STEP 07-C2.4 */
    }
    (void)battery_model_step(&w->batt, w->sil.now_ms, 0U, &d);
    gbt_node_set_vehicle_demand(&w->sil.node[1], &d);
    if ((connect_mask & 2U) != 0U) {
        gbt_node_set_connected(&w->sil.node[1], 1U, w->sil.now_ms);
    }
    if ((connect_mask & 1U) != 0U) {
        gbt_node_set_connected(&w->sil.node[0], 1U, w->sil.now_ms);
    }
}

static uint8_t phase(world_t *w, unsigned i)
{
    gbt_node_status_t st;
    gbt_node_get_status(&w->sil.node[i], &st);
    return st.phase;
}

static uint32_t run_until_phase(world_t *w, unsigned i, uint8_t ph, uint32_t max_ms)
{
    uint32_t t;
    for (t = 0U; t < max_ms; t++) {
        if (phase(w, i) == ph) {
            return t;
        }
        gbt_sil_tick(&w->sil);
    }
    return 0xFFFFFFFFu;
}

static uint32_t latency(uint32_t t0, uint32_t t1)
{
    return (uint32_t)(t1 - t0);
}

/* STEP 07 FI-04: charger (node 0) shutdown measurement after one stop. ref_flags = expected
 * reference flags (0 = valid). legacy = latency already measured by the scenario (trigger ->
 * sim_plant first_disable_ms); the conservative bound must never be smaller than it. */
static void check_fi04(world_t *w, uint8_t ref_flags, uint32_t legacy, const char *what)
{
    const gbt_fi04_t *m = &w->ig[0].fi04;
    CHECK_EQ_U(m->shutdowns, 1U);                          /* exactly one counted shutdown */
    CHECK_EQ_U(m->open, 0U);                               /* nothing pending after acceptance */
    CHECK_EQ_U(m->apply_status, GBT_HAL_OK);
    CHECK_EQ_U(m->accept_seq, m->confirmed_seq);
    CHECK_EQ_U(m->accept_seq, w->sil.node[0].out_accepted_seq);
    CHECK_EQ_U(m->accept_seq, w->sil.hal[0].accepted_seq); /* HAL recorded the same sequence */
    CHECK_EQ_U(m->accept_seq, w->plant[0].last.seq);
    CHECK_EQ_U(w->plant[0].last.enabled, 0U);
    CHECK_EQ_U(m->t1_ms, w->plant[0].first_disable_ms);    /* T1 = sink acceptance tick */
    CHECK_EQ_U(m->elapsed_ms, (uint32_t)(m->t1_ms - m->t0_ms));
    CHECK(m->elapsed_ms >= legacy);                        /* conservative: never below the trigger latency */
    CHECK_EQ_U(m->ref_flags, ref_flags);
    CHECK_EQ_U(m->refused + m->mismatch + m->aborted, 0U);
    if (ref_flags == 0U) {
        CHECK_EQ_U(m->verdict, GBT_FI04_VERDICT_PASS);
        CHECK(m->elapsed_ms <= GBT_FI04_LIMIT_MS);
        CHECK_EQ_U(m->pass_count, 1U);
        CHECK_EQ_U(m->max_valid_ms, m->elapsed_ms);
    } else {
        CHECK_EQ_U(m->verdict, GBT_FI04_VERDICT_INVALID);  /* never reported as PASS */
        CHECK_EQ_U(m->pass_count, 0U);
        CHECK_EQ_U(m->invalid_count, 1U);
        CHECK_EQ_U(m->max_valid_ms, 0U);
    }
    printf("SIL FI-04 %s: shutdowns %lu, seq %lu, T0 %lu, T1 %lu, elapsed %lu ms (trigger latency %lu ms), "
           "ref flags 0x%02X, verdict %u\n", what, (unsigned long)m->shutdowns, (unsigned long)m->accept_seq,
           (unsigned long)m->t0_ms, (unsigned long)m->t1_ms, (unsigned long)m->elapsed_ms,
           (unsigned long)legacy, (unsigned)m->ref_flags, (unsigned)m->verdict);
}

static void reach_charging(world_t *w)
{
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_CHARGING, 20000U) != 0xFFFFFFFFu);
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_CHARGING, 2000U) != 0xFFFFFFFFu);
    gbt_sil_run(&w->sil, 300U);                            /* settle: BSM, BCS, BCL flowing */
}

/* STEP 07-C2: TP statistics after a complete normal session (both nodes in END).
 * Node 0 = charger (TP receiver), node 1 = vehicle (TP sender). The number of TP sessions N
 * is taken from the run, not hard-coded: BRM repeats every 250 ms until CRM 0xAA arrives and
 * BCS repeats every 250 ms while charging, so N depends on protocol timing. */
static void check_normal_session_tp_stats(world_t *w, uint8_t first)
{
    const gbt_tp_stats_t *v = &w->sil.node[1].tp.stats;
    const gbt_tp_stats_t *c = &w->sil.node[0].tp.stats;
    uint32_t n;

    gbt_sil_run(&w->sil, 1000U);                           /* both in END: vehicle sends no more TP */
    n = v->tx_started;
    CHECK(n >= 3U);                                        /* at least BRM, BCP, BCS */
    /* sender (vehicle) */
    CHECK_EQ_U(v->rts_tx, n);
    CHECK_EQ_U(v->cts_rx, n);
    CHECK_EQ_U(v->eom_rx, n);
    CHECK_EQ_U(v->tx_complete, n);
    CHECK_EQ_U(v->abort_tx, 0U);
    CHECK_EQ_U(v->abort_rx, 0U);
    CHECK_EQ_U(v->timeout_tx, 0U);
    CHECK_EQ_U(v->rts_rx + v->cts_tx + v->dt_rx_ok + v->eom_tx + v->rx_started + v->rx_complete
               + v->timeout_rx, 0U);
    CHECK_EQ_U(v->last_tx_complete_pgn, 0x001100u);        /* last TP message of a session: BCS */
    CHECK_EQ_U(v->last_tx_complete_len, 9U);
    CHECK_EQ_U(v->last_end_ev, GBT_TP_EV_TX_COMPLETE);
    CHECK_EQ_U(w->sil.node[1].tp.tx.state, GBT_TP_IDLE);
    /* receiver (charger) */
    CHECK_EQ_U(c->rts_rx, n);
    CHECK_EQ_U(c->rx_started, n);
    CHECK_EQ_U(c->cts_tx, n);                              /* one CTS per session (RTS max = 0xFF) */
    CHECK_EQ_U(c->eom_tx, n);
    CHECK_EQ_U(c->rx_complete, n);
    CHECK_EQ_U(c->dt_rx_ok, v->dt_tx);
    CHECK_EQ_U(c->abort_tx, 0U);
    CHECK_EQ_U(c->abort_rx, 0U);
    CHECK_EQ_U(c->timeout_rx, 0U);
    CHECK_EQ_U(c->rts_tx + c->cts_rx + c->dt_tx + c->eom_rx + c->tx_started + c->tx_complete
               + c->timeout_tx, 0U);
    CHECK_EQ_U(c->last_rx_complete_pgn, 0x001100u);
    CHECK_EQ_U(c->last_rx_complete_len, 9U);
    CHECK_EQ_U(c->last_end_ev, GBT_TP_EV_RX_COMPLETE);
    CHECK_EQ_U(w->sil.node[0].tp.rx.state, GBT_TP_IDLE);
    CHECK_EQ_U(w->sil.node[0].tp.outbox_full + w->sil.node[1].tp.outbox_full, 0U);
    /* independent bus tally: what the counters claim was really delivered */
    CHECK_EQ_U(w->bus_rts, n);
    CHECK_EQ_U(w->bus_cts, n);
    CHECK_EQ_U(w->bus_eom, n);
    CHECK_EQ_U(w->bus_dt, v->dt_tx);
    CHECK_EQ_U(w->bus_abort, 0U);
    CHECK(w->bus_rts_brm >= 1U && w->bus_rts_bcp >= 1U && w->bus_rts_bcs >= 1U);
    CHECK_EQ_U(w->bus_rts_brm + w->bus_rts_bcp + w->bus_rts_bcs, n);
    CHECK_EQ_U(v->dt_tx, 7U * w->bus_rts_brm + 2U * w->bus_rts_bcp + 2U * w->bus_rts_bcs);
    /* integration diagnostics */
    CHECK_EQ_U(w->ig[0].output_shutdowns, 1U);             /* charging output disabled once (T-1) */
    CHECK_EQ_U(w->ig[0].last_cmd.enabled, 0U);
    CHECK_EQ_U(w->ig[0].last_apply_status, GBT_HAL_OK);
    CHECK_EQ_U(w->ig[1].output_shutdowns, 0U);             /* vehicle never enables an output */
    CHECK_EQ_U(w->ig[0].last_rx_valid, 1U);
    CHECK_EQ_U(w->ig[1].last_rx_valid, 1U);
    CHECK_EQ_U(w->ig[0].rx_seq, w->sil.node[0].diag.rx_frames);   /* every popped frame reached the node */
    CHECK_EQ_U(w->ig[1].rx_seq, w->sil.node[1].diag.rx_frames);
    CHECK_EQ_U(w->ig[1].last_rx.id, 0x181DF456u);          /* charger keeps sending CSD after END */
    CHECK_EQ_U(w->ig[1].last_rx.flags, GBT_CAN_FLAG_EXT);
    CHECK_EQ_U(w->ig[1].last_rx.dlc, 8U);
    /* driver acceptance (event level 3): every TP frame queued by the TP layer was taken by the HAL */
    CHECK_EQ_U(w->ig[1].tx_hal_accepted_tp_dt, v->dt_tx);
    CHECK_EQ_U(w->ig[1].tx_hal_accepted_tp_cm, v->rts_tx);        /* vehicle TP.CM = RTS only (no aborts) */
    CHECK_EQ_U(w->ig[0].tx_hal_accepted_tp_cm, c->cts_tx + c->eom_tx);
    CHECK_EQ_U(w->ig[0].tx_hal_accepted_tp_dt, 0U);
    CHECK_EQ_U(w->ig[1].tx_hal_accepted, w->sil.node[1].diag.tx_frames);   /* no send error, no filter */
    CHECK_EQ_U(w->ig[0].tx_hal_accepted, w->sil.node[0].diag.tx_frames);
    /* first session of each TP message: BRM 7 DT, BCP 2 DT, BCS 2 DT = 3 sessions / 11 DT,
       each opened by RTS, authorised by one CTS and closed by EOM (measured on the bus) */
    CHECK(w->first_seen[0] == 1U && w->first_seen[1] == 1U && w->first_seen[2] == 1U);
    CHECK_EQ_U(w->first_dt[0], 7U);
    CHECK_EQ_U(w->first_dt[1], 2U);
    CHECK_EQ_U(w->first_dt[2], 2U);
    CHECK_EQ_U(w->first_dt[0] + w->first_dt[1] + w->first_dt[2], 11U);
    CHECK(w->first_cts[0] == 1U && w->first_cts[1] == 1U && w->first_cts[2] == 1U);
    CHECK(w->first_eom[0] == 1U && w->first_eom[1] == 1U && w->first_eom[2] == 1U);
    printf("SIL first BRM/BCP/BCS sessions (order %u): DT %lu/%lu/%lu = %lu, CTS %u/%u/%u, EOM %u/%u/%u\n",
           (unsigned)first, (unsigned long)w->first_dt[0], (unsigned long)w->first_dt[1],
           (unsigned long)w->first_dt[2], (unsigned long)(w->first_dt[0] + w->first_dt[1] + w->first_dt[2]),
           (unsigned)w->first_cts[0], (unsigned)w->first_cts[1], (unsigned)w->first_cts[2],
           (unsigned)w->first_eom[0], (unsigned)w->first_eom[1], (unsigned)w->first_eom[2]);
    printf("SIL driver acceptance (order %u): vehicle TP.DT %lu TP.CM %lu, charger TP.CM %lu, "
           "all frames vehicle %lu charger %lu\n", (unsigned)first,
           (unsigned long)w->ig[1].tx_hal_accepted_tp_dt, (unsigned long)w->ig[1].tx_hal_accepted_tp_cm,
           (unsigned long)w->ig[0].tx_hal_accepted_tp_cm, (unsigned long)w->ig[1].tx_hal_accepted,
           (unsigned long)w->ig[0].tx_hal_accepted);
    printf("SIL TP statistics (order %u): sessions %lu (BRM %lu, BCP %lu, BCS %lu), DT %lu, "
           "RTS/CTS/EOM %lu/%lu/%lu, completions tx %lu rx %lu, aborts 0, timeouts 0, "
           "charger output shutdowns %lu\n", (unsigned)first, (unsigned long)n,
           (unsigned long)w->bus_rts_brm, (unsigned long)w->bus_rts_bcp, (unsigned long)w->bus_rts_bcs,
           (unsigned long)v->dt_tx, (unsigned long)v->rts_tx, (unsigned long)c->cts_tx,
           (unsigned long)c->eom_tx, (unsigned long)v->tx_complete, (unsigned long)c->rx_complete,
           (unsigned long)w->ig[0].output_shutdowns);
}

/* ---- STEP 07-C2.4: event log, per-message completions, BRM snapshots, timing ---- */
/* Step sequence recorded in an event log (STEP entries, a = new step). Returns the count. */
static unsigned log_steps(const gbt_evlog_t *lg, uint8_t *out, unsigned max)
{
    unsigned i;
    unsigned n = 0U;
    for (i = 0U; i < lg->count && n < max; i++) {
        if (lg->e[i].type == GBT_EV_STEP) {
            out[n++] = lg->e[i].a;
        }
    }
    return n;
}

/* TP entries (STEP entries skipped) of the first TP session found for pgn, from its RTS up to
 * and including both its EOM_ACK and its TP_END entry (the receiver records TP_END when it
 * accepts the last DT, i.e. BEFORE its EOM_ACK leaves in the send phase; the sender records
 * TP_END after the received EOM_ACK). Writes up to max entries. */
static unsigned log_tp_session(const gbt_evlog_t *lg, uint32_t pgn, gbt_evlog_entry_t *out, unsigned max)
{
    unsigned i;
    unsigned n = 0U;
    uint8_t open = 0U;
    uint8_t seen_end = 0U;
    uint8_t seen_eom = 0U;
    for (i = 0U; i < lg->count && n < max; i++) {
        const gbt_evlog_entry_t *e = &lg->e[i];
        if (e->type == GBT_EV_STEP) {
            continue;
        }
        if (open == 0U) {
            if ((e->type == GBT_EV_RX_TP_CM || e->type == GBT_EV_TX_TP_CM) && e->a == 16U && e->pgn == pgn) {
                open = 1U;
            } else {
                continue;
            }
        }
        out[n++] = *e;
        seen_end = (uint8_t)(seen_end | ((e->type == GBT_EV_TP_END) ? 1U : 0U));
        seen_eom = (uint8_t)(seen_eom | (((e->type == GBT_EV_RX_TP_CM || e->type == GBT_EV_TX_TP_CM)
                                          && e->a == 19U) ? 1U : 0U));
        if (seen_end != 0U && seen_eom != 0U) {
            break;
        }
    }
    return n;
}

static void check_brm_sequence(const gbt_evlog_t *lg, uint8_t receiver)
{
    gbt_evlog_entry_t e[16];
    unsigned n = log_tp_session(lg, 0x000200u, e, 16U);
    unsigned i;
    uint8_t rx_cm = (uint8_t)GBT_EV_RX_TP_CM;
    uint8_t tx_cm = (uint8_t)GBT_EV_TX_TP_CM;
    uint8_t dt = (uint8_t)((receiver != 0U) ? GBT_EV_RX_TP_DT : GBT_EV_TX_TP_DT);
    CHECK_EQ_U(n, 11U);                                     /* RTS, CTS, DT 1..7, EOM_ACK, END */
    if (n != 11U) {
        return;
    }
    /* 1. RTS (vehicle -> charger), 49 bytes, 7 packets */
    CHECK_EQ_U(e[0].type, (receiver != 0U) ? rx_cm : tx_cm);
    CHECK_EQ_U(e[0].a, 16U);
    CHECK_EQ_U(e[0].b, 49U);
    CHECK_EQ_U(e[0].c, 0U);
    /* 2./3. CTS (charger -> vehicle): 7 packets granted from sequence 1 */
    CHECK_EQ_U(e[1].type, (receiver != 0U) ? tx_cm : rx_cm);
    CHECK_EQ_U(e[1].a, 17U);
    CHECK_EQ_U(e[1].b, 7U);
    CHECK_EQ_U(e[1].c, 1U);
    CHECK_EQ_U(e[1].pgn, 0x000200u);
    /* 5./6. DT 1..7 in order */
    for (i = 0U; i < 7U; i++) {
        CHECK_EQ_U(e[2U + i].type, dt);
        CHECK_EQ_U(e[2U + i].a, i + 1U);
        CHECK_EQ_U(e[2U + i].pgn, 0x000200u);
    }
    {
        /* 7./8./9. charger: reassembly complete (TP_END) then EOM_ACK sent;
         *          vehicle: EOM_ACK received then send session complete (TP_END) */
        const gbt_evlog_entry_t *eom = &e[(receiver != 0U) ? 10U : 9U];
        const gbt_evlog_entry_t *end = &e[(receiver != 0U) ? 9U : 10U];
        CHECK_EQ_U(eom->type, (receiver != 0U) ? tx_cm : rx_cm);
        CHECK_EQ_U(eom->a, 19U);
        CHECK_EQ_U(eom->b, 49U);
        CHECK_EQ_U(eom->pgn, 0x000200u);
        CHECK_EQ_U(end->type, GBT_EV_TP_END);
        CHECK_EQ_U(end->a, (receiver != 0U) ? GBT_TP_EV_RX_COMPLETE : GBT_TP_EV_TX_COMPLETE);
        CHECK_EQ_U(end->b, 7U);                             /* 7 of 7 DT */
        CHECK_EQ_U(end->c, 7U);
        CHECK_EQ_U(end->pgn, 0x000200u);
    }
    for (i = 1U; i < 11U; i++) {
        CHECK(e[i].t_ms >= e[i - 1U].t_ms);                /* processing order is time order */
    }
}

static void check_c24_normal(world_t *w, uint8_t first)
{
    static const uint8_t chg_steps[] = { 2U, 3U, 4U, 5U, 7U, 8U, 9U, 10U, 13U, 41U };
    static const uint8_t veh_steps[] = { 22U, 23U, 25U, 26U, 29U, 30U, 31U, 32U, 33U, 41U };
    uint8_t st[32];
    unsigned n;
    unsigned i;
    const gbt_tp_t *ct = &w->sil.node[0].tp;
    const gbt_tp_t *vt = &w->sil.node[1].tp;
    const gbt_integ_t *ci = &w->ig[0];
    const gbt_integ_t *vi = &w->ig[1];

    /* protocol step paths (derived from this run, then fixed here) */
    n = log_steps(&w->evlog[0], st, 32U);
    CHECK_EQ_U(n, sizeof chg_steps);
    CHECK(n == sizeof chg_steps && memcmp(st, chg_steps, n) == 0);
    printf("SIL C2.4 (order %u): charger steps", (unsigned)first);
    for (i = 0U; i < n; i++) {
        printf(" %u", (unsigned)st[i]);
    }
    n = log_steps(&w->evlog[1], st, 32U);
    CHECK_EQ_U(n, sizeof veh_steps);
    CHECK(n == sizeof veh_steps && memcmp(st, veh_steps, n) == 0);
    printf("; vehicle steps");
    for (i = 0U; i < n; i++) {
        printf(" %u", (unsigned)st[i]);
    }
    printf("\n");
    CHECK_EQ_U(ci->step_changes, sizeof chg_steps);
    CHECK_EQ_U(vi->step_changes, sizeof veh_steps);
    /* BRM TP sequence, both sides, from the event logs */
    check_brm_sequence(&w->evlog[0], 1U);
    check_brm_sequence(&w->evlog[1], 0U);
    /* per-message completions: exactly one BRM and one BCP, BCS = BCS RTS on the bus, nothing else */
    CHECK_EQ_U(ci->tp_rx_complete_msg[0], 1U);
    CHECK_EQ_U(ci->tp_rx_complete_msg[1], 1U);
    CHECK_EQ_U(ci->tp_rx_complete_msg[2], w->bus_rts_bcs);
    CHECK_EQ_U(ci->tp_rx_complete_msg[3], 0U);
    CHECK_EQ_U(vi->tp_tx_complete_msg[0], 1U);
    CHECK_EQ_U(vi->tp_tx_complete_msg[1], 1U);
    CHECK_EQ_U(vi->tp_tx_complete_msg[2], w->bus_rts_bcs);
    CHECK_EQ_U(vi->tp_tx_complete_msg[3], 0U);
    CHECK_EQ_U(ci->tp_tx_complete_msg[0] + ci->tp_tx_complete_msg[1] + ci->tp_tx_complete_msg[2]
               + ci->tp_tx_complete_msg[3], 0U);           /* the charger never sends TP data */
    CHECK_EQ_U(vi->tp_rx_complete_msg[0] + vi->tp_rx_complete_msg[1] + vi->tp_rx_complete_msg[2]
               + vi->tp_rx_complete_msg[3], 0U);
    CHECK(w->bus_rts_bcs >= 1U);
    /* BRM snapshots: frozen, 49 bytes, identical end to end, unchanged by BCP/BCS */
    CHECK_EQ_U(vt->snap_tx.state, GBT_TP_SNAP_FROZEN);
    CHECK_EQ_U(ct->snap_rx.state, GBT_TP_SNAP_FROZEN);
    CHECK_EQ_U(vt->snap_tx.len, 49U);
    CHECK_EQ_U(ct->snap_rx.len, 49U);
    CHECK_EQ_U(vt->snap_tx.captures, 1U);
    CHECK_EQ_U(ct->snap_rx.captures, 1U);
    CHECK(memcmp(vt->snap_tx.buf, ct->snap_rx.buf, 49U) == 0);
    CHECK_EQ_U(ct->rx.pgn, 0x001100u);                      /* live buffer now holds the last BCS */
    CHECK_EQ_U(ct->stats.last_rx_complete_pgn, 0x001100u);
    /* timing in the 1 ms SIL: 10 ms DT spacing, BRM (7 DT) is the longest session */
    CHECK_EQ_U(vt->stats.max_tx_dt_interval_ms, 10U);
    CHECK_EQ_U(ct->stats.max_rx_dt_interval_ms, 10U);
    CHECK(vt->stats.max_tx_duration_ms >= 60U && vt->stats.max_tx_duration_ms <= 62U);
    CHECK(ct->stats.max_rx_duration_ms >= 60U && ct->stats.max_rx_duration_ms <= 62U);
    CHECK_EQ_U(ct->stats.sessions_ended, ct->stats.rx_complete);
    CHECK_EQ_U(vt->stats.sessions_ended, vt->stats.tx_complete);
    CHECK_EQ_U(ct->rx.state, GBT_TP_IDLE);
    CHECK_EQ_U(ct->tx.state, GBT_TP_IDLE);
    CHECK_EQ_U(vt->rx.state, GBT_TP_IDLE);
    CHECK_EQ_U(vt->tx.state, GBT_TP_IDLE);
    /* event logs fit: not frozen, nothing dropped */
    CHECK_EQ_U(w->evlog[0].frozen, 0U);
    CHECK_EQ_U(w->evlog[1].frozen, 0U);
    CHECK_EQ_U(w->evlog[0].dropped, 0U);
    CHECK_EQ_U(w->evlog[1].dropped, 0U);
    printf("SIL C2.4 (order %u): completions charger rx BRM/BCP/BCS/other %lu/%lu/%lu/%lu, vehicle tx %lu/%lu/%lu/%lu; "
           "BRM snapshot 49 B identical; max DT interval tx %lu rx %lu ms; max session tx %lu rx %lu ms; "
           "event log entries charger %lu vehicle %lu\n", (unsigned)first,
           (unsigned long)ci->tp_rx_complete_msg[0], (unsigned long)ci->tp_rx_complete_msg[1],
           (unsigned long)ci->tp_rx_complete_msg[2], (unsigned long)ci->tp_rx_complete_msg[3],
           (unsigned long)vi->tp_tx_complete_msg[0], (unsigned long)vi->tp_tx_complete_msg[1],
           (unsigned long)vi->tp_tx_complete_msg[2], (unsigned long)vi->tp_tx_complete_msg[3],
           (unsigned long)vt->stats.max_tx_dt_interval_ms, (unsigned long)ct->stats.max_rx_dt_interval_ms,
           (unsigned long)vt->stats.max_tx_duration_ms, (unsigned long)ct->stats.max_rx_duration_ms,
           (unsigned long)w->evlog[0].count, (unsigned long)w->evlog[1].count);
}

static void scenario_normal_vehicle_stop(uint8_t first)
{
    world_t *w = &W;
    gbt_node_status_t cs;
    world_init(w, 520U, first);
    reach_charging(w);
    CHECK_EQ_U(w->plant[0].last.enabled, 1U);
    CHECK_EQ_U(w->plant[0].last.voltage_mv, 400000U);      /* BCL 400.0 V */
    CHECK_EQ_U(w->plant[0].last.current_ma, 100000U);      /* BCL 100 A, within CML and BCP limits */
    gbt_node_get_status(&w->sil.node[0], &cs);
    CHECK_EQ_U(cs.diag.transport_failures, 0U);            /* BRM, BCP, BCS via TP */
    sim_plant_arm_disable_capture(&w->plant[0]);
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(w->first_bst_ms != 0U);
    CHECK(w->plant[0].disable_seen == 1U);
    CHECK(latency(w->first_bst_ms, w->plant[0].first_disable_ms) <= 10U);   /* T-1 */
    CHECK_EQ_U(w->plant[0].last.enabled, 0U);
    CHECK_EQ_U(w->first_cst.data[0], 0x40U);               /* SPN3521 bits 7-8: vehicle stop (K-3) */
    CHECK_EQ_U(w->cem_count + w->bem_count, 0U);
    printf("SIL normal session + vehicle stop (order %u): T-1 latency %lu ms\n", (unsigned)first,
           (unsigned long)latency(w->first_bst_ms, w->plant[0].first_disable_ms));
    check_normal_session_tp_stats(w, first);               /* STEP 07-C2 */
    check_c24_normal(w, first);                            /* STEP 07-C2.4 */
    check_fi04(w, 0U, latency(w->first_bst_ms, w->plant[0].first_disable_ms), "vehicle stop (BST)");
}

static void scenario_charger_stop(void)
{
    world_t *w = &W;
    uint32_t t;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    t = w->sil.now_ms;
    gbt_node_request_stop(&w->sil.node[0], GBT_STOP_REASON_REQUEST, t);    /* T-2 */
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(latency(t, w->plant[0].first_disable_ms) <= 10U);
    CHECK(latency(t, w->first_cst_ms) <= 10U);
    CHECK(w->first_cst_ms >= w->plant[0].first_disable_ms);  /* output before CST */
    CHECK_EQ_U(w->bst_count, 5U);                          /* TM-06: 5..10 BST */
    CHECK_EQ_U(w->first_cst.data[0], 0x04U);               /* SPN3521 bits 3-4: manual stop */
    printf("SIL charger stop: T-2 latency %lu ms, BST frames %lu\n",
           (unsigned long)latency(t, w->plant[0].first_disable_ms), (unsigned long)w->bst_count);
    check_fi04(w, 0U, latency(t, w->plant[0].first_disable_ms), "charger stop");
}

static void scenario_bcl_timeout(void)
{
    world_t *w = &W;
    uint32_t deadline;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    w->drop_bcl = 1U;
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_TIMEOUT, 3000U) != 0xFFFFFFFFu);
    deadline = w->last_bcl_ms + 1000U;                     /* TO-14 */
    CHECK(latency(deadline, w->plant[0].first_disable_ms) <= 10U);     /* T-4 */
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_EXIT, 2000U) != 0xFFFFFFFFu);
    CHECK_EQ_U(w->cem_count, 4U);                          /* K-6A: 250 ms for 1000 ms */
    CHECK_EQ_U(w->first_cem.data[2], 0xC4U);               /* SPN3925 = 01 (K-3) */
    CHECK_EQ_U(phase(w, 1U), GBT_NODE_PHASE_EXIT);         /* vehicle: CEM received -> EXIT */
    CHECK_EQ_U(w->plant[0].last.enabled, 0U);
    printf("SIL BCL timeout: T-4 latency %lu ms after TO-14 deadline, CEM frames %lu\n",
           (unsigned long)latency(deadline, w->plant[0].first_disable_ms), (unsigned long)w->cem_count);
    check_fi04(w, 0U, latency(deadline, w->plant[0].first_disable_ms), "BCL timeout (TO-14)");
}

static void scenario_ccs_timeout(void)
{
    world_t *w = &W;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    w->drop_ccs = 1U;
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_TIMEOUT, 3000U) != 0xFFFFFFFFu);
    CHECK_EQ_U(w->first_bem.data[2], 0xF1U);               /* SPN3905 = 01 (K-3) */
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_EXIT, 100U) != 0xFFFFFFFFu);
    CHECK(latency(w->first_bem_ms, w->plant[0].first_disable_ms) <= 10U);
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_EXIT, 2000U) != 0xFFFFFFFFu);
    printf("SIL CCS timeout: charger output disabled %lu ms after BEM\n",
           (unsigned long)latency(w->first_bem_ms, w->plant[0].first_disable_ms));
    check_fi04(w, 0U, latency(w->first_bem_ms, w->plant[0].first_disable_ms), "CCS timeout (BEM)");
}

static void scenario_current_out_of_range(void)
{
    world_t *w = &W;
    gbt_can_frame_t f;
    uint32_t t;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    memset(&f, 0, sizeof f);
    f.id = ID_BCL;
    f.dlc = 5U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f.data[0] = 0xA0U; f.data[1] = 0x0FU;                  /* 400.0 V */
    f.data[2] = 0xA1U; f.data[3] = 0x0FU;                  /* raw 4001 = +0.1 A: outside -400..0 A */
    f.data[4] = 0x02U;
    t = w->sil.now_ms;
    CHECK(gbt_fake_hal_inject_rx(&w->sil.hal[0], &f) == GBT_HAL_OK);   /* T-3a */
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(latency(t, w->plant[0].first_disable_ms) <= 10U);
    CHECK_EQ_U(w->first_cst.data[0], 0x10U);               /* fault stop (S-1 PROVISIONAL) */
    CHECK_EQ_U(w->first_cst.data[2], 0x04U);               /* SPN3522 bits 11-12 other fault */
    printf("SIL out-of-range current: T-3a latency %lu ms\n",
           (unsigned long)latency(t, w->plant[0].first_disable_ms));
    check_fi04(w, 0U, latency(t, w->plant[0].first_disable_ms), "out-of-range current");
}

static void scenario_missing_tp_dt(void)
{
    world_t *w = &W;
    gbt_node_status_t cs;
    world_init(w, 1000U, 0U);
    w->drop_dt2_once = 1U;                                 /* first BRM transfer loses DT 2 */
    reach_charging(w);
    gbt_node_get_status(&w->sil.node[0], &cs);
    CHECK_EQ_U(w->dt_dropped, 1U);
    CHECK(cs.diag.transport_failures >= 1U);               /* sequence-gap abort */
    CHECK_EQ_U(w->cem_count, 0U);                          /* TP-6: no CEM from TP failure alone */
    CHECK_EQ_U(phase(w, 0U), GBT_NODE_PHASE_CHARGING);     /* TP-7: BRM resent next period */
    printf("SIL missing TP_DT: transport failures %lu, CEM %lu, session continued\n",
           (unsigned long)cs.diag.transport_failures, (unsigned long)w->cem_count);
    {   /* STEP 07-C2: the lost DT is visible in the TP statistics of both nodes */
        const gbt_tp_stats_t *v = &w->sil.node[1].tp.stats;
        const gbt_tp_stats_t *c = &w->sil.node[0].tp.stats;
        CHECK_EQ_U(c->abort_tx, 1U);                       /* charger: one sequence-gap abort */
        CHECK_EQ_U(c->last_abort_reason_tx, 7U);
        CHECK_EQ_U(c->timeout_rx, 0U);
        CHECK_EQ_U(v->abort_rx, 1U);                       /* vehicle: its BRM session ended by that abort */
        CHECK_EQ_U(v->last_abort_reason_rx, 7U);
        CHECK_EQ_U(v->timeout_tx, 0U);
        CHECK_EQ_U(v->abort_tx, 0U);
        CHECK_EQ_U(cs.diag.transport_failures, c->abort_tx);
        CHECK_EQ_U(w->bus_abort, 1U);
        CHECK(c->rx_complete >= 3U);                       /* BRM retried, BCP, BCS completed */
        CHECK_EQ_U(c->rx_complete, v->tx_complete);
        CHECK_EQ_U(c->rts_rx, v->rts_tx);
        /* every send session is either complete, aborted by the peer, timed out or still open */
        CHECK_EQ_U(v->tx_started, v->tx_complete + v->abort_rx + v->timeout_tx
                                  + gbt_tp_tx_busy(&w->sil.node[1].tp));
        printf("SIL missing TP_DT statistics: charger abort_tx %lu (reason %u), vehicle abort_rx %lu, "
               "completions %lu\n", (unsigned long)c->abort_tx, (unsigned)c->last_abort_reason_tx,
               (unsigned long)v->abort_rx, (unsigned long)c->rx_complete);
    }
}

/* ---- T-3c: BCL voltage demand at / below the charger minimum (AC-7, PT L426) ---- */
static void check_err4_clean(const gbt_node_status_t *st)
{
    CHECK_EQ_U(st->diag.timeouts, 0U);
    CHECK_EQ_U(st->diag.transport_failures, 0U);
    CHECK_EQ_U(st->diag.controller_failures, 0U);
    CHECK_EQ_U(st->diag.physical_failures, 0U);
    CHECK_EQ_U(st->diag.rx_invalid, 0U);
    CHECK_EQ_U(st->diag.output_failures, 0U);
}

static void scenario_voltage_min_boundary(void)
{
    world_t *w = &W;
    gbt_node_status_t cs;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    CHECK_EQ_U(w->cp.min_voltage_mv, 200000U);             /* project charger minimum (CML SPN2825) */

    /* At the minimum (200.0 V, raw 2000): not below -> charging continues at 200.0 V. */
    w->batt.voltage_mv = 200000U;
    gbt_sil_run(&w->sil, 1000U);                           /* ~20 BCL periods */
    CHECK_EQ_U(phase(w, 0U), GBT_NODE_PHASE_CHARGING);
    CHECK_EQ_U(phase(w, 1U), GBT_NODE_PHASE_CHARGING);
    CHECK_EQ_U(w->first_cst_ms, 0U);
    CHECK_EQ_U(w->first_low_bcl_ms, 0U);
    CHECK_EQ_U(w->plant[0].last.enabled, 1U);
    CHECK_EQ_U(w->plant[0].last.voltage_mv, 200000U);
    CHECK_EQ_U(w->plant[0].last.current_ma, 100000U);
    gbt_node_get_status(&w->sil.node[0], &cs);
    CHECK_EQ_U(cs.inhibit_latched, 0U);

    /* One resolution step below (199.9 V, raw 1999): PROT-7 -> CST, output disabled first. */
    sim_plant_arm_disable_capture(&w->plant[0]);
    w->batt.voltage_mv = 199900U;
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(w->first_low_bcl_ms != 0U);
    CHECK(w->plant[0].disable_seen == 1U);
    CHECK(latency(w->first_low_bcl_ms, w->plant[0].first_disable_ms) <= 10U);   /* SAF-1 */
    CHECK(w->first_cst_ms >= w->plant[0].first_disable_ms);                     /* output before CST */
    CHECK(latency(w->first_low_bcl_ms, w->first_cst_ms) <= 10U);
    CHECK_EQ_U(w->plant[0].last.enabled, 0U);
    CHECK_EQ_U(w->plant[0].last.voltage_mv, 0U);
    CHECK_EQ_U(w->plant[0].last.current_ma, 0U);
    CHECK_EQ_U(w->plant[0].max_current_ma, 100000U);       /* never above the BCL demand */
    /* CST (K-3 Candidate A; content P-S11-01 PROVISIONAL): SPN3521 bits 5-6 = 01 fault stop,
       SPN3522 = 0, SPN3523 bits 3-4 = 01 voltage abnormal; bits 7-8 of byte 4 are fill bits (1). */
    CHECK_EQ_U(w->first_cst.data[0], 0x10U);
    CHECK_EQ_U(w->first_cst.data[1], 0x00U);
    CHECK_EQ_U(w->first_cst.data[2], 0x00U);
    CHECK_EQ_U(w->first_cst.data[3], 0xC4U);
    CHECK_EQ_U(w->bst_count, 5U);                          /* charger-initiated stop: TM-06 */
    CHECK_EQ_U(w->cem_count + w->bem_count, 0U);
    gbt_node_get_status(&w->sil.node[0], &cs);
    CHECK_EQ_U(cs.inhibit_latched, 1U);
    CHECK_EQ_U(cs.diag.rx_overflow, 0U);
    check_err4_clean(&cs);
    printf("SIL T-3c voltage minimum: 200.0 V accepted for 1000 ms; 199.9 V -> output disabled %lu ms, "
           "CST %lu ms after the BCL; CST %02X %02X %02X %02X; BST frames %lu\n",
           (unsigned long)latency(w->first_low_bcl_ms, w->plant[0].first_disable_ms),
           (unsigned long)latency(w->first_low_bcl_ms, w->first_cst_ms),
           (unsigned)w->first_cst.data[0], (unsigned)w->first_cst.data[1],
           (unsigned)w->first_cst.data[2], (unsigned)w->first_cst.data[3], (unsigned long)w->bst_count);
    check_fi04(w, 0U, latency(w->first_low_bcl_ms, w->plant[0].first_disable_ms), "voltage minimum");
}

/* ---- T-5: RX queue overflow (SAF-1, AC-6, TST-3) ---- */
static void check_overflow_stop(world_t *w, uint32_t t, uint32_t lost, const char *what)
{
    gbt_node_status_t cs;
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(w->plant[0].disable_seen == 1U);
    CHECK(latency(t, w->plant[0].first_disable_ms) <= 10U);           /* SAF-1 */
    CHECK(w->first_cst_ms >= w->plant[0].first_disable_ms);           /* output before CST */
    CHECK(latency(t, w->first_cst_ms) <= 10U);
    CHECK_EQ_U(w->plant[0].last.enabled, 0U);
    CHECK_EQ_U(w->plant[0].last.voltage_mv, 0U);
    CHECK_EQ_U(w->plant[0].last.current_ma, 0U);
    /* CST (K-3 Candidate A; content P-S1-01 PROVISIONAL): SPN3521 bits 5-6 = 01 fault stop,
       SPN3522 bits 11-12 = 01 other fault, SPN3523 = 0; byte 4 bits 7-8 are fill bits (1). */
    CHECK_EQ_U(w->first_cst.data[0], 0x10U);
    CHECK_EQ_U(w->first_cst.data[1], 0x00U);
    CHECK_EQ_U(w->first_cst.data[2], 0x04U);
    CHECK_EQ_U(w->first_cst.data[3], 0xC0U);
    CHECK_EQ_U(w->bst_count, 5U);
    CHECK_EQ_U(w->cem_count + w->bem_count, 0U);
    gbt_node_get_status(&w->sil.node[0], &cs);
    CHECK_EQ_U(cs.diag.rx_overflow, lost);                 /* ERR-4: own class, exact count */
    CHECK_EQ_U(cs.inhibit_latched, 1U);
    check_err4_clean(&cs);
    printf("SIL T-5 %s: %lu frame(s) lost -> output disabled %lu ms, CST %lu ms after the loss\n", what,
           (unsigned long)lost, (unsigned long)latency(t, w->plant[0].first_disable_ms),
           (unsigned long)latency(t, w->first_cst_ms));
    check_fi04(w, (uint8_t)GBT_FI04_REF_OVERFLOW, latency(t, w->plant[0].first_disable_ms), what);
}

static void scenario_rx_queue_overflow(void)
{
    world_t *w = &W;
    gbt_can_frame_t f;
    uint32_t before;
    uint32_t t;
    unsigned i;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    memset(&f, 0, sizeof f);
    f.id = 0x18FF5601u;                                    /* not from the vehicle: ignored (§5.5) */
    f.dlc = 8U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    before = w->sil.hal[0].status.rx_overflow_sw;
    CHECK_EQ_U(before, 0U);
    t = w->sil.now_ms;
    for (i = 0U; i < GBT_FAKE_RXQ_LEN + 4U; i++) {         /* flood: 20 frames into a 16-entry queue */
        CHECK(gbt_fake_hal_inject_rx(&w->sil.hal[0], &f) == GBT_HAL_OK);
    }
    CHECK(w->sil.hal[0].status.rx_overflow_sw >= 4U);      /* real software queue drops */
    check_overflow_stop(w, t, w->sil.hal[0].status.rx_overflow_sw - before, "software RX queue full");
}

static void scenario_rx_hw_overflow(void)
{
    world_t *w = &W;
    uint32_t t;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    t = w->sil.now_ms;
    CHECK(gbt_fake_hal_inject_overflow(&w->sil.hal[0], 1U) == GBT_HAL_OK);   /* controller FIFO loss */
    check_overflow_stop(w, t, 1U, "hardware FIFO overflow");
}

/* Outside the charging phase: counted only, no stop and no inhibit latch ("while charging",
   SAF-1; RQ-04 OPEN). Vehicle-side overflow: counted only (see report, limitation). */
static void scenario_rx_overflow_not_charging(void)
{
    world_t *w = &W;
    gbt_node_status_t cs;
    gbt_node_status_t vs;
    world_init(w, 1000U, 0U);
    gbt_sil_run(&w->sil, 50U);                             /* handshake in progress */
    CHECK(phase(w, 0U) != GBT_NODE_PHASE_CHARGING);
    CHECK(gbt_fake_hal_inject_overflow(&w->sil.hal[0], 0U) == GBT_HAL_OK);
    reach_charging(w);
    CHECK_EQ_U(w->plant[0].last.enabled, 1U);
    CHECK_EQ_U(w->plant[0].last.current_ma, 100000U);
    CHECK_EQ_U(w->first_cst_ms, 0U);
    gbt_node_get_status(&w->sil.node[0], &cs);
    CHECK_EQ_U(cs.diag.rx_overflow, 1U);
    CHECK_EQ_U(cs.inhibit_latched, 0U);
    check_err4_clean(&cs);
    CHECK(gbt_fake_hal_inject_overflow(&w->sil.hal[1], 1U) == GBT_HAL_OK);   /* vehicle side */
    gbt_sil_run(&w->sil, 20U);
    gbt_node_get_status(&w->sil.node[1], &vs);
    CHECK_EQ_U(vs.diag.rx_overflow, 1U);
    printf("SIL T-5 outside charging: charger overflow counted (%lu), no stop, charging reached; "
           "vehicle overflow counted (%lu), vehicle phase %u\n", (unsigned long)cs.diag.rx_overflow,
           (unsigned long)vs.diag.rx_overflow, (unsigned)vs.phase);
}

/* STEP 07-C2: the event levels are distinct. While the vehicle's CAN driver refuses every frame
 * (E_IO), the TP layer still queues an RTS (rts_tx, level 2) but the driver-acceptance counters
 * (level 3) must not move, and the refused frames are counted as dropped. */
static void scenario_driver_refusal_not_counted(void)
{
    world_t *w = &W;
    const gbt_tp_stats_t *v;
    uint32_t acc0;
    uint32_t cm0;
    uint32_t dt0;
    uint32_t drop0;
    uint32_t rts0;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    v = &w->sil.node[1].tp.stats;
    acc0 = w->ig[1].tx_hal_accepted;
    cm0 = w->ig[1].tx_hal_accepted_tp_cm;
    dt0 = w->ig[1].tx_hal_accepted_tp_dt;
    drop0 = w->ig[1].tx_dropped;
    rts0 = v->rts_tx;
    CHECK(gbt_fake_hal_set_tx_status(&w->sil.hal[1], GBT_HAL_E_IO) == GBT_HAL_OK);
    gbt_sil_run(&w->sil, 300U);                            /* > one BCS period (250 ms) */
    CHECK(v->rts_tx > rts0);                               /* TP layer queued an RTS ... */
    CHECK_EQ_U(w->ig[1].tx_hal_accepted, acc0);            /* ... but the driver accepted nothing */
    CHECK_EQ_U(w->ig[1].tx_hal_accepted_tp_cm, cm0);
    CHECK_EQ_U(w->ig[1].tx_hal_accepted_tp_dt, dt0);
    CHECK(w->ig[1].tx_dropped > drop0);                    /* refused frames are counted as dropped */
    {
        uint32_t win_rts = v->rts_tx - rts0;
        uint32_t win_acc = w->ig[1].tx_hal_accepted - acc0;
        uint32_t win_drop = w->ig[1].tx_dropped - drop0;
        CHECK(gbt_fake_hal_set_tx_status(&w->sil.hal[1], GBT_HAL_OK) == GBT_HAL_OK);
        gbt_sil_run(&w->sil, 3000U);
        CHECK(w->ig[1].tx_hal_accepted > acc0);            /* acceptance resumes */
        CHECK(w->ig[1].tx_hal_accepted_tp_cm > cm0);
        printf("SIL driver refusal (300 ms window): TP RTS queued +%lu, driver accepted +%lu, dropped +%lu; "
               "3 s after recovery: accepted +%lu, sender TP timeouts %lu\n", (unsigned long)win_rts,
               (unsigned long)win_acc, (unsigned long)win_drop,
               (unsigned long)(w->ig[1].tx_hal_accepted - acc0), (unsigned long)v->timeout_tx);
    }
}

/* ---- STEP 07-C2.4: start order (hardware procedure basis) ----
 * A: vehicle (F103 B) running first, charger (G474) started 30 s later -> complete session.
 *    The vehicle waits in V02 without transmitting (no wait timer, gbt_vehicle_sm.c).
 * B: charger first, vehicle 12 s later (after the 10 s CHM window): the charger takes the
 *    compatibility path (CRM 00, 5 s), sends CEM; the vehicle, still in V02, receives the CEM
 *    and goes to EXIT (phase 7, step 42): the state observed on hardware before STEP 07-C2.4.
 *    EXIT is terminal in both main.c files: only a reset recovers. */
static void scenario_start_order(void)
{
    world_t *w = &W;
    gbt_node_status_t cs;
    gbt_node_status_t vs;
    /* A */
    world_init_ex(w, 520U, 0U, 2U);
    gbt_sil_run(&w->sil, 30000U);
    gbt_node_get_status(&w->sil.node[1], &vs);
    CHECK_EQ_U(vs.step, 22U);                              /* V02, waiting */
    CHECK_EQ_U(vs.diag.tx_frames, 0U);                     /* silent while waiting */
    gbt_node_set_connected(&w->sil.node[0], 1U, w->sil.now_ms);
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 30000U) != 0xFFFFFFFFu);
    CHECK(run_until_phase(w, 1U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    gbt_sil_run(&w->sil, 2000U);
    gbt_node_get_status(&w->sil.node[0], &cs);
    gbt_node_get_status(&w->sil.node[1], &vs);
    CHECK_EQ_U(cs.step, 41U);
    CHECK_EQ_U(vs.step, 41U);
    CHECK_EQ_U(w->ig[0].tp_rx_complete_msg[0], 1U);
    CHECK_EQ_U(w->ig[0].tp_rx_complete_msg[1], 1U);
    CHECK(w->ig[0].tp_rx_complete_msg[2] >= 1U);
    CHECK_EQ_U(cs.diag.transport_failures + vs.diag.transport_failures, 0U);
    printf("SIL start order A (vehicle first, charger +30 s): both END (steps %u/%u), TP BRM/BCP/BCS %lu/%lu/%lu\n",
           (unsigned)cs.step, (unsigned)vs.step, (unsigned long)w->ig[0].tp_rx_complete_msg[0],
           (unsigned long)w->ig[0].tp_rx_complete_msg[1], (unsigned long)w->ig[0].tp_rx_complete_msg[2]);
    /* B */
    world_init_ex(w, 520U, 0U, 1U);
    gbt_sil_run(&w->sil, 12000U);
    gbt_node_get_status(&w->sil.node[0], &cs);
    CHECK_EQ_U(cs.step, 4U);                               /* C04: CRM 00 (compatibility path) */
    gbt_node_set_connected(&w->sil.node[1], 1U, w->sil.now_ms);
    gbt_sil_run(&w->sil, 8000U);
    gbt_node_get_status(&w->sil.node[0], &cs);
    gbt_node_get_status(&w->sil.node[1], &vs);
    CHECK_EQ_U(vs.phase, GBT_NODE_PHASE_EXIT);
    CHECK_EQ_U(vs.step, 42U);
    CHECK_EQ_U(w->ig[1].last_rx.id, ID_CEM);               /* 0x081FF456, as observed on hardware */
    CHECK_EQ_U(vs.diag.tx_frames, 0U);                     /* the vehicle never transmitted */
    CHECK_EQ_U(cs.step, 42U);
    CHECK_EQ_U(w->ig[0].rx_seq, 0U);                       /* charger received nothing */
    CHECK(w->cem_count >= 1U);
    printf("SIL start order B (charger first, vehicle +12 s): vehicle phase %u step %u last RX %08lX, "
           "charger step %u rx %lu (reproduces the pre-C2.4 hardware observation)\n", (unsigned)vs.phase,
           (unsigned)vs.step, (unsigned long)w->ig[1].last_rx.id, (unsigned)cs.step,
           (unsigned long)w->ig[0].rx_seq);
}

/* ---- STEP 07 FI-04: shutdown response-time bookkeeping, unit level (pure functions) ---- */
static gbt_output_cmd_t fi04_cmd(uint32_t seq, uint8_t enabled)
{
    gbt_output_cmd_t c;
    memset(&c, 0, sizeof c);
    c.seq = seq;
    c.enabled = enabled;
    if (enabled != 0U) {
        c.voltage_mv = 400000U;
        c.current_ma = 100000U;
    }
    return c;
}

static void fi04_apply(gbt_fi04_t *m, uint32_t seq, uint8_t enabled, gbt_hal_status_t st,
                       uint32_t confirmed, uint32_t t1)
{
    gbt_output_cmd_t c = fi04_cmd(seq, enabled);
    gbt_fi04_on_apply(m, &c, st, confirmed, t1);
}

/* attach-like history: pass at t (initial disabled seq 1, enabled seq 2 accepted), pass at t+1.
 * The next pass_begin(t+2) then has a valid reference T0 = t+1. */
static void fi04_setup(gbt_fi04_t *m, uint32_t t)
{
    memset(m, 0, sizeof *m);
    gbt_fi04_pass_begin(m, t);
    fi04_apply(m, 1U, 0U, GBT_HAL_OK, 1U, t);
    fi04_apply(m, 2U, 1U, GBT_HAL_OK, 2U, t);
    gbt_fi04_pass_end(m);
    gbt_fi04_pass_begin(m, t + 1U);
    gbt_fi04_pass_end(m);
    CHECK_EQ_U(m->shutdowns, 0U);                          /* initial disabled command not counted */
    CHECK_EQ_U(m->acc_enabled, 1U);
}

static void scenario_fi04_unit(void)
{
    gbt_fi04_t m;

    /* 1. enabled -> disabled transition */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1003U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.shutdowns, 1U);
    CHECK_EQ_U(m.open, 0U);
    CHECK_EQ_U(m.t0_ms, 1001U);
    CHECK_EQ_U(m.t1_ms, 1003U);
    CHECK_EQ_U(m.elapsed_ms, 2U);
    CHECK_EQ_U(m.first_seq, 3U);
    CHECK_EQ_U(m.accept_seq, 3U);
    CHECK_EQ_U(m.confirmed_seq, 3U);
    CHECK_EQ_U(m.apply_status, GBT_HAL_OK);
    CHECK_EQ_U(m.ref_flags, 0U);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_PASS);
    CHECK_EQ_U(m.pass_count, 1U);
    CHECK_EQ_U(m.acc_enabled, 0U);

    /* 2. repeated disabled command: same sequence again and a further disabled sequence */
    gbt_fi04_pass_begin(&m, 1004U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1004U);
    fi04_apply(&m, 4U, 0U, GBT_HAL_OK, 4U, 1004U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.shutdowns, 1U);
    CHECK_EQ_U(m.pass_count, 1U);
    CHECK_EQ_U(m.open, 0U);

    /* 3. sink error: never counted; stays open; the accepted retry closes it (latency includes it) */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_E_IO, 2U, 1002U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.open, 1U);
    CHECK_EQ_U(m.shutdowns, 0U);
    CHECK_EQ_U(m.refused, 1U);
    CHECK_EQ_U(m.apply_status, GBT_HAL_E_IO);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_NONE);
    CHECK_EQ_U(m.acc_enabled, 1U);                         /* sink still holds the enabled command */
    gbt_fi04_pass_begin(&m, 1003U);
    fi04_apply(&m, 4U, 0U, GBT_HAL_E_IO, 2U, 1003U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.refused, 2U);
    CHECK_EQ_U(m.shutdowns, 0U);
    gbt_fi04_pass_begin(&m, 1005U);
    fi04_apply(&m, 5U, 0U, GBT_HAL_OK, 5U, 1005U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.shutdowns, 1U);
    CHECK_EQ_U(m.first_seq, 3U);
    CHECK_EQ_U(m.accept_seq, 5U);
    CHECK_EQ_U(m.t0_ms, 1001U);                            /* reference of the FIRST delivery */
    CHECK_EQ_U(m.elapsed_ms, 4U);
    CHECK_EQ_U(m.apply_status, GBT_HAL_OK);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_PASS);

    /* 3b. open measurement superseded by an accepted enabled command: aborted, not counted */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_E_IO, 2U, 1002U);
    fi04_apply(&m, 4U, 1U, GBT_HAL_OK, 4U, 1002U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.aborted, 1U);
    CHECK_EQ_U(m.open, 0U);
    CHECK_EQ_U(m.shutdowns, 0U);

    /* 4. sequence mismatch: GBT_HAL_OK but the node did not confirm that sequence */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 2U, 1002U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.mismatch, 1U);
    CHECK_EQ_U(m.shutdowns, 0U);
    CHECK_EQ_U(m.open, 1U);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_NONE);
    gbt_fi04_pass_begin(&m, 1003U);
    fi04_apply(&m, 4U, 0U, GBT_HAL_OK, 4U, 1003U);         /* a later confirmed disabled command */
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.shutdowns, 1U);
    CHECK_EQ_U(m.accept_seq, 4U);

    /* 5. invalid T0: no previous pass */
    memset(&m, 0, sizeof m);
    fi04_apply(&m, 2U, 1U, GBT_HAL_OK, 2U, 500U);          /* outside any pass, before the first one */
    gbt_fi04_pass_begin(&m, 501U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 501U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.shutdowns, 1U);
    CHECK_EQ_U(m.ref_flags, GBT_FI04_REF_NO_PREV);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_INVALID);
    CHECK_EQ_U(m.invalid_count, 1U);
    CHECK_EQ_U(m.pass_count + m.fail_count, 0U);
    CHECK_EQ_U(m.max_valid_ms, 0U);
    memset(&m, 0, sizeof m);                               /* outside a pass with no pass at all */
    fi04_apply(&m, 2U, 1U, GBT_HAL_OK, 2U, 500U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 500U);
    CHECK_EQ_U(m.ref_flags, GBT_FI04_REF_NO_PREV);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_INVALID);
    fi04_setup(&m, 1000U);                                 /* outside a pass after passes: T0 = last pass start */
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1004U);
    CHECK_EQ_U(m.t0_ms, 1001U);
    CHECK_EQ_U(m.elapsed_ms, 3U);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_PASS);

    /* 6. RX frame limit in the previous pass -> invalid; in the current pass only -> still valid */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    gbt_fi04_pass_note(&m, (uint8_t)GBT_FI04_REF_RX_LIMIT);
    gbt_fi04_pass_end(&m);
    gbt_fi04_pass_begin(&m, 1003U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1003U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.ref_flags, GBT_FI04_REF_RX_LIMIT);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_INVALID);
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1002U);         /* frame popped before the limit was reached */
    gbt_fi04_pass_note(&m, (uint8_t)GBT_FI04_REF_RX_LIMIT);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.ref_flags, 0U);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_PASS);

    /* 7. unexpected RX HAL error (not GBT_HAL_E_EMPTY) in the previous pass -> invalid */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    gbt_fi04_pass_note(&m, (uint8_t)GBT_FI04_REF_RX_ERROR);
    gbt_fi04_pass_end(&m);
    gbt_fi04_pass_begin(&m, 1003U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1003U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.ref_flags, GBT_FI04_REF_RX_ERROR);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_INVALID);

    /* 8. RX overflow: previous pass, or current pass before the command -> invalid; status read failure too */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    gbt_fi04_pass_note(&m, (uint8_t)GBT_FI04_REF_OVERFLOW);
    gbt_fi04_pass_end(&m);
    gbt_fi04_pass_begin(&m, 1003U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1003U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.ref_flags, GBT_FI04_REF_OVERFLOW);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_INVALID);
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    gbt_fi04_pass_note(&m, (uint8_t)GBT_FI04_REF_OVERFLOW);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1002U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.ref_flags, GBT_FI04_REF_OVERFLOW);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_INVALID);
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    gbt_fi04_pass_note(&m, (uint8_t)GBT_FI04_REF_STATUS_ERR);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1002U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.ref_flags, GBT_FI04_REF_STATUS_ERR);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_INVALID);

    /* 9. timestamp wraparound */
    fi04_setup(&m, 0xFFFFFFFDu);                           /* passes at ...FD, ...FE */
    gbt_fi04_pass_begin(&m, 0xFFFFFFFFu);
    gbt_fi04_pass_end(&m);
    gbt_fi04_pass_begin(&m, 0x00000001u);                  /* T0 = 0xFFFFFFFF */
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 0x00000002u);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.t0_ms, 0xFFFFFFFFu);
    CHECK_EQ_U(m.t1_ms, 0x00000002u);
    CHECK_EQ_U(m.elapsed_ms, 3U);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_PASS);

    /* 10. 9 ms boundary: PASS */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1010U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.elapsed_ms, 9U);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_PASS);
    CHECK_EQ_U(m.pass_count, 1U);
    CHECK_EQ_U(m.max_valid_ms, 9U);

    /* 11. 10 ms boundary: FAIL */
    fi04_setup(&m, 1000U);
    gbt_fi04_pass_begin(&m, 1002U);
    fi04_apply(&m, 3U, 0U, GBT_HAL_OK, 3U, 1011U);
    gbt_fi04_pass_end(&m);
    CHECK_EQ_U(m.elapsed_ms, 10U);
    CHECK_EQ_U(m.verdict, GBT_FI04_VERDICT_FAIL);
    CHECK_EQ_U(m.fail_count, 1U);
    CHECK_EQ_U(m.pass_count, 0U);
    CHECK_EQ_U(m.max_valid_ms, 10U);
    printf("FI-04 unit: transition, repeat, sink error, abort, seq mismatch, no-prev / RX-limit / RX-error / "
           "overflow / status references, wrap, 9 ms PASS, 10 ms FAIL checked\n");
}

/* ---- STEP 07 FI-04: integration level (gbt_integ_step + fake HAL + sim plant) ---- */
/* Repeated stop requests produce one disabled sequence and one counted shutdown. */
static void scenario_fi04_repeated_stop(void)
{
    world_t *w = &W;
    uint32_t t;
    uint32_t seq;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    t = w->sil.now_ms;
    gbt_node_request_stop(&w->sil.node[0], GBT_STOP_REASON_REQUEST, t);
    gbt_node_request_stop(&w->sil.node[0], GBT_STOP_REASON_REQUEST, t);
    gbt_sil_run(&w->sil, 5U);
    seq = w->sil.node[0].out_cmd.seq;
    gbt_node_request_stop(&w->sil.node[0], GBT_STOP_REASON_REQUEST, w->sil.now_ms);
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    gbt_sil_run(&w->sil, 2000U);
    CHECK_EQ_U(w->sil.node[0].out_cmd.seq, seq);           /* no new command after the first disable */
    CHECK_EQ_U(w->ig[0].output_shutdowns, 1U);
    check_fi04(w, 0U, latency(t, w->plant[0].first_disable_ms), "repeated stop requests");
}

/* The sink refuses the disabled command for 20 ms: no shutdown is counted while refused; the
 * accepted retry closes the measurement and the 20 ms are inside the latency -> FAIL. */
static void scenario_fi04_sink_refusal(void)
{
    world_t *w = &W;
    const gbt_fi04_t *m = &w->ig[0].fi04;
    uint32_t t;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    w->plant[0].reply = GBT_HAL_E_IO;
    t = w->sil.now_ms;
    gbt_node_request_stop(&w->sil.node[0], GBT_STOP_REASON_REQUEST, t);
    gbt_sil_run(&w->sil, 20U);
    CHECK_EQ_U(m->open, 1U);
    CHECK_EQ_U(m->shutdowns, 0U);
    CHECK(m->refused >= 1U);
    CHECK_EQ_U(m->apply_status, GBT_HAL_E_IO);
    CHECK_EQ_U(m->verdict, GBT_FI04_VERDICT_NONE);
    CHECK_EQ_U(w->plant[0].disable_seen, 0U);
    w->plant[0].reply = GBT_HAL_OK;
    gbt_sil_run(&w->sil, 2000U);
    CHECK_EQ_U(m->open, 0U);
    CHECK_EQ_U(m->shutdowns, 1U);
    CHECK(m->accept_seq > m->first_seq);                   /* accepted on a retry sequence */
    CHECK_EQ_U(m->accept_seq, m->confirmed_seq);
    CHECK_EQ_U(m->apply_status, GBT_HAL_OK);
    CHECK_EQ_U(m->t1_ms, w->plant[0].first_disable_ms);
    CHECK(m->elapsed_ms >= 20U);
    CHECK(m->elapsed_ms >= latency(t, w->plant[0].first_disable_ms));
    CHECK_EQ_U(m->ref_flags, 0U);
    CHECK_EQ_U(m->verdict, GBT_FI04_VERDICT_FAIL);
    CHECK_EQ_U(m->fail_count, 1U);
    CHECK_EQ_U(m->pass_count, 0U);
    printf("SIL FI-04 sink refusal 20 ms: refused %lu, first seq %lu, accepted seq %lu, elapsed %lu ms, "
           "verdict %u (FAIL expected); legacy output_shutdowns %lu\n", (unsigned long)m->refused,
           (unsigned long)m->first_seq, (unsigned long)m->accept_seq, (unsigned long)m->elapsed_ms,
           (unsigned)m->verdict, (unsigned long)w->ig[0].output_shutdowns);
}

/* The pass before the stop hit GBT_INTEG_RX_PER_STEP (16 frames, no overflow): reference invalid. */
static void scenario_fi04_rx_limit(void)
{
    world_t *w = &W;
    gbt_can_frame_t f;
    uint32_t t;
    uint32_t n;
    uint32_t i;
    world_init(w, 1000U, 0U);                              /* order: charger first in every tick */
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    memset(&f, 0, sizeof f);
    f.id = 0x18FF5601u;                                    /* not from the vehicle: ignored (§5.5) */
    f.dlc = 8U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    n = GBT_FAKE_RXQ_LEN - w->sil.hal[0].rx_count;         /* fill the queue exactly, no overflow */
    for (i = 0U; i < n; i++) {
        CHECK(gbt_fake_hal_inject_rx(&w->sil.hal[0], &f) == GBT_HAL_OK);
    }
    CHECK_EQ_U(w->sil.hal[0].rx_count, GBT_INTEG_RX_PER_STEP);
    gbt_sil_tick(&w->sil);                                 /* charger pass pops 16 frames: limit reached */
    CHECK_EQ_U(w->sil.hal[0].status.rx_overflow_sw + w->sil.hal[0].status.rx_overflow_hw, 0U);
    CHECK_EQ_U(w->ig[0].fi04.cur_ref_flags, GBT_FI04_REF_RX_LIMIT);
    t = w->sil.now_ms;
    gbt_node_request_stop(&w->sil.node[0], GBT_STOP_REASON_REQUEST, t);
    CHECK(run_until_phase(w, 0U, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    check_fi04(w, (uint8_t)GBT_FI04_REF_RX_LIMIT, latency(t, w->plant[0].first_disable_ms), "RX frame limit");
}

/* T1 freshness: a test sink that delegates to the sim plant and then lets 3 ms of virtual time
 * pass for a disabled command (a slow backend). T1 must include those 3 ms; the plant itself
 * timestamps on entry. A T1 taken from the clock sampled before the pass would not. */
static gbt_output_sink_t g_fi04_inner;
static uint32_t         *g_fi04_clock;

static gbt_hal_status_t fi04_slow_apply(void *ctx, const gbt_output_cmd_t *cmd)
{
    gbt_hal_status_t st = g_fi04_inner.apply(ctx, cmd);
    if (cmd->enabled == 0U) {
        *g_fi04_clock += 3U;                               /* time spent inside the backend */
    }
    return st;
}

static void scenario_fi04_t1_fresh(void)
{
    world_t *w = &W;
    const gbt_fi04_t *m = &w->ig[0].fi04;
    uint32_t t;
    world_init(w, 1000U, 0U);
    reach_charging(w);
    sim_plant_arm_disable_capture(&w->plant[0]);
    g_fi04_inner = w->sil.hal[0].sink;                     /* fake HAL test hook: wrap the bound sink */
    g_fi04_clock = &w->sil.now_ms;
    w->sil.hal[0].sink.apply = fi04_slow_apply;
    t = w->sil.now_ms;
    gbt_node_request_stop(&w->sil.node[0], GBT_STOP_REASON_REQUEST, t);
    gbt_sil_tick(&w->sil);
    w->sil.hal[0].sink = g_fi04_inner;
    CHECK_EQ_U(m->shutdowns, 1U);
    CHECK_EQ_U(m->t1_ms, w->plant[0].first_disable_ms + 3U);   /* read AFTER the sink returned */
    CHECK_EQ_U(m->t0_ms, t);
    CHECK_EQ_U(m->elapsed_ms, 4U);                         /* 1 ms tick + 3 ms in the backend */
    CHECK_EQ_U(m->verdict, GBT_FI04_VERDICT_PASS);
    printf("SIL FI-04 T1 freshness: plant accepted at %lu, T1 %lu (3 ms backend time included), elapsed %lu ms\n",
           (unsigned long)w->plant[0].first_disable_ms, (unsigned long)m->t1_ms, (unsigned long)m->elapsed_ms);
}

int main(void)
{
    scenario_normal_vehicle_stop(0U);
    scenario_normal_vehicle_stop(1U);                       /* swapped execution order */
    scenario_charger_stop();
    scenario_bcl_timeout();
    scenario_ccs_timeout();
    scenario_current_out_of_range();
    scenario_missing_tp_dt();
    scenario_voltage_min_boundary();                        /* T-3c */
    scenario_rx_queue_overflow();                           /* T-5 software queue */
    scenario_rx_hw_overflow();                              /* T-5 hardware FIFO */
    scenario_rx_overflow_not_charging();                    /* T-5 outside charging */
    scenario_driver_refusal_not_counted();                  /* STEP 07-C2 event levels */
    scenario_start_order();                 /* STEP 07-C2.4 */
    scenario_fi04_unit();                                   /* STEP 07 FI-04 */
    scenario_fi04_repeated_stop();
    scenario_fi04_sink_refusal();
    scenario_fi04_rx_limit();
    scenario_fi04_t1_fresh();
    return TEST_SUMMARY("test_sil_session");
}
