/*
 * protocol/gbt_node.c - node runtime: init, RX dispatch, periodic scheduler,
 * timers, TP integration, TX gate (ARCHITECTURE.md §4.4), output state.
 */
#include "protocol/gbt_node_internal.h"

#include <stddef.h>
#include <string.h>

/* TO-01..TO-25 durations, ms (01.2_TIMEOUT_TABLE.md §1; TO-25 has no GB/T value) */
const uint32_t gbt_to_duration_ms[GBT_NODE_TIMER_SLOTS] = {
    10000U, 30000U, 5000U, 5000U, 5000U, 5000U, 5000U, 60000U, 5000U, 5000U,   /* TO-01..10 */
    60000U, 1000U, 1000U, 1000U, 5000U, 5000U, 1000U, 5000U, 10000U, 5000U,    /* TO-11..20 */
    10000U, 5000U, 30000U, 30000U, 0U                                          /* TO-21..25 */
};

static void sat_inc(uint32_t *c)
{
    if (*c != 0xFFFFFFFFu) {
        (*c)++;
    }
}

uint32_t gbt_node_period_ms(gbt_msg_t msg)
{
    const gbt_msg_def_t *d = gbt_msg_def(msg);
    return (d != NULL) ? d->period_ms : 0U;
}

/* ---- shared helpers ---- */
void gbt_node_tx_enable(gbt_node_t *n, gbt_msg_t msg)
{
    if (n->s.tx_on[msg] == 0U) {
        n->s.tx_on[msg] = 2U;               /* 2 = send at the next scheduler pass */
    }
}

void gbt_node_tx_disable(gbt_node_t *n, gbt_msg_t msg)
{
    n->s.tx_on[msg] = 0U;
}

void gbt_node_tx_disable_all(gbt_node_t *n)
{
    memset(n->s.tx_on, 0, sizeof n->s.tx_on);
}

void gbt_node_timer_start(gbt_node_t *n, uint8_t to_index)
{
    gbt_timer_arm(&n->timer[to_index], n->now_ms, gbt_to_duration_ms[to_index]);
}

void gbt_node_timer_stop(gbt_node_t *n, uint8_t to_index)
{
    gbt_timer_disarm(&n->timer[to_index]);
}

uint8_t gbt_node_timer_expired(const gbt_node_t *n, uint8_t to_index)
{
    return gbt_timer_expired(&n->timer[to_index], n->now_ms);
}

static void stop_all_timers(gbt_node_t *n)
{
    unsigned i;
    for (i = 0U; i < GBT_NODE_TIMER_SLOTS; i++) {
        gbt_timer_disarm(&n->timer[i]);
    }
}

void gbt_node_set_step(gbt_node_t *n, uint8_t step)
{
    n->step = step;
    switch (step) {
    case GBT_STEP_IDLE:  n->phase = GBT_NODE_PHASE_IDLE; break;
    case GBT_STEP_C02: case GBT_STEP_C03: case GBT_STEP_C04:
    case GBT_STEP_V02: case GBT_STEP_V03: case GBT_STEP_V05:
        n->phase = GBT_NODE_PHASE_HANDSHAKE; break;
    case GBT_STEP_C05: case GBT_STEP_C07: case GBT_STEP_C08: case GBT_STEP_C09:
    case GBT_STEP_V06: case GBT_STEP_V09: case GBT_STEP_V10:
        n->phase = GBT_NODE_PHASE_CONFIG; break;
    case GBT_STEP_C10: case GBT_STEP_C10P: case GBT_STEP_V11:
        n->phase = GBT_NODE_PHASE_CHARGING; break;
    case GBT_STEP_C11: case GBT_STEP_C12: case GBT_STEP_V12: case GBT_STEP_V13:
        n->phase = GBT_NODE_PHASE_STOPPING; break;
    case GBT_STEP_TO:    n->phase = GBT_NODE_PHASE_TIMEOUT; break;
    case GBT_STEP_END:   n->phase = GBT_NODE_PHASE_END; break;
    default:             n->phase = GBT_NODE_PHASE_EXIT; break;
    }
}

void gbt_node_push(gbt_node_t *n, const gbt_can_frame_t *f)
{
    if (gbt_outbox_push(&n->outbox, f) != GBT_RING_OK) {
        sat_inc(&n->diag.outbox_overflow);
    }
}

