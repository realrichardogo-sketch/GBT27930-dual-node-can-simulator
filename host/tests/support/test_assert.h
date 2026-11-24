/* tests/support/test_assert.h - minimal C99 unit-test macros (no external framework). */
#ifndef GBT_TEST_ASSERT_H
#define GBT_TEST_ASSERT_H

#include <stdio.h>

static unsigned g_test_checks;
static unsigned g_test_failures;

#define CHECK(cond)                                                          \
    do {                                                                     \
        g_test_checks++;                                                     \
        if (!(cond)) {                                                       \
            g_test_failures++;                                               \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);  \
        }                                                                    \
    } while (0)

#define CHECK_EQ_U(a, b)                                                     \
    do {                                                                     \
        unsigned long check_a_ = (unsigned long)(a);                         \
        unsigned long check_b_ = (unsigned long)(b);                         \
        g_test_checks++;                                                     \
        if (check_a_ != check_b_) {                                          \
            g_test_failures++;                                               \
            fprintf(stderr, "FAIL %s:%d: %s == %s (0x%lX != 0x%lX)\n",       \
                    __FILE__, __LINE__, #a, #b, check_a_, check_b_);         \
        }                                                                    \
    } while (0)

#define TEST_SUMMARY(name)                                                   \
    (printf("%s: %u checks, %u failures\n", (name), g_test_checks,           \
            g_test_failures),                                                \
     (g_test_failures == 0U) ? 0 : 1)

#endif /* GBT_TEST_ASSERT_H */
