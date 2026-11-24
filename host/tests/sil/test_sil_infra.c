/*
 * tests/sil/test_sil_infra.c - dual-context SIL infrastructure (M12, B1-15).
 * Checks: shared 1 ms virtual clock (incl. wrap), deterministic and swappable
 * per-tick order, bus isolation between the two contexts, independent node
 * storage. No protocol scenarios.
 */
#include "tests/sil/gbt_sil.h"
#include "tests/support/test_assert.h"

#include <string.h>

#define TRACE_MAX 64U

typedef struct {
    gbt_sil_t *sil;
    uint8_t    node[TRACE_MAX];
    uint32_t   time[TRACE_MAX];
    uint32_t   n;
    uint8_t    send_from_node0;
} trace_t;

static void step_fn(void *ctx, uint8_t node_index, uint32_t now_ms)
{
    trace_t *t = (trace_t *)ctx;
    if (t->n < TRACE_MAX) {
        t->node[t->n] = node_index;
        t->time[t->n] = now_ms;
        t->n++;
    }
    if (t->send_from_node0 != 0U && node_index == 0U) {
        gbt_can_frame_t f;
        memset(&f, 0, sizeof f);
        f.id = 0x1826F456u;
        f.dlc = 1U;
        f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
        f.data[0] = (uint8_t)(now_ms & 0xFFU);
        (void)gbt_hal_can_send(&t->sil->hal[0], &f);
    }
}

static void test_order_and_clock(void)
{
    static gbt_sil_t sil;
    static trace_t tr;
    uint32_t i;

    memset(&sil, 0, sizeof sil);
    memset(&tr, 0, sizeof tr);
    tr.sil = &sil;
    CHECK(gbt_sil_init(&sil, 0xFFFFFFFEu, step_fn, &tr) == GBT_HAL_OK);
    gbt_sil_run(&sil, 3U);                      /* 0xFFFFFFFF, 0, 1 */
    CHECK_EQ_U(tr.n, 6U);
    CHECK_EQ_U(tr.time[0], 0xFFFFFFFFu);
    CHECK_EQ_U(tr.time[2], 0x00000000u);        /* wraps */
    CHECK_EQ_U(tr.time[4], 0x00000001u);
    for (i = 0U; i < 6U; i += 2U) {
        CHECK_EQ_U(tr.node[i], 0U);             /* default order 0,1 */
        CHECK_EQ_U(tr.node[i + 1U], 1U);
        CHECK_EQ_U(tr.time[i], tr.time[i + 1U]); /* both nodes see the same tick */
    }
    CHECK_EQ_U(gbt_hal_now_ms(&sil.hal[1]), 1U);

    CHECK(gbt_sil_set_order(&sil, 1U) == GBT_HAL_OK);
    CHECK(gbt_sil_set_order(&sil, 2U) == GBT_HAL_E_PARAM);
    gbt_sil_tick(&sil);
    CHECK_EQ_U(tr.node[6], 1U);                 /* swapped */
    CHECK_EQ_U(tr.node[7], 0U);
    CHECK_EQ_U(sil.ticks, 4U);
    CHECK(gbt_sil_deinit(&sil) == GBT_HAL_OK);
}

static void test_isolation(void)
{
    static gbt_sil_t sil;
    static trace_t tr;
    static gbt_node_t zero;
    gbt_can_frame_t f;
    uint32_t got = 0U;

    memset(&sil, 0, sizeof sil);
    memset(&tr, 0, sizeof tr);
    tr.sil = &sil;
    tr.send_from_node0 = 1U;
    CHECK(gbt_sil_init(&sil, 100U, step_fn, &tr) == GBT_HAL_OK);
    gbt_sil_run(&sil, 5U);
    CHECK(gbt_hal_can_receive(&sil.hal[0], &f) == GBT_HAL_E_EMPTY);   /* no self-reception */
    while (gbt_hal_can_receive(&sil.hal[1], &f) == GBT_HAL_OK) {
        CHECK_EQ_U(f.data[0], 101U + got);                            /* in order */
        got++;
    }
    CHECK_EQ_U(got, 5U);
    CHECK_EQ_U(sil.bus.frames_delivered, 5U);

    /* independent node storage: writing node 0 leaves node 1 untouched */
    CHECK(gbt_outbox_init(&sil.node[0].outbox) == GBT_RING_OK);
    f.id = 1U;
    CHECK(gbt_outbox_push(&sil.node[0].outbox, &f) == GBT_RING_OK);
    CHECK(memcmp(&sil.node[1], &zero, sizeof zero) == 0);
    CHECK((void *)&sil.node[0] != (void *)&sil.node[1]);
    CHECK(gbt_sil_deinit(&sil) == GBT_HAL_OK);
}

static void test_determinism(void)
{
    static gbt_sil_t a;
    static gbt_sil_t b;
    static trace_t ta;
    static trace_t tb;

    memset(&a, 0, sizeof a);
    memset(&b, 0, sizeof b);
    memset(&ta, 0, sizeof ta);
    memset(&tb, 0, sizeof tb);
    ta.sil = &a;
    tb.sil = &b;
    ta.send_from_node0 = 1U;
    tb.send_from_node0 = 1U;
    CHECK(gbt_sil_init(&a, 42U, step_fn, &ta) == GBT_HAL_OK);
    CHECK(gbt_sil_init(&b, 42U, step_fn, &tb) == GBT_HAL_OK);
    gbt_sil_run(&a, 10U);
    gbt_sil_run(&b, 10U);
    CHECK(memcmp(ta.node, tb.node, sizeof ta.node) == 0);
    CHECK(memcmp(ta.time, tb.time, sizeof ta.time) == 0);
    CHECK_EQ_U(a.bus.frames_delivered, b.bus.frames_delivered);
}

int main(void)
{
    test_order_and_clock();
    test_isolation();
    test_determinism();
    return TEST_SUMMARY("test_sil_infra");
}
