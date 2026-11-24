/*
 * tests/unit/test_port_f103.c - host tests of ports/stm32/f103/port_f103.c against the bxCAN
 * fake (tests/support/fake_stm32f1). fake_f1_irq() plays HAL_CAN_IRQHandler on the RX0 line;
 * fake_f1_bus_tx() plays the bus completing a pending mailbox. Also built with ASan/UBSan.
 */
#include "ports/stm32/f103/port_f103.h"
#include "fake_stm32f1.h"
#include "protocol/gbt_id.h"
#include "tests/support/test_assert.h"

#include <string.h>

static CAN_HandleTypeDef H;
static gbt_hal_t P;
static uint8_t D8[8];

#define ID_CHM   0x1826F456u   /* charger -> vehicle */
#define ID_BCL   0x181056F4u   /* vehicle -> charger */
#define ID_TPDT  0x1CEB56F4u   /* vehicle TP.DT */

static void setup(void)
{
    gbt_port_f103_cfg_t cfg;
    if (P.lifecycle != GBT_HAL_LC_UNINIT) {           /* release the instance slot */
        H.State = HAL_CAN_STATE_LISTENING;
        P.lifecycle = (uint8_t)GBT_HAL_LC_READY;
        P.in_sink = 0U;
        (void)gbt_port_f103_deinit(&P);
    }
    memset(&P, 0, sizeof P);
    fake_f1_reset(&H);
    cfg.hcan = &H;
    cfg.own_sa = 0xF4U;
    cfg.peer_sa = 0x56U;
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_OK);
}

static void fill8(uint8_t seed)
{
    unsigned i;
    for (i = 0U; i < 8U; i++) {
        D8[i] = (uint8_t)(seed + i * 5U + 1U);
    }
}

static int bus(uint32_t id, uint8_t raw_dlc)            /* ext data frame + its interrupt */
{
    int r = fake_f1_bus_rx(id, 1U, 0U, raw_dlc, D8);
    fake_f1_irq();
    return r;
}

static gbt_can_status_t status(void)
{
    gbt_can_status_t s;
    memset(&s, 0xA5, sizeof s);
    CHECK(gbt_hal_can_get_status(&P, &s) == GBT_HAL_OK);
    return s;
}

static gbt_port_f103_diag_t diag(void)
{
    gbt_port_f103_diag_t d;
    CHECK(gbt_port_f103_get_diag(&P, &d) == GBT_HAL_OK);
    return d;
}

static gbt_can_frame_t txf(uint32_t id, uint8_t dlc, uint8_t flags, uint8_t seed)
{
    gbt_can_frame_t f;
    unsigned i;
    f.id = id;
    f.dlc = dlc;
    f.flags = flags;
    for (i = 0U; i < 8U; i++) {
        f.data[i] = (uint8_t)(seed + i);
    }
    return f;
}

