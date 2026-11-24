/*
 * tests/sil/test_port_g474_session.c - protocol integration through the G474
 * adapter (STEP 07-C1.2-B1, M8). The charger runs exactly as planned for the
 * target: gbt_integ_step() over ports/stm32/g474 (FDCAN fake underneath).
 * The vehicle is a bare gbt_node driven through its public API (one concrete
 * gbt_hal type per executable, so it has no HAL). Bus model per 1 ms tick:
 *   vehicle frames -> fake_g4_bus_rx() + fake_g4_irq() each (RX ISR -> SPSC)
 *   charger gbt_integ_step()   (main loop: overflow, RX, tick, APPLY, SEND)
 *   charger TX FIFO -> completed -> delivered to the vehicle node
 * Host evidence only: no hardware, no real timing.
 */
#include "ports/stm32/g474/port_g474.h"
#include "fake_stm32g4.h"
#include "app/battery_model.h"
#include "app/gbt_integration.h"
#include "app/sim_plant.h"
#include "protocol/gbt_node.h"
#include "tests/support/test_assert.h"

#include <stdio.h>
#include <string.h>

static FDCAN_HandleTypeDef H;
static gbt_hal_t P;
static gbt_integ_t IG;
static gbt_node_t CHG;
static gbt_node_t VEH;
static output_if_backend_t PLANT;
static battery_model_t BATT;
static gbt_charger_params_t CP;
static gbt_vehicle_params_t VP;
static gbt_node_config_t CC;
static gbt_node_config_t VC;
static gbt_can_frame_t v_pending;
static uint8_t v_has_pending;
static uint32_t v_applied_seq;
static uint8_t v_stop_sent;
static uint32_t frames_v2c;
static uint32_t frames_c2v;
static gbt_evlog_t EVLOG;                                  /* STEP 07-C2.4 */

static uint8_t phase(const gbt_node_t *n)
{
    gbt_node_status_t st;
    gbt_node_get_status(n, &st);
    return st.phase;
}

static void vehicle_step(uint32_t now)
{
    gbt_vehicle_demand_t d;
    gbt_output_cmd_t cmd;
    unsigned guard;
    if (battery_model_step(&BATT, now, (uint8_t)(phase(&VEH) == GBT_NODE_PHASE_CHARGING), &d) != 0U
        && phase(&VEH) == GBT_NODE_PHASE_CHARGING && v_stop_sent == 0U) {
        v_stop_sent = 1U;
        gbt_node_request_stop(&VEH, GBT_STOP_REASON_SOC_TARGET, now);
    }
    gbt_node_set_vehicle_demand(&VEH, &d);
    gbt_node_tick(&VEH, now);
    gbt_node_get_output(&VEH, &cmd);
    if (cmd.seq != v_applied_seq) {                       /* vehicle has no power stage */
        v_applied_seq = cmd.seq;
        gbt_node_confirm_output(&VEH, cmd.seq, true, now);
    }
    for (guard = 0U; guard < 32U; guard++) {
        uint8_t data64[64];
        uint8_t chk;
        if (v_has_pending == 0U) {
            if (!gbt_node_pop_tx(&VEH, &v_pending)) {
                break;
            }
            v_has_pending = 1U;
        }
        chk = gbt_node_tx_check(&VEH, &v_pending);
        if (chk == GBT_NODE_TX_HOLD) {
            break;
        }
        v_has_pending = 0U;
        if (chk == GBT_NODE_TX_DISCARD) {
            continue;
        }
        memset(data64, 0, sizeof data64);
        memcpy(data64, v_pending.data, 8U);
        CHECK_EQ_U(fake_g4_bus_rx(v_pending.id, 1U, 0U, 0U, 0U, v_pending.dlc, data64), 1);
        fake_g4_irq();                                    /* RX interrupt per frame */
        frames_v2c++;
    }
}

static void charger_to_vehicle(uint32_t now)
{
    fake_g4_tx_elem_t e;
    fake_g4_tx_complete(3U);                              /* bus transmitted the TX FIFO */
    while (fake_g4_tx_pop(&e) != 0) {
        gbt_can_frame_t f;
        CHECK_EQ_U(e.hdr.IdType, FDCAN_EXTENDED_ID);
        CHECK_EQ_U(e.hdr.FDFormat, FDCAN_CLASSIC_CAN);
        f.id = e.hdr.Identifier;
        f.dlc = (uint8_t)e.hdr.DataLength;
        f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
        memcpy(f.data, e.data, 8U);
        gbt_node_on_rx(&VEH, &f, now);
        frames_c2v++;
    }
}

