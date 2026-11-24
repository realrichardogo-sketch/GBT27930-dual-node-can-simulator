/*
 * tests/unit/test_tp.c - transport protocol (Stage 3).
 * Vehicle (SA 0xF4) sends, charger (SA 0x56) receives, 1 ms virtual clock.
 * Timer and layout values used here are the PROVISIONAL defaults (P-K4-xx);
 * the 10 ms DT spacing is GB/T §7.5 (VERIFIED_GBT).
 */
#include "protocol/gbt_tp.h"
#include "protocol/gbt_id.h"
#include "tests/support/test_assert.h"

#include <string.h>

#define PGN_BRM 0x000200u
#define PGN_BCP 0x000600u
#define PGN_BCS 0x001100u

typedef struct {
    gbt_tp_t     veh;
    gbt_tp_t     chg;
    gbt_outbox_t veh_ob;
    gbt_outbox_t chg_ob;
    uint32_t     now;
    uint8_t      veh_ev;
    uint8_t      chg_ev;
    uint32_t     dt_times[16];
    uint32_t     dt_count;
    int          drop_dt_seq;      /* drop the DT with this sequence (or -1) */
    int          drop_dt_from;     /* drop every DT with seq >= this (or -1) */
    uint8_t      block_cts;        /* charger never answers */
    gbt_can_frame_t last_abort;
    uint8_t      abort_seen;
    uint8_t      rts_max_patch;    /* STEP 07-C2: != 0 -> rewrite RTS byte 4 (max packets per CTS) in transit */
} rig_t;

static uint8_t accept_all(uint32_t pgn, uint16_t len)
{
    return (uint8_t)(((pgn == PGN_BRM && len == 49U) || (pgn == PGN_BCP && len == 13U)
                      || (pgn == PGN_BCS && len == 9U)) ? 1U : 0U);
}

static void rig_init(rig_t *r)
{
    memset(r, 0, sizeof *r);
    gbt_tp_init(&r->veh, (uint8_t)GBT_ADDR_EVCC, (uint8_t)GBT_ADDR_SECC, NULL);
    gbt_tp_init(&r->chg, (uint8_t)GBT_ADDR_SECC, (uint8_t)GBT_ADDR_EVCC, NULL);
    (void)gbt_outbox_init(&r->veh_ob);
    (void)gbt_outbox_init(&r->chg_ob);
    r->now = 1000U;
    r->drop_dt_seq = -1;
    r->drop_dt_from = -1;
}

static void shuttle_once(rig_t *r)
{
    gbt_can_frame_t f;
    while (gbt_outbox_pop(&r->veh_ob, &f) == GBT_RING_OK) {
        gbt_id_fields_t id;
        (void)gbt_id_parse(f.id, &id);
        if (id.pf == 0xEBU) {
            if (r->dt_count < 16U) {
                r->dt_times[r->dt_count++] = r->now;
            }
            if ((int)f.data[0] == r->drop_dt_seq
                || (r->drop_dt_from >= 0 && (int)f.data[0] >= r->drop_dt_from)) {
                continue;
            }
        }
        if (id.pf == 0xECU && f.data[0] == 16U && r->rts_max_patch != 0U) {
            f.data[4] = r->rts_max_patch;           /* STEP 07-C2: exercise the window-CTS path */
        }
        r->chg_ev = (uint8_t)(r->chg_ev | gbt_tp_on_frame(&r->chg, &f, r->now, &r->chg_ob, accept_all));
    }
    while (gbt_outbox_pop(&r->chg_ob, &f) == GBT_RING_OK) {
        if (f.data[0] == 255U) {
            r->last_abort = f;
            r->abort_seen = 1U;
        }
        if (r->block_cts != 0U) {
            continue;
        }
        r->veh_ev = (uint8_t)(r->veh_ev | gbt_tp_on_frame(&r->veh, &f, r->now, &r->veh_ob, accept_all));
    }
}

/* deliver until both directions are quiet within the same millisecond */
static void shuttle(rig_t *r)
{
    while (gbt_outbox_count(&r->veh_ob) != 0U || gbt_outbox_count(&r->chg_ob) != 0U) {
        shuttle_once(r);
    }
}

static void run(rig_t *r, uint32_t ms)
{
    uint32_t i;
    for (i = 0U; i < ms; i++) {
        r->now++;
        r->veh_ev = (uint8_t)(r->veh_ev | gbt_tp_tick(&r->veh, r->now, &r->veh_ob));
        r->chg_ev = (uint8_t)(r->chg_ev | gbt_tp_tick(&r->chg, r->now, &r->chg_ob));
        shuttle(r);
    }
}

static void test_happy_brm(void)
{
    static rig_t r;
    uint8_t payload[49];
    gbt_can_frame_t rts;
    uint32_t i;

    rig_init(&r);
    for (i = 0U; i < 49U; i++) {
        payload[i] = (uint8_t)(i * 3U + 1U);
    }
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 0U);  /* busy */
    CHECK(gbt_outbox_peek(&r.veh_ob, &rts) == GBT_RING_OK);
    CHECK_EQ_U(rts.id, 0x1CEC56F4u);            /* P 7, PF 0xEC, DA 0x56, SA 0xF4 */
    CHECK_EQ_U(rts.dlc, 8U);
    CHECK_EQ_U(rts.data[0], 16U);
    CHECK_EQ_U(rts.data[1], 49U);
    CHECK_EQ_U(rts.data[2], 0U);
    CHECK_EQ_U(rts.data[3], 7U);                /* ceil(49 / 7) */
    CHECK_EQ_U(rts.data[5], 0x00U);
    CHECK_EQ_U(rts.data[6], 0x02U);             /* PGN 512 */
    CHECK_EQ_U(rts.data[7], 0x00U);
    shuttle(&r);
    run(&r, 200U);
    CHECK((r.chg_ev & GBT_TP_EV_RX_COMPLETE) != 0U);
    CHECK((r.veh_ev & GBT_TP_EV_TX_COMPLETE) != 0U);
    CHECK_EQ_U(r.chg.rx.pgn, PGN_BRM);
    CHECK_EQ_U(r.chg.rx.len, 49U);
    CHECK(memcmp(r.chg.rx.buf, payload, 49U) == 0);
    CHECK_EQ_U(r.dt_count, 7U);
    for (i = 1U; i < r.dt_count; i++) {
        CHECK_EQ_U(r.dt_times[i] - r.dt_times[i - 1U], 10U);   /* GB/T §7.5 */
    }
    CHECK_EQ_U(gbt_tp_tx_busy(&r.veh), 0U);
    CHECK_EQ_U(r.abort_seen, 0U);
}

