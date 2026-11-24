/*
 * protocol/gbt_tp.c - see gbt_tp.h.
 * Byte layouts below are PROVISIONAL (P-K4-07, model knowledge of SAE
 * J1939-21; not verified against the 2006 edition):
 *   RTS  [ctrl_rts, size_lo, size_hi, packets, max_per_cts, pgn0, pgn1, pgn2]
 *   CTS  [ctrl_cts, num_packets, next_seq, 0xFF, 0xFF, pgn0, pgn1, pgn2]
 *   EOM  [ctrl_eom, size_lo, size_hi, packets, 0xFF, pgn0, pgn1, pgn2]
 *   ABRT [ctrl_abort, reason, 0xFF, 0xFF, 0xFF, pgn0, pgn1, pgn2]
 *   DT   [seq, d0..d6]   unused bytes of the last DT = 0xFF
 *
 * STEP 07-C2: tp->stats (gbt_tp_stats_t) is diagnostic accounting only. The
 * statistics are written next to existing transitions and are never read by
 * this module; no condition, state, timer or frame content depends on them.
 * STEP 07-C2.4: the same rule applies to the session timing fields of tp->stats
 * and to the BRM payload snapshots tp->snap_tx / tp->snap_rx.
 */
#include "protocol/gbt_tp.h"
#include "protocol/gbt_id.h"

#include <string.h>

void gbt_tp_default_params(gbt_tp_params_t *p)
{
    if (p == NULL) {
        return;
    }
    p->pf_cm = 0xECU;            /* P-K4-01 SECONDARY_CORROBORATION (AUTOSAR R4.2.2 -> J1939-21 2006-12) */
    p->pf_dt = 0xEBU;            /* P-K4-02 SECONDARY_CORROBORATION */
    p->priority = 7U;            /* P-K4-03 PROVISIONAL */
    p->ctrl_rts = 16U;           /* P-K4-04 SECONDARY_CORROBORATION (Linux j1939; edition unverified) */
    p->ctrl_cts = 17U;
    p->ctrl_eom = 19U;
    p->ctrl_abort = 255U;
    p->first_seq = 1U;           /* P-K4-09 PROVISIONAL */
    p->abort_busy = 1U;          /* P-K4-10 */
    p->abort_resources = 2U;
    p->abort_timeout = 3U;       /* SECONDARY_CORROBORATION (AUTOSAR R4.2.2 SWS_J1939Tp_00100) */
    p->abort_seq = 7U;           /* PROVISIONAL: later-edition code, 2006 value UNKNOWN */
    p->dt_interval_ms = 10U;     /* VERIFIED_GBT: GB/T 27930-2023 §7.5 */
    p->t1_ms = 750U;             /* P-K4-05 PROVISIONAL (model knowledge) */
    p->t2_ms = 1250U;
    p->t3_ms = 1250U;
    p->t4_ms = 1050U;
}

void gbt_tp_init(gbt_tp_t *tp, uint8_t own_sa, uint8_t peer_sa, const gbt_tp_params_t *prm)
{
    if (tp == NULL) {
        return;
    }
    memset(tp, 0, sizeof *tp);
    if (prm != NULL) {
        tp->prm = *prm;
    } else {
        gbt_tp_default_params(&tp->prm);
    }
    tp->own_sa = own_sa;
    tp->peer_sa = peer_sa;
}

uint8_t gbt_tp_tx_busy(const gbt_tp_t *tp)
{
    return (tp != NULL && tp->tx.state != GBT_TP_IDLE) ? 1U : 0U;
}

/* ---- diagnostics (STEP 07-C2): saturating increment, never read by the TP logic ---- */
static void st_inc(uint32_t *c)
{
    if (*c != 0xFFFFFFFFu) {
        (*c)++;
    }
}

/* STEP 07-C2.4: record a session end (last_end_ev + sessions_ended). */
static void st_end(gbt_tp_t *tp, uint8_t ev)
{
    tp->stats.last_end_ev = ev;
    st_inc(&tp->stats.sessions_ended);
}

