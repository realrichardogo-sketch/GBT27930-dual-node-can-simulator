/* tests/unit/test_ring.c - gbt_ring (adapted from OLD tests/unit/ring_buffer_test.c, K-9b). */
#include "protocol/gbt_ring.h"
#include "tests/support/test_assert.h"

#include <stdint.h>

static void test_init(void)
{
    gbt_ring_t r = { 0 };
    uint32_t storage[3] = { 0U };

    CHECK(gbt_ring_init(&r, storage, 3U, sizeof(uint32_t)) == GBT_RING_OK);
    CHECK(gbt_ring_init(NULL, storage, 3U, sizeof(uint32_t)) == GBT_RING_E_PARAM);
    CHECK(gbt_ring_init(&r, NULL, 3U, sizeof(uint32_t)) == GBT_RING_E_PARAM);
    CHECK(gbt_ring_init(&r, storage, 0U, sizeof(uint32_t)) == GBT_RING_E_PARAM);
    CHECK(gbt_ring_init(&r, storage, 3U, 0U) == GBT_RING_E_PARAM);
    CHECK(gbt_ring_init(&r, storage, SIZE_MAX, 2U) == GBT_RING_E_PARAM);   /* overflow guard */
}

static void test_uninitialised(void)
{
    gbt_ring_t r = { 0 };
    uint32_t v = 1U;
    CHECK(gbt_ring_push(&r, &v) == GBT_RING_E_NOT_INIT);
    CHECK(gbt_ring_pop(&r, &v) == GBT_RING_E_NOT_INIT);
    CHECK(gbt_ring_peek(&r, &v) == GBT_RING_E_NOT_INIT);
    CHECK_EQ_U(gbt_ring_count(&r), 0U);
    CHECK_EQ_U(gbt_ring_capacity(&r), 0U);
    CHECK(gbt_ring_push(NULL, &v) == GBT_RING_E_PARAM);
}

static void test_fifo_full_empty_wrap(void)
{
    gbt_ring_t r = { 0 };
    uint32_t storage[3] = { 0U };
    uint32_t v;
    uint32_t i;

    CHECK(gbt_ring_init(&r, storage, 3U, sizeof(uint32_t)) == GBT_RING_OK);
    CHECK(gbt_ring_pop(&r, &v) == GBT_RING_E_EMPTY);
    for (i = 1U; i <= 3U; i++) {
        CHECK(gbt_ring_push(&r, &i) == GBT_RING_OK);
    }
    v = 4U;
    CHECK(gbt_ring_push(&r, &v) == GBT_RING_E_FULL);   /* never overwrites */
    CHECK_EQ_U(gbt_ring_count(&r), 3U);
    CHECK(gbt_ring_peek(&r, &v) == GBT_RING_OK);
    CHECK_EQ_U(v, 1U);
    CHECK_EQ_U(gbt_ring_count(&r), 3U);                /* peek keeps the head */
    CHECK(gbt_ring_pop(&r, &v) == GBT_RING_OK);
    CHECK_EQ_U(v, 1U);
    v = 4U;
    CHECK(gbt_ring_push(&r, &v) == GBT_RING_OK);       /* wraps */
    for (i = 2U; i <= 4U; i++) {
        CHECK(gbt_ring_pop(&r, &v) == GBT_RING_OK);
        CHECK_EQ_U(v, i);
    }
    CHECK(gbt_ring_pop(&r, &v) == GBT_RING_E_EMPTY);
    CHECK_EQ_U(gbt_ring_capacity(&r), 3U);
}

int main(void)
{
    test_init();
    test_uninitialised();
    test_fifo_full_empty_wrap();
    return TEST_SUMMARY("test_ring");
}