/* ---- init and filter (Task A, D) ---- */
static void test_init(void)
{
    gbt_port_f103_cfg_t cfg;
    gbt_hal_t other;
    memset(&P, 0, sizeof P);
    fake_f1_reset(&H);
    cfg.hcan = &H;
    cfg.own_sa = 0xF4U;
    cfg.peer_sa = 0x56U;
    CHECK(gbt_port_f103_init(NULL, &cfg) == GBT_HAL_E_PARAM);
    CHECK(gbt_port_f103_init(&P, NULL) == GBT_HAL_E_PARAM);
    cfg.peer_sa = 0xF4U;
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_E_PARAM);
    cfg.peer_sa = 0x56U;
    CHECK_EQ_U(g_fake_f1.order_len, 0U);
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_OK);
    CHECK(strcmp(g_fake_f1.order, "CNS") == 0);          /* filter, notify, start */
    CHECK_EQ_U(g_fake_f1.filter_bank, 0U);
    CHECK_EQ_U(g_fake_f1.filter_mode, CAN_FILTERMODE_IDMASK);
    CHECK_EQ_U(g_fake_f1.filter_scale, CAN_FILTERSCALE_32BIT);
    CHECK_EQ_U(g_fake_f1.filter_fifo, CAN_FILTER_FIFO0);
    CHECK_EQ_U(g_fake_f1.filter_active, 1U);
    CHECK_EQ_U(g_fake_f1.filter_id_word, 0x0007A2B4u);  /* (0xF456 << 3) | IDE */
    CHECK_EQ_U(g_fake_f1.filter_mask_word, 0x0007FFFEu); /* (0xFFFF << 3) | IDE | RTR */
    CHECK_EQ_U(GBT_PORT_F103_FILTER_ID(0x0000F456u) >> 16, 0x0007U);
    CHECK_EQ_U(GBT_PORT_F103_FILTER_ID(0x0000F456u) & 0xFFFFU, 0xA2B4U);
    CHECK_EQ_U(GBT_PORT_F103_FILTER_MASK(0x0000FFFFu) >> 16, 0x0007U);
    CHECK_EQ_U(GBT_PORT_F103_FILTER_MASK(0x0000FFFFu) & 0xFFFFU, 0xFFFEU);
    CHECK_EQ_U(g_fake_f1.regs.IER, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO0_OVERRUN);
    CHECK_EQ_U(H.State, HAL_CAN_STATE_LISTENING);
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_E_STATE);
    memset(&other, 0, sizeof other);
    CHECK(gbt_port_f103_init(&other, &cfg) == GBT_HAL_E_STATE);   /* one instance */
    CHECK(gbt_port_f103_deinit(&P) == GBT_HAL_OK);
    CHECK_EQ_U(H.State, HAL_CAN_STATE_READY);
    CHECK_EQ_U(g_fake_f1.regs.IER, 0U);
    /* failures leave the controller unstarted and the instance slot free */
    fake_f1_reset(&H);
    g_fake_f1.fail_filter = HAL_ERROR;
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_E_IO);
    fake_f1_reset(&H);
    g_fake_f1.fail_notify = HAL_ERROR;
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_E_IO);
    fake_f1_reset(&H);
    g_fake_f1.fail_start = HAL_ERROR;
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_E_IO);
    CHECK_EQ_U(g_fake_f1.regs.IER, 0U);
    fake_f1_reset(&H);
    H.State = HAL_CAN_STATE_LISTENING;                   /* already started by user code */
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_E_IO);
    CHECK_EQ_U(P.lifecycle, GBT_HAL_LC_UNINIT);
    fake_f1_reset(&H);
    CHECK(gbt_port_f103_init(&P, &cfg) == GBT_HAL_OK);
    CHECK(gbt_port_f103_deinit(&P) == GBT_HAL_OK);
}

/* every charger -> vehicle identifier is accepted, everything else rejected */
static void test_filter_acceptance(void)
{
    unsigned m;
    uint32_t id;
    gbt_can_frame_t f;
    unsigned c2v = 0U;
    setup();
    fill8(1U);
    for (m = 0U; m < (unsigned)GBT_MSG_COUNT; m++) {
        const gbt_msg_def_t *d = gbt_msg_def((gbt_msg_t)m);
        CHECK(gbt_id_for_msg((gbt_msg_t)m, &id) == GBT_ID_OK);
        if (d->dir == GBT_DIR_C2V) {
            CHECK_EQ_U(bus(id, 8U), 1U);
            CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
            CHECK_EQ_U(f.id, id);
            c2v++;
        } else {
            CHECK_EQ_U(bus(id, 8U), 0U);                 /* own (V->C) identifiers rejected */
        }
    }
    CHECK(c2v >= 8U);
    CHECK(gbt_id_make(7U, 0xECU, 0xF4U, 0x56U, &id) == GBT_ID_OK);   /* TP.CM C->V (CTS/EOM) */
    CHECK_EQ_U(id, 0x1CECF456u);
    CHECK_EQ_U(bus(id, 8U), 1U);
    CHECK(gbt_id_make(7U, 0xEBU, 0xF4U, 0x56U, &id) == GBT_ID_OK);   /* TP.DT C->V */
    CHECK_EQ_U(bus(id, 8U), 1U);
    CHECK_EQ_U(bus(0x1CEC56F4u, 8U), 0U);               /* TP.CM V->C */
    CHECK_EQ_U(bus(0x1826F457u, 8U), 0U);               /* other source */
    CHECK_EQ_U(bus(0x1826F556u, 8U), 0U);               /* other destination */
    CHECK_EQ_U(fake_f1_bus_rx(0x456u, 0U, 0U, 8U, D8), 0);           /* 11-bit */
    CHECK_EQ_U(fake_f1_bus_rx(ID_CHM, 1U, 1U, 0U, D8), 0);           /* RTR */
    CHECK_EQ_U(status().rx_invalid, 0U);
}