static void test_bcs_fill(void)
{
    static rig_t r;
    uint8_t payload[9] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U };
    gbt_can_frame_t f;
    uint32_t n = 0U;

    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BCS, payload, 9U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    /* capture the second DT directly from the vehicle outbox */
    while (n < 50U) {
        r.now++;
        (void)gbt_tp_tick(&r.veh, r.now, &r.veh_ob);
        if (gbt_outbox_count(&r.veh_ob) != 0U) {
            CHECK(gbt_outbox_peek(&r.veh_ob, &f) == GBT_RING_OK);
            if (f.data[0] == 2U) {
                break;
            }
        }
        shuttle(&r);
        n++;
    }
    CHECK_EQ_U(f.id, 0x1CEB56F4u);
    CHECK_EQ_U(f.data[1], 8U);
    CHECK_EQ_U(f.data[2], 9U);
    CHECK_EQ_U(f.data[3], 0xFFU);               /* unused bytes of the last DT */
    CHECK_EQ_U(f.data[7], 0xFFU);
    shuttle(&r);
    run(&r, 5U);
    CHECK((r.chg_ev & GBT_TP_EV_RX_COMPLETE) != 0U);
    CHECK(memcmp(r.chg.rx.buf, payload, 9U) == 0);
}

static void test_sequence_gap(void)
{
    static rig_t r;
    uint8_t payload[49];
    memset(payload, 0x5A, sizeof payload);
    rig_init(&r);
    r.drop_dt_seq = 2;
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK((r.chg_ev & GBT_TP_EV_RX_ABORTED) != 0U);
    CHECK((r.chg_ev & GBT_TP_EV_RX_COMPLETE) == 0U);
    CHECK((r.veh_ev & GBT_TP_EV_TX_ABORTED) != 0U);
    CHECK(r.abort_seen == 1U);
    CHECK_EQ_U(r.last_abort.data[1], 7U);       /* P-K4-10 sequence-error reason */
    CHECK_EQ_U(r.chg.rx.state, GBT_TP_IDLE);
}

static void test_missing_dt_timeout(void)
{
    static rig_t r;
    uint8_t payload[49];
    uint32_t t_last_dt;
    memset(payload, 0x33, sizeof payload);
    rig_init(&r);
    r.drop_dt_from = 2;                         /* only DT 1 arrives */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    t_last_dt = r.now;                          /* DT 1 is sent in reply to CTS, same ms */
    run(&r, 749U);                              /* T1 - 1 */
    CHECK((r.chg_ev & GBT_TP_EV_RX_ABORTED) == 0U);
    run(&r, 1U);                                /* T1 = 750 ms after DT 1 (S-5) */
    CHECK_EQ_U(r.now - t_last_dt, 750U);
    CHECK((r.chg_ev & GBT_TP_EV_RX_ABORTED) != 0U);
    CHECK_EQ_U(r.last_abort.data[1], 3U);       /* timeout reason */
}

static void test_sender_timeout_and_rejects(void)
{
    static rig_t r;
    uint8_t payload[49];
    gbt_can_frame_t f;
    memset(payload, 0x11, sizeof payload);

    rig_init(&r);
    r.block_cts = 1U;                           /* charger replies are lost */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 1249U);
    CHECK((r.veh_ev & GBT_TP_EV_TX_ABORTED) == 0U);
    run(&r, 1U);                                /* T3 = 1250 ms */
    CHECK((r.veh_ev & GBT_TP_EV_TX_ABORTED) != 0U);

    /* RTS for an unexpected PGN/size is refused */
    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, 0x001500u, payload, 20U, r.now, &r.veh_ob) == 1U);   /* BMV not accepted */
    shuttle(&r);
    CHECK(r.abort_seen == 1U);
    CHECK_EQ_U(r.last_abort.data[1], 2U);
    CHECK((r.veh_ev & GBT_TP_EV_TX_ABORTED) != 0U);

    /* second RTS while a reception is active is refused with "busy" */
    rig_init(&r);
    r.drop_dt_from = 1;
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    memset(&f, 0xFF, sizeof f);
    f.id = 0x1CEC56F4u;
    f.dlc = 8U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f.data[0] = 16U; f.data[1] = 13U; f.data[2] = 0U; f.data[3] = 2U;
    f.data[5] = 0x00U; f.data[6] = 0x06U; f.data[7] = 0x00U;
    (void)gbt_tp_on_frame(&r.chg, &f, r.now, &r.chg_ob, accept_all);
    r.abort_seen = 0U;
    r.block_cts = 1U;
    shuttle(&r);
    CHECK(r.abort_seen == 1U);
    CHECK_EQ_U(r.last_abort.data[1], 1U);
    CHECK_EQ_U(r.chg.rx.pgn, PGN_BRM);          /* original session kept */

    /* sizes outside 9..64 are refused at the sender */
    CHECK(gbt_tp_tx_start(&r.chg, PGN_BRM, payload, 8U, r.now, &r.chg_ob) == 0U);
    CHECK(gbt_tp_tx_start(&r.chg, PGN_BRM, payload, 65U, r.now, &r.chg_ob) == 0U);
}

