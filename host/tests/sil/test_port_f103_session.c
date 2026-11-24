/*
 * tests/sil/test_port_f103_session.c - protocol integration through the F103 bxCAN adapter
 * (STEP 07-C2.1). The VEHICLE runs exactly as planned for the target: battery model ->
 * gbt_node_set_vehicle_demand -> gbt_integ_step() over ports/stm32/f103 (bxCAN fake below).
 * The CHARGER is a bare gbt_node driven through its public API (one concrete gbt_hal type per
 * executable). Bus model per 1 ms: charger frames -> fake_f1_bus_rx() + fake_f1_irq() each;
 * four main-loop passes per ms, each followed by bus completion of the pending mailbox
 * (single mailbox in flight, D1); transmitted vehicle frames are delivered to the charger.
 * Scenarios: normal session with vehicle SOC stop; charger-initiated stop.
 * Host evidence only: no hardware, no real timing.
 */
#include "ports/stm32/f103/port_f103.h"
#include "fake_stm32f1.h"
#include "app/battery_model.h"
#include "app/gbt_integration.h"
#include "app/sim_plant.h"
#include "protocol/gbt_node.h"
#include "tests/support/test_assert.h"

#include <stdio.h>
#include <string.h>

#define ID_TPCM_V2C 0x1CEC56F4u
#define ID_TPDT_V2C 0x1CEB56F4u
#define ID_TPCM_C2V 0x1CECF456u
#define ID_BST      0x101956F4u

static CAN_HandleTypeDef H;
static gbt_hal_t P;
static gbt_integ_t IG;
static gbt_node_t VEH;
static gbt_node_t CHG;
static output_if_backend_t PLANT;
static battery_model_t BATT;
static gbt_charger_params_t CP;
static gbt_vehicle_params_t VP;
static gbt_node_config_t CC;
static gbt_node_config_t VC;
static uint32_t c_applied_seq;
static uint8_t v_stop_sent;
static uint32_t frames_v2c, frames_c2v, rts_v2c, cts_c2v, eom_c2v, bst_count, dt_order_errors;
static uint8_t dt_expect;            /* next expected TP.DT sequence number (0 = none open) */
static uint8_t dt_total;
static gbt_evlog_t EVLOG;                                  /* STEP 07-C2.4 */

static uint8_t phase(const gbt_node_t *n)
{
    gbt_node_status_t st;
    gbt_node_get_status(n, &st);
    return st.phase;
}

static void charger_step(uint32_t now)
{
    gbt_output_cmd_t cmd;
    gbt_output_fb_t fb;
    gbt_can_frame_t f;
    unsigned guard;
    gbt_node_tick(&CHG, now);
    gbt_node_get_output(&CHG, &cmd);
    if (cmd.seq != c_applied_seq) {                       /* ideal output stage */
        c_applied_seq = cmd.seq;
        gbt_node_confirm_output(&CHG, cmd.seq, true, now);
    }
    fb.valid = 1U;
    fb.voltage_mv = cmd.enabled != 0U ? cmd.voltage_mv : 0U;
    fb.current_ma = cmd.enabled != 0U ? cmd.current_ma : 0U;
    fb.t_ms = now;
    gbt_node_set_feedback(&CHG, &fb);
    for (guard = 0U; guard < 32U && gbt_node_pop_tx(&CHG, &f); guard++) {
        if (gbt_node_tx_check(&CHG, &f) != GBT_NODE_TX_SEND) {
            continue;                                     /* charger output always accepted */
        }
        if (f.id == ID_TPCM_C2V) {
            cts_c2v += (f.data[0] == 17U) ? 1U : 0U;
            eom_c2v += (f.data[0] == 19U) ? 1U : 0U;
        }
        CHECK_EQ_U(fake_f1_bus_rx(f.id, 1U, 0U, f.dlc, f.data), 1);
        fake_f1_irq();                                    /* RX interrupt per frame */
        frames_c2v++;
    }
}

static void vehicle_to_charger(uint32_t now)
{
    fake_f1_tx_t t;
    while (fake_f1_tx_pop(&t) != 0) {
        gbt_can_frame_t f;
        CHECK_EQ_U(t.ide, 1U);
        CHECK_EQ_U(t.rtr, 0U);
        f.id = t.id;
        f.dlc = t.dlc;
        f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
        memcpy(f.data, t.data, 8U);
        if (f.id == ID_TPCM_V2C && f.data[0] == 16U) {   /* RTS opens a transfer */
            rts_v2c++;
            dt_expect = 1U;
            dt_total = f.data[3];
        } else if (f.id == ID_TPDT_V2C) {
            if (dt_expect == 0U || f.data[0] != dt_expect) {
                dt_order_errors++;
            }
            dt_expect = (dt_expect != 0U && dt_expect < dt_total) ? (uint8_t)(dt_expect + 1U) : 0U;
        } else if (f.id == ID_BST) {
            bst_count++;
        }
        gbt_node_on_rx(&CHG, &f, now);
        frames_v2c++;
    }
}