static void st_max(uint32_t *m, uint32_t v)
{
    if (v > *m) {
        *m = v;
    }
}

/* STEP 07-C2.4: copy a payload into a diagnostic snapshot (len <= GBT_TP_MAX_LEN checked by callers). */
static void snap_copy(gbt_tp_snap_t *sn, uint32_t pgn, const uint8_t *buf, uint16_t len, uint8_t state)
{
    memcpy(sn->buf, buf, len);
    sn->len = len;
    sn->pgn = pgn;
    sn->state = state;
    st_inc(&sn->captures);
}

/* ---- frame helpers ---- */
/* Returns 1 if the frame was queued in the outbox, 0 if the outbox was full
 * (the return value is used for statistics only). */
static uint8_t emit(gbt_tp_t *tp, gbt_outbox_t *ob, uint8_t pf, const uint8_t d[8])
{
    gbt_can_frame_t f;
    uint32_t id = 0U;
    (void)gbt_id_make(tp->prm.priority, pf, tp->peer_sa, tp->own_sa, &id);
    f.id = id;
    f.dlc = 8U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    memcpy(f.data, d, 8U);
    if (gbt_outbox_push(ob, &f) != GBT_RING_OK) {
        tp->outbox_full++;
        return 0U;
    }
    return 1U;
}

static void put_pgn(uint8_t d[8], uint32_t pgn)
{
    d[5] = (uint8_t)(pgn & 0xFFU);
    d[6] = (uint8_t)((pgn >> 8) & 0xFFU);
    d[7] = (uint8_t)((pgn >> 16) & 0xFFU);
}

static uint8_t emit_cm(gbt_tp_t *tp, gbt_outbox_t *ob, uint8_t ctrl, uint8_t b1, uint8_t b2,
                       uint8_t b3, uint8_t b4, uint32_t pgn)
{
    uint8_t d[8];
    d[0] = ctrl;
    d[1] = b1;
    d[2] = b2;
    d[3] = b3;
    d[4] = b4;
    put_pgn(d, pgn);
    return emit(tp, ob, tp->prm.pf_cm, d);
}

static void emit_abort(gbt_tp_t *tp, gbt_outbox_t *ob, uint8_t reason, uint32_t pgn)
{
    if (emit_cm(tp, ob, tp->prm.ctrl_abort, reason, 0xFFU, 0xFFU, 0xFFU, pgn) != 0U) {
        st_inc(&tp->stats.abort_tx);                      /* STEP 07-C2: abort queued */
        tp->stats.last_abort_reason_tx = reason;
    }
    tp->aborts_sent++;
}

static uint8_t packets_for(uint16_t len)
{
    return (uint8_t)((len + GBT_TP_BYTES_PER_DT - 1U) / GBT_TP_BYTES_PER_DT);
}

static uint8_t last_seq(const gbt_tp_session_t *s, uint8_t first)
{
    return (uint8_t)(first + s->packets - 1U);
}

/* ---- sender ---- */
uint8_t gbt_tp_tx_start(gbt_tp_t *tp, uint32_t pgn, const uint8_t *data, uint16_t len,
                        uint32_t now_ms, gbt_outbox_t *ob)
{
    gbt_tp_session_t *s;
    if (tp == NULL || data == NULL || ob == NULL || len < 9U || len > GBT_TP_MAX_LEN) {
        return 0U;
    }
    s = &tp->tx;
    if (s->state != GBT_TP_IDLE) {
        return 0U;
    }
    memcpy(s->buf, data, len);
    s->pgn = pgn;
    s->len = len;
    s->packets = packets_for(len);
    s->next_seq = tp->prm.first_seq;
    s->window_end = 0U;
    if (emit_cm(tp, ob, tp->prm.ctrl_rts, (uint8_t)(len & 0xFFU), (uint8_t)(len >> 8),
                s->packets, 0xFFU, pgn) != 0U) {          /* 0xFF: no per-CTS limit (P-K4-07) */
        st_inc(&tp->stats.rts_tx);                        /* STEP 07-C2: RTS queued */
    }
    s->state = GBT_TP_TX_WAIT_CTS;
    gbt_timer_arm(&s->wait, now_ms, tp->prm.t3_ms);
    st_inc(&tp->stats.tx_started);                        /* STEP 07-C2: send session opened */
    tp->stats.tx_sess_start_ms = now_ms;                  /* STEP 07-C2.4 */
    tp->stats.tx_sess_dt = 0U;
    if (pgn == GBT_TP_SNAP_PGN && tp->snap_tx.state != GBT_TP_SNAP_FROZEN) {
        snap_copy(&tp->snap_tx, pgn, s->buf, len, GBT_TP_SNAP_CAPTURED);
    }
    return 1U;
}