void gbt_node_enter_timeout(gbt_node_t *n, uint8_t field)
{
    gbt_safety_disable(n, GBT_SAFE_REASON_TIMEOUT);           /* inhibit first */
    gbt_node_tx_disable_all(n);
    stop_all_timers(n);
    memset(n->s.err_codes, 0, sizeof n->s.err_codes);         /* other fields 00 */
    if (field < GBT_CEM_FIELDS) {
        n->s.err_codes[field] = GBT_CODE_01;
    }
    n->s.handling_start_ms = n->now_ms;
    sat_inc(&n->diag.timeouts);
    gbt_node_set_step(n, GBT_STEP_TO);
    gbt_node_tx_enable(n, (n->cfg.role == GBT_NODE_ROLE_CHARGER) ? GBT_MSG_CEM : GBT_MSG_BEM);
}

void gbt_node_peer_error(gbt_node_t *n)
{
    gbt_safety_disable(n, GBT_SAFE_REASON_TIMEOUT);
    gbt_node_tx_disable_all(n);
    stop_all_timers(n);
    gbt_node_set_step(n, GBT_STEP_EXIT);                      /* K-6A rule 5 */
}

static uint8_t accept_none(uint32_t pgn, uint16_t len)
{
    (void)pgn;
    (void)len;
    return 0U;
}

/* ---- public API ---- */
void gbt_node_init(gbt_node_t *node, const gbt_node_config_t *config)
{
    uint8_t own;
    uint8_t peer;

    if (node == NULL) {
        return;
    }
    memset(node, 0, sizeof *node);
    (void)gbt_outbox_init(&node->outbox);
    node->out_cmd.seq = 1U;                                    /* initial disabled command */
    node->out_state = GBT_NODE_OUT_PENDING;                    /* must be applied and confirmed */
    if (config == NULL) {
        sat_inc(&node->diag.api_errors);
        node->phase = GBT_NODE_PHASE_CONFIG_ERROR;
        node->step = GBT_STEP_EXIT;
        return;
    }
    node->cfg = *config;
    if ((config->role == GBT_NODE_ROLE_CHARGER && config->charger == NULL)
        || (config->role == GBT_NODE_ROLE_VEHICLE && config->vehicle == NULL)
        || (config->role != GBT_NODE_ROLE_CHARGER && config->role != GBT_NODE_ROLE_VEHICLE)
        || (config->apply_fail_tx_policy != GBT_NODE_FAILTX_OPTION_A
            && config->apply_fail_tx_policy != GBT_NODE_FAILTX_OPTION_B)) {   /* ND-6: no default */
        node->phase = GBT_NODE_PHASE_CONFIG_ERROR;
        node->step = GBT_STEP_EXIT;
        return;
    }
    own = (config->role == GBT_NODE_ROLE_CHARGER) ? (uint8_t)GBT_ADDR_SECC : (uint8_t)GBT_ADDR_EVCC;
    peer = (config->role == GBT_NODE_ROLE_CHARGER) ? (uint8_t)GBT_ADDR_EVCC : (uint8_t)GBT_ADDR_SECC;
    gbt_tp_init(&node->tp, own, peer, NULL);                   /* P-K4-xx defaults */
    gbt_node_set_step(node, GBT_STEP_IDLE);
}

void gbt_node_set_connected(gbt_node_t *node, uint8_t connected, uint32_t now_ms)
{
    if (node == NULL || node->phase == GBT_NODE_PHASE_CONFIG_ERROR) {
        return;
    }
    node->now_ms = now_ms;
    if (connected != 0U && node->connected == 0U && node->step == GBT_STEP_IDLE) {
        node->connected = 1U;
        if (node->cfg.role == GBT_NODE_ROLE_CHARGER) {
            gbt_csm_start(node);
        } else {
            gbt_vsm_start(node);
        }
    } else if (connected == 0U && node->connected != 0U) {
        node->connected = 0U;
        gbt_safety_disable(node, GBT_SAFE_REASON_SHUTDOWN);
        gbt_node_tx_disable_all(node);
        stop_all_timers(node);
        gbt_node_set_step(node, GBT_STEP_EXIT);
    }
}

static void on_tp_events(gbt_node_t *n, uint8_t ev)
{
    if ((ev & (GBT_TP_EV_RX_ABORTED | GBT_TP_EV_TX_ABORTED)) != 0U) {
        sat_inc(&n->diag.transport_failures);       /* TP-6: no BEM/CEM from TP alone */
    }
    if ((ev & GBT_TP_EV_RX_COMPLETE) != 0U && n->cfg.role == GBT_NODE_ROLE_CHARGER) {
        gbt_csm_on_tp(n, n->tp.rx.pgn, n->tp.rx.buf, n->tp.rx.len);
    }
}