/* ---- RX (Task A, D2) ---- */
static void test_rx_dlc_and_data(void)
{
    gbt_can_frame_t f;
    uint8_t dlc;
    gbt_port_f103_diag_t d;
    setup();
    for (dlc = 0U; dlc <= 8U; dlc++) {
        unsigned b;
        fill8((uint8_t)(0x30U + dlc));
        CHECK_EQ_U(bus(0x1FFFF456u, dlc), 1U);           /* max 29-bit id with 0xF456 */
        CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
        CHECK_EQ_U(f.id, 0x1FFFF456u);
        CHECK_EQ_U(f.flags, GBT_CAN_FLAG_EXT);
        CHECK_EQ_U(f.dlc, dlc);
        for (b = 0U; b < 8U; b++) {
            CHECK_EQ_U(f.data[b], (b < dlc) ? D8[b] : 0U);
        }
    }
    for (dlc = 9U; dlc <= 15U; dlc++) {                  /* D2: original DLC > 8 */
        fill8(dlc);
        CHECK_EQ_U(bus(ID_CHM, dlc), 1U);
        CHECK_EQ_U(g_fake_f1.fifo_count, 0U);           /* released */
        CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_EMPTY);   /* never forwarded */
    }
    d = diag();
    CHECK_EQ_U(d.rx_bad_dlc, 7U);
    CHECK_EQ_U(d.rx_frames, 9U);
    CHECK_EQ_U(status().rx_invalid, 7U);
    /* standard and RTR frames when the filter would let them through (white-box) */
    g_fake_f1.filter_mask_word = 0U;
    CHECK_EQ_U(fake_f1_bus_rx(0x123u, 0U, 0U, 8U, D8), 1);
    fake_f1_irq();
    CHECK_EQ_U(fake_f1_bus_rx(ID_CHM, 1U, 1U, 2U, D8), 1);
    fake_f1_irq();
    d = diag();
    CHECK_EQ_U(d.rx_std_id, 1U);
    CHECK_EQ_U(d.rx_remote, 1U);
    CHECK_EQ_U(status().rx_invalid, 9U);
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_EMPTY);
    /* GetRxMessage failure */
    CHECK_EQ_U(fake_f1_bus_rx(ID_CHM, 1U, 0U, 8U, D8), 1);
    g_fake_f1.fail_getrx = HAL_ERROR;
    fake_f1_irq();
    g_fake_f1.fail_getrx = HAL_OK;
    CHECK(diag().rx_hal_error >= 1U);                    /* level-triggered IRQ re-enters */
    CHECK_EQ_U(H.ErrorCode, HAL_CAN_ERROR_NONE);
}

static void test_rx_overrun(void)
{
    gbt_can_frame_t f;
    unsigned i;
    setup();
    fill8(3U);
    for (i = 0U; i < 3U; i++) {                          /* three before the ISR runs */
        D8[0] = (uint8_t)i;
        CHECK_EQ_U(fake_f1_bus_rx(ID_CHM, 1U, 0U, 8U, D8), 1);
    }
    D8[0] = 9U;
    CHECK_EQ_U(fake_f1_bus_rx(ID_CHM, 1U, 0U, 8U, D8), 2);       /* overwrites newest, FOVR0 */
    fake_f1_irq();
    CHECK_EQ_U(g_fake_f1.fifo_count, 0U);
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
    CHECK_EQ_U(f.data[0], 0U);
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
    CHECK_EQ_U(f.data[0], 1U);
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
    CHECK_EQ_U(f.data[0], 9U);                           /* RFLM = 0: third replaced */
    CHECK_EQ_U(status().rx_overflow_hw, 1U);
    CHECK_EQ_U(diag().rx_fifo_overruns, 1U);
    CHECK_EQ_U(diag().hal_error_callbacks, 1U);
    CHECK_EQ_U(H.ErrorCode & HAL_CAN_ERROR_RX_FOV0, 0U);         /* cleared by the ISR */
    fake_f1_irq();                                                /* no double counting */
    CHECK_EQ_U(status().rx_overflow_hw, 1U);
    /* software queue overflow: 20 frames, consumer idle */
    for (i = 0U; i < 20U; i++) {
        D8[1] = (uint8_t)i;
        CHECK_EQ_U(bus(ID_CHM, 8U), 1U);
    }
    CHECK_EQ_U(status().rx_overflow_sw, 4U);
    for (i = 0U; i < 16U; i++) {
        CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
        CHECK_EQ_U(f.data[1], i);
    }
}

