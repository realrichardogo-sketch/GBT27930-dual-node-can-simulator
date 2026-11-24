/*
 * app/linux_node.c - one protocol node as a Linux process on a SocketCAN
 * interface. Same protocol core and integration loop as the deterministic SIL;
 * only the HAL differs (ports/linux). Main loop period 1 ms (nanosleep).
 *
 * Options (all optional except where noted):
 *   --if NAME              CAN interface (default vcan0)
 *   --duration-ms N        hard limit for the run (default 30000)
 *   --expect end|exit      final phase that counts as success (default end)
 *   --linger-ms N          keep running N ms after reaching END/EXIT (default 600)
 *   charger: --stop-after-ms N     request a charger stop N ms after charging starts
 *            --drop-ccs-after-ms N stop sending CCS N ms after charging starts (CCS timeout)
 *   vehicle: --soc-target P        SOC target in permille (default 520 -> vehicle stop)
 *            --drop-bcl-after-ms N stop sending BCL N ms after charging starts (BCL timeout)
 * ND-6 is OPEN: the process sets OPTION_A explicitly as a SIL test setting.
 */
#define _DEFAULT_SOURCE 1

#include "app/linux_node.h"
#include "app/battery_model.h"
#include "app/gbt_integration.h"
#include "app/sim_plant.h"
#include "ports/linux/port_linux.h"
#include "protocol/gbt_id.h"
#include "protocol/gbt_node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    const char *ifname;
    uint32_t    duration_ms;
    uint8_t     expect_phase;
    uint32_t    linger_ms;
    long        stop_after_ms;
    long        drop_ccs_after_ms;
    long        drop_bcl_after_ms;
    uint16_t    soc_target;
} opts_t;

typedef struct {
    uint8_t  role;
    uint32_t now;
    uint32_t charging_since;
    uint8_t  charging_seen;
    long     drop_after_ms;
    uint32_t drop_id;
} inject_t;

static gbt_hal_t           g_hal;
static gbt_node_t          g_node;
static gbt_integ_t         g_ig;
static output_if_backend_t g_plant;
static battery_model_t     g_batt;

static const char *phase_name(uint8_t p)
{
    static const char *const names[] = { "IDLE", "HANDSHAKE", "CONFIG", "CHARGING", "STOPPING",
                                         "END", "TIMEOUT", "EXIT", "CONFIG_ERROR" };
    return (p < 9U) ? names[p] : "?";
}

static uint8_t drop_filter(void *ctx, const gbt_can_frame_t *f)
{
    inject_t *in = (inject_t *)ctx;
    if (in->drop_after_ms >= 0 && in->charging_seen != 0U && f->id == in->drop_id
        && (uint32_t)(in->now - in->charging_since) >= (uint32_t)in->drop_after_ms) {
        return 0U;
    }
    return 1U;
}

static int parse(int argc, char **argv, opts_t *o)
{
    int i;
    o->ifname = "vcan0";
    o->duration_ms = 30000U;
    o->expect_phase = GBT_NODE_PHASE_END;
    o->linger_ms = 600U;
    o->stop_after_ms = -1;
    o->drop_ccs_after_ms = -1;
    o->drop_bcl_after_ms = -1;
    o->soc_target = 520U;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        const char *v = (i + 1 < argc) ? argv[i + 1] : NULL;
        if (v == NULL) {
            return -1;
        }
        if (strcmp(a, "--if") == 0) {
            o->ifname = v;
        } else if (strcmp(a, "--duration-ms") == 0) {
            o->duration_ms = (uint32_t)strtoul(v, NULL, 10);
        } else if (strcmp(a, "--linger-ms") == 0) {
            o->linger_ms = (uint32_t)strtoul(v, NULL, 10);
        } else if (strcmp(a, "--expect") == 0) {
            o->expect_phase = (strcmp(v, "exit") == 0) ? GBT_NODE_PHASE_EXIT : GBT_NODE_PHASE_END;
        } else if (strcmp(a, "--stop-after-ms") == 0) {
            o->stop_after_ms = strtol(v, NULL, 10);
        } else if (strcmp(a, "--drop-ccs-after-ms") == 0) {
            o->drop_ccs_after_ms = strtol(v, NULL, 10);
        } else if (strcmp(a, "--drop-bcl-after-ms") == 0) {
            o->drop_bcl_after_ms = strtol(v, NULL, 10);
        } else if (strcmp(a, "--soc-target") == 0) {
            o->soc_target = (uint16_t)strtoul(v, NULL, 10);
        } else {
            return -1;
        }
        i++;
    }
    return 0;
}