void gbt_node_on_rx(gbt_node_t *node, const gbt_can_frame_t *frame, uint32_t now_ms)
{
    gbt_msg_t msg;
    const gbt_msg_def_t *d;
    uint8_t ev;
    uint8_t from_peer_dir;

    if (node == NULL || frame == NULL) {
        if (node != NULL) {
            sat_inc(&node->diag.api_errors);
        }
        return;
    }
    node->now_ms = now_ms;
    sat_inc(&node->diag.rx_frames);
    if (node->phase == GBT_NODE_PHASE_CONFIG_ERROR || node->step == GBT_STEP_EXIT
        || node->step == GBT_STEP_IDLE || gbt_can_frame_is_valid(frame) == 0U) {
        sat_inc(&node->diag.rx_ignored);
        return;
    }
    ev = gbt_tp_on_frame(&node->tp, frame, now_ms, &node->outbox,
                         (node->cfg.role == GBT_NODE_ROLE_CHARGER) ? gbt_csm_accept_tp : accept_none);
    if ((ev & GBT_TP_EV_NOT_TP) == 0U) {
        on_tp_events(node, ev);
        return;
    }
    if (gbt_id_classify(frame->id, &msg) != GBT_ID_OK) {
        sat_inc(&node->diag.rx_ignored);           /* GB/T §5.5: not processed */
        return;
    }
    d = gbt_msg_def(msg);
    from_peer_dir = (node->cfg.role == GBT_NODE_ROLE_CHARGER) ? (uint8_t)GBT_DIR_V2C : (uint8_t)GBT_DIR_C2V;
    if (d == NULL || d->dir != from_peer_dir || d->tp != 0U) {
        sat_inc(&node->diag.rx_ignored);
        return;
    }
    if ((msg == GBT_MSG_BEM || msg == GBT_MSG_CEM) && node->step != GBT_STEP_END) {
        node->err_fields[0] = frame->data[0];
        node->err_fields[1] = frame->data[1];
        node->err_fields[2] = frame->data[2];
        node->err_fields[3] = frame->data[3];
        gbt_node_peer_error(node);                 /* TC44 / TV36 */
        return;
    }
    if (node->cfg.role == GBT_NODE_ROLE_CHARGER) {
        gbt_csm_on_msg(node, msg, frame);
    } else {
        gbt_vsm_on_msg(node, msg, frame);
    }
}

void gbt_node_on_rx_overflow(gbt_node_t *node, uint32_t lost_frames, uint32_t now_ms)
{
    if (node == NULL || lost_frames == 0U) {
        return;
    }
    node->now_ms = now_ms;
    node->diag.rx_overflow = (node->diag.rx_overflow > 0xFFFFFFFFu - lost_frames)
                                 ? 0xFFFFFFFFu : node->diag.rx_overflow + lost_frames;
    if (node->cfg.role == GBT_NODE_ROLE_CHARGER
        && (node->step == GBT_STEP_C10 || node->step == GBT_STEP_C10P)) {
        gbt_csm_stop(node, GBT_SAFE_REASON_OVERFLOW);       /* T-5 */
    }
}

static void scheduler(gbt_node_t *n)
{
    unsigned i;
    for (i = 0U; i < (unsigned)GBT_MSG_COUNT; i++) {
        gbt_msg_t msg = (gbt_msg_t)i;
        const gbt_msg_def_t *d = gbt_msg_def(msg);
        uint8_t due;
        if (n->s.tx_on[i] == 0U || d == NULL) {
            continue;
        }
        due = (uint8_t)((n->s.tx_on[i] == 2U)
              || gbt_time_elapsed(n->now_ms, n->s.tx_due[i]) >= d->period_ms);
        if (!due) {
            continue;
        }
        if (d->tp != 0U) {
            uint8_t buf[GBT_TP_MAX_LEN];
            uint16_t len;
            if (gbt_tp_tx_busy(&n->tp) != 0U) {
                continue;                              /* TP-7: try at the next pass/period */
            }
            len = gbt_vsm_tp_payload(n, msg, buf);
            if (len == 0U || gbt_tp_tx_start(&n->tp, d->pgn, buf, len, n->now_ms, &n->outbox) == 0U) {
                continue;
            }
        } else {
            gbt_can_frame_t f;
            uint8_t ok = (n->cfg.role == GBT_NODE_ROLE_CHARGER) ? gbt_csm_build(n, msg, &f)
                                                                : gbt_vsm_build(n, msg, &f);
            if (ok == 0U) {
                continue;
            }
            gbt_node_push(n, &f);
        }
        n->s.tx_on[i] = (uint8_t)((n->s.tx_on[i] == 0U) ? 0U : 1U);     /* builders may disable */
        n->s.tx_due[i] = n->now_ms;
    }
}