static void tick(void)
{
    uint32_t now;
    g_fake_g4.tick++;
    now = g_fake_g4.tick;
    vehicle_step(now);
    gbt_integ_step(&IG);
    charger_to_vehicle(now);
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

/* STEP 07-C2.4: host replica of the acceptance mask computed by gbt_watch_update() in
 * Project1_G474/Core/Src/main.c (same sources, same bit order; keep both in step).
 * Bit set = criterion not met. Bit 16 uses the TXBTO confirmation of this port. */
static uint32_t accept_mask_charger(const gbt_node_status_t *ns, const gbt_can_status_t *cs,
                                    const gbt_port_g474_diag_t *pd)
{
    const gbt_tp_t *tp = &CHG.tp;
    const gbt_tp_stats_t *ts = &tp->stats;
    const gbt_node_diag_t *nd = &ns->diag;
    uint32_t f = 0U;
    uint32_t tx_failures = pd->tx_bus_off + pd->tx_hal_error + pd->tx_unconfirmed_reuse + IG.tx_dropped;
    if (ns->step != 41U) { f |= 1U << 0; }
    if (IG.tp_rx_complete_msg[0] == 0U) { f |= 1U << 1; }
    if (IG.tp_rx_complete_msg[1] == 0U) { f |= 1U << 2; }
    if (IG.tp_rx_complete_msg[2] == 0U) { f |= 1U << 3; }
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
    if (tp->snap_rx.state != GBT_TP_SNAP_FROZEN || tp->snap_rx.len != 49U) { f |= 1U << 13; }
    if (nd->timeouts != 0U || nd->output_failures != 0U || nd->api_errors != 0U) { f |= 1U << 14; }
    if (pd->hal_error_callbacks + pd->rx_hal_error != 0U) { f |= 1U << 15; }
    if (pd->tx_confirmed + pd->tx_pending != pd->tx_frames) { f |= 1U << 16; }
    if (IG.tp_rx_complete_msg[3] != 0U) { f |= 1U << 17; }
    return f;
}

int main(void)
{
    gbt_port_g474_cfg_t pc;
    gbt_output_sink_t sink;
    gbt_can_status_t cs;
    gbt_node_status_t st;
    gbt_port_g474_diag_t dg;

    fake_g4_reset(&H);
    g_fake_g4.tick = 100000U;
    pc.hfdcan = &H;
    pc.own_sa = 0x56U;
    pc.peer_sa = 0xF4U;
    CHECK(gbt_port_g474_init(&P, &pc) == GBT_HAL_OK);

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
    CC.t_handle_ms = 1000U;                                /* K-6A PROVISIONAL */
    CC.selfcheck_delay_ms = 0U;
    CC.apply_fail_tx_policy = GBT_NODE_FAILTX_OPTION_A;    /* test setting; ND-6 OPEN */
    VC = CC;
    VC.role = GBT_NODE_ROLE_VEHICLE;
    VC.charger = NULL;
    VC.vehicle = &VP;

    sim_plant_init(&PLANT, &g_fake_g4.tick);
    CHECK(output_if_make_sink(&PLANT, &sink) == GBT_HAL_OK);
    CHECK(gbt_integ_attach(&IG, &CHG, &P, &CC, &sink) == GBT_HAL_OK);
    CHECK_EQ_U(P.lifecycle, GBT_HAL_LC_CLAIMED);
    gbt_integ_set_evlog(&IG, &EVLOG);                      /* STEP 07-C2.4: as main.c */
    gbt_node_init(&VEH, &VC);
    battery_model_init(&BATT, 500U, 520U, 10U, 400000U, 100000U, 0x02U);
    gbt_node_set_connected(&CHG, 1U, g_fake_g4.tick);
    gbt_node_set_connected(&VEH, 1U, g_fake_g4.tick);

    CHECK(run_until(&CHG, GBT_NODE_PHASE_CHARGING, 20000U) != 0xFFFFFFFFu);
    CHECK(run_until(&VEH, GBT_NODE_PHASE_CHARGING, 2000U) != 0xFFFFFFFFu);
    {
        uint32_t i;
        for (i = 0U; i < 300U; i++) {
            tick();
        }
    }
    CHECK_EQ_U(PLANT.last.enabled, 1U);
    CHECK_EQ_U(PLANT.last.voltage_mv, 400000U);            /* BCL 400.0 V via the adapter */
    CHECK_EQ_U(PLANT.last.current_ma, 100000U);
    gbt_node_get_status(&CHG, &st);
    CHECK_EQ_U(st.diag.transport_failures, 0U);            /* BRM/BCP/BCS TP via FDCAN path */
    CHECK_EQ_U(st.diag.rx_overflow, 0U);
    CHECK(gbt_hal_can_get_status(&P, &cs) == GBT_HAL_OK);  /* STEP 07-C2.4: not END yet -> bit 0 */
    CHECK(gbt_port_g474_get_diag(&P, &dg) == GBT_HAL_OK);
    CHECK((accept_mask_charger(&st, &cs, &dg) & 1U) == 1U);

    CHECK(run_until(&CHG, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);   /* vehicle stop */
    CHECK(run_until(&VEH, GBT_NODE_PHASE_END, 15000U) != 0xFFFFFFFFu);
    CHECK_EQ_U(PLANT.last.enabled, 0U);
    CHECK_EQ_U(PLANT.last.current_ma, 0U);
    {                                                      /* STEP 07-C2.4: 2 s settle at END */
        uint32_t i;
        for (i = 0U; i < 2000U; i++) {
            tick();
        }
    }

    CHECK(gbt_hal_can_get_status(&P, &cs) == GBT_HAL_OK);
    CHECK_EQ_U(cs.state, GBT_CAN_STATE_ERROR_ACTIVE);
    CHECK_EQ_U(cs.rx_overflow_sw, 0U);
    CHECK_EQ_U(cs.rx_overflow_hw, 0U);
    CHECK_EQ_U(cs.rx_invalid, 0U);
    CHECK_EQ_U(cs.tx_error, 0U);
    CHECK(gbt_port_g474_get_diag(&P, &dg) == GBT_HAL_OK);
    CHECK_EQ_U(dg.rx_frames, frames_v2c);                  /* every vehicle frame queued */
    CHECK_EQ_U(dg.tx_frames, frames_c2v);
    CHECK(frames_v2c > 50U && frames_c2v > 50U);          /* sanity bound; full session ~160 frames */
    CHECK_EQ_U(g_fake_g4.rx_filtered, 0U);
    CHECK_EQ_U(g_fake_g4.rx_lost_frames, 0U);
    /* STEP 07-C2.4: every FIFO-accepted frame confirmed by TXBTO; acceptance mask clean;
     * a mid-session acceptance mask is NOT clean (END not reached) */
    CHECK_EQ_U(dg.tx_confirmed, dg.tx_frames);
    CHECK_EQ_U(dg.tx_pending, 0U);
    CHECK_EQ_U(dg.tx_unconfirmed_reuse, 0U);
    gbt_node_get_status(&CHG, &st);
    CHECK_EQ_U(accept_mask_charger(&st, &cs, &dg), 0U);
    CHECK_EQ_U(EVLOG.frozen, 0U);
    CHECK(EVLOG.count > 30U);
    CHECK(memcmp(CHG.tp.snap_rx.buf, VEH.tp.snap_tx.buf, 49U) == 0);
    printf("G474 adapter session (C2.4): TX FIFO accepted %lu, TXBTO confirmed %lu, pending %lu; "
           "acceptance mask 0x%lX; event log %lu entries; BRM RX snapshot == vehicle TX snapshot\n",
           (unsigned long)dg.tx_frames, (unsigned long)dg.tx_confirmed, (unsigned long)dg.tx_pending,
           (unsigned long)accept_mask_charger(&st, &cs, &dg), (unsigned long)EVLOG.count);
    /* STEP 07 FI-04: vehicle stop -> exactly one accepted + confirmed shutdown, valid reference,
     * software command acceptance <= 9 ms (port_g474 clock = HAL_GetTick fake). */
    CHECK_EQ_U(IG.fi04.shutdowns, 1U);
    CHECK_EQ_U(IG.fi04.open, 0U);
    CHECK_EQ_U(IG.fi04.ref_flags, 0U);
    CHECK_EQ_U(IG.fi04.verdict, GBT_FI04_VERDICT_PASS);
    CHECK_EQ_U(IG.fi04.pass_count, 1U);
    CHECK_EQ_U(IG.fi04.fail_count + IG.fi04.invalid_count + IG.fi04.refused + IG.fi04.mismatch, 0U);
    CHECK(IG.fi04.elapsed_ms <= GBT_FI04_LIMIT_MS);
    CHECK_EQ_U(IG.fi04.max_valid_ms, IG.fi04.elapsed_ms);
    CHECK_EQ_U(IG.fi04.apply_status, GBT_HAL_OK);
    CHECK_EQ_U(IG.fi04.accept_seq, IG.fi04.confirmed_seq);
    CHECK_EQ_U(IG.fi04.accept_seq, CHG.out_accepted_seq);
    CHECK_EQ_U(IG.fi04.accept_seq, P.accepted_seq);        /* port recorded the same sequence */
    CHECK_EQ_U(PLANT.last.seq, IG.fi04.accept_seq);        /* last command the plant accepted */
    CHECK_EQ_U(IG.fi04.t1_ms, PLANT.last_accept_ms);       /* same tick as the plant's acceptance */
    printf("G474 adapter session FI-04: shutdowns %lu, seq %lu, T0 %lu, T1 %lu, elapsed %lu ms, verdict %u\n",
           (unsigned long)IG.fi04.shutdowns, (unsigned long)IG.fi04.accept_seq, (unsigned long)IG.fi04.t0_ms,
           (unsigned long)IG.fi04.t1_ms, (unsigned long)IG.fi04.elapsed_ms, (unsigned)IG.fi04.verdict);
    CHECK(gbt_integ_detach(&IG) == GBT_HAL_OK);
    CHECK_EQ_U(IG.fi04.shutdowns, 1U);                     /* detach: output already disabled, no new count */
    CHECK(gbt_port_g474_deinit(&P) == GBT_HAL_OK);
    printf("G474 adapter session: charger END via FDCAN fake; V->C frames %lu, C->V frames %lu, "
           "TX FIFO full retries %lu\n", (unsigned long)frames_v2c, (unsigned long)frames_c2v,
           (unsigned long)dg.tx_fifo_full);
    return TEST_SUMMARY("test_port_g474_session");
}