static void tick(void)
{
    uint32_t now;
    unsigned pass;
    gbt_vehicle_demand_t d;
    g_fake_f1.tick++;
    now = g_fake_f1.tick;
    charger_step(now);
    for (pass = 0U; pass < 4U; pass++) {                  /* main loop runs faster than 1 kHz */
        if (battery_model_step(&BATT, now, (uint8_t)(phase(&VEH) == GBT_NODE_PHASE_CHARGING), &d) != 0U
            && phase(&VEH) == GBT_NODE_PHASE_CHARGING && v_stop_sent == 0U) {
            v_stop_sent = 1U;
            gbt_node_request_stop(&VEH, GBT_STOP_REASON_SOC_TARGET, now);
        }
        gbt_node_set_vehicle_demand(&VEH, &d);
        gbt_integ_step(&IG);
        (void)fake_f1_bus_tx(1U);                         /* bus completes the mailbox */
        vehicle_to_charger(now);
    }
}

static uint32_t run_until(const gbt_node_t *n, uint8_t ph, uint32_t max_ms)
{
    uint32_t t;
    for (t = 0U; t < max_ms; t++) {
        if (phase(n) == ph) {
            return t;
        }
        tick();
    }
    return 0xFFFFFFFFu;
}

static void start(uint16_t soc_target)
{
    gbt_port_f103_cfg_t pc;
    gbt_output_sink_t sink;
    gbt_vehicle_demand_t d;
    fake_f1_reset(&H);
    g_fake_f1.tick = 200000U;
    c_applied_seq = 0U;
    v_stop_sent = 0U;
    frames_v2c = frames_c2v = rts_v2c = cts_c2v = eom_c2v = bst_count = dt_order_errors = 0U;
    dt_expect = 0U;
    pc.hcan = &H;
    pc.own_sa = 0xF4U;
    pc.peer_sa = 0x56U;
    CHECK(gbt_port_f103_init(&P, &pc) == GBT_HAL_OK);
    sim_plant_init(&PLANT, &g_fake_f1.tick);
    CHECK(output_if_make_sink(&PLANT, &sink) == GBT_HAL_OK);
    CHECK(gbt_integ_attach(&IG, &VEH, &P, &VC, &sink) == GBT_HAL_OK);
    gbt_integ_set_evlog(&IG, &EVLOG);                     /* STEP 07-C2.4: as main.c */
    gbt_node_init(&CHG, &CC);
    battery_model_init(&BATT, 500U, soc_target, 10U, 400000U, 100000U, 0x02U);
    (void)battery_model_step(&BATT, g_fake_f1.tick, 0U, &d);
    gbt_node_set_vehicle_demand(&VEH, &d);
    gbt_node_set_connected(&VEH, 1U, g_fake_f1.tick);
    gbt_node_set_connected(&CHG, 1U, g_fake_f1.tick);
}

/* STEP 07-C2.4: host replica of the acceptance mask computed by gbt_watch_update() in
 * Project1_F103B_Receiver_CubeMX/Core/Src/main.c (same sources, same bit order; keep both in step).
 * Bit set = criterion not met. Bit 16 uses TXOK completions of this port. */
static uint32_t accept_mask_vehicle(const gbt_node_status_t *ns, const gbt_can_status_t *cs,
                                    const gbt_port_f103_diag_t *pd)
{
    const gbt_tp_t *tp = &VEH.tp;
    const gbt_tp_stats_t *ts = &tp->stats;
    const gbt_node_diag_t *nd = &ns->diag;
    uint32_t f = 0U;
    uint32_t tx_failures = pd->tx_completed_fail + pd->tx_bus_off + pd->tx_hal_error + IG.tx_dropped;
    if (ns->step != 41U) { f |= 1U << 0; }
    if (IG.tp_tx_complete_msg[0] == 0U) { f |= 1U << 1; }
    if (IG.tp_tx_complete_msg[1] == 0U) { f |= 1U << 2; }
    if (IG.tp_tx_complete_msg[2] == 0U) { f |= 1U << 3; }
    if (ts->abort_tx != 0U || ts->abort_rx != 0U) { f |= 1U << 4; }
    if (ts->timeout_tx != 0U || ts->timeout_rx != 0U) { f |= 1U << 5; }
    if (nd->transport_failures != 0U) { f |= 1U << 6; }
    if (tp->tx.state != 0U || tp->rx.state != 0U) { f |= 1U << 7; }
    if (cs->rx_overflow_sw != 0U || cs->rx_overflow_hw != 0U || nd->rx_overflow != 0U) { f |= 1U << 8; }
    if (cs->rx_invalid != 0U) { f |= 1U << 9; }
    if (nd->rx_invalid != 0U || nd->outbox_overflow != 0U || nd->tx_discarded != 0U || tp->outbox_full != 0U) {
        f |= 1U << 10;
    }
    if (tx_failures != 0U) { f |= 1U << 11; }
    if (cs->state != 1U || pd->error_passive_events != 0U || pd->bus_off_events != 0U) { f |= 1U << 12; }
    if (tp->snap_tx.state != GBT_TP_SNAP_FROZEN || tp->snap_tx.len != 49U) { f |= 1U << 13; }
    if (nd->timeouts != 0U || nd->output_failures != 0U || nd->api_errors != 0U) { f |= 1U << 14; }
    if (pd->hal_error_callbacks + pd->rx_hal_error != 0U) { f |= 1U << 15; }
    if (pd->tx_completed_ok + pd->tx_in_flight != pd->tx_frames) { f |= 1U << 16; }
    if (IG.tp_tx_complete_msg[3] != 0U) { f |= 1U << 17; }
    return f;
}