static void test_cts_window_and_hold(void)
{
    static rig_t r;
    gbt_can_frame_t f;
    uint8_t payload[49];
    memset(payload, 0x44, sizeof payload);

    /* receiver honours "max packets per CTS" = 3 from the RTS */
    rig_init(&r);
    memset(&f, 0xFF, sizeof f);
    f.id = 0x1CEC56F4u;
    f.dlc = 8U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f.data[0] = 16U; f.data[1] = 49U; f.data[2] = 0U; f.data[3] = 7U; f.data[4] = 3U;
    f.data[5] = 0x00U; f.data[6] = 0x02U; f.data[7] = 0x00U;
    (void)gbt_tp_on_frame(&r.chg, &f, r.now, &r.chg_ob, accept_all);
    CHECK(gbt_outbox_pop(&r.chg_ob, &f) == GBT_RING_OK);
    CHECK_EQ_U(f.id, 0x1CECF456u);
    CHECK_EQ_U(f.data[0], 17U);
    CHECK_EQ_U(f.data[1], 3U);                  /* 3 packets */
    CHECK_EQ_U(f.data[2], 1U);                  /* starting at 1 */

    /* sender: CTS(0) holds, then T4 expires */
    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    (void)gbt_outbox_pop(&r.veh_ob, &f);        /* RTS consumed */
    memset(&f, 0xFF, sizeof f);
    f.id = 0x1CECF456u;
    f.dlc = 8U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f.data[0] = 17U; f.data[1] = 0U; f.data[2] = 1U;
    f.data[5] = 0x00U; f.data[6] = 0x02U; f.data[7] = 0x00U;
    CHECK_EQ_U(gbt_tp_on_frame(&r.veh, &f, r.now, &r.veh_ob, accept_all), GBT_TP_EV_NONE);
    CHECK_EQ_U(r.veh.tx.state, GBT_TP_TX_HOLD);
    CHECK_EQ_U(gbt_tp_tick(&r.veh, r.now + 1049U, &r.veh_ob), GBT_TP_EV_NONE);
    CHECK_EQ_U(gbt_tp_tick(&r.veh, r.now + 1050U, &r.veh_ob), GBT_TP_EV_TX_ABORTED);

    /* non-TP frames are reported as such */
    f.id = 0x181056F4u;
    CHECK_EQ_U(gbt_tp_on_frame(&r.chg, &f, r.now, &r.chg_ob, accept_all), GBT_TP_EV_NOT_TP);
}

/* ======================================================================================
 * STEP 07-C2: TP statistics (gbt_tp_stats_t). Every test checks exact counter values
 * at the documented increment points (docs/TP_COUNTER_SEMANTICS.md). The original
 * tests above are unchanged.
 * ====================================================================================== */

static void make_cm(gbt_can_frame_t *f, uint32_t id, uint8_t ctrl, uint8_t b1, uint8_t b2,
                    uint8_t b3, uint32_t pgn)
{
    memset(f, 0xFF, sizeof *f);
    f->id = id;
    f->dlc = 8U;
    f->flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f->data[0] = ctrl;
    f->data[1] = b1;
    f->data[2] = b2;
    f->data[3] = b3;
    f->data[5] = (uint8_t)(pgn & 0xFFU);
    f->data[6] = (uint8_t)((pgn >> 8) & 0xFFU);
    f->data[7] = (uint8_t)((pgn >> 16) & 0xFFU);
}

static uint8_t stats_are_zero(const gbt_tp_stats_t *s)
{
    gbt_tp_stats_t z;
    memset(&z, 0, sizeof z);
    return (uint8_t)((memcmp(s, &z, sizeof z) == 0) ? 1U : 0U);
}

/* sender-side receive counters and receiver-side send counters stay zero */
static void check_sender_only(const gbt_tp_stats_t *s)
{
    CHECK_EQ_U(s->rts_rx, 0U);
    CHECK_EQ_U(s->cts_tx, 0U);
    CHECK_EQ_U(s->dt_rx_ok, 0U);
    CHECK_EQ_U(s->eom_tx, 0U);
    CHECK_EQ_U(s->rx_started, 0U);
    CHECK_EQ_U(s->rx_complete, 0U);
    CHECK_EQ_U(s->timeout_rx, 0U);
    CHECK_EQ_U(s->last_rx_complete_pgn, 0U);
}

static void check_receiver_only(const gbt_tp_stats_t *s)
{
    CHECK_EQ_U(s->rts_tx, 0U);
    CHECK_EQ_U(s->cts_rx, 0U);
    CHECK_EQ_U(s->dt_tx, 0U);
    CHECK_EQ_U(s->eom_rx, 0U);
    CHECK_EQ_U(s->tx_started, 0U);
    CHECK_EQ_U(s->tx_complete, 0U);
    CHECK_EQ_U(s->timeout_tx, 0U);
    CHECK_EQ_U(s->last_tx_complete_pgn, 0U);
}

static void test_stats_init_and_reset(void)
{
    static rig_t r;
    uint8_t payload[9];
    memset(payload, 0x21, sizeof payload);
    rig_init(&r);
    CHECK(stats_are_zero(&r.veh.stats) == 1U);           /* initialisation */
    CHECK(stats_are_zero(&r.chg.stats) == 1U);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BCS, payload, 9U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 1U);
    CHECK(stats_are_zero(&r.veh.stats) == 0U);
    gbt_tp_init(&r.veh, (uint8_t)GBT_ADDR_EVCC, (uint8_t)GBT_ADDR_SECC, NULL);   /* reset */
    gbt_tp_init(&r.chg, (uint8_t)GBT_ADDR_SECC, (uint8_t)GBT_ADDR_EVCC, NULL);
    CHECK(stats_are_zero(&r.veh.stats) == 1U);
    CHECK(stats_are_zero(&r.chg.stats) == 1U);
}