/* ---- TX (Task A, D1) ---- */
static void test_tx_single_mailbox(void)
{
    gbt_can_frame_t f;
    fake_f1_tx_t t;
    uint8_t dlc;
    uint8_t seq;
    setup();
    for (dlc = 0U; dlc <= 8U; dlc++) {
        unsigned b;
        f = txf(ID_BCL, dlc, (uint8_t)GBT_CAN_FLAG_EXT, (uint8_t)(0x11U * dlc));
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_FULL);        /* one in flight */
        CHECK_EQ_U(fake_f1_bus_tx(1U), 1U);
        CHECK(fake_f1_tx_pop(&t) == 1);
        CHECK_EQ_U(t.id, ID_BCL);
        CHECK_EQ_U(t.ide, 1U);
        CHECK_EQ_U(t.rtr, 0U);
        CHECK_EQ_U(t.dlc, dlc);
        for (b = 0U; b < dlc; b++) {
            CHECK_EQ_U(t.data[b], f.data[b]);
        }
    }
    CHECK_EQ_U(g_fake_f1.calls_add, 9U);                 /* busy attempts never reached the HAL */
    f = txf(0x7FFu, 2U, (uint8_t)GBT_CAN_FLAG_STD, 1U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    CHECK_EQ_U(fake_f1_bus_tx(1U), 1U);
    CHECK(fake_f1_tx_pop(&t) == 1);
    CHECK_EQ_U(t.ide, 0U);
    CHECK_EQ_U(t.id, 0x7FFu);
    /* TP.DT order: decreasing priority would reorder across mailboxes; one-in-flight cannot */
    for (seq = 1U; seq <= 7U; seq++) {
        uint32_t id = (seq % 2U != 0U) ? ID_TPDT : 0x181056F4u;   /* interleave higher-priority id */
        f = txf(id, 8U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
        f.data[0] = seq;
        while (gbt_hal_can_send(&P, &f) == GBT_HAL_E_FULL) {
            CHECK_EQ_U(fake_f1_bus_tx(1U), 1U);
        }
    }
    while (fake_f1_bus_tx(1U) == 1) {
    }
    for (seq = 1U; seq <= 7U; seq++) {
        CHECK(fake_f1_tx_pop(&t) == 1);
        CHECK_EQ_U(t.data[0], seq);                      /* transmission order preserved */
    }
    CHECK_EQ_U(diag().tx_completed_ok, 17U);
    CHECK_EQ_U(diag().tx_completed_fail, 0U);
    CHECK_EQ_U(H.ErrorCode, HAL_CAN_ERROR_NONE);
}

/* demonstrates why D1 is needed: three mailboxes in flight are reordered by identifier */
static void test_fake_arbitration_reorders(void)
{
    CAN_TxHeaderTypeDef th;
    uint8_t data[8] = {0};
    uint32_t mb;
    fake_f1_tx_t t;
    uint8_t i;
    setup();
    memset(&th, 0, sizeof th);
    th.IDE = CAN_ID_EXT;
    th.DLC = 8U;
    for (i = 0U; i < 3U; i++) {
        th.ExtId = 0x1CEB56F4u - i;                      /* later frames have higher priority */
        data[0] = (uint8_t)(i + 1U);
        CHECK(HAL_CAN_AddTxMessage(&H, &th, data, &mb) == HAL_OK);
    }
    for (i = 0U; i < 3U; i++) {
        CHECK_EQ_U(fake_f1_bus_tx(1U), 1U);
    }
    CHECK(fake_f1_tx_pop(&t) == 1);
    CHECK_EQ_U(t.data[0], 3U);                           /* reordered on the bus */
}

static void test_tx_failure_busy_busoff(void)
{
    gbt_can_frame_t f = txf(ID_BCL, 5U, (uint8_t)GBT_CAN_FLAG_EXT, 1U);
    CAN_TxHeaderTypeDef th;
    uint8_t data[8] = {0};
    uint32_t mb;
    gbt_port_f103_diag_t d;
    setup();
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    g_fake_f1.tick = 37U;
    CHECK_EQ_U(diag().tx_in_flight, 1U);
    CHECK_EQ_U(diag().tx_in_flight_ms, 37U);
    CHECK_EQ_U(fake_f1_bus_tx(0U), 1U);                  /* completed with TXOK = 0 */
    d = diag();
    CHECK_EQ_U(d.tx_completed_fail, 1U);
    CHECK_EQ_U(d.tx_in_flight, 0U);
    CHECK_EQ_U(status().tx_error, 1U);
    (void)HAL_CAN_GetTxMailboxesFreeLevel(&H);           /* next HAL access applies the w1c write */
    CHECK_EQ_U(g_fake_f1.regs.TSR & (CAN_TSR_RQCP0 | CAN_TSR_TERR0), 0U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);       /* next frame goes out */
    CHECK_EQ_U(fake_f1_bus_tx(1U), 1U);
    /* a mailbox occupied by someone else: no submission, no ErrorCode write */
    memset(&th, 0, sizeof th);
    th.IDE = CAN_ID_EXT;
    CHECK(HAL_CAN_AddTxMessage(&H, &th, data, &mb) == HAL_OK);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_FULL);
    CHECK_EQ_U(H.ErrorCode, HAL_CAN_ERROR_NONE);
    CHECK_EQ_U(fake_f1_bus_tx(1U), 1U);
    /* HAL failure */
    g_fake_f1.fail_add = HAL_ERROR;
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_IO);
    g_fake_f1.fail_add = HAL_OK;
    CHECK_EQ_U(diag().last_tx_hal_status, HAL_ERROR);
    /* bus-off inhibits TX */
    fake_f1_set_esr(1U, 1U, 1U, 255U, 0U);
    {
        uint32_t calls = g_fake_f1.calls_add;
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_BUS_OFF);
        CHECK_EQ_U(g_fake_f1.calls_add, calls);
    }
    fake_f1_set_esr(0U, 0U, 0U, 0U, 0U);
    d = diag();
    CHECK_EQ_U(d.tx_bus_off, 1U);
    CHECK_EQ_U(d.tx_hal_error, 1U);
    CHECK(d.tx_busy >= 1U);
    CHECK_EQ_U(status().tx_error, 3U);                   /* fail + hal error + bus-off */
    {
        gbt_can_frame_t bad = txf(ID_BCL, 9U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
        CHECK(gbt_hal_can_send(&P, &bad) == GBT_HAL_E_PARAM);
    }
}

