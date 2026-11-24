/* app/gbt_integration.c - see gbt_integration.h and protocol/gbt_node.h (integration loop). */
#include "app/gbt_integration.h"

#include <stddef.h>
#include <string.h>

/* ---- STEP 07 FI-04: shutdown response-time bookkeeping (see gbt_integration.h) ---- */
static void fi04_sat_inc(uint32_t *c)
{
    if (*c != 0xFFFFFFFFu) {
        (*c)++;
    }
}

void gbt_fi04_pass_begin(gbt_fi04_t *m, uint32_t now_ms)
{
    if (m == NULL) {
        return;
    }
    if (m->passes == 0U) {
        m->prev_ref_flags = (uint8_t)GBT_FI04_REF_NO_PREV;
    } else {
        m->prev_ref_flags = m->cur_ref_flags;
    }
    m->prev_pass_start_ms = m->pass_start_ms;
    m->pass_start_ms = now_ms;
    m->cur_ref_flags = 0U;
    m->in_pass = 1U;
    fi04_sat_inc(&m->passes);
}

void gbt_fi04_pass_note(gbt_fi04_t *m, uint8_t ref_flag)
{
    if (m != NULL) {
        m->cur_ref_flags = (uint8_t)(m->cur_ref_flags | ref_flag);
    }
}

void gbt_fi04_pass_end(gbt_fi04_t *m)
{
    if (m != NULL) {
        m->in_pass = 0U;
    }
}

static void fi04_open(gbt_fi04_t *m, uint32_t seq)
{
    m->open = 1U;
    m->open_seq = seq;
    if (m->in_pass != 0U) {
        /* inside pass P: reference = start of pass P-1, validity of P-1 plus overflow / status
         * problems already seen in P (before this command was delivered) */
        m->open_t0_ms = m->prev_pass_start_ms;
        m->open_ref_flags = (uint8_t)(m->prev_ref_flags
            | (m->cur_ref_flags & (uint8_t)(GBT_FI04_REF_OVERFLOW | GBT_FI04_REF_STATUS_ERR)));
    } else {
        /* outside a pass (attach / detach): the triggering call came after the last pass began */
        m->open_t0_ms = m->pass_start_ms;
        m->open_ref_flags = (uint8_t)((m->passes == 0U) ? GBT_FI04_REF_NO_PREV : 0U);
    }
}

static void fi04_close(gbt_fi04_t *m, const gbt_output_cmd_t *cmd, uint32_t confirmed_seq, uint32_t t1_ms)
{
    m->open = 0U;
    m->t0_ms = m->open_t0_ms;
    m->t1_ms = t1_ms;
    m->elapsed_ms = t1_ms - m->open_t0_ms;              /* unsigned: wrap-safe */
    m->first_seq = m->open_seq;
    m->accept_seq = cmd->seq;
    m->confirmed_seq = confirmed_seq;
    m->ref_flags = m->open_ref_flags;
    fi04_sat_inc(&m->shutdowns);
    if (m->ref_flags != 0U) {
        m->verdict = (uint8_t)GBT_FI04_VERDICT_INVALID;
        fi04_sat_inc(&m->invalid_count);
        return;
    }
    if (m->elapsed_ms > m->max_valid_ms) {
        m->max_valid_ms = m->elapsed_ms;
    }
    if (m->elapsed_ms <= GBT_FI04_LIMIT_MS) {
        m->verdict = (uint8_t)GBT_FI04_VERDICT_PASS;
        fi04_sat_inc(&m->pass_count);
    } else {
        m->verdict = (uint8_t)GBT_FI04_VERDICT_FAIL;
        fi04_sat_inc(&m->fail_count);
    }
}

void gbt_fi04_on_apply(gbt_fi04_t *m, const gbt_output_cmd_t *cmd, gbt_hal_status_t st,
                       uint32_t confirmed_seq, uint32_t t1_ms)
{
    if (m == NULL || cmd == NULL) {
        return;
    }
    if (cmd->enabled == 0U && m->acc_enabled != 0U && m->open == 0U) {
        fi04_open(m, cmd->seq);
    }
    if (m->open != 0U) {
        m->apply_status = st;
        if (st != GBT_HAL_OK) {
            fi04_sat_inc(&m->refused);                  /* stays open: the retry is measured */
        } else if (cmd->enabled != 0U) {
            m->open = 0U;                               /* output re-enabled before a confirmed shutdown */
            fi04_sat_inc(&m->aborted);
        } else if (confirmed_seq != cmd->seq) {
            fi04_sat_inc(&m->mismatch);                 /* not a confirmed shutdown: stays open */
        } else {
            fi04_close(m, cmd, confirmed_seq, t1_ms);
        }
    }
    if (st == GBT_HAL_OK) {
        m->acc_enabled = (uint8_t)((cmd->enabled != 0U) ? 1U : 0U);
    }
}

