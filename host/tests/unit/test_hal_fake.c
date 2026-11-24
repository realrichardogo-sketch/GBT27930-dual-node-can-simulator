/*
 * tests/unit/test_hal_fake.c - HAL API v1 contract via the fake HAL (ND-1 / B1-05).
 * Covers the ND-1 lifecycle, sink binding, acceptance bookkeeping and
 * re-entrancy, frame validation, TX status injection, RX queue and overflow
 * counters, shared clock, status and events.
 */
#include "hal/hal.h"
#include "tests/support/fake_hal.h"
#include "tests/support/test_assert.h"

#include <string.h>

typedef struct {
    gbt_hal_t       *h;           /* instance under test, for the re-entry probe */
    gbt_hal_status_t reply;       /* status the sink returns */
    uint32_t         calls;
    gbt_output_cmd_t last;
    gbt_hal_status_t reentry;     /* status seen when the sink re-enters the HAL */
    uint8_t          probe;
} sink_ctx_t;

static gbt_hal_status_t sink_apply(void *ctx, const gbt_output_cmd_t *cmd)
{
    sink_ctx_t *c = (sink_ctx_t *)ctx;
    gbt_can_frame_t f;
    memset(&f, 0, sizeof f);
    c->calls++;
    c->last = *cmd;
    if (c->probe != 0U) {
        f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
        c->reentry = gbt_hal_can_send(c->h, &f);   /* forbidden by contract */
    }
    return c->reply;
}

static gbt_hal_status_t sink_feedback(void *ctx, gbt_output_fb_t *fb)
{
    (void)ctx;
    fb->valid = 1U;
    fb->voltage_mv = 1234U;
    fb->current_ma = 5678U;
    fb->t_ms = 9U;
    return GBT_HAL_OK;
}

static gbt_can_frame_t mk(uint32_t id, uint8_t dlc, uint8_t flags)
{
    gbt_can_frame_t f;
    memset(&f, 0, sizeof f);
    f.id = id;
    f.dlc = dlc;
    f.flags = flags;
    f.data[0] = 0xA5U;
    return f;
}

static void test_version(void)
{
    CHECK_EQ_U(GBT_HAL_API_VERSION, 1U);
    CHECK((int)GBT_HAL_E_FULL != (int)GBT_HAL_OK);
    CHECK((int)GBT_HAL_E_STATE != (int)GBT_HAL_E_NOT_READY);
}