static void test_stats_happy_and_no_double_count(void)
{
    static rig_t r;
    uint8_t brm[49];
    uint8_t bcs[9];
    gbt_tp_stats_t sv;
    gbt_tp_stats_t sc;
    gbt_can_frame_t f;
    uint32_t i;

    rig_init(&r);
    for (i = 0U; i < 49U; i++) {
        brm[i] = (uint8_t)(i + 7U);
    }
    memset(bcs, 0x5A, sizeof bcs);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, brm, 49U, r.now, &r.veh_ob) == 1U);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, brm, 49U, r.now, &r.veh_ob) == 0U);   /* busy: not counted */
    CHECK_EQ_U(r.veh.stats.rts_tx, 1U);
    CHECK_EQ_U(r.veh.stats.tx_started, 1U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 0U);              /* not complete before EOM */
    shuttle(&r);
    CHECK_EQ_U(r.chg.stats.rts_rx, 1U);
    CHECK_EQ_U(r.chg.stats.cts_tx, 1U);
    CHECK_EQ_U(r.veh.stats.cts_rx, 1U);
    CHECK_EQ_U(r.veh.stats.dt_tx, 1U);                    /* DT 1 in reply to CTS */
    CHECK_EQ_U(r.chg.stats.dt_rx_ok, 1U);
    run(&r, 200U);

    /* sender (vehicle) */
    CHECK_EQ_U(r.veh.stats.rts_tx, 1U);
    CHECK_EQ_U(r.veh.stats.cts_rx, 1U);
    CHECK_EQ_U(r.veh.stats.dt_tx, 7U);
    CHECK_EQ_U(r.veh.stats.eom_rx, 1U);
    CHECK_EQ_U(r.veh.stats.tx_started, 1U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 1U);
    CHECK_EQ_U(r.veh.stats.abort_tx, 0U);
    CHECK_EQ_U(r.veh.stats.abort_rx, 0U);
    CHECK_EQ_U(r.veh.stats.timeout_tx, 0U);
    CHECK_EQ_U(r.veh.stats.last_tx_complete_pgn, PGN_BRM);
    CHECK_EQ_U(r.veh.stats.last_tx_complete_len, 49U);
    CHECK_EQ_U(r.veh.stats.last_end_ev, GBT_TP_EV_TX_COMPLETE);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_tx, 0U);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_rx, 0U);
    check_sender_only(&r.veh.stats);
    /* receiver (charger) */
    CHECK_EQ_U(r.chg.stats.rts_rx, 1U);
    CHECK_EQ_U(r.chg.stats.cts_tx, 1U);
    CHECK_EQ_U(r.chg.stats.dt_rx_ok, 7U);
    CHECK_EQ_U(r.chg.stats.eom_tx, 1U);
    CHECK_EQ_U(r.chg.stats.rx_started, 1U);
    CHECK_EQ_U(r.chg.stats.rx_complete, 1U);
    CHECK_EQ_U(r.chg.stats.abort_tx, 0U);
    CHECK_EQ_U(r.chg.stats.abort_rx, 0U);
    CHECK_EQ_U(r.chg.stats.timeout_rx, 0U);
    CHECK_EQ_U(r.chg.stats.last_rx_complete_pgn, PGN_BRM);
    CHECK_EQ_U(r.chg.stats.last_rx_complete_len, 49U);
    CHECK_EQ_U(r.chg.stats.last_end_ev, GBT_TP_EV_RX_COMPLETE);
    check_receiver_only(&r.chg.stats);
    CHECK(memcmp(r.chg.rx.buf, brm, 49U) == 0);           /* payload reconstructed */

    /* no double counting: idle time, a stray EOM and a stray CTS change nothing */
    sv = r.veh.stats;
    sc = r.chg.stats;
    run(&r, 2000U);
    make_cm(&f, 0x1CECF456u, 19U, 49U, 0U, 7U, PGN_BRM);
    CHECK_EQ_U(gbt_tp_on_frame(&r.veh, &f, r.now, &r.veh_ob, accept_all), GBT_TP_EV_NONE);
    make_cm(&f, 0x1CECF456u, 17U, 7U, 1U, 0xFFU, PGN_BRM);
    CHECK_EQ_U(gbt_tp_on_frame(&r.veh, &f, r.now, &r.veh_ob, accept_all), GBT_TP_EV_NONE);
    CHECK_EQ_U(gbt_outbox_count(&r.veh_ob), 0U);
    CHECK(memcmp(&sv, &r.veh.stats, sizeof sv) == 0);
    CHECK(memcmp(&sc, &r.chg.stats, sizeof sc) == 0);

    /* a second transfer accumulates (BCS: 2 DT) */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BCS, bcs, 9U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK_EQ_U(r.veh.stats.rts_tx, 2U);
    CHECK_EQ_U(r.veh.stats.cts_rx, 2U);
    CHECK_EQ_U(r.veh.stats.dt_tx, 9U);
    CHECK_EQ_U(r.veh.stats.eom_rx, 2U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 2U);
    CHECK_EQ_U(r.veh.stats.last_tx_complete_pgn, PGN_BCS);
    CHECK_EQ_U(r.veh.stats.last_tx_complete_len, 9U);
    CHECK_EQ_U(r.chg.stats.rts_rx, 2U);
    CHECK_EQ_U(r.chg.stats.cts_tx, 2U);
    CHECK_EQ_U(r.chg.stats.dt_rx_ok, 9U);
    CHECK_EQ_U(r.chg.stats.eom_tx, 2U);
    CHECK_EQ_U(r.chg.stats.rx_complete, 2U);
    CHECK_EQ_U(r.chg.stats.last_rx_complete_pgn, PGN_BCS);
    CHECK_EQ_U(r.chg.stats.last_rx_complete_len, 9U);
    CHECK_EQ_U(r.dt_count, 9U);                            /* DT frames seen on the rig bus */
    CHECK_EQ_U(r.abort_seen, 0U);
}

static void test_stats_window_cts(void)
{
    static rig_t r;
    uint8_t payload[49];
    memset(payload, 0x6B, sizeof payload);
    rig_init(&r);
    r.rts_max_patch = 3U;                                  /* receiver grants 3, then the rest */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 300U);
    CHECK((r.chg_ev & GBT_TP_EV_RX_COMPLETE) != 0U);
    CHECK((r.veh_ev & GBT_TP_EV_TX_COMPLETE) != 0U);
    CHECK_EQ_U(r.chg.stats.cts_tx, 2U);                    /* session-opening CTS + one window CTS */
    CHECK_EQ_U(r.veh.stats.cts_rx, 2U);
    CHECK_EQ_U(r.veh.stats.dt_tx, 7U);
    CHECK_EQ_U(r.chg.stats.dt_rx_ok, 7U);
    CHECK_EQ_U(r.chg.stats.eom_tx, 1U);
    CHECK_EQ_U(r.veh.stats.eom_rx, 1U);
    CHECK_EQ_U(r.chg.stats.rx_complete, 1U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 1U);
    CHECK(memcmp(r.chg.rx.buf, payload, 49U) == 0);
}