void gbt_node_tick(gbt_node_t *node, uint32_t now_ms)
{
    uint8_t ev;
    if (node == NULL) {
        return;
    }
    node->now_ms = now_ms;
    if (node->phase == GBT_NODE_PHASE_CONFIG_ERROR || node->step == GBT_STEP_EXIT
        || node->step == GBT_STEP_IDLE) {
        return;
    }
    ev = gbt_tp_tick(&node->tp, now_ms, &node->outbox);
    on_tp_events(node, ev);
    if (node->step == GBT_STEP_TO) {
        if (gbt_time_elapsed(now_ms, node->s.handling_start_ms) >= node->cfg.t_handle_ms) {
            gbt_node_tx_disable_all(node);                 /* K-6A: handling complete -> EXIT */
            gbt_node_set_step(node, GBT_STEP_EXIT);
            return;
        }
    } else if (node->cfg.role == GBT_NODE_ROLE_CHARGER) {
        gbt_csm_tick(node);
    } else {
        gbt_vsm_tick(node);
    }
    if (node->step != GBT_STEP_EXIT) {
        scheduler(node);
    }
}

/* ---- TX gate (ARCHITECTURE.md §4.4) ---- */
static uint8_t tx_class(const gbt_can_frame_t *f)
{
    gbt_msg_t msg;
    if (gbt_id_classify(f->id, &msg) != GBT_ID_OK) {
        return GBT_NODE_TXCLASS_NORMAL;                    /* TP.CM / TP.DT */
    }
    switch (msg) {
    case GBT_MSG_BST: case GBT_MSG_CST: case GBT_MSG_BSD: case GBT_MSG_CSD:
        return GBT_NODE_TXCLASS_STOP;
    case GBT_MSG_BEM: case GBT_MSG_CEM:
        return GBT_NODE_TXCLASS_ERROR;
    default:
        return GBT_NODE_TXCLASS_NORMAL;
    }
}

uint8_t gbt_node_tx_check(const gbt_node_t *node, const gbt_can_frame_t *frame)
{
    if (node == NULL || frame == NULL) {
        return GBT_NODE_TX_HOLD;
    }
    if (node->out_state == GBT_NODE_OUT_PENDING) {
        return GBT_NODE_TX_HOLD;                           /* SAF-1: output first */
    }
    if (node->out_state == GBT_NODE_OUT_FAILED) {
        if (tx_class(frame) == GBT_NODE_TXCLASS_NORMAL) {
            return GBT_NODE_TX_DISCARD;
        }
        return (uint8_t)((node->cfg.apply_fail_tx_policy == GBT_NODE_FAILTX_OPTION_B)
                   ? GBT_NODE_TX_SEND : GBT_NODE_TX_HOLD);  /* ND-6 OPEN: configured explicitly */
    }
    return GBT_NODE_TX_SEND;
}

bool gbt_node_pop_tx(gbt_node_t *node, gbt_can_frame_t *frame)
{
    gbt_can_frame_t head;
    if (node == NULL || frame == NULL) {
        return false;
    }
    while (gbt_outbox_peek(&node->outbox, &head) == GBT_RING_OK) {
        uint8_t c = gbt_node_tx_check(node, &head);
        if (c == GBT_NODE_TX_HOLD) {
            return false;
        }
        (void)gbt_outbox_pop(&node->outbox, &head);
        if (c == GBT_NODE_TX_DISCARD) {
            sat_inc(&node->diag.tx_discarded);
            continue;
        }
        *frame = head;
        sat_inc(&node->diag.tx_frames);
        return true;
    }
    return false;
}

/* ---- output ---- */
void gbt_node_get_output(const gbt_node_t *node, gbt_output_cmd_t *cmd)
{
    if (node != NULL && cmd != NULL) {
        *cmd = node->out_cmd;
    }
}