static void test_lifecycle(void)
{
    static gbt_hal_t h;
    static gbt_sim_bus_t bus;
    uint32_t now = 0U;
    sink_ctx_t ctx;
    gbt_output_sink_t sink;
    gbt_output_sink_t bad;

    memset(&ctx, 0, sizeof ctx);
    sink.apply = sink_apply;
    sink.feedback = sink_feedback;
    sink.ctx = &ctx;
    bad = sink;
    bad.apply = NULL;

    /* UNINIT */
    CHECK(gbt_hal_bind_output_sink(&h, &sink) == GBT_HAL_E_NOT_READY);
    CHECK(gbt_hal_claim(&h) == GBT_HAL_E_NOT_READY);
    CHECK(gbt_hal_release(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_unbind_output_sink(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_fake_hal_deinit(&h) == GBT_HAL_E_STATE);
    CHECK_EQ_U(gbt_hal_now_ms(&h), 0U);

    CHECK(gbt_fake_hal_init(&h, &bus, 2U, &now) == GBT_HAL_E_PARAM);
    CHECK(gbt_fake_hal_init(&h, &bus, 0U, &now) == GBT_HAL_OK);          /* -> READY */
    CHECK_EQ_U(h.lifecycle, GBT_HAL_LC_READY);
    CHECK(bus.node[0] == &h);
    CHECK(gbt_fake_hal_init(&h, &bus, 0U, &now) == GBT_HAL_E_STATE);

    /* READY */
    CHECK(gbt_hal_claim(&h) == GBT_HAL_E_NOT_READY);                     /* no sink */
    CHECK(gbt_hal_unbind_output_sink(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_bind_output_sink(&h, NULL) == GBT_HAL_E_PARAM);
    CHECK(gbt_hal_bind_output_sink(&h, &bad) == GBT_HAL_E_PARAM);
    CHECK(gbt_hal_bind_output_sink(&h, &sink) == GBT_HAL_OK);            /* -> BOUND */
    CHECK_EQ_U(h.lifecycle, GBT_HAL_LC_BOUND);

    /* BOUND */
    CHECK(gbt_hal_bind_output_sink(&h, &sink) == GBT_HAL_E_STATE);
    CHECK(gbt_fake_hal_deinit(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_release(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_claim(&h) == GBT_HAL_OK);                              /* -> CLAIMED */
    CHECK_EQ_U(h.lifecycle, GBT_HAL_LC_CLAIMED);

    /* CLAIMED */
    CHECK(gbt_hal_claim(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_bind_output_sink(&h, &sink) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_unbind_output_sink(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_fake_hal_deinit(&h) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_release(&h) == GBT_HAL_OK);                            /* -> BOUND */
    CHECK(gbt_hal_unbind_output_sink(&h) == GBT_HAL_OK);                 /* -> READY */
    CHECK(gbt_fake_hal_deinit(&h) == GBT_HAL_OK);                        /* -> UNINIT */
    CHECK_EQ_U(h.lifecycle, GBT_HAL_LC_UNINIT);
    CHECK(bus.node[0] == NULL);
}

static void test_output_apply(void)
{
    static gbt_hal_t h;
    static gbt_sim_bus_t bus;
    uint32_t now = 77U;
    sink_ctx_t ctx;
    gbt_output_sink_t sink;
    gbt_output_cmd_t cmd = { 0U, 0U, 0U, 0U };
    gbt_output_fb_t fb = { 0U, 0U, 0U, 0U };

    memset(&ctx, 0, sizeof ctx);
    sink.apply = sink_apply;
    sink.feedback = sink_feedback;
    sink.ctx = &ctx;
    ctx.h = &h;
    ctx.reply = GBT_HAL_OK;

    CHECK(gbt_fake_hal_init(&h, &bus, 0U, &now) == GBT_HAL_OK);
    CHECK(gbt_hal_output_apply(&h, &cmd) == GBT_HAL_E_STATE);            /* not CLAIMED */
    CHECK(gbt_hal_bind_output_sink(&h, &sink) == GBT_HAL_OK);
    CHECK(gbt_hal_output_apply(&h, &cmd) == GBT_HAL_E_STATE);            /* BOUND only */
    CHECK(gbt_hal_claim(&h) == GBT_HAL_OK);
    CHECK(gbt_hal_output_apply(&h, NULL) == GBT_HAL_E_PARAM);

    cmd.seq = 5U;
    cmd.enabled = 1U;
    cmd.voltage_mv = 400000U;
    cmd.current_ma = 10000U;
    CHECK(gbt_hal_output_apply(&h, &cmd) == GBT_HAL_OK);
    CHECK_EQ_U(ctx.calls, 1U);
    CHECK_EQ_U(ctx.last.current_ma, 10000U);
    CHECK_EQ_U(h.accepted_seq, 5U);
    CHECK_EQ_U(h.accept_time_ms, 77U);

    /* sink refuses: status propagated, acceptance bookkeeping unchanged */
    ctx.reply = GBT_HAL_E_IO;
    cmd.seq = 6U;
    now = 80U;
    CHECK(gbt_hal_output_apply(&h, &cmd) == GBT_HAL_E_IO);
    CHECK_EQ_U(h.accepted_seq, 5U);
    CHECK_EQ_U(h.accept_time_ms, 77U);

    /* re-entry from inside the sink is rejected */
    ctx.reply = GBT_HAL_OK;
    ctx.probe = 1U;
    cmd.seq = 7U;
    CHECK(gbt_hal_output_apply(&h, &cmd) == GBT_HAL_OK);
    CHECK(ctx.reentry == GBT_HAL_E_STATE);
    CHECK_EQ_U(h.in_sink, 0U);
    CHECK_EQ_U(h.accepted_seq, 7U);

    CHECK(gbt_hal_output_feedback(&h, &fb) == GBT_HAL_OK);
    CHECK_EQ_U(fb.voltage_mv, 1234U);
    CHECK_EQ_U(fb.current_ma, 5678U);
}

static void test_can_paths(void)
{
    static gbt_hal_t a;
    static gbt_hal_t b;
    static gbt_sim_bus_t bus;
    uint32_t now = 1000U;
    gbt_can_frame_t f;
    gbt_can_frame_t r;
    gbt_can_status_t s;
    uint32_t i;

    f = mk(0x1826F456u, 3U, (uint8_t)GBT_CAN_FLAG_EXT);
    CHECK(gbt_hal_can_send(&a, &f) == GBT_HAL_E_NOT_READY);              /* UNINIT */
    CHECK(gbt_fake_hal_init(&a, &bus, 0U, &now) == GBT_HAL_OK);
    CHECK(gbt_fake_hal_init(&b, &bus, 1U, &now) == GBT_HAL_OK);
    CHECK_EQ_U(gbt_hal_now_ms(&a), 1000U);
    now = 0xFFFFFFFFu;
    CHECK_EQ_U(gbt_hal_now_ms(&b), 0xFFFFFFFFu);                         /* shared clock */

    /* frame validation */
    r = mk(0x1u, 9U, (uint8_t)GBT_CAN_FLAG_EXT);
    CHECK(gbt_hal_can_send(&a, &r) == GBT_HAL_E_PARAM);                  /* dlc > 8 */
    r = mk(0x1u, 1U, 0x02U);
    CHECK(gbt_hal_can_send(&a, &r) == GBT_HAL_E_PARAM);                  /* unknown flag */
    r = mk(0x20000000u, 1U, (uint8_t)GBT_CAN_FLAG_EXT);
    CHECK(gbt_hal_can_send(&a, &r) == GBT_HAL_E_PARAM);                  /* > 29 bits */
    r = mk(0x800u, 1U, 0U);
    CHECK(gbt_hal_can_send(&a, &r) == GBT_HAL_E_PARAM);                  /* > 11 bits */
    CHECK(gbt_hal_can_send(&a, NULL) == GBT_HAL_E_PARAM);

    /* delivery a -> b only */
    CHECK(gbt_hal_can_send(&a, &f) == GBT_HAL_OK);
    CHECK_EQ_U(bus.frames_delivered, 1U);
    CHECK(gbt_hal_can_receive(&a, &r) == GBT_HAL_E_EMPTY);
    CHECK(gbt_hal_can_receive(&b, &r) == GBT_HAL_OK);
    CHECK_EQ_U(r.id, 0x1826F456u);
    CHECK_EQ_U(r.dlc, 3U);
    CHECK_EQ_U(r.data[0], 0xA5U);
    CHECK(gbt_hal_can_receive(&b, &r) == GBT_HAL_E_EMPTY);

    /* injected TX failures: frame not accepted, not delivered, tx_error counted */
    CHECK(gbt_fake_hal_set_tx_status(&a, GBT_HAL_E_FULL) == GBT_HAL_OK);
    CHECK(gbt_hal_can_send(&a, &f) == GBT_HAL_E_FULL);
    CHECK(gbt_fake_hal_set_tx_status(&a, GBT_HAL_E_BUS_OFF) == GBT_HAL_OK);
    CHECK(gbt_hal_can_send(&a, &f) == GBT_HAL_E_BUS_OFF);
    CHECK(gbt_fake_hal_set_tx_status(&a, GBT_HAL_E_IO) == GBT_HAL_OK);
    CHECK(gbt_hal_can_send(&a, &f) == GBT_HAL_E_IO);
    CHECK_EQ_U(bus.frames_delivered, 1U);
    CHECK(gbt_hal_can_get_status(&a, &s) == GBT_HAL_OK);
    CHECK_EQ_U(s.tx_error, 3U);
    CHECK(gbt_fake_hal_set_tx_status(&a, GBT_HAL_OK) == GBT_HAL_OK);

    /* RX queue overflow at b: 16 accepted, the rest counted as software drops */
    for (i = 0U; i < 20U; i++) {
        f.data[1] = (uint8_t)i;
        CHECK(gbt_hal_can_send(&a, &f) == GBT_HAL_OK);
    }
    CHECK(gbt_hal_can_get_status(&b, &s) == GBT_HAL_OK);
    CHECK_EQ_U(s.rx_overflow_sw, 4U);
    for (i = 0U; i < 16U; i++) {
        CHECK(gbt_hal_can_receive(&b, &r) == GBT_HAL_OK);
        CHECK_EQ_U(r.data[1], i);                                        /* FIFO order */
    }
    CHECK(gbt_hal_can_receive(&b, &r) == GBT_HAL_E_EMPTY);

    /* hardware-overflow injection and counter saturation */
    CHECK(gbt_fake_hal_inject_overflow(&b, 1U) == GBT_HAL_OK);
    b.status.rx_overflow_hw = 0xFFFFFFFEu;
    CHECK(gbt_fake_hal_inject_overflow(&b, 1U) == GBT_HAL_OK);
    CHECK(gbt_fake_hal_inject_overflow(&b, 1U) == GBT_HAL_OK);
    CHECK(gbt_hal_can_get_status(&b, &s) == GBT_HAL_OK);
    CHECK_EQ_U(s.rx_overflow_hw, 0xFFFFFFFFu);
    CHECK_EQ_U(s.state, GBT_CAN_STATE_ERROR_ACTIVE);

    /* injected RX */
    r = mk(0x181056F4u, 5U, (uint8_t)GBT_CAN_FLAG_EXT);
    CHECK(gbt_fake_hal_inject_rx(&a, &r) == GBT_HAL_OK);
    CHECK(gbt_hal_can_receive(&a, &f) == GBT_HAL_OK);
    CHECK_EQ_U(f.id, 0x181056F4u);

    gbt_hal_event(&a, 42U, 1, 2);
    CHECK_EQ_U(a.event_count, 1U);
    CHECK_EQ_U(a.last_event_code, 42U);

    /* sending with no live peer is accepted but not delivered */
    CHECK(gbt_fake_hal_deinit(&b) == GBT_HAL_OK);
    CHECK(gbt_hal_can_send(&a, &f) == GBT_HAL_OK);
    CHECK_EQ_U(bus.frames_delivered, 21U);
}

int main(void)
{
    test_version();
    test_lifecycle();
    test_output_apply();
    test_can_paths();
    return TEST_SUMMARY("test_hal_fake");
}
