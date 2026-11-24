/* tests/unit/test_outbox.c - protocol outbox: 16 usable entries (B1-06, PROVISIONAL S-9). */
#include "protocol/gbt_outbox.h"
#include "tests/support/test_assert.h"

static gbt_can_frame_t mk(uint32_t id)
{
    gbt_can_frame_t f = { 0 };
    f.id = id;
    f.dlc = 1U;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f.data[0] = (uint8_t)(id & 0xFFU);
    return f;
}

static void test_capacity_and_order(void)
{
    gbt_outbox_t ob;
    gbt_can_frame_t f;
    uint32_t i;
    uint32_t round;

    CHECK(gbt_outbox_init(&ob) == GBT_RING_OK);
    CHECK_EQ_U(GBT_OUTBOX_LEN, 16U);
    CHECK_EQ_U(gbt_outbox_count(&ob), 0U);
    CHECK(gbt_outbox_pop(&ob, &f) == GBT_RING_E_EMPTY);
    CHECK(gbt_outbox_peek(&ob, &f) == GBT_RING_E_EMPTY);

    for (i = 0U; i < 16U; i++) {
        f = mk(i);
        CHECK(gbt_outbox_push(&ob, &f) == GBT_RING_OK);   /* all 16 usable */
    }
    CHECK_EQ_U(gbt_outbox_count(&ob), 16U);
    f = mk(99U);
    CHECK(gbt_outbox_push(&ob, &f) == GBT_RING_E_FULL);   /* 17th rejected */
    CHECK_EQ_U(gbt_outbox_count(&ob), 16U);

    CHECK(gbt_outbox_peek(&ob, &f) == GBT_RING_OK);
    CHECK_EQ_U(f.id, 0U);
    CHECK_EQ_U(gbt_outbox_count(&ob), 16U);               /* peek keeps head (ND-3 E_FULL use) */

    /* FIFO order across several wrap-arounds */
    for (round = 0U; round < 3U; round++) {
        for (i = 0U; i < 16U; i++) {
            CHECK(gbt_outbox_pop(&ob, &f) == GBT_RING_OK);
            CHECK_EQ_U(f.id, round * 100U + i);
            CHECK_EQ_U(f.data[0], (round * 100U + i) & 0xFFU);
            f = mk((round + 1U) * 100U + i);
            CHECK(gbt_outbox_push(&ob, &f) == GBT_RING_OK);
        }
    }
    CHECK_EQ_U(gbt_outbox_count(&ob), 16U);
}

static void test_null(void)
{
    gbt_can_frame_t f = { 0 };
    CHECK(gbt_outbox_init(NULL) == GBT_RING_E_PARAM);
    CHECK(gbt_outbox_push(NULL, &f) == GBT_RING_E_PARAM);
    CHECK(gbt_outbox_pop(NULL, &f) == GBT_RING_E_PARAM);
    CHECK(gbt_outbox_peek(NULL, &f) == GBT_RING_E_PARAM);
    CHECK_EQ_U(gbt_outbox_count(NULL), 0U);
}

int main(void)
{
    test_capacity_and_order();
    test_null();
    return TEST_SUMMARY("test_outbox");
}