void gbt_node_confirm_output(gbt_node_t *node, uint32_t seq, bool accepted, uint32_t now_ms)
{
    if (node == NULL) {
        return;
    }
    node->now_ms = now_ms;
    if (seq != node->out_cmd.seq) {
        sat_inc(&node->diag.api_errors);
        return;
    }
    if (accepted) {
        node->out_state = GBT_NODE_OUT_ACCEPTED;
        node->out_accepted_seq = seq;
        return;
    }
    sat_inc(&node->diag.output_failures);
    if (node->out_cmd.enabled != 0U && node->cfg.role == GBT_NODE_ROLE_CHARGER
        && (node->step == GBT_STEP_C10 || node->step == GBT_STEP_C10P)) {
        gbt_csm_stop(node, GBT_SAFE_REASON_OUTPUT_FAIL);   /* ND-7 (PROVISIONAL) */
        return;
    }
    node->out_state = GBT_NODE_OUT_FAILED;                 /* ND-6 OPEN applies */
}

void gbt_node_set_feedback(gbt_node_t *node, const gbt_output_fb_t *fb)
{
    if (node == NULL || fb == NULL) {
        if (node != NULL) {
            sat_inc(&node->diag.api_errors);
        }
        return;
    }
    node->feedback = *fb;
}

void gbt_node_request_stop(gbt_node_t *node, uint8_t reason, uint32_t now_ms)
{
    if (node == NULL) {
        return;
    }
    node->now_ms = now_ms;
    node->stop_request = 1U;
    node->stop_reason = reason;
    if (node->cfg.role == GBT_NODE_ROLE_CHARGER) {
        if (node->step == GBT_STEP_C10 || node->step == GBT_STEP_C10P) {
            gbt_csm_stop(node, (reason == GBT_STOP_REASON_FAULT) ? GBT_SAFE_REASON_OUTPUT_FAIL
                                                                 : GBT_SAFE_REASON_REQUEST);   /* T-2 */
        }
    } else if (node->step == GBT_STEP_V11) {
        gbt_vsm_stop(node, reason);
    }
}

void gbt_node_set_vehicle_demand(gbt_node_t *node, const gbt_vehicle_demand_t *demand)
{
    if (node == NULL || demand == NULL) {
        if (node != NULL) {
            sat_inc(&node->diag.api_errors);
        }
        return;
    }
    node->demand = *demand;
}

bool gbt_node_restart_session(gbt_node_t *node, uint32_t now_ms)
{
    gbt_node_config_t cfg;
    gbt_output_cmd_t cmd;
    uint32_t accepted;
    gbt_node_diag_t diag;
    if (node == NULL || (node->step != GBT_STEP_END && node->step != GBT_STEP_EXIT)
        || node->phase == GBT_NODE_PHASE_CONFIG_ERROR || node->out_state != GBT_NODE_OUT_ACCEPTED) {
        return false;
    }
    cfg = node->cfg;
    cmd = node->out_cmd;
    accepted = node->out_accepted_seq;
    diag = node->diag;
    gbt_node_init(node, &cfg);
    node->out_cmd = cmd;                    /* keep seq monotonic; still disabled */
    node->out_cmd.enabled = 0U;
    node->out_cmd.voltage_mv = 0U;
    node->out_cmd.current_ma = 0U;
    node->out_accepted_seq = accepted;
    node->out_state = GBT_NODE_OUT_ACCEPTED;
    node->diag = diag;
    node->now_ms = now_ms;
    return true;
}

void gbt_node_shutdown(gbt_node_t *node, uint32_t now_ms)
{
    if (node == NULL) {
        return;
    }
    node->now_ms = now_ms;
    gbt_safety_disable(node, GBT_SAFE_REASON_SHUTDOWN);
    gbt_node_tx_disable_all(node);
    stop_all_timers(node);
    gbt_node_set_step(node, GBT_STEP_EXIT);
}

void gbt_node_get_status(const gbt_node_t *node, gbt_node_status_t *status)
{
    if (node == NULL || status == NULL) {
        return;
    }
    memset(status, 0, sizeof *status);
    status->role = node->cfg.role;
    status->phase = node->phase;
    status->step = node->step;
    status->stop_reason = node->stop_reason;
    status->output_state = node->out_state;
    status->inhibit_latched = node->inhibit_latched;
    memcpy(status->err_fields, node->err_fields, sizeof status->err_fields);
    status->diag = node->diag;
}