static void test_stats_sequence_gap(void)
{
    static rig_t r;
    uint8_t payload[49];
    memset(payload, 0x5A, sizeof payload);
    rig_init(&r);
    r.drop_dt_seq = 2;
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK_EQ_U(r.chg.stats.rts_rx, 1U);
    CHECK_EQ_U(r.chg.stats.dt_rx_ok, 1U);                  /* only DT 1 was in sequence */
    CHECK_EQ_U(r.chg.stats.eom_tx, 0U);
    CHECK_EQ_U(r.chg.stats.rx_complete, 0U);
    CHECK_EQ_U(r.chg.stats.abort_tx, 1U);
    CHECK_EQ_U(r.chg.stats.last_abort_reason_tx, 7U);      /* P-K4-10 sequence error */
    CHECK_EQ_U(r.chg.stats.timeout_rx, 0U);
    CHECK_EQ_U(r.chg.stats.last_end_ev, GBT_TP_EV_RX_ABORTED);
    CHECK_EQ_U(r.veh.stats.tx_started, 1U);
    CHECK_EQ_U(r.veh.stats.dt_tx, r.dt_count);             /* every DT the rig saw was queued once */
    CHECK_EQ_U(r.veh.stats.eom_rx, 0U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 0U);
    CHECK_EQ_U(r.veh.stats.abort_rx, 1U);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_rx, 7U);
    CHECK_EQ_U(r.veh.stats.last_end_ev, GBT_TP_EV_TX_ABORTED);
    CHECK_EQ_U(r.veh.stats.abort_rx, r.veh.aborts_received);   /* consistent with legacy counters */
    CHECK_EQ_U(r.chg.stats.abort_tx, r.chg.aborts_sent);
}

static void test_stats_receiver_timeout(void)
{
    static rig_t r;
    uint8_t payload[49];
    memset(payload, 0x33, sizeof payload);
    rig_init(&r);
    r.drop_dt_from = 2;                                    /* only DT 1 arrives */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 749U);
    CHECK_EQ_U(r.chg.stats.timeout_rx, 0U);
    run(&r, 1U);                                           /* T1 */
    CHECK_EQ_U(r.chg.stats.timeout_rx, 1U);
    CHECK_EQ_U(r.chg.stats.abort_tx, 1U);
    CHECK_EQ_U(r.chg.stats.last_abort_reason_tx, 3U);
    CHECK_EQ_U(r.chg.stats.dt_rx_ok, 1U);
    CHECK_EQ_U(r.chg.stats.rx_complete, 0U);
    CHECK_EQ_U(r.chg.stats.last_end_ev, GBT_TP_EV_RX_ABORTED);
    CHECK_EQ_U(r.veh.stats.dt_tx, 7U);
    CHECK_EQ_U(r.veh.stats.abort_rx, 1U);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_rx, 3U);
    CHECK_EQ_U(r.veh.stats.timeout_tx, 0U);                /* ended by the peer's abort, not by T3 */
    CHECK_EQ_U(r.veh.stats.tx_complete, 0U);
    CHECK_EQ_U(r.veh.stats.last_end_ev, GBT_TP_EV_TX_ABORTED);
}

static void test_stats_sender_timeout(void)
{
    static rig_t r;
    uint8_t payload[49];
    memset(payload, 0x11, sizeof payload);
    rig_init(&r);
    r.block_cts = 1U;                                      /* charger replies are lost */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 1249U);
    CHECK_EQ_U(r.veh.stats.timeout_tx, 0U);
    run(&r, 1U);                                           /* T3 */
    CHECK_EQ_U(r.veh.stats.rts_tx, 1U);
    CHECK_EQ_U(r.veh.stats.cts_rx, 0U);
    CHECK_EQ_U(r.veh.stats.dt_tx, 0U);
    CHECK_EQ_U(r.veh.stats.timeout_tx, 1U);
    CHECK_EQ_U(r.veh.stats.abort_tx, 1U);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_tx, 3U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 0U);
    CHECK_EQ_U(r.veh.stats.last_end_ev, GBT_TP_EV_TX_ABORTED);
    CHECK_EQ_U(r.chg.stats.rts_rx, 1U);
    CHECK_EQ_U(r.chg.stats.cts_tx, 1U);                    /* queued by the charger, lost by the rig */
    CHECK_EQ_U(r.chg.stats.timeout_rx, 1U);                /* T2 expired in the same millisecond */
    CHECK_EQ_U(r.chg.stats.abort_rx, 0U);                  /* vehicle abort arrived after the session ended */
}

static void test_stats_rejects_and_busy(void)
{
    static rig_t r;
    uint8_t payload[49];
    gbt_can_frame_t f;
    memset(payload, 0x11, sizeof payload);

    /* RTS for an unexpected PGN/size: refused, no receive session */
    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, 0x001500u, payload, 20U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    CHECK_EQ_U(r.chg.stats.rts_rx, 0U);
    CHECK_EQ_U(r.chg.stats.rx_started, 0U);
    CHECK_EQ_U(r.chg.stats.cts_tx, 0U);
    CHECK_EQ_U(r.chg.stats.abort_tx, 1U);
    CHECK_EQ_U(r.chg.stats.last_abort_reason_tx, 2U);
    CHECK_EQ_U(r.chg.stats.last_end_ev, 0U);               /* no session ended on the charger */
    CHECK_EQ_U(r.veh.stats.tx_started, 1U);
    CHECK_EQ_U(r.veh.stats.abort_rx, 1U);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_rx, 2U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 0U);
    CHECK_EQ_U(r.veh.stats.last_end_ev, GBT_TP_EV_TX_ABORTED);

    /* second RTS while a reception is active: "busy" abort, session kept, not counted as started */
    rig_init(&r);
    r.drop_dt_from = 1;
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    CHECK_EQ_U(r.chg.stats.rts_rx, 1U);
    make_cm(&f, 0x1CEC56F4u, 16U, 13U, 0U, 2U, PGN_BCP);
    (void)gbt_tp_on_frame(&r.chg, &f, r.now, &r.chg_ob, accept_all);
    CHECK_EQ_U(r.chg.stats.rts_rx, 1U);
    CHECK_EQ_U(r.chg.stats.rx_started, 1U);
    CHECK_EQ_U(r.chg.stats.abort_tx, 1U);
    CHECK_EQ_U(r.chg.stats.last_abort_reason_tx, 1U);
    CHECK_EQ_U(r.chg.rx.pgn, PGN_BRM);
}