/* APPLY: deliver a changed command through the ND-1 sink, then confirm it. */
static void apply(gbt_integ_t *ig)
{
    gbt_output_cmd_t cmd;
    gbt_hal_status_t st;
    uint32_t t1;
    gbt_node_get_output(ig->node, &cmd);
    if (cmd.seq == ig->applied_seq) {
        return;
    }
    st = gbt_hal_output_apply(ig->hal, &cmd);
    t1 = gbt_hal_now_ms(ig->hal);               /* FI-04 T1: fresh, right after the sink returned */
    ig->applied_seq = cmd.seq;
    /* STEP 07-C2 diagnostics: an enabled -> disabled change between two commands delivered
     * to the sink. The initial disabled command (seq 1) does not count: last_cmd starts zeroed. */
    if (ig->last_cmd.enabled != 0U && cmd.enabled == 0U && ig->output_shutdowns != 0xFFFFFFFFu) {
        ig->output_shutdowns++;
    }
    ig->last_cmd = cmd;
    ig->last_apply_status = st;
    gbt_node_confirm_output(ig->node, cmd.seq, st == GBT_HAL_OK, gbt_hal_now_ms(ig->hal));
    gbt_fi04_on_apply(&ig->fi04, &cmd, st, ig->node->out_accepted_seq, t1);   /* FI-04 diagnostics */
}

gbt_hal_status_t gbt_integ_attach(gbt_integ_t *ig, gbt_node_t *node, gbt_hal_t *hal,
                                  const gbt_node_config_t *cfg, const gbt_output_sink_t *sink)
{
    gbt_hal_status_t st;
    if (ig == NULL || node == NULL || hal == NULL || cfg == NULL || sink == NULL) {
        return GBT_HAL_E_PARAM;
    }
    memset(ig, 0, sizeof *ig);
    ig->node = node;
    ig->hal = hal;
    st = gbt_hal_bind_output_sink(hal, sink);
    if (st != GBT_HAL_OK) {
        return st;
    }
    st = gbt_hal_claim(hal);
    if (st != GBT_HAL_OK) {
        (void)gbt_hal_unbind_output_sink(hal);
        return st;
    }
    gbt_node_init(node, cfg);
    apply(ig);                                  /* initial disabled command (seq 1) */
    return GBT_HAL_OK;
}

/* STEP 07-C2 diagnostics: a frame was accepted by the HAL driver (gbt_hal_can_send == OK).
 * TP frames are recognised by the PDU format configured in the node's TP layer. */
static void sat_inc_u32(uint32_t *c)
{
    if (*c != 0xFFFFFFFFu) {
        (*c)++;
    }
}

static void count_hal_accepted(gbt_integ_t *ig, const gbt_can_frame_t *f)
{
    uint8_t pf = (uint8_t)((f->id >> 16) & 0xFFU);
    sat_inc_u32(&ig->tx_hal_accepted);
    if ((f->flags & GBT_CAN_FLAG_EXT) != 0U) {
        if (pf == ig->node->tp.prm.pf_cm) {
            sat_inc_u32(&ig->tx_hal_accepted_tp_cm);
        } else if (pf == ig->node->tp.prm.pf_dt) {
            sat_inc_u32(&ig->tx_hal_accepted_tp_dt);
        }
    }
}

/* ---- STEP 07-C2.4 diagnostics: event log and per-message TP completions (main context) ---- */
static void ev_put(gbt_integ_t *ig, uint8_t type, uint32_t pgn, uint8_t a, uint8_t b, uint8_t c,
                   uint32_t now)
{
    gbt_evlog_t *lg = ig->evlog;
    gbt_evlog_entry_t *e;
    if (lg == NULL) {
        return;
    }
    if (lg->count >= GBT_EVLOG_LEN) {
        lg->frozen = 1U;                        /* full: freeze, never overwrite */
        sat_inc_u32(&lg->dropped);
        return;
    }
    e = &lg->e[lg->count];
    e->t_ms = now;
    e->pgn = pgn;
    e->type = type;
    e->a = a;
    e->b = b;
    e->c = c;
    lg->count++;
    if (lg->count >= GBT_EVLOG_LEN) {
        lg->frozen = 1U;
    }
}

/* 1 = TP.CM, 2 = TP.DT, 0 = not a TP frame of this node pair. rx: frame from the peer. */
static uint8_t tp_kind(const gbt_integ_t *ig, const gbt_can_frame_t *f, uint8_t rx)
{
    const gbt_tp_t *tp = &ig->node->tp;
    uint8_t pf = (uint8_t)((f->id >> 16) & 0xFFU);
    uint8_t ps = (uint8_t)((f->id >> 8) & 0xFFU);
    uint8_t sa = (uint8_t)(f->id & 0xFFU);
    if ((f->flags & GBT_CAN_FLAG_EXT) == 0U
        || ps != ((rx != 0U) ? tp->own_sa : tp->peer_sa) || sa != ((rx != 0U) ? tp->peer_sa : tp->own_sa)) {
        return 0U;
    }
    if (pf == tp->prm.pf_cm) {
        return 1U;
    }
    return (uint8_t)((pf == tp->prm.pf_dt) ? 2U : 0U);
}

