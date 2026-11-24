/*
 * tests/unit/test_spsc_signal.c - host stress test of ports/common/gbt_spsc
 * with real asynchronous preemption: a SIGALRM handler (interval timer) is
 * the producer and interrupts the consumer loop at arbitrary instructions,
 * the closest host analogue of a CAN RX ISR preempting the main loop on the
 * same core. Checks: no torn or corrupted frame (all bytes derived from the
 * sequence number), strictly increasing order, and exact accounting
 * produced == consumed + dropped.
 *
 * Supplementary evidence only: it does not prove the absence of races on
 * Cortex-M; the argument for the target is in docs/STEP07C1_SPSC_DESIGN.md.
 */
#define _XOPEN_SOURCE 700
#include "ports/common/gbt_spsc.h"
#include "tests/support/test_assert.h"

#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static gbt_spsc_t Q;
static volatile uint32_t g_produced;      /* written by the handler only */
static volatile uint32_t g_signals;       /* written by the handler only */
static volatile sig_atomic_t g_stop;

static void fill(gbt_can_frame_t *f, uint32_t seq)
{
    unsigned i;
    f->id = seq & 0x1FFFFFFFu;
    f->flags = (uint8_t)GBT_CAN_FLAG_EXT;
    f->dlc = (uint8_t)(seq % 9U);
    for (i = 0U; i < 8U; i++) {
        f->data[i] = (uint8_t)(((seq >> (8U * (i % 4U))) ^ (0x5AU + i * 0x11U)) & 0xFFU);
    }
}

static int same(const gbt_can_frame_t *a, const gbt_can_frame_t *b)
{
    return a->id == b->id && a->dlc == b->dlc && a->flags == b->flags
           && memcmp(a->data, b->data, 8U) == 0;      /* fields only: no padding */
}

static unsigned long now_ms(void)
{
    struct timespec ts;
    (void)clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)ts.tv_sec * 1000UL + (unsigned long)ts.tv_nsec / 1000000UL;
}

static void on_alarm(int sig)
{
    gbt_can_frame_t f;
    uint32_t n;
    uint32_t k;
    (void)sig;
    if (g_stop != 0) {
        return;
    }
    g_signals = g_signals + 1U;
    n = 1U + (g_signals % 4U);            /* bursts of 1..4 frames per "interrupt" */
    for (k = 0U; k < n; k++) {
        fill(&f, g_produced);
        (void)gbt_spsc_push(&Q, &f);      /* full -> counted in dropped */
        g_produced = g_produced + 1U;
    }
}

int main(void)
{
    struct sigaction sa;
    struct itimerval it;
    gbt_can_frame_t out;
    gbt_can_frame_t exp;
    uint32_t consumed = 0U;
    uint32_t last = 0U;
    uint8_t have_last = 0U;
    unsigned corrupt = 0U;
    unsigned order = 0U;
    unsigned long t0;
    volatile unsigned sink = 0U;

    CHECK(gbt_spsc_init(&Q) == GBT_SPSC_OK);
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    CHECK(sigaction(SIGALRM, &sa, NULL) == 0);
    memset(&it, 0, sizeof it);
    it.it_interval.tv_usec = 50;          /* as fast as the host timer allows */
    it.it_value.tv_usec = 50;
    CHECK(setitimer(ITIMER_REAL, &it, NULL) == 0);

    t0 = now_ms();
    while (g_signals < 20000U && now_ms() - t0 < 10000UL) {   /* bounded: 10 s */
        if (gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK) {
            fill(&exp, out.id);           /* id carries the sequence (< 2^29 here) */
            if (!same(&exp, &out)) {
                corrupt++;
            }
            if (have_last != 0U && out.id <= last) {
                order++;
            }
            last = out.id;
            have_last = 1U;
            consumed++;
            if ((consumed % 64U) == 0U) {  /* slow consumer phase: >= 12 interrupts */
                uint32_t s0 = g_signals;   /* (bursts 1..4) so the queue fills and drops */
                unsigned long tw = now_ms();
                while ((uint32_t)(g_signals - s0) < 12U && now_ms() - tw < 100UL) {
                    sink = sink + 1U;
                }
            }
        }
    }
    memset(&it, 0, sizeof it);
    (void)setitimer(ITIMER_REAL, &it, NULL);
    g_stop = 1;
    while (gbt_spsc_pop(&Q, &out) == GBT_SPSC_OK) {   /* drain */
        fill(&exp, out.id);
        corrupt += same(&exp, &out) ? 0U : 1U;
        order += (have_last != 0U && out.id <= last) ? 1U : 0U;
        last = out.id;
        have_last = 1U;
        consumed++;
    }
    CHECK(g_signals >= 2000U);            /* enough preemptions happened */
    CHECK_EQ_U(corrupt, 0U);
    CHECK_EQ_U(order, 0U);
    CHECK_EQ_U(consumed + gbt_spsc_dropped(&Q), g_produced);
    CHECK(gbt_spsc_dropped(&Q) > 0U);     /* full path exercised under preemption */
    CHECK(consumed > 0U);
    printf("spsc signal stress: %lu interrupts, %lu produced, %lu consumed, %lu dropped\n",
           (unsigned long)g_signals, (unsigned long)g_produced, (unsigned long)consumed,
           (unsigned long)gbt_spsc_dropped(&Q));
    return TEST_SUMMARY("test_spsc_signal");
}