static void stop_and_check_port(const char *name)
{
    gbt_can_status_t cs;
    gbt_port_f103_diag_t dg;
    CHECK(gbt_hal_can_get_status(&P, &cs) == GBT_HAL_OK);
    CHECK_EQ_U(cs.state, GBT_CAN_STATE_ERROR_ACTIVE);
    CHECK_EQ_U(cs.rx_overflow_sw, 0U);
    CHECK_EQ_U(cs.rx_overflow_hw, 0U);
    CHECK_EQ_U(cs.rx_invalid, 0U);
    CHECK_EQ_U(cs.tx_error, 0U);
    CHECK(gbt_port_f103_get_diag(&P, &dg) == GBT_HAL_OK);
    CHECK_EQ_U(dg.rx_frames, frames_c2v);                 /* every charger frame delivered */
    CHECK_EQ_U(dg.tx_completed_ok, frames_v2c);           /* every vehicle frame on the bus */
    CHECK_EQ_U(dt_order_errors, 0U);                      /* TP.DT order preserved (D1) */
    CHECK_EQ_U(g_fake_f1.rx_overwritten, 0U);
    CHECK_EQ_U(H.ErrorCode, HAL_CAN_ERROR_NONE);
    /* STEP 07 FI-04: the vehicle output is never enabled -> no shutdown is measured or counted. */
    CHECK_EQ_U(IG.fi04.shutdowns, 0U);
    CHECK_EQ_U(IG.fi04.open, 0U);
    CHECK_EQ_U(IG.fi04.verdict, GBT_FI04_VERDICT_NONE);
    CHECK(IG.fi04.passes > 0U);
    CHECK(gbt_integ_detach(&IG) == GBT_HAL_OK);
    CHECK_EQ_U(IG.fi04.shutdowns, 0U);
    CHECK(gbt_port_f103_deinit(&P) == GBT_HAL_OK);
    printf("F103 adapter %s: V->C %lu frames (RTS %lu), C->V %lu frames (CTS %lu, EOM %lu), "
           "TX busy retries %lu, BST %lu\n", name, (unsigned long)frames_v2c, (unsigned long)rts_v2c,
           (unsigned long)frames_c2v, (unsigned long)cts_c2v, (unsigned long)eom_c2v,
           (unsigned long)dg.tx_busy, (unsigned long)bst_count);
}

