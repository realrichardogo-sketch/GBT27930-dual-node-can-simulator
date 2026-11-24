/*
 * tests/unit/test_spsc.c - deterministic tests of ports/common/gbt_spsc.
 * Producer/consumer interleaving is exercised at API granularity here; the
 * asynchronous (signal-preemption) host stress test is test_spsc_signal.c.
 */
#include "ports/common/gbt_spsc.h"
#include "tests/support/test_assert.h"

#include <string.h>

static gbt_spsc_t Q;

static gbt_can_frame_t mk(uint32_t seq)
{
    gbt_can_frame_t f;
    unsigned i;
    memset(&f, 0, sizeof f);
    f.id = 0x1C000000u | (seq & 0x00FFFFFFu);
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f.dlc = (uint8_t)(seq % 9U);                     /* 0..8 */
    for (i = 0U; i < 8U; i++) {
        f.data[i] = (uint8_t)((seq * 31U + i * 7U + 1U) & 0xFFU);
    }
    return f;
}

static int same(const gbt_can_frame_t *a, const gbt_can_frame_t *b)
{
    return a->id == b->id && a->dlc == b->dlc && a->flags == b->flags
           && memcmp(a->data, b->data, 8U) == 0;
}

static void test_params(void)
{
    gbt_can_frame_t f = mk(1U);
    CHECK(gbt_spsc_init(NULL) == GBT_SPSC_E_PARAM);
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    CHECK(gbt_spsc_push(NULL, &f) == GBT_SPSC_E_PARAM);
    CHECK(gbt_spsc_push(&Q, NULL) == GBT_SPSC_E_PARAM);
    CHECK(gbt_spsc_pop(NULL, &f) == GBT_SPSC_E_PARAM);
    CHECK(gbt_spsc_pop(&Q, NULL) == GBT_SPSC_E_PARAM);
    CHECK_EQ_U(gbt_spsc_count(NULL), 0U);
    CHECK_EQ_U(gbt_spsc_dropped(NULL), 0U);
    CHECK_EQ_U(gbt_spsc_count(&Q), 0U);              /* parameter errors changed nothing */
    CHECK_EQ_U(gbt_spsc_dropped(&Q), 0U);
}

static void test_empty_and_single(void)
{
    gbt_can_frame_t f = mk(5U);
    gbt_can_frame_t out;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    memset(&out, 0xAA, sizeof out);
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_E_EMPTY);
    CHECK_EQ_U(out.id, 0xAAAAAAAAu);                 /* *out untouched on EMPTY */
    CHECK_EQ_U(gbt_spsc_count(&Q), 0U);
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);
    CHECK_EQ_U(gbt_spsc_count(&Q), 1U);
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
    CHECK(same(&f, &out));
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_E_EMPTY);
    CHECK_EQ_U(gbt_spsc_count(&Q), 0U);
}

static void test_data_bytes(void)
{
    gbt_can_frame_t f;
    gbt_can_frame_t out;
    unsigned i;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    memset(&f, 0, sizeof f);
    f.id = 0x1FFFFFFFu;                              /* max 29-bit identifier */
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f.dlc = 3U;                                      /* DLC < 8: bytes 3..7 still carried */
    f.data[0] = 0x00U; f.data[1] = 0xFFU; f.data[2] = 0x80U; f.data[3] = 0x7FU;
    f.data[4] = 0x01U; f.data[5] = 0xFEU; f.data[6] = 0x55U; f.data[7] = 0xAAU;
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);
    memset(&out, 0, sizeof out);
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
    CHECK_EQ_U(out.id, 0x1FFFFFFFu);
    CHECK_EQ_U(out.dlc, 3U);
    CHECK_EQ_U(out.flags, GBT_CAN_FLAG_EXT);
    for (i = 0U; i < 8U; i++) {
        CHECK_EQ_U(out.data[i], f.data[i]);
    }
}

static void test_fifo_full_overflow(void)
{
    gbt_can_frame_t f;
    gbt_can_frame_t out;
    uint32_t i;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    for (i = 0U; i < GBT_SPSC_CAPACITY; i++) {       /* all 16 slots usable */
        f = mk(i);
        CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);
    }
    CHECK_EQ_U(gbt_spsc_count(&Q), 16U);
    CHECK_EQ_U(gbt_spsc_dropped(&Q), 0U);
    for (i = 100U; i < 103U; i++) {                  /* full: 3 rejected */
        f = mk(i);
        CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_E_FULL);
    }
    CHECK_EQ_U(gbt_spsc_count(&Q), 16U);
    CHECK_EQ_U(gbt_spsc_dropped(&Q), 3U);
    for (i = 0U; i < GBT_SPSC_CAPACITY; i++) {       /* FIFO order; rejected frames absent */
        gbt_can_frame_t e = mk(i);
        CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
        CHECK(same(&e, &out));
    }
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_E_EMPTY);
    f = mk(200U);                                    /* space again after draining */
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
    CHECK(same(&f, &out));
    CHECK_EQ_U(gbt_spsc_dropped(&Q), 3U);            /* counter is cumulative */
}

static void test_one_slot_freed(void)
{
    gbt_can_frame_t f;
    gbt_can_frame_t out;
    uint32_t i;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    for (i = 0U; i < GBT_SPSC_CAPACITY; i++) {
        f = mk(i);
        (void)gbt_spsc_push(&Q, &f);
    }
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);    /* frees exactly one slot */
    f = mk(16U);
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);
    f = mk(17U);
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_E_FULL);
    for (i = 1U; i <= 16U; i++) {
        gbt_can_frame_t e = mk(i);
        CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
        CHECK(same(&e, &out));
    }
    CHECK_EQ_U(gbt_spsc_dropped(&Q), 1U);
}