static void sleep_1ms(void)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000L;
    (void)nanosleep(&ts, NULL);
}

int gbt_linux_node_main(uint8_t role, int argc, char **argv)
{
    static gbt_charger_params_t cp;
    static gbt_vehicle_params_t vp;
    static inject_t inj;
    gbt_node_config_t cfg;
    gbt_output_sink_t sink;
    gbt_port_linux_cfg_t pc;
    gbt_node_status_t st;
    opts_t o;
    const char *rn = (role == GBT_NODE_ROLE_CHARGER) ? "charger" : "vehicle";
    int err = 0;
    uint32_t t0;
    uint32_t elapsed = 0U;
    uint32_t final_at = 0U;
    uint8_t last_phase = 0xFFU;
    uint8_t stop_sent = 0U;
    int rc;

    if (parse(argc, argv, &o) != 0) {
        fprintf(stderr, "usage: %s [--if vcan0] [--duration-ms N] [--expect end|exit] ...\n", argv[0]);
        return 3;
    }
    pc.ifname = o.ifname;
    if (gbt_port_linux_init(&g_hal, &pc, &err) != GBT_HAL_OK) {
        fprintf(stderr, "[%s] SocketCAN interface '%s' unavailable (errno %d: %s)\n", rn, o.ifname, err,
                strerror(err));
        return 2;
    }
    memset(&cfg, 0, sizeof cfg);
    cfg.role = role;
    cfg.t_handle_ms = 1000U;                                   /* K-6A PROVISIONAL */
    cfg.selfcheck_delay_ms = 0U;                               /* S-8 PROVISIONAL */
    cfg.apply_fail_tx_policy = GBT_NODE_FAILTX_OPTION_A;       /* SIL test setting; ND-6 OPEN */
    cp.max_voltage_mv = 750000U;
    cp.min_voltage_mv = 200000U;
    cp.max_current_ma = 250000U;
    cp.min_current_ma = 1000U;
    cp.charger_number = 0x01020304u;
    vp.battery_type = 0x03U;
    vp.rated_capacity_mah = 150000U;
    vp.rated_voltage_mv = 350000U;
    vp.max_cell_voltage_mv = 3650U;
    vp.max_charge_current_ma = 200000U;
    vp.nominal_energy_wh = 60000U;
    vp.max_charge_voltage_mv = 420000U;
    vp.max_cell_temp_c = 55;
    vp.initial_soc_permille = 500U;
    cfg.charger = (role == GBT_NODE_ROLE_CHARGER) ? &cp : NULL;
    cfg.vehicle = (role == GBT_NODE_ROLE_VEHICLE) ? &vp : NULL;

    inj.role = role;
    inj.now = gbt_hal_now_ms(&g_hal);
    inj.drop_after_ms = (role == GBT_NODE_ROLE_CHARGER) ? o.drop_ccs_after_ms : o.drop_bcl_after_ms;
    inj.drop_id = (role == GBT_NODE_ROLE_CHARGER) ? 0x1812F456u : 0x181056F4u;   /* CCS / BCL */

    sim_plant_init(&g_plant, &inj.now);
    (void)output_if_make_sink(&g_plant, &sink);
    if (gbt_integ_attach(&g_ig, &g_node, &g_hal, &cfg, &sink) != GBT_HAL_OK) {
        fprintf(stderr, "[%s] attach failed\n", rn);
        (void)gbt_port_linux_deinit(&g_hal);
        return 1;
    }
    g_ig.tx_filter = drop_filter;
    g_ig.tx_filter_ctx = &inj;
    battery_model_init(&g_batt, 500U, o.soc_target, 10U, 400000U, 100000U, 0x02U);
    t0 = gbt_hal_now_ms(&g_hal);
    gbt_node_set_connected(&g_node, 1U, t0);
    printf("[%s] started on %s (pid-independent node, own context)\n", rn, o.ifname);
    fflush(stdout);

    while (elapsed < o.duration_ms) {
        inj.now = gbt_hal_now_ms(&g_hal);
        elapsed = (uint32_t)(inj.now - t0);
        gbt_node_get_status(&g_node, &st);
        if (st.phase == GBT_NODE_PHASE_CHARGING && inj.charging_seen == 0U) {
            inj.charging_seen = 1U;
            inj.charging_since = inj.now;
        }
        if (role == GBT_NODE_ROLE_VEHICLE) {
            gbt_vehicle_demand_t d;
            if (battery_model_step(&g_batt, inj.now, (uint8_t)(st.phase == GBT_NODE_PHASE_CHARGING), &d) != 0U
                && st.phase == GBT_NODE_PHASE_CHARGING && stop_sent == 0U) {
                stop_sent = 1U;
                gbt_node_request_stop(&g_node, GBT_STOP_REASON_SOC_TARGET, inj.now);
                printf("[%s] t=%lu ms SOC target reached -> vehicle stop request\n", rn, (unsigned long)elapsed);
            }
            gbt_node_set_vehicle_demand(&g_node, &d);
        } else if (o.stop_after_ms >= 0 && inj.charging_seen != 0U && stop_sent == 0U
                   && (uint32_t)(inj.now - inj.charging_since) >= (uint32_t)o.stop_after_ms) {
            stop_sent = 1U;
            gbt_node_request_stop(&g_node, GBT_STOP_REASON_REQUEST, inj.now);
            printf("[%s] t=%lu ms charger stop request\n", rn, (unsigned long)elapsed);
        }
        gbt_integ_step(&g_ig);
        gbt_node_get_status(&g_node, &st);
        if (st.phase != last_phase) {
            printf("[%s] t=%lu ms phase %s (step %u) output %s %lu mV %lu mA\n", rn, (unsigned long)elapsed,
                   phase_name(st.phase), (unsigned)st.step, g_plant.last.enabled ? "ON" : "OFF",
                   (unsigned long)g_plant.last.voltage_mv, (unsigned long)g_plant.last.current_ma);
            fflush(stdout);
            last_phase = st.phase;
        }
        if ((st.phase == GBT_NODE_PHASE_END || st.phase == GBT_NODE_PHASE_EXIT) && final_at == 0U) {
            final_at = elapsed + 1U;
        }
        if (final_at != 0U && elapsed >= final_at + o.linger_ms) {
            break;
        }
        sleep_1ms();
    }
    gbt_node_get_status(&g_node, &st);
    rc = (st.phase == o.expect_phase) ? 0 : 1;
    printf("[%s] RESULT final_phase=%s expected=%s rx=%lu tx=%lu rx_ignored=%lu timeouts=%lu "
           "transport_failures=%lu tx_filtered=%lu tx_dropped=%lu max_current_ma=%lu output=%s -> %s\n",
           rn, phase_name(st.phase), phase_name(o.expect_phase), (unsigned long)st.diag.rx_frames,
           (unsigned long)st.diag.tx_frames, (unsigned long)st.diag.rx_ignored,
           (unsigned long)st.diag.timeouts, (unsigned long)st.diag.transport_failures,
           (unsigned long)g_ig.tx_filtered, (unsigned long)g_ig.tx_dropped,
           (unsigned long)g_plant.max_current_ma, g_plant.last.enabled ? "ON" : "OFF",
           (rc == 0) ? "PASS" : "FAIL");
    (void)gbt_integ_detach(&g_ig);
    (void)gbt_port_linux_deinit(&g_hal);
    return rc;
}