static void test_stats_cts_hold_and_mismatch(void)
{
    static rig_t r;
    gbt_can_frame_t f;
    uint8_t payload[49];
    memset(payload, 0x44, sizeof payload);

    /* CTS(0) hold is an accepted CTS; T4 expiry is a sender timeout */
    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    (void)gbt_outbox_pop(&r.veh_ob, &f);
    make_cm(&f, 0x1CECF456u, 17U, 0U, 1U, 0xFFU, PGN_BRM);
    CHECK_EQ_U(gbt_tp_on_frame(&r.veh, &f, r.now, &r.veh_ob, accept_all), GBT_TP_EV_NONE);
    CHECK_EQ_U(r.veh.stats.cts_rx, 1U);
    CHECK_EQ_U(r.veh.stats.dt_tx, 0U);
    CHECK_EQ_U(gbt_tp_tick(&r.veh, r.now + 1050U, &r.veh_ob), GBT_TP_EV_TX_ABORTED);
    CHECK_EQ_U(r.veh.stats.timeout_tx, 1U);
    CHECK_EQ_U(r.veh.stats.abort_tx, 1U);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_tx, 3U);
    CHECK_EQ_U(r.veh.stats.last_end_ev, GBT_TP_EV_TX_ABORTED);

    /* CTS with an unexpected next sequence: rejected (not counted as CTS), abort queued */
    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    (void)gbt_outbox_pop(&r.veh_ob, &f);
    make_cm(&f, 0x1CECF456u, 17U, 7U, 2U, 0xFFU, PGN_BRM);
    CHECK_EQ_U(gbt_tp_on_frame(&r.veh, &f, r.now, &r.veh_ob, accept_all), GBT_TP_EV_TX_ABORTED);
    CHECK_EQ_U(r.veh.stats.cts_rx, 0U);
    CHECK_EQ_U(r.veh.stats.dt_tx, 0U);
    CHECK_EQ_U(r.veh.stats.abort_tx, 1U);
    CHECK_EQ_U(r.veh.stats.last_abort_reason_tx, 2U);
    CHECK_EQ_U(r.veh.stats.timeout_tx, 0U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 0U);
    CHECK_EQ_U(r.veh.stats.last_end_ev, GBT_TP_EV_TX_ABORTED);

    /* CTS for another PGN is ignored */
    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    (void)gbt_outbox_pop(&r.veh_ob, &f);
    make_cm(&f, 0x1CECF456u, 17U, 7U, 1U, 0xFFU, PGN_BCP);
    CHECK_EQ_U(gbt_tp_on_frame(&r.veh, &f, r.now, &r.veh_ob, accept_all), GBT_TP_EV_NONE);
    CHECK_EQ_U(r.veh.stats.cts_rx, 0U);
}

static void test_stats_outbox_full_not_counted(void)
{
    static rig_t r;
    uint8_t payload[49];
    gbt_can_frame_t f;
    unsigned n = 0U;
    memset(payload, 0x77, sizeof payload);
    memset(&f, 0, sizeof f);
    f.id = 0x18FF0000u;
    f.dlc = 8U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;

    /* sender: RTS cannot be queued -> session opened, RTS not counted */
    rig_init(&r);
    while (gbt_outbox_push(&r.veh_ob, &f) == GBT_RING_OK && n < 100U) {
        n++;
    }
    CHECK(n > 0U && n < 100U);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    CHECK_EQ_U(r.veh.stats.rts_tx, 0U);
    CHECK_EQ_U(r.veh.stats.tx_started, 1U);
    CHECK_EQ_U(r.veh.outbox_full, 1U);

    /* receiver: CTS cannot be queued -> session opened, CTS not counted */
    rig_init(&r);
    n = 0U;
    while (gbt_outbox_push(&r.chg_ob, &f) == GBT_RING_OK && n < 100U) {
        n++;
    }
    make_cm(&f, 0x1CEC56F4u, 16U, 49U, 0U, 7U, PGN_BRM);
    (void)gbt_tp_on_frame(&r.chg, &f, r.now, &r.chg_ob, accept_all);
    CHECK_EQ_U(r.chg.stats.rts_rx, 1U);
    CHECK_EQ_U(r.chg.stats.rx_started, 1U);
    CHECK_EQ_U(r.chg.stats.cts_tx, 0U);
    CHECK_EQ_U(r.chg.outbox_full, 1U);
}

static void test_stats_saturation(void)
{
    static rig_t r;
    uint8_t payload[9];
    memset(payload, 0x99, sizeof payload);
    rig_init(&r);
    r.veh.stats.rts_tx = 0xFFFFFFFFu;
    r.veh.stats.dt_tx = 0xFFFFFFFEu;
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BCS, payload, 9U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK_EQ_U(r.veh.stats.rts_tx, 0xFFFFFFFFu);
    CHECK_EQ_U(r.veh.stats.dt_tx, 0xFFFFFFFFu);           /* 2 DT: FE -> FF -> FF */
    CHECK_EQ_U(r.veh.stats.tx_complete, 1U);
}

/* ======================================================================================
 * STEP 07-C2.4: session timing / packet accounting and BRM payload snapshots.
 * Diagnostics only (never read by the TP logic). Rig timing: RTS, CTS and DT 1 in the
 * same millisecond, then one DT every 10 ms (GB/T §7.5).
 * ====================================================================================== */