static void log_tp_frame(gbt_integ_t *ig, const gbt_can_frame_t *f, uint8_t rx, uint32_t now)
{
    const gbt_tp_t *tp = &ig->node->tp;
    uint8_t k = tp_kind(ig, f, rx);
    if (ig->evlog == NULL || k == 0U) {
        return;
    }
    if (k == 1U) {
        uint32_t pgn = (uint32_t)f->data[5] | ((uint32_t)f->data[6] << 8) | ((uint32_t)f->data[7] << 16);
        ev_put(ig, (uint8_t)((rx != 0U) ? GBT_EV_RX_TP_CM : GBT_EV_TX_TP_CM), pgn, f->data[0], f->data[1],
               f->data[2], now);
    } else if (rx != 0U) {
        ev_put(ig, (uint8_t)GBT_EV_RX_TP_DT, (tp->rx.state == GBT_TP_RX_DATA) ? tp->rx.pgn : 0U, f->data[0],
               f->data[1], 0U, now);
    } else {
        ev_put(ig, (uint8_t)GBT_EV_TX_TP_DT, tp->tx.pgn, f->data[0], f->data[1], 0U, now);
    }
}

static unsigned msg_index(uint32_t pgn)
{
    static const gbt_msg_t tp_msgs[3] = { GBT_MSG_BRM, GBT_MSG_BCP, GBT_MSG_BCS };
    unsigned i;
    for (i = 0U; i < 3U; i++) {
        const gbt_msg_def_t *d = gbt_msg_def(tp_msgs[i]);
        if (d != NULL && d->pgn == pgn) {
            return i;
        }
    }
    return 3U;
}

static uint8_t u8_sat(uint32_t v)
{
    return (uint8_t)((v > 0xFFU) ? 0xFFU : v);
}

static void log_tp_end(gbt_integ_t *ig, uint8_t ev, uint32_t now)
{
    const gbt_tp_t *tp = &ig->node->tp;
    if ((ev & (GBT_TP_EV_RX_COMPLETE | GBT_TP_EV_RX_ABORTED)) != 0U) {
        ev_put(ig, (uint8_t)GBT_EV_TP_END, tp->rx.pgn, ev, u8_sat(tp->stats.rx_sess_dt), tp->rx.packets, now);
    } else {
        ev_put(ig, (uint8_t)GBT_EV_TP_END, tp->tx.pgn, ev, u8_sat(tp->stats.tx_sess_dt), tp->tx.packets, now);
    }
}

/* Called after every node interaction: step changes, TP completions, TP session ends. */
static void observe(gbt_integ_t *ig, uint32_t now)
{
    const gbt_tp_stats_t *ts = &ig->node->tp.stats;
    if (ts->rx_complete != ig->seen_rx_complete) {          /* at most one per node call */
        sat_inc_u32(&ig->tp_rx_complete_msg[msg_index(ts->last_rx_complete_pgn)]);
        ig->seen_rx_complete = ts->rx_complete;
    }
    if (ts->tx_complete != ig->seen_tx_complete) {
        sat_inc_u32(&ig->tp_tx_complete_msg[msg_index(ts->last_tx_complete_pgn)]);
        ig->seen_tx_complete = ts->tx_complete;
    }
    if (ts->sessions_ended != ig->seen_sessions_ended) {
        if ((uint32_t)(ts->sessions_ended - ig->seen_sessions_ended) > 1U) {
            /* two ends in one call: the receive session always ends first (gbt_tp_tick, gbt_tp_on_frame) */
            log_tp_end(ig, (uint8_t)GBT_TP_EV_RX_ABORTED, now);
        }
        log_tp_end(ig, ts->last_end_ev, now);
        ig->seen_sessions_ended = ts->sessions_ended;
    }
    if (ig->node->step != ig->seen_step) {
        ev_put(ig, (uint8_t)GBT_EV_STEP, 0U, ig->node->step, ig->seen_step, ig->node->phase, now);
        ig->seen_step = ig->node->step;
        sat_inc_u32(&ig->step_changes);
    }
}

void gbt_integ_set_evlog(gbt_integ_t *ig, gbt_evlog_t *log)
{
    if (ig == NULL) {
        return;
    }
    if (log != NULL) {
        memset(log, 0, sizeof *log);
    }
    ig->evlog = log;
}

