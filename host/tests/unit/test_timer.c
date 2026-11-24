/*
 * tests/unit/test_timer.c - wrap-safe timer primitives (M2, B1-08).
 * gbt_timer_expired implements S-5 (PROVISIONAL P-S5-01): elapsed >= T.
 */
#include "protocol/gbt_timer.h"
#include "tests/support/test_assert.h"

#include <stddef.h>

static void test_elapsed(void)
{
    CHECK_EQ_U(gbt_time_elapsed(0U, 0U), 0U);
    CHECK_EQ_U(gbt_time_elapsed(1000U, 250U), 750U);
    CHECK_EQ_U(gbt_time_elapsed(0xFFFFFFFFu, 0xFFFFFFFFu), 0U);
    /* rollover: start just before 2^32, now just after */
    CHECK_EQ_U(gbt_time_elapsed(0x00000000u, 0xFFFFFFFFu), 1U);
    CHECK_EQ_U(gbt_time_elapsed(0x00000004u, 0xFFFFFFF0u), 0x14U);
    CHECK_EQ_U(gbt_time_elapsed(999U, 0xFFFFFC18u), 1999U);   /* start = 2^32 - 1000 */
    /* maximum representable elapsed time */
    CHECK_EQ_U(gbt_time_elapsed(0xFFFFFFFEu, 0xFFFFFFFFu), 0xFFFFFFFFu);
}

static void test_arm_disarm(void)
{
    gbt_timer_t t = { 0U, 0U, 0U };

    CHECK_EQ_U(gbt_timer_is_armed(&t), 0U);
    CHECK_EQ_U(gbt_timer_elapsed(&t, 1234U), 0U);           /* disarmed -> 0 */

    gbt_timer_arm(&t, 0xFFFFFF00u, 1000U);
    CHECK_EQ_U(gbt_timer_is_armed(&t), 1U);
    CHECK_EQ_U(t.start_ms, 0xFFFFFF00u);
    CHECK_EQ_U(t.dur_ms, 1000U);
    CHECK_EQ_U(gbt_timer_elapsed(&t, 0xFFFFFF00u), 0U);
    CHECK_EQ_U(gbt_timer_elapsed(&t, 0x00000100u), 0x200U);  /* across the wrap */

    gbt_timer_arm(&t, 50U, 7U);                              /* re-arm restarts */
    CHECK_EQ_U(gbt_timer_elapsed(&t, 60U), 10U);

    gbt_timer_disarm(&t);
    CHECK_EQ_U(gbt_timer_is_armed(&t), 0U);
    CHECK_EQ_U(gbt_timer_elapsed(&t, 60U), 0U);
}

static void test_expiry_boundary(void)
{
    gbt_timer_t t = { 0U, 0U, 0U };
    CHECK_EQ_U(gbt_timer_expired(&t, 5U), 0U);               /* disarmed */
    gbt_timer_arm(&t, 0xFFFFFC18u, 1000U);                   /* T = 1000 ms across the wrap */
    CHECK_EQ_U(gbt_timer_expired(&t, 0xFFFFFFFEu), 0U);      /* elapsed 998 */
    CHECK_EQ_U(gbt_timer_expired(&t, 0xFFFFFFFFu), 0U);      /* elapsed 999 = T - 1 */
    CHECK_EQ_U(gbt_timer_expired(&t, 0U), 1U);               /* elapsed 1000 = T: expired (S-5) */
    CHECK_EQ_U(gbt_timer_expired(&t, 1U), 1U);               /* T + 1 */
    gbt_timer_arm(&t, 100U, 0U);
    CHECK_EQ_U(gbt_timer_expired(&t, 100U), 1U);             /* zero duration */
    CHECK_EQ_U(gbt_timer_expired(NULL, 100U), 0U);
}

static void test_null(void)
{
    gbt_timer_arm(NULL, 1U, 1U);
    gbt_timer_disarm(NULL);
    CHECK_EQ_U(gbt_timer_is_armed(NULL), 0U);
    CHECK_EQ_U(gbt_timer_elapsed(NULL, 5U), 0U);
}

int main(void)
{
    test_elapsed();
    test_arm_disarm();
    test_expiry_boundary();
    test_null();
    return TEST_SUMMARY("test_timer");
}