static void scenario_normal_vehicle_stop(void)
{
    gbt_output_cmd_t cmd;
    gbt_node_status_t cs;
    uint32_t i;
    start(520U);
    CHECK(run_until(&CHG, GBT_NODE_PHASE_CHARGING, 20000U) != 0xFFFFFFFFu);
    CHECK(run_until(&VEH, GBT_NODE_PHASE_CHARGING, 2000U) != 0xFFFFFFFFu);
    for (i = 0U; i < 300U; i++) {
        tick();
    }
    gbt_node_get_output(&CHG, &cmd);
    CHECK_EQ_U(cmd.enabled, 1U);
    CHECK_EQ_U(cmd.voltage_mv, 400000U);                  /* BCL via the F103 adapter */
    CHECK_EQ_U(cmd.current_ma, 100000U);
    gbt_node_get_status(&CHG, &cs);
    CHECK_EQ_U(cs.diag.transport_failures, 0U);           /* BRM, BCP, BCS transfers */
    CHECK(rts_v2c >= 3U);                                 /* BRM, BCP and BCS RTS */
    CHECK(cts_c2v >= 3U);
    CHECK(eom_c2v >= 3U);
    CHECK(run_until(&CHG, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);   /* vehicle SOC stop */
    CHECK(run_until(&VEH, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK_EQ_U(v_stop_sent, 1U);
    gbt_node_get_output(&CHG, &cmd);
    CHECK_EQ_U(cmd.enabled, 0U);
    {                                                     /* STEP 07-C2.4 acceptance replica */
        gbt_can_status_t s;
        gbt_port_f103_diag_t dg;
        gbt_node_status_t vs;
        /* Settle 2 s at END (> T3 1250 ms) as the hardware procedure prescribes: a periodic BCS TP
         * session opened just before the vehicle stop may still be running when both nodes
         * reach END; it then completes normally (TP keeps running in END). */
        for (i = 0U; i < 2000U; i++) {
            tick();
        }
        CHECK(gbt_hal_can_get_status(&P, &s) == GBT_HAL_OK);
        CHECK(gbt_port_f103_get_diag(&P, &dg) == GBT_HAL_OK);
        gbt_node_get_status(&VEH, &vs);
        CHECK_EQ_U(accept_mask_vehicle(&vs, &s, &dg), 0U);
        CHECK_EQ_U(dg.tx_completed_ok, dg.tx_frames);
        CHECK_EQ_U(dg.error_passive_events + dg.bus_off_events + dg.warning_events, 0U);
        CHECK_EQ_U(IG.tp_tx_complete_msg[0], 1U);
        CHECK_EQ_U(IG.tp_tx_complete_msg[1], 1U);
        CHECK(IG.tp_tx_complete_msg[2] >= 1U);
        CHECK(memcmp(VEH.tp.snap_tx.buf, CHG.tp.snap_rx.buf, 49U) == 0);
        CHECK_EQ_U(EVLOG.frozen, 0U);
        printf("F103 adapter normal (C2.4): mailbox loads %lu, TXOK %lu; TP tx complete BRM/BCP/BCS %lu/%lu/%lu; "
               "acceptance mask 0x%lX; event log %lu entries\n", (unsigned long)dg.tx_frames,
               (unsigned long)dg.tx_completed_ok, (unsigned long)IG.tp_tx_complete_msg[0],
               (unsigned long)IG.tp_tx_complete_msg[1], (unsigned long)IG.tp_tx_complete_msg[2],
               (unsigned long)accept_mask_vehicle(&vs, &s, &dg), (unsigned long)EVLOG.count);
    }
    stop_and_check_port("normal + vehicle stop");
}

static void scenario_charger_stop(void)
{
    gbt_output_cmd_t cmd;
    uint32_t i;
    start(1000U);
    CHECK(run_until(&CHG, GBT_NODE_PHASE_CHARGING, 20000U) != 0xFFFFFFFFu);
    CHECK(run_until(&VEH, GBT_NODE_PHASE_CHARGING, 2000U) != 0xFFFFFFFFu);
    for (i = 0U; i < 300U; i++) {
        tick();
    }
    gbt_node_request_stop(&CHG, GBT_STOP_REASON_REQUEST, g_fake_f1.tick);
    CHECK(run_until(&CHG, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK(run_until(&VEH, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK_EQ_U(bst_count, 5U);                            /* charger-first: 5 BST (TM-06) */
    gbt_node_get_output(&CHG, &cmd);
    CHECK_EQ_U(cmd.enabled, 0U);
    stop_and_check_port("charger stop");
}

int main(void)
{
    CP.max_voltage_mv = 750000U;
    CP.min_voltage_mv = 200000U;
    CP.max_current_ma = 250000U;
    CP.min_current_ma = 1000U;
    CP.charger_number = 0x01020304u;
    VP.battery_type = 0x03U;
    VP.rated_capacity_mah = 150000U;
    VP.rated_voltage_mv = 350000U;
    VP.max_cell_voltage_mv = 3650U;
    VP.max_charge_current_ma = 200000U;
    VP.nominal_energy_wh = 60000U;
    VP.max_charge_voltage_mv = 420000U;
    VP.max_cell_temp_c = 55;
    VP.initial_soc_permille = 500U;
    CC.role = GBT_NODE_ROLE_CHARGER;
    CC.charger = &CP;
    CC.t_handle_ms = 1000U;                                 /* K-6A PROVISIONAL */
    CC.selfcheck_delay_ms = 0U;
    CC.apply_fail_tx_policy = GBT_NODE_FAILTX_OPTION_A;     /* test setting; ND-6 OPEN */
    VC = CC;
    VC.role = GBT_NODE_ROLE_VEHICLE;
    VC.charger = NULL;
    VC.vehicle = &VP;
    scenario_normal_vehicle_stop();
    scenario_charger_stop();
    return TEST_SUMMARY("test_port_f103_session");
}