/* Repeated mixed operation against an exact reference FIFO (bursts of 0..17). */
static void test_repeated(void)
{
    gbt_can_frame_t f;
    gbt_can_frame_t out;
    uint32_t model[GBT_SPSC_CAPACITY];
    uint32_t m_head = 0U;
    uint32_t m_tail = 0U;
    uint32_t model_dropped = 0U;
    uint32_t seq = 0U;
    uint32_t pops_ok = 0U;
    uint32_t round;
    unsigned errors = 0U;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    for (round = 0U; round < 5000U; round++) {
        uint32_t pushes = (round * 7U) % 18U;
        uint32_t pops = (round * 5U) % 18U;
        uint32_t k;
        for (k = 0U; k < pushes; k++, seq++) {
            f = mk(seq);
            if (m_head - m_tail < GBT_SPSC_CAPACITY) {
                errors += (gbt_spsc_push(&Q, &f) == GBT_SPSC_OK) ? 0U : 1U;
                model[m_head % GBT_SPSC_CAPACITY] = seq;
                m_head++;
            } else {
                errors += (gbt_spsc_push(&Q, &f) == GBT_SPSC_E_FULL) ? 0U : 1U;
                model_dropped++;
            }
        }
        for (k = 0U; k < pops; k++) {
            if (m_head == m_tail) {
                errors += (gbt_spsc_pop(&Q, &out) == GBT_SPSC_E_EMPTY) ? 0U : 1U;
            } else {
                gbt_can_frame_t e = mk(model[m_tail % GBT_SPSC_CAPACITY]);
                m_tail++;
                errors += (gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK) ? 0U : 1U;
                errors += same(&e, &out) ? 0U : 1U;
                pops_ok++;
            }
        }
        errors += (gbt_spsc_count(&Q) == m_head - m_tail) ? 0U : 1U;
    }
    CHECK_EQ_U(errors, 0U);
    CHECK_EQ_U(gbt_spsc_dropped(&Q), model_dropped);
    CHECK(model_dropped > 0U);                       /* full path exercised */
    CHECK(pops_ok > 20000U);
    CHECK_EQ_U(pops_ok + model_dropped + (m_head - m_tail), seq);   /* every frame accounted */
}

/* Index wraparound: counters cross 2^32 (white-box set-up while no producer runs). */
static void test_wraparound(void)
{
    gbt_can_frame_t f;
    gbt_can_frame_t out;
    uint32_t i;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    Q.head = 0xFFFFFFF8u;
    Q.tail = 0xFFFFFFF8u;
    CHECK_EQ_U(gbt_spsc_count(&Q), 0U);
    for (i = 0U; i < GBT_SPSC_CAPACITY; i++) {       /* head wraps 0xFFFFFFF8 -> 0x8 */
        f = mk(1000U + i);
        CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);
    }
    CHECK_EQ_U(Q.head, 0x00000008u);
    CHECK_EQ_U(gbt_spsc_count(&Q), 16U);             /* head - tail across the wrap */
    f = mk(9999U);
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_E_FULL);
    for (i = 0U; i < GBT_SPSC_CAPACITY; i++) {
        gbt_can_frame_t e = mk(1000U + i);
        CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
        CHECK(same(&e, &out));
    }
    CHECK_EQ_U(Q.tail, 0x00000008u);
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_E_EMPTY);
    for (i = 0U; i < 100U; i++) {                    /* steady state after the wrap */
        f = mk(i);
        CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);
        CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
        CHECK(same(&f, &out));
    }
}

static void test_dropped_saturates(void)
{
    gbt_can_frame_t f = mk(1U);
    uint32_t i;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    for (i = 0U; i < GBT_SPSC_CAPACITY; i++) {
        (void)gbt_spsc_push(&Q, &f);
    }
    Q.dropped = 0xFFFFFFFEu;                         /* white-box: near saturation */
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_E_FULL);
    CHECK_EQ_U(gbt_spsc_dropped(&Q), 0xFFFFFFFFu);
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_E_FULL);
    CHECK_EQ_U(gbt_spsc_dropped(&Q), 0xFFFFFFFFu);  /* saturates, no wrap to 0 */
}

/* Producer "preempts" the consumer between API calls: count is a lower bound. */
static void test_interleaving_api(void)
{
    gbt_can_frame_t f;
    gbt_can_frame_t out;
    uint32_t c;
    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    f = mk(1U);
    (void)gbt_spsc_push(&Q, &f);
    c = gbt_spsc_count(&Q);                          /* consumer snapshot: 1 */
    f = mk(2U);
    CHECK(gbt_spsc_push(&Q, &f) == GBT_SPSC_OK);     /* ISR arrives after the snapshot */
    CHECK_EQ_U(c, 1U);
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);
    CHECK_EQ_U(out.id, mk(1U).id);
    CHECK(gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK);    /* frame pushed later is still delivered */
    CHECK_EQ_U(out.id, mk(2U).id);
}

int main(void)
{
    CHECK_EQ_U(GBT_SPSC_CAPACITY & (GBT_SPSC_CAPACITY - 1U), 0U);   /* power of two */
    test_params();
    test_empty_and_single();
    test_data_bytes();
    test_fifo_full_overflow();
    test_one_slot_freed();
    test_repeated();
    test_wraparound();
    test_dropped_saturates();
    test_interleaving_api();
    return TEST_SUMMARY("test_spsc");
}