/* ---- status (Task A) ---- */
static void test_status(void)
{
    gbt_can_status_t s;
    setup();
    CHECK_EQ_U(status().state, GBT_CAN_STATE_ERROR_ACTIVE);
    fake_f1_set_esr(0U, 0U, 1U, 96U, 97U);
    s = status();
    CHECK_EQ_U(s.state, GBT_CAN_STATE_ERROR_WARNING);
    CHECK_EQ_U(s.tec, 96U);
    CHECK_EQ_U(s.rec, 97U);
    CHECK_EQ_U(s.tec_rec_valid, 1U);
    fake_f1_set_esr(0U, 1U, 1U, 130U, 128U);
    CHECK_EQ_U(status().state, GBT_CAN_STATE_ERROR_PASSIVE);
    fake_f1_set_esr(1U, 1U, 1U, 255U, 0U);
    CHECK_EQ_U(status().state, GBT_CAN_STATE_BUS_OFF);   /* observable; no recovery (RQ-01) */
    fake_f1_set_esr(0U, 0U, 0U, 0U, 0U);
    CHECK_EQ_U(status().state, GBT_CAN_STATE_ERROR_ACTIVE);
    {
        CAN_HandleTypeDef other;
        memset(&other, 0, sizeof other);
        HAL_CAN_ErrorCallback(&other);
        HAL_CAN_RxFifo0MsgPendingCallback(&other);
        CHECK_EQ_U(diag().hal_error_callbacks, 0U);
    }
}

/* ---- time, events, lifecycle, sink ---- */
static gbt_output_cmd_t g_applied;
static gbt_hal_status_t g_reentry;