static void fill(uint8_t *p, uint32_t n, uint8_t seed)
{
    uint32_t i;
    for (i = 0U; i < n; i++) {
        p[i] = (uint8_t)(seed + i * 7U);
    }
}

static void test_c24_snap_pgn_is_brm(void)
{
    const gbt_msg_def_t *d = gbt_msg_def(GBT_MSG_BRM);
    CHECK(d != NULL);
    if (d != NULL) {
        CHECK_EQ_U(GBT_TP_SNAP_PGN, d->pgn);
        CHECK_EQ_U(d->length, 49U);
    }
    CHECK_EQ_U(GBT_TP_SNAP_PGN, PGN_BRM);
}

static void test_c24_timing_brm(void)
{
    static rig_t r;
    uint8_t payload[49];
    uint32_t t0;
    fill(payload, 49U, 0x11U);
    rig_init(&r);
    t0 = r.now;
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, payload, 49U, r.now, &r.veh_ob) == 1U);
    CHECK_EQ_U(r.veh.stats.tx_sess_start_ms, t0);
    CHECK_EQ_U(r.veh.stats.tx_sess_dt, 0U);
    shuttle(&r);
    CHECK_EQ_U(r.chg.stats.rx_sess_start_ms, t0);
    CHECK_EQ_U(r.chg.stats.rx_sess_dt, 1U);                 /* DT 1 sent with the CTS answer */
    run(&r, 200U);
    CHECK_EQ_U(r.veh.stats.tx_sess_dt, 7U);
    CHECK_EQ_U(r.chg.stats.rx_sess_dt, 7U);
    CHECK_EQ_U(r.veh.stats.last_tx_duration_ms, 60U);       /* DT 7 at t0 + 60, EOM same ms */
    CHECK_EQ_U(r.chg.stats.last_rx_duration_ms, 60U);
    CHECK_EQ_U(r.veh.stats.max_tx_duration_ms, 60U);
    CHECK_EQ_U(r.chg.stats.max_rx_duration_ms, 60U);
    CHECK_EQ_U(r.veh.stats.max_tx_dt_interval_ms, 10U);
    CHECK_EQ_U(r.chg.stats.max_rx_dt_interval_ms, 10U);
    CHECK_EQ_U(r.veh.stats.sessions_ended, 1U);
    CHECK_EQ_U(r.chg.stats.sessions_ended, 1U);
    CHECK_EQ_U(r.chg.stats.rx_last_dt_ms, t0 + 60U);
    /* second (shorter) session: last_* follow, max_* keep the BRM values, per-session DT reset */
    {
        uint8_t bcs[9];
        fill(bcs, 9U, 0x40U);
        CHECK(gbt_tp_tx_start(&r.veh, PGN_BCS, bcs, 9U, r.now, &r.veh_ob) == 1U);
        shuttle(&r);
        run(&r, 100U);
        CHECK_EQ_U(r.veh.stats.tx_sess_dt, 2U);
        CHECK_EQ_U(r.chg.stats.rx_sess_dt, 2U);
        CHECK_EQ_U(r.veh.stats.last_tx_duration_ms, 10U);
        CHECK_EQ_U(r.chg.stats.last_rx_duration_ms, 10U);
        CHECK_EQ_U(r.veh.stats.max_tx_duration_ms, 60U);
        CHECK_EQ_U(r.chg.stats.max_rx_duration_ms, 60U);
        CHECK_EQ_U(r.veh.stats.sessions_ended, 2U);
        CHECK_EQ_U(r.chg.stats.sessions_ended, 2U);
    }
    /* a gap in the DT stream is measured (receiver), the gap DT is never sent twice */
}

static void test_c24_dt_interval_gap(void)
{
    static rig_t r;
    uint8_t payload[13];
    uint32_t i;
    fill(payload, 13U, 0x21U);
    rig_init(&r);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BCP, payload, 13U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);                                            /* RTS, CTS, DT 1 */
    for (i = 0U; i < 35U; i++) {                            /* sender main loop stalls 35 ms */
        r.now++;
    }
    run(&r, 50U);
    CHECK_EQ_U(r.veh.stats.max_tx_dt_interval_ms, 36U);     /* DT 2 at the first tick after the stall */
    CHECK_EQ_U(r.chg.stats.max_rx_dt_interval_ms, 36U);
    CHECK_EQ_U(r.chg.stats.rx_complete, 1U);
    CHECK_EQ_U(r.chg.stats.last_rx_duration_ms, 36U);
}

static void test_c24_snapshot_frozen(void)
{
    static rig_t r;
    uint8_t brm[49];
    uint8_t brm2[49];
    uint8_t bcp[13];
    uint8_t bcs[9];
    fill(brm, 49U, 0x01U);
    fill(brm2, 49U, 0x80U);
    fill(bcp, 13U, 0x55U);
    fill(bcs, 9U, 0x66U);
    rig_init(&r);
    CHECK_EQ_U(r.veh.snap_tx.state, GBT_TP_SNAP_NONE);
    CHECK_EQ_U(r.chg.snap_rx.state, GBT_TP_SNAP_NONE);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, brm, 49U, r.now, &r.veh_ob) == 1U);
    CHECK_EQ_U(r.veh.snap_tx.state, GBT_TP_SNAP_CAPTURED);  /* copied at start, not yet delivered */
    shuttle(&r);
    run(&r, 200U);
    CHECK_EQ_U(r.veh.snap_tx.state, GBT_TP_SNAP_FROZEN);
    CHECK_EQ_U(r.veh.snap_tx.len, 49U);
    CHECK_EQ_U(r.veh.snap_tx.pgn, PGN_BRM);
    CHECK_EQ_U(r.veh.snap_tx.captures, 1U);
    CHECK(memcmp(r.veh.snap_tx.buf, brm, 49U) == 0);
    CHECK_EQ_U(r.chg.snap_rx.state, GBT_TP_SNAP_FROZEN);
    CHECK_EQ_U(r.chg.snap_rx.len, 49U);
    CHECK_EQ_U(r.chg.snap_rx.pgn, PGN_BRM);
    CHECK_EQ_U(r.chg.snap_rx.captures, 1U);
    CHECK(memcmp(r.chg.snap_rx.buf, brm, 49U) == 0);
    CHECK(memcmp(r.chg.snap_rx.buf, r.veh.snap_tx.buf, 49U) == 0);   /* end-to-end equality */
    CHECK_EQ_U(r.veh.snap_rx.state, GBT_TP_SNAP_NONE);      /* other directions untouched */
    CHECK_EQ_U(r.chg.snap_tx.state, GBT_TP_SNAP_NONE);
    /* BCP, BCS and a second BRM do not touch the frozen copies */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BCP, bcp, 13U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BCS, bcs, 9U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, brm2, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 200U);
    CHECK_EQ_U(r.chg.stats.rx_complete, 4U);
    CHECK(memcmp(r.chg.rx.buf, brm2, 49U) == 0);            /* the live buffer did change ... */
    CHECK(memcmp(r.chg.snap_rx.buf, brm, 49U) == 0);        /* ... the snapshots did not */
    CHECK(memcmp(r.veh.snap_tx.buf, brm, 49U) == 0);
    CHECK_EQ_U(r.chg.snap_rx.captures, 1U);
    CHECK_EQ_U(r.veh.snap_tx.captures, 1U);
    CHECK_EQ_U(r.chg.snap_rx.state, GBT_TP_SNAP_FROZEN);
    CHECK_EQ_U(r.veh.snap_tx.state, GBT_TP_SNAP_FROZEN);
}