static void tx_send_dt(gbt_tp_t *tp, gbt_outbox_t *ob, uint32_t now_ms)
{
    gbt_tp_session_t *s = &tp->tx;
    uint8_t d[8];
    uint32_t off = (uint32_t)(s->next_seq - tp->prm.first_seq) * GBT_TP_BYTES_PER_DT;
    uint32_t i;

    d[0] = s->next_seq;
    for (i = 0U; i < GBT_TP_BYTES_PER_DT; i++) {
        d[i + 1U] = (off + i < s->len) ? s->buf[off + i] : 0xFFU;
    }
    if (s->next_seq != tp->prm.first_seq) {               /* STEP 07-C2.4: not the first DT */
        st_max(&tp->stats.max_tx_dt_interval_ms, gbt_time_elapsed(now_ms, s->last_dt_ms));
    }
    if (emit(tp, ob, tp->prm.pf_dt, d) != 0U) {
        st_inc(&tp->stats.dt_tx);                         /* STEP 07-C2: DT queued */
        st_inc(&tp->stats.tx_sess_dt);                    /* STEP 07-C2.4 */
    }
    s->last_dt_ms = now_ms;
    if (s->next_seq == s->window_end) {
        s->state = GBT_TP_TX_WAIT_CTS;                    /* wait CTS or EOM */
        gbt_timer_arm(&s->wait, now_ms, tp->prm.t3_ms);
    }
    s->next_seq++;
}