static gbt_hal_status_t sink_apply(void *ctx, const gbt_output_cmd_t *cmd)
{
    gbt_can_frame_t f = txf(ID_BCL, 1U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
    (void)ctx;
    g_applied = *cmd;
    g_reentry = gbt_hal_can_send(&P, &f);
    return GBT_HAL_OK;
}

static gbt_hal_status_t sink_feedback(void *ctx, gbt_output_fb_t *fb)
{
    (void)ctx;
    memset(fb, 0, sizeof *fb);
    fb->valid = 1U;
    return GBT_HAL_OK;
}

static void test_misc(void)
{
    gbt_output_sink_t sink;
    gbt_output_cmd_t cmd;
    gbt_output_fb_t fb;
    gbt_can_frame_t f;
    setup();
    g_fake_f1.tick = 0xFFFFFFFFu;
    CHECK_EQ_U(gbt_hal_now_ms(&P), 0xFFFFFFFFu);
    g_fake_f1.tick = 4U;
    CHECK_EQ_U((uint32_t)(gbt_hal_now_ms(&P) - 0xFFFFFFFFu), 5U);
    gbt_hal_event(&P, 0x77U, 1, -2);
    CHECK_EQ_U(P.event_count, 1U);
    CHECK_EQ_U(P.last_event_code, 0x77U);
    sink.apply = sink_apply;
    sink.feedback = sink_feedback;
    sink.ctx = NULL;
    cmd.seq = 3U;
    cmd.enabled = 0U;
    cmd.voltage_mv = 0U;
    cmd.current_ma = 0U;
    CHECK(gbt_hal_claim(&P) == GBT_HAL_E_NOT_READY);
    CHECK(gbt_hal_bind_output_sink(&P, &sink) == GBT_HAL_OK);
    CHECK(gbt_hal_claim(&P) == GBT_HAL_OK);
    CHECK(gbt_hal_output_apply(&P, &cmd) == GBT_HAL_OK);
    CHECK_EQ_U(g_applied.seq, 3U);
    CHECK(g_reentry == GBT_HAL_E_STATE);
    CHECK(gbt_hal_output_feedback(&P, &fb) == GBT_HAL_OK);
    CHECK(gbt_port_f103_deinit(&P) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_release(&P) == GBT_HAL_OK);
    CHECK(gbt_hal_unbind_output_sink(&P) == GBT_HAL_OK);
    CHECK(gbt_port_f103_deinit(&P) == GBT_HAL_OK);
    CHECK_EQ_U(gbt_hal_now_ms(&P), 0U);
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_NOT_READY);
    f = txf(ID_BCL, 1U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_NOT_READY);
}

/* ---- STEP 07-C2.4: polled error-state entry events (ESR, main context) ---- */
static void test_state_events(void)
{
    gbt_port_f103_diag_t d;
    setup();
    (void)status();
    d = diag();
    CHECK_EQ_U(d.warning_events, 0U);
    CHECK_EQ_U(d.error_passive_events, 0U);
    CHECK_EQ_U(d.bus_off_events, 0U);
    CHECK_EQ_U(d.last_polled_state, GBT_CAN_STATE_ERROR_ACTIVE);
    fake_f1_set_esr(0U, 0U, 1U, 100U, 0U);                 /* warning */
    (void)status();
    (void)status();                                        /* staying there is not a new event */
    d = diag();
    CHECK_EQ_U(d.warning_events, 1U);
    CHECK_EQ_U(d.error_passive_events, 0U);
    fake_f1_set_esr(0U, 1U, 1U, 130U, 0U);                 /* passive */
    (void)status();
    d = diag();
    CHECK_EQ_U(d.warning_events, 1U);
    CHECK_EQ_U(d.error_passive_events, 1U);
    CHECK_EQ_U(d.bus_off_events, 0U);
    fake_f1_set_esr(0U, 0U, 0U, 10U, 0U);                  /* recovered */
    (void)status();
    fake_f1_set_esr(1U, 1U, 1U, 255U, 0U);                 /* straight to bus-off */
    (void)status();
    d = diag();
    CHECK_EQ_U(d.warning_events, 2U);
    CHECK_EQ_U(d.error_passive_events, 2U);
    CHECK_EQ_U(d.bus_off_events, 1U);
    CHECK_EQ_U(d.last_polled_state, GBT_CAN_STATE_BUS_OFF);
    (void)status();
    CHECK_EQ_U(diag().bus_off_events, 1U);
}

int main(void)
{
    test_init();
    test_filter_acceptance();
    test_rx_dlc_and_data();
    test_rx_overrun();
    test_tx_single_mailbox();
    test_fake_arbitration_reorders();
    test_tx_failure_busy_busoff();
    test_status();
    test_misc();
    test_state_events();                 /* STEP 07-C2.4 */
    return TEST_SUMMARY("test_port_f103");
}