static void test_c24_snapshot_after_abort(void)
{
    static rig_t r;
    uint8_t brm[49];
    uint8_t brm2[49];
    fill(brm, 49U, 0x02U);
    fill(brm2, 49U, 0x90U);
    rig_init(&r);
    r.drop_dt_seq = 2;                                      /* first BRM: sequence-gap abort */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, brm, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 100U);
    CHECK_EQ_U(r.veh.stats.tx_complete, 0U);
    CHECK_EQ_U(r.veh.snap_tx.state, GBT_TP_SNAP_CAPTURED);  /* not delivered: not frozen */
    CHECK_EQ_U(r.chg.snap_rx.state, GBT_TP_SNAP_NONE);      /* nothing reassembled */
    CHECK_EQ_U(r.veh.stats.sessions_ended, 1U);
    CHECK_EQ_U(r.chg.stats.sessions_ended, 1U);
    r.drop_dt_seq = -1;                                     /* retry delivers */
    CHECK(gbt_tp_tx_start(&r.veh, PGN_BRM, brm2, 49U, r.now, &r.veh_ob) == 1U);
    shuttle(&r);
    run(&r, 200U);
    CHECK_EQ_U(r.veh.snap_tx.state, GBT_TP_SNAP_FROZEN);
    CHECK_EQ_U(r.veh.snap_tx.captures, 2U);
    CHECK(memcmp(r.veh.snap_tx.buf, brm2, 49U) == 0);
    CHECK_EQ_U(r.chg.snap_rx.state, GBT_TP_SNAP_FROZEN);
    CHECK_EQ_U(r.chg.snap_rx.captures, 1U);
    CHECK(memcmp(r.chg.snap_rx.buf, brm2, 49U) == 0);
}

/* Both sessions of one node end in the same tick: receive side first, sessions_ended += 2
 * (the integration event log relies on this order). */
static void test_c24_double_end_in_tick(void)
{
    static gbt_tp_t tp;
    static gbt_outbox_t ob;
    gbt_can_frame_t f;
    uint8_t bcs[9];
    uint8_t ev;
    fill(bcs, 9U, 0x10U);
    gbt_tp_init(&tp, (uint8_t)GBT_ADDR_SECC, (uint8_t)GBT_ADDR_EVCC, NULL);
    (void)gbt_outbox_init(&ob);
    make_cm(&f, 0x1CEC56F4u, 16U, 13U, 0U, 2U, PGN_BCP);     /* RTS from the peer */
    f.data[4] = 0xFFU;
    CHECK_EQ_U(gbt_tp_on_frame(&tp, &f, 5000U, &ob, accept_all), GBT_TP_EV_NONE);
    CHECK_EQ_U(tp.rx.state, GBT_TP_RX_DATA);
    CHECK(gbt_tp_tx_start(&tp, PGN_BCS, bcs, 9U, 5000U, &ob) == 1U);
    CHECK_EQ_U(gbt_tp_tick(&tp, 5000U + 1249U, &ob), GBT_TP_EV_NONE);
    ev = gbt_tp_tick(&tp, 5000U + 1250U, &ob);              /* T2 (1250) and T3 (1250) together */
    CHECK_EQ_U(ev, (unsigned)(GBT_TP_EV_RX_ABORTED | GBT_TP_EV_TX_ABORTED));
    CHECK_EQ_U(tp.stats.sessions_ended, 2U);
    CHECK_EQ_U(tp.stats.last_end_ev, GBT_TP_EV_TX_ABORTED);
    CHECK_EQ_U(tp.stats.timeout_rx, 1U);
    CHECK_EQ_U(tp.stats.timeout_tx, 1U);
}

int main(void)
{
    test_happy_brm();
    test_bcs_fill();
    test_sequence_gap();
    test_missing_dt_timeout();
    test_sender_timeout_and_rejects();
    test_cts_window_and_hold();
    /* STEP 07-C2 statistics */
    test_stats_init_and_reset();
    test_stats_happy_and_no_double_count();
    test_stats_window_cts();
    test_stats_sequence_gap();
    test_stats_receiver_timeout();
    test_stats_sender_timeout();
    test_stats_rejects_and_busy();
    test_stats_cts_hold_and_mismatch();
    test_stats_outbox_full_not_counted();
    test_stats_saturation();
    /* STEP 07-C2.4 timing / snapshots */
    test_c24_snap_pgn_is_brm();
    test_c24_timing_brm();
    test_c24_dt_interval_gap();
    test_c24_snapshot_frozen();
    test_c24_snapshot_after_abort();
    test_c24_double_end_in_tick();
    return TEST_SUMMARY("test_tp");
}