static void send(gbt_integ_t *ig, uint32_t now)
{
    unsigned budget = GBT_OUTBOX_LEN + 1U;      /* bounded */
    while (budget-- > 0U) {
        if (ig->has_pending == 0U) {
            if (!gbt_node_pop_tx(ig->node, &ig->pending)) {
                return;
            }
            ig->has_pending = 1U;
        }
        switch (gbt_node_tx_check(ig->node, &ig->pending)) {
        case GBT_NODE_TX_HOLD:
            return;
        case GBT_NODE_TX_DISCARD:
            ig->has_pending = 0U;
            continue;
        default:
            break;
        }
        if (ig->tx_filter != NULL && ig->tx_filter(ig->tx_filter_ctx, &ig->pending) == 0U) {
            ig->has_pending = 0U;               /* injected loss */
            ig->tx_filtered++;
            continue;
        }
        switch (gbt_hal_can_send(ig->hal, &ig->pending)) {
        case GBT_HAL_OK:
            count_hal_accepted(ig, &ig->pending);   /* STEP 07-C2 diagnostics (before release) */
            log_tp_frame(ig, &ig->pending, 0U, now);   /* STEP 07-C2.4 event log */
            ig->has_pending = 0U;
            break;
        case GBT_HAL_E_FULL:
            ig->tx_full_retries++;              /* keep it; retry first next step */
            return;
        default:
            ig->has_pending = 0U;               /* ND-3 PROVISIONAL: discard and count */
            ig->tx_dropped++;
            break;
        }
    }
}

void gbt_integ_step(gbt_integ_t *ig)
{
    gbt_can_status_t cs;
    gbt_can_frame_t f;
    gbt_hal_status_t rs = GBT_HAL_E_EMPTY;      /* FI-04: status that ended the RX loop */
    uint32_t now;
    unsigned i;

    if (ig == NULL || ig->node == NULL) {
        return;
    }
    now = gbt_hal_now_ms(ig->hal);
    gbt_fi04_pass_begin(&ig->fi04, now);        /* FI-04: reference bookkeeping only */
    observe(ig, now);                           /* STEP 07-C2.4: changes made outside the loop */
    if (gbt_hal_can_get_status(ig->hal, &cs) == GBT_HAL_OK) {
        uint32_t total = cs.rx_overflow_sw + cs.rx_overflow_hw;
        if (total != ig->last_ovf_total) {
            gbt_fi04_pass_note(&ig->fi04, (uint8_t)GBT_FI04_REF_OVERFLOW);
            gbt_node_on_rx_overflow(ig->node, total - ig->last_ovf_total, now);
            ig->last_ovf_total = total;
            apply(ig);
        }
    } else {
        gbt_fi04_pass_note(&ig->fi04, (uint8_t)GBT_FI04_REF_STATUS_ERR);
    }
    for (i = 0U; i < GBT_INTEG_RX_PER_STEP; i++) {
        rs = gbt_hal_can_receive(ig->hal, &f);
        if (rs != GBT_HAL_OK) {
            break;
        }
        ig->last_rx = f;                        /* STEP 07-C2 diagnostics (main context) */
        ig->last_rx_valid = 1U;
        if (ig->rx_seq != 0xFFFFFFFFu) {
            ig->rx_seq++;
        }
        log_tp_frame(ig, &f, 1U, now);          /* STEP 07-C2.4 event log (before the node) */
        gbt_node_on_rx(ig->node, &f, now);
        apply(ig);                              /* output before any frame of this event */
        observe(ig, now);
    }
    if (i >= GBT_INTEG_RX_PER_STEP) {
        gbt_fi04_pass_note(&ig->fi04, (uint8_t)GBT_FI04_REF_RX_LIMIT);   /* queue may not be drained */
    } else if (rs != GBT_HAL_E_EMPTY) {
        gbt_fi04_pass_note(&ig->fi04, (uint8_t)GBT_FI04_REF_RX_ERROR);   /* not the normal empty status */
    }
    {
        gbt_output_fb_t fb;
        if (gbt_hal_output_feedback(ig->hal, &fb) == GBT_HAL_OK) {
            gbt_node_set_feedback(ig->node, &fb);
        }
    }
    gbt_node_tick(ig->node, now);
    apply(ig);
    observe(ig, now);
    send(ig, now);
    gbt_fi04_pass_end(&ig->fi04);
}

gbt_hal_status_t gbt_integ_detach(gbt_integ_t *ig)
{
    gbt_hal_status_t st;
    if (ig == NULL || ig->node == NULL) {
        return GBT_HAL_E_PARAM;
    }
    gbt_node_shutdown(ig->node, gbt_hal_now_ms(ig->hal));
    apply(ig);
    st = gbt_hal_release(ig->hal);
    if (st == GBT_HAL_OK) {
        st = gbt_hal_unbind_output_sink(ig->hal);
    }
    return st;
}