static uint8_t tx_on_cm(gbt_tp_t *tp, uint8_t ctrl, const uint8_t *d, uint32_t pgn,
                        uint32_t now_ms, gbt_outbox_t *ob)
{
    gbt_tp_session_t *s = &tp->tx;
    if (s->state == GBT_TP_IDLE || pgn != s->pgn) {
        return GBT_TP_EV_NONE;
    }
    if (ctrl == tp->prm.ctrl_cts && (s->state == GBT_TP_TX_WAIT_CTS || s->state == GBT_TP_TX_HOLD)) {
        uint8_t n = d[1];
        uint8_t next = d[2];
        if (n == 0U) {
            s->state = GBT_TP_TX_HOLD;
            gbt_timer_arm(&s->wait, now_ms, tp->prm.t4_ms);
            st_inc(&tp->stats.cts_rx);                     /* STEP 07-C2: CTS(0) hold accepted */
            return GBT_TP_EV_NONE;
        }
        if (next != s->next_seq || next > last_seq(s, tp->prm.first_seq)) {
            emit_abort(tp, ob, tp->prm.abort_resources, s->pgn);   /* no retransmission (Lean) */
            s->state = GBT_TP_IDLE;
            st_end(tp, (uint8_t)GBT_TP_EV_TX_ABORTED);              /* STEP 07-C2 */
            return GBT_TP_EV_TX_ABORTED;
        }
        st_inc(&tp->stats.cts_rx);                         /* STEP 07-C2: CTS window accepted */
        s->window_end = (uint8_t)(next + n - 1U);
        if (s->window_end > last_seq(s, tp->prm.first_seq)) {
            s->window_end = last_seq(s, tp->prm.first_seq);
        }
        s->state = GBT_TP_TX_SENDING;
        gbt_timer_disarm(&s->wait);
        tx_send_dt(tp, ob, now_ms);                        /* first DT immediately */
        return GBT_TP_EV_NONE;
    }
    if (ctrl == tp->prm.ctrl_eom && s->state == GBT_TP_TX_WAIT_CTS
        && s->next_seq > last_seq(s, tp->prm.first_seq)) {
        s->state = GBT_TP_IDLE;
        gbt_timer_disarm(&s->wait);
        st_inc(&tp->stats.eom_rx);                         /* STEP 07-C2: EOM accepted */
        st_inc(&tp->stats.tx_complete);
        tp->stats.last_tx_complete_pgn = s->pgn;
        tp->stats.last_tx_complete_len = s->len;
        st_end(tp, (uint8_t)GBT_TP_EV_TX_COMPLETE);
        tp->stats.last_tx_duration_ms = gbt_time_elapsed(now_ms, tp->stats.tx_sess_start_ms);   /* C2.4 */
        st_max(&tp->stats.max_tx_duration_ms, tp->stats.last_tx_duration_ms);
        if (s->pgn == GBT_TP_SNAP_PGN && tp->snap_tx.state == GBT_TP_SNAP_CAPTURED) {
            tp->snap_tx.state = GBT_TP_SNAP_FROZEN;            /* the captured payload was delivered */
        }
        return GBT_TP_EV_TX_COMPLETE;
    }
    if (ctrl == tp->prm.ctrl_abort) {
        s->state = GBT_TP_IDLE;
        tp->aborts_received++;
        st_inc(&tp->stats.abort_rx);                       /* STEP 07-C2: send session aborted by peer */
        tp->stats.last_abort_reason_rx = d[1];
        st_end(tp, (uint8_t)GBT_TP_EV_TX_ABORTED);
        return GBT_TP_EV_TX_ABORTED;
    }
    return GBT_TP_EV_NONE;
}

/* ---- receiver ---- */
static uint8_t rx_on_cm(gbt_tp_t *tp, uint8_t ctrl, const uint8_t *d, uint32_t pgn,
                        uint32_t now_ms, gbt_outbox_t *ob, gbt_tp_accept_fn accept)
{
    gbt_tp_session_t *s = &tp->rx;
    if (ctrl == tp->prm.ctrl_rts) {
        uint16_t size = (uint16_t)((uint16_t)d[1] | (uint16_t)((uint16_t)d[2] << 8));
        uint8_t max = d[4];
        if (s->state != GBT_TP_IDLE) {
            emit_abort(tp, ob, tp->prm.abort_busy, pgn);   /* one session per direction (TP-4) */
            return GBT_TP_EV_NONE;
        }
        if (size < 9U || size > GBT_TP_MAX_LEN || d[3] != packets_for(size)
            || accept == NULL || accept(pgn, size) == 0U) {
            emit_abort(tp, ob, tp->prm.abort_resources, pgn);
            return GBT_TP_EV_NONE;
        }
        s->pgn = pgn;
        s->len = size;
        s->packets = d[3];
        s->next_seq = tp->prm.first_seq;
        s->window_end = (max == 0xFFU || max == 0U || max >= s->packets)
                            ? last_seq(s, tp->prm.first_seq)
                            : (uint8_t)(tp->prm.first_seq + max - 1U);
        memset(s->buf, 0xFF, sizeof s->buf);
        s->state = GBT_TP_RX_DATA;
        st_inc(&tp->stats.rts_rx);                         /* STEP 07-C2: RTS accepted */
        st_inc(&tp->stats.rx_started);                     /* receive session opened */
        tp->stats.rx_sess_start_ms = now_ms;               /* STEP 07-C2.4 */
        tp->stats.rx_sess_dt = 0U;
        if (emit_cm(tp, ob, tp->prm.ctrl_cts, (uint8_t)(s->window_end - s->next_seq + 1U),
                    s->next_seq, 0xFFU, 0xFFU, pgn) != 0U) {
            st_inc(&tp->stats.cts_tx);                     /* STEP 07-C2: CTS queued */
        }
        gbt_timer_arm(&s->wait, now_ms, tp->prm.t2_ms);
        return GBT_TP_EV_NONE;
    }
    if (ctrl == tp->prm.ctrl_abort && s->state != GBT_TP_IDLE && pgn == s->pgn) {
        s->state = GBT_TP_IDLE;
        gbt_timer_disarm(&s->wait);
        tp->aborts_received++;
        st_inc(&tp->stats.abort_rx);                       /* STEP 07-C2: receive session aborted by peer */
        tp->stats.last_abort_reason_rx = d[1];
        st_end(tp, (uint8_t)GBT_TP_EV_RX_ABORTED);
        return GBT_TP_EV_RX_ABORTED;
    }
    return GBT_TP_EV_NONE;
}

static uint8_t rx_on_dt(gbt_tp_t *tp, const uint8_t *d, uint32_t now_ms, gbt_outbox_t *ob)
{
    gbt_tp_session_t *s = &tp->rx;
    uint32_t off;
    uint32_t i;
    uint8_t last;

    if (s->state != GBT_TP_RX_DATA) {
        return GBT_TP_EV_NONE;
    }
    if (d[0] != s->next_seq) {                     /* gap or duplicate: abort (TP-5, Lean) */
        emit_abort(tp, ob, tp->prm.abort_seq, s->pgn);
        s->state = GBT_TP_IDLE;
        gbt_timer_disarm(&s->wait);
        st_end(tp, (uint8_t)GBT_TP_EV_RX_ABORTED);              /* STEP 07-C2 */
        return GBT_TP_EV_RX_ABORTED;
    }
    off = (uint32_t)(d[0] - tp->prm.first_seq) * GBT_TP_BYTES_PER_DT;
    for (i = 0U; i < GBT_TP_BYTES_PER_DT && off + i < s->len; i++) {
        s->buf[off + i] = d[i + 1U];
    }
    st_inc(&tp->stats.dt_rx_ok);                   /* STEP 07-C2: expected DT accepted and stored */
    if (tp->stats.rx_sess_dt != 0U) {              /* STEP 07-C2.4: not the first DT of the session */
        st_max(&tp->stats.max_rx_dt_interval_ms, gbt_time_elapsed(now_ms, tp->stats.rx_last_dt_ms));
    }
    tp->stats.rx_last_dt_ms = now_ms;
    st_inc(&tp->stats.rx_sess_dt);
    last = last_seq(s, tp->prm.first_seq);
    if (d[0] == last) {
        if (emit_cm(tp, ob, tp->prm.ctrl_eom, (uint8_t)(s->len & 0xFFU), (uint8_t)(s->len >> 8),
                    s->packets, 0xFFU, s->pgn) != 0U) {
            st_inc(&tp->stats.eom_tx);             /* STEP 07-C2: EOM queued */
        }
        s->state = GBT_TP_IDLE;
        gbt_timer_disarm(&s->wait);
        st_inc(&tp->stats.rx_complete);            /* STEP 07-C2: receive session complete */
        tp->stats.last_rx_complete_pgn = s->pgn;
        tp->stats.last_rx_complete_len = s->len;
        st_end(tp, (uint8_t)GBT_TP_EV_RX_COMPLETE);
        tp->stats.last_rx_duration_ms = gbt_time_elapsed(now_ms, tp->stats.rx_sess_start_ms);   /* C2.4 */
        st_max(&tp->stats.max_rx_duration_ms, tp->stats.last_rx_duration_ms);
        if (s->pgn == GBT_TP_SNAP_PGN && tp->snap_rx.state != GBT_TP_SNAP_FROZEN) {
            snap_copy(&tp->snap_rx, s->pgn, s->buf, s->len, GBT_TP_SNAP_FROZEN);
        }
        return GBT_TP_EV_RX_COMPLETE;
    }
    s->next_seq++;
    if (d[0] == s->window_end) {                   /* window done: request the rest */
        s->window_end = last;
        if (emit_cm(tp, ob, tp->prm.ctrl_cts, (uint8_t)(last - s->next_seq + 1U), s->next_seq,
                    0xFFU, 0xFFU, s->pgn) != 0U) {
            st_inc(&tp->stats.cts_tx);             /* STEP 07-C2: window CTS queued */
        }
        gbt_timer_arm(&s->wait, now_ms, tp->prm.t2_ms);
    } else {
        gbt_timer_arm(&s->wait, now_ms, tp->prm.t1_ms);
    }
    return GBT_TP_EV_NONE;
}

uint8_t gbt_tp_on_frame(gbt_tp_t *tp, const gbt_can_frame_t *f, uint32_t now_ms,
                        gbt_outbox_t *ob, gbt_tp_accept_fn accept)
{
    gbt_id_fields_t id;
    uint32_t pgn;
    uint8_t ev;

    if (tp == NULL || f == NULL || ob == NULL) {
        return GBT_TP_EV_NONE;
    }
    if ((f->flags & GBT_CAN_FLAG_EXT) == 0U || gbt_id_parse(f->id, &id) != GBT_ID_OK
        || id.ps != tp->own_sa || id.sa != tp->peer_sa
        || (id.pf != tp->prm.pf_cm && id.pf != tp->prm.pf_dt)) {
        return GBT_TP_EV_NOT_TP;
    }
    if (f->dlc != 8U) {
        return GBT_TP_EV_NONE;                     /* malformed TP frame: ignored */
    }
    if (id.pf == tp->prm.pf_dt) {
        return rx_on_dt(tp, f->data, now_ms, ob);
    }
    pgn = (uint32_t)f->data[5] | ((uint32_t)f->data[6] << 8) | ((uint32_t)f->data[7] << 16);
    ev = rx_on_cm(tp, f->data[0], f->data, pgn, now_ms, ob, accept);
    if (f->data[0] != tp->prm.ctrl_rts) {
        ev = (uint8_t)(ev | tx_on_cm(tp, f->data[0], f->data, pgn, now_ms, ob));
    }
    return ev;
}

uint8_t gbt_tp_tick(gbt_tp_t *tp, uint32_t now_ms, gbt_outbox_t *ob)
{
    uint8_t ev = GBT_TP_EV_NONE;
    gbt_tp_session_t *s;

    if (tp == NULL || ob == NULL) {
        return ev;
    }
    s = &tp->rx;
    if (s->state == GBT_TP_RX_DATA && gbt_timer_expired(&s->wait, now_ms) != 0U) {
        emit_abort(tp, ob, tp->prm.abort_timeout, s->pgn);   /* T1/T2 (missing TP_DT) */
        s->state = GBT_TP_IDLE;
        gbt_timer_disarm(&s->wait);
        ev = (uint8_t)(ev | GBT_TP_EV_RX_ABORTED);
        st_inc(&tp->stats.timeout_rx);                       /* STEP 07-C2 */
        st_end(tp, (uint8_t)GBT_TP_EV_RX_ABORTED);
    }
    s = &tp->tx;
    if ((s->state == GBT_TP_TX_WAIT_CTS || s->state == GBT_TP_TX_HOLD)
        && gbt_timer_expired(&s->wait, now_ms) != 0U) {
        emit_abort(tp, ob, tp->prm.abort_timeout, s->pgn);   /* T3/T4 */
        s->state = GBT_TP_IDLE;
        gbt_timer_disarm(&s->wait);
        ev = (uint8_t)(ev | GBT_TP_EV_TX_ABORTED);
        st_inc(&tp->stats.timeout_tx);                       /* STEP 07-C2 */
        st_end(tp, (uint8_t)GBT_TP_EV_TX_ABORTED);
    } else if (s->state == GBT_TP_TX_SENDING
               && gbt_time_elapsed(now_ms, s->last_dt_ms) >= tp->prm.dt_interval_ms) {
        tx_send_dt(tp, ob, now_ms);                          /* GB/T §7.5: 10 ms spacing */
    }
    return ev;
}
