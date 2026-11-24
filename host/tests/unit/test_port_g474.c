/*
 * tests/unit/test_port_g474.c - host tests of ports/stm32/g474/port_g474.c
 * against the FDCAN fake (tests/support/fake_stm32g4). The "ISR" is
 * fake_g4_irq(), which calls the port's HAL_FDCAN_*Callback overrides the way
 * HAL_FDCAN_IRQHandler does. Also built with AddressSanitizer
 * (test_port_g474_asan) to detect overruns of the RX scratch buffer.
 */
#include "ports/stm32/g474/port_g474.h"
#include "fake_stm32g4.h"
#include "protocol/gbt_id.h"
#include "tests/support/test_assert.h"

#include <string.h>

static FDCAN_HandleTypeDef H;
static gbt_hal_t P;
static uint8_t D64[64];

#define ID_BCL   0x181056F4u   /* vehicle -> charger */
#define ID_CHM   0x1826F456u   /* charger -> vehicle */

static void setup(void)
{
    gbt_port_g474_cfg_t cfg;
    if (P.lifecycle != GBT_HAL_LC_UNINIT) {
        H.State = HAL_FDCAN_STATE_BUSY;                    /* release the instance slot */
        P.lifecycle = (uint8_t)GBT_HAL_LC_READY;
        P.in_sink = 0U;
        (void)gbt_port_g474_deinit(&P);
    }
    memset(&P, 0, sizeof P);
    fake_g4_reset(&H);
    cfg.hfdcan = &H;
    cfg.own_sa = 0x56U;
    cfg.peer_sa = 0xF4U;
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_OK);
}

static void fill64(uint8_t seed)
{
    unsigned i;
    for (i = 0U; i < 64U; i++) {
        D64[i] = (uint8_t)(seed + i * 3U + 1U);
    }
}

/* one classic extended data frame on the bus + its interrupt */
static int bus(uint32_t id, uint32_t dlc_code)
{
    int r = fake_g4_bus_rx(id, 1U, 0U, 0U, 0U, dlc_code, D64);
    fake_g4_irq();
    return r;
}

static gbt_can_status_t status(void)
{
    gbt_can_status_t s;
    memset(&s, 0xA5, sizeof s);
    CHECK(gbt_hal_can_get_status(&P, &s) == GBT_HAL_OK);
    return s;
}

static gbt_port_g474_diag_t diag(void)
{
    gbt_port_g474_diag_t d;
    CHECK(gbt_port_g474_get_diag(&P, &d) == GBT_HAL_OK);
    return d;
}

/* ---- M2: initialisation and filters ---- */
static void test_init(void)
{
    gbt_port_g474_cfg_t cfg;
    gbt_hal_t other;

    memset(&P, 0, sizeof P);
    fake_g4_reset(&H);
    cfg.hfdcan = &H;
    cfg.own_sa = 0x56U;
    cfg.peer_sa = 0xF4U;
    CHECK(gbt_port_g474_init(NULL, &cfg) == GBT_HAL_E_PARAM);
    CHECK(gbt_port_g474_init(&P, NULL) == GBT_HAL_E_PARAM);
    cfg.hfdcan = NULL;
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_E_PARAM);
    cfg.hfdcan = &H;
    cfg.peer_sa = 0x56U;
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_E_PARAM);
    cfg.peer_sa = 0xF4U;
    CHECK_EQ_U(g_fake_g4.order_len, 0U);                  /* nothing touched on bad args */

    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_OK);
    CHECK(strcmp(g_fake_g4.order, "FGNS") == 0);          /* filter, global, notify, start */
    CHECK_EQ_U(g_fake_g4.filter.IdType, FDCAN_EXTENDED_ID);
    CHECK_EQ_U(g_fake_g4.filter.FilterIndex, 0U);
    CHECK_EQ_U(g_fake_g4.filter.FilterType, FDCAN_FILTER_MASK);
    CHECK_EQ_U(g_fake_g4.filter.FilterConfig, FDCAN_FILTER_TO_RXFIFO0);
    CHECK_EQ_U(g_fake_g4.filter.FilterID1, 0x000056F4u);
    CHECK_EQ_U(g_fake_g4.filter.FilterID2, 0x0000FFFFu);
    CHECK_EQ_U(g_fake_g4.anfs, FDCAN_REJECT);
    CHECK_EQ_U(g_fake_g4.anfe, FDCAN_REJECT);
    CHECK_EQ_U(g_fake_g4.rrfs, FDCAN_REJECT_REMOTE);
    CHECK_EQ_U(g_fake_g4.rrfe, FDCAN_REJECT_REMOTE);
    CHECK_EQ_U(g_fake_g4.ie, FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO0_MESSAGE_LOST | FDCAN_IT_BUS_OFF
                                 | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ERROR_WARNING);
    CHECK_EQ_U(H.State, HAL_FDCAN_STATE_BUSY);
    CHECK_EQ_U(P.lifecycle, GBT_HAL_LC_READY);
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_E_STATE);         /* already READY */
    memset(&other, 0, sizeof other);
    CHECK(gbt_port_g474_init(&other, &cfg) == GBT_HAL_E_STATE);     /* one instance only */
    CHECK(gbt_port_g474_deinit(&P) == GBT_HAL_OK);
    CHECK_EQ_U(H.State, HAL_FDCAN_STATE_READY);
    CHECK_EQ_U(g_fake_g4.ie, 0U);
    CHECK(gbt_port_g474_deinit(&P) == GBT_HAL_E_STATE);

    /* failures: E_IO, instance released, not started */
    fake_g4_reset(&H);
    g_fake_g4.fail_filter = HAL_ERROR;
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_E_IO);
    CHECK_EQ_U(P.lifecycle, GBT_HAL_LC_UNINIT);
    CHECK_EQ_U(H.State, HAL_FDCAN_STATE_READY);
    fake_g4_reset(&H);
    g_fake_g4.fail_start = HAL_ERROR;
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_E_IO);
    CHECK_EQ_U(g_fake_g4.ie, 0U);                         /* notifications withdrawn */
    fake_g4_reset(&H);
    g_fake_g4.fail_notify = HAL_ERROR;
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_E_IO);
    /* FDCAN already started by user code: the READY-only global filter call fails */
    fake_g4_reset(&H);
    H.State = HAL_FDCAN_STATE_BUSY;
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_E_IO);
    CHECK_EQ_U(P.lifecycle, GBT_HAL_LC_UNINIT);
    /* after all failures a normal init still works (instance slot was released) */
    fake_g4_reset(&H);
    CHECK(gbt_port_g474_init(&P, &cfg) == GBT_HAL_OK);
    CHECK(gbt_port_g474_deinit(&P) == GBT_HAL_OK);
}

/* every vehicle -> charger identifier passes; everything else is rejected */
static void test_filter_acceptance(void)
{
    unsigned m;
    uint32_t id;
    setup();
    fill64(1U);
    for (m = 0U; m < (unsigned)GBT_MSG_COUNT; m++) {
        const gbt_msg_def_t *d = gbt_msg_def((gbt_msg_t)m);
        CHECK(d != NULL);
        CHECK(gbt_id_for_msg((gbt_msg_t)m, &id) == GBT_ID_OK);
        if (d->dir == GBT_DIR_V2C) {
            CHECK_EQ_U(bus(id, FDCAN_DLC_BYTES_8), 1U);
        } else {
            CHECK_EQ_U(bus(id, FDCAN_DLC_BYTES_8), 0U);  /* C2V (own frames) rejected */
        }
    }
    CHECK(gbt_id_make(7U, 0xECU, 0x56U, 0xF4U, &id) == GBT_ID_OK);   /* TP.CM V->C */
    CHECK_EQ_U(id, 0x1CEC56F4u);
    CHECK_EQ_U(bus(id, FDCAN_DLC_BYTES_8), 1U);
    CHECK(gbt_id_make(7U, 0xEBU, 0x56U, 0xF4U, &id) == GBT_ID_OK);   /* TP.DT V->C */
    CHECK_EQ_U(bus(id, FDCAN_DLC_BYTES_8), 1U);
    CHECK_EQ_U(bus(0x1CECF456u, FDCAN_DLC_BYTES_8), 0U);  /* TP.CM C->V */
    CHECK_EQ_U(bus(0x181056F5u, FDCAN_DLC_BYTES_8), 0U);  /* other source address */
    CHECK_EQ_U(bus(0x181057F4u, FDCAN_DLC_BYTES_8), 0U);  /* other destination */
    CHECK_EQ_U(fake_g4_bus_rx(0x6F4u, 0U, 0U, 0U, 0U, FDCAN_DLC_BYTES_8, D64), 0); /* 11-bit */
    CHECK_EQ_U(fake_g4_bus_rx(ID_BCL, 1U, 1U, 0U, 0U, FDCAN_DLC_BYTES_0, D64), 0); /* remote */
    CHECK_EQ_U(status().rx_invalid, 0U);
}

/* ---- M3: RX path ---- */
static void test_rx_valid_dlc(void)
{
    uint8_t dlc;
    gbt_can_frame_t f;
    setup();
    for (dlc = 0U; dlc <= 8U; dlc++) {
        unsigned b;
        fill64((uint8_t)(0x40U + dlc));
        CHECK_EQ_U(bus(0x1FFF56F4u, (uint32_t)dlc), 1U);              /* max 29-bit id with 0x56F4 */
        CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
        CHECK_EQ_U(f.id, 0x1FFF56F4u);
        CHECK_EQ_U(f.flags, GBT_CAN_FLAG_EXT);
        CHECK_EQ_U(f.dlc, dlc);
        for (b = 0U; b < 8U; b++) {
            CHECK_EQ_U(f.data[b], (b < dlc) ? D64[b] : 0U);           /* stale scratch never leaks */
        }
    }
    fill64(0x90U);
    CHECK_EQ_U(bus(ID_BCL, FDCAN_DLC_BYTES_8), 1U);
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
    CHECK(memcmp(f.data, D64, 8U) == 0);                              /* all eight bytes */
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_EMPTY);
    CHECK_EQ_U(diag().rx_frames, 10U);
    CHECK_EQ_U(status().rx_invalid, 0U);
}

static void test_rx_invalid(void)
{
    gbt_can_frame_t f;
    gbt_port_g474_diag_t d;
    uint32_t code;
    setup();
    fill64(7U);
    /* CAN FD frame, 64 bytes: HAL copies 64 bytes into the scratch -> rejected */
    CHECK_EQ_U(fake_g4_bus_rx(ID_BCL, 1U, 0U, 1U, 1U, 15U, D64), 1);
    fake_g4_irq();
    /* FD flag clear but BRS set */
    CHECK_EQ_U(fake_g4_bus_rx(ID_BCL, 1U, 0U, 0U, 1U, FDCAN_DLC_BYTES_8, D64), 1);
    fake_g4_irq();
    /* classic frames with DLC codes 9..15 (legal on CAN 2.0; HAL copies 12..64 bytes) */
    for (code = 9U; code <= 15U; code++) {
        CHECK_EQ_U(bus(ID_BCL, code), 1U);
    }
    /* frames that only reach FIFO 0 if the global filter were open (white-box) */
    g_fake_g4.rrfe = 0U;
    CHECK_EQ_U(fake_g4_bus_rx(ID_BCL, 1U, 1U, 0U, 0U, FDCAN_DLC_BYTES_2, D64), 1);   /* remote */
    fake_g4_irq();
    g_fake_g4.anfs = FDCAN_ACCEPT_IN_RX_FIFO0;
    CHECK_EQ_U(fake_g4_bus_rx(0x123u, 0U, 0U, 0U, 0U, FDCAN_DLC_BYTES_8, D64), 1);   /* 11-bit */
    fake_g4_irq();
    d = diag();
    CHECK_EQ_U(d.rx_fd, 2U);
    CHECK_EQ_U(d.rx_bad_dlc, 7U);
    CHECK_EQ_U(d.rx_remote, 1U);
    CHECK_EQ_U(d.rx_std_id, 1U);
    CHECK_EQ_U(d.rx_frames, 0U);
    CHECK_EQ_U(status().rx_invalid, 11U);                  /* counted, never silent */
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_EMPTY); /* nothing queued */
    /* GetRxMessage failure (controller not BUSY while elements remain) */
    g_fake_g4.anfs = FDCAN_REJECT;
    CHECK_EQ_U(fake_g4_bus_rx(ID_BCL, 1U, 0U, 0U, 0U, FDCAN_DLC_BYTES_8, D64), 1);
    H.State = HAL_FDCAN_STATE_READY;
    fake_g4_irq();
    H.State = HAL_FDCAN_STATE_BUSY;
    CHECK_EQ_U(diag().rx_hal_error, 1U);
    CHECK_EQ_U(status().rx_invalid, 12U);
}

static void test_rx_drain_and_loss(void)
{
    gbt_can_frame_t f;
    unsigned i;
    setup();
    fill64(3U);
    /* three frames before the ISR runs: one callback drains all three */
    for (i = 0U; i < 3U; i++) {
        D64[0] = (uint8_t)i;
        CHECK_EQ_U(fake_g4_bus_rx(ID_BCL, 1U, 0U, 0U, 0U, FDCAN_DLC_BYTES_8, D64), 1);
    }
    D64[0] = 3U;
    CHECK_EQ_U(fake_g4_bus_rx(ID_BCL, 1U, 0U, 0U, 0U, FDCAN_DLC_BYTES_8, D64), 2);   /* 4th: lost */
    fake_g4_irq();
    CHECK_EQ_U(g_fake_g4.rx_count, 0U);
    for (i = 0U; i < 3U; i++) {
        CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
        CHECK_EQ_U(f.data[0], i);                          /* FIFO order */
    }
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_EMPTY);
    CHECK_EQ_U(status().rx_overflow_hw, 1U);               /* one RF0L event */
    CHECK_EQ_U(diag().rx_fifo_loss_events, 1U);
    CHECK_EQ_U(status().rx_overflow_sw, 0U);
}

static void test_rx_queue_overflow(void)
{
    gbt_can_frame_t f;
    unsigned i;
    setup();
    fill64(9U);
    for (i = 0U; i < 20U; i++) {                           /* main loop does not consume */
        D64[1] = (uint8_t)i;
        CHECK_EQ_U(bus(ID_BCL, FDCAN_DLC_BYTES_8), 1U);
    }
    CHECK_EQ_U(status().rx_overflow_sw, 4U);               /* 16 queued, 4 dropped */
    CHECK_EQ_U(status().rx_overflow_hw, 0U);
    CHECK_EQ_U(diag().rx_frames, 16U);
    for (i = 0U; i < 16U; i++) {
        CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_OK);
        CHECK_EQ_U(f.data[1], i);                          /* oldest kept */
    }
    CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_EMPTY);
}

/* ---- M4: TX path ---- */
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

static void test_tx(void)
{
    gbt_can_frame_t f;
    fake_g4_tx_elem_t e;
    uint8_t dlc;
    setup();
    for (dlc = 0U; dlc <= 8U; dlc++) {
        unsigned b;
        f = txf(ID_CHM, dlc, (uint8_t)GBT_CAN_FLAG_EXT, (uint8_t)(0x10U * dlc));
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
        fake_g4_tx_complete(1U);
        CHECK(fake_g4_tx_pop(&e) == 1);
        CHECK_EQ_U(e.hdr.Identifier, ID_CHM);
        CHECK_EQ_U(e.hdr.IdType, FDCAN_EXTENDED_ID);
        CHECK_EQ_U(e.hdr.TxFrameType, FDCAN_DATA_FRAME);
        CHECK_EQ_U(e.hdr.DataLength, (uint32_t)dlc);       /* FDCAN_DLC_BYTES_n */
        CHECK_EQ_U(e.hdr.FDFormat, FDCAN_CLASSIC_CAN);
        CHECK_EQ_U(e.hdr.BitRateSwitch, FDCAN_BRS_OFF);
        CHECK_EQ_U(e.hdr.ErrorStateIndicator, FDCAN_ESI_ACTIVE);
        CHECK_EQ_U(e.hdr.TxEventFifoControl, FDCAN_NO_TX_EVENTS);
        for (b = 0U; b < dlc; b++) {
            CHECK_EQ_U(e.data[b], f.data[b]);
        }
    }
    f = txf(0x1FFFFFFFu, 8U, (uint8_t)GBT_CAN_FLAG_EXT, 0x55U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    fake_g4_tx_complete(1U);
    CHECK(fake_g4_tx_pop(&e) == 1);
    CHECK_EQ_U(e.hdr.Identifier, 0x1FFFFFFFu);
    f = txf(0x7FFu, 2U, (uint8_t)GBT_CAN_FLAG_STD, 1U);   /* 11-bit allowed by the HAL contract */
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    fake_g4_tx_complete(1U);
    CHECK(fake_g4_tx_pop(&e) == 1);
    CHECK_EQ_U(e.hdr.IdType, FDCAN_STANDARD_ID);
    /* invalid frames: no HAL call */
    {
        uint32_t calls = g_fake_g4.calls_add;
        f = txf(ID_CHM, 9U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_PARAM);
        f = txf(0x20000000u, 8U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_PARAM);
        CHECK(gbt_hal_can_send(&P, NULL) == GBT_HAL_E_PARAM);
        CHECK_EQ_U(g_fake_g4.calls_add, calls);
    }
    CHECK_EQ_U(diag().tx_frames, 11U);
}

static void test_tx_full_busoff_error(void)
{
    gbt_can_frame_t f = txf(ID_CHM, 3U, (uint8_t)GBT_CAN_FLAG_EXT, 1U);
    gbt_port_g474_diag_t d;
    uint32_t calls;
    setup();
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    calls = g_fake_g4.calls_add;
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_FULL);     /* 3-element TX FIFO full */
    CHECK_EQ_U(g_fake_g4.calls_add, calls);                /* AddMessage not called */
    CHECK_EQ_U(H.ErrorCode, HAL_FDCAN_ERROR_NONE);         /* shared ErrorCode untouched */
    fake_g4_tx_complete(1U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);         /* retry succeeds */
    fake_g4_tx_complete(3U);
    g_fake_g4.fail_add = HAL_ERROR;
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_IO);
    g_fake_g4.fail_add = HAL_OK;
    fake_g4_set_bus_state(1U, 1U, 1U, 255U, 0U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_BUS_OFF);
    fake_g4_set_bus_state(0U, 0U, 0U, 0U, 0U);
    g_fake_g4.fail_psr = HAL_ERROR;
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_IO);
    g_fake_g4.fail_psr = HAL_OK;
    d = diag();
    CHECK_EQ_U(d.tx_fifo_full, 1U);
    CHECK_EQ_U(d.tx_bus_off, 1U);
    CHECK_EQ_U(d.tx_hal_error, 2U);
    CHECK_EQ_U(d.tx_frames, 4U);
    CHECK_EQ_U(d.last_tx_hal_status, HAL_ERROR);           /* actual HAL status recorded */
    CHECK_EQ_U(status().tx_error, 3U);                     /* bus-off + 2 HAL errors */
}

/* ---- M5: status mapping and error notifications ---- */
static void test_status(void)
{
    gbt_can_status_t s;
    gbt_port_g474_diag_t d;
    setup();
    s = status();
    CHECK_EQ_U(s.state, GBT_CAN_STATE_ERROR_ACTIVE);
    CHECK_EQ_U(s.tec_rec_valid, 1U);
    fake_g4_set_bus_state(0U, 0U, 1U, 97U, 12U);
    fake_g4_irq();
    s = status();
    CHECK_EQ_U(s.state, GBT_CAN_STATE_ERROR_WARNING);
    CHECK_EQ_U(s.tec, 97U);
    CHECK_EQ_U(s.rec, 12U);
    fake_g4_set_bus_state(0U, 1U, 1U, 130U, 127U);
    fake_g4_irq();
    CHECK_EQ_U(status().state, GBT_CAN_STATE_ERROR_PASSIVE);
    CHECK_EQ_U(status().rec, 127U);
    fake_g4_set_bus_state(1U, 1U, 1U, 255U, 0U);
    fake_g4_irq();
    s = status();
    CHECK_EQ_U(s.state, GBT_CAN_STATE_BUS_OFF);            /* bus-off observable */
    CHECK_EQ_U(s.tec, 255U);
    fake_g4_set_bus_state(0U, 0U, 0U, 0U, 0U);             /* recovered (by hardware/user) */
    fake_g4_irq();
    CHECK_EQ_U(status().state, GBT_CAN_STATE_ERROR_ACTIVE);
    d = diag();
    CHECK_EQ_U(d.warning_events, 2U);
    CHECK_EQ_U(d.error_passive_events, 2U);
    CHECK_EQ_U(d.bus_off_events, 2U);
    g_fake_g4.fail_psr = HAL_ERROR;
    g_fake_g4.fail_ecr = HAL_ERROR;
    s = status();
    CHECK_EQ_U(s.state, GBT_CAN_STATE_UNKNOWN);
    CHECK_EQ_U(s.tec_rec_valid, 0U);
    g_fake_g4.fail_psr = HAL_OK;
    g_fake_g4.fail_ecr = HAL_OK;
    /* HAL error callback: counted, ErrorCode neither used nor cleared */
    H.ErrorCode = HAL_FDCAN_ERROR_FIFO_FULL;
    fake_g4_irq();
    CHECK_EQ_U(diag().hal_error_callbacks, 1U);
    CHECK_EQ_U(H.ErrorCode, HAL_FDCAN_ERROR_FIFO_FULL);
    /* callbacks for another handle are ignored */
    {
        FDCAN_HandleTypeDef other;
        memset(&other, 0, sizeof other);
        HAL_FDCAN_ErrorStatusCallback(&other, FDCAN_IT_BUS_OFF);
        HAL_FDCAN_RxFifo0Callback(&other, FDCAN_IT_RX_FIFO0_MESSAGE_LOST);
        CHECK_EQ_U(diag().bus_off_events, 2U);
        CHECK_EQ_U(status().rx_overflow_hw, 0U);
    }
}

/* ---- M6, M7 ---- */
static void test_time_and_event(void)
{
    setup();
    g_fake_g4.tick = 12345U;
    CHECK_EQ_U(gbt_hal_now_ms(&P), 12345U);
    g_fake_g4.tick = 0xFFFFFFFFu;
    CHECK_EQ_U(gbt_hal_now_ms(&P), 0xFFFFFFFFu);
    g_fake_g4.tick = 2U;
    CHECK_EQ_U((uint32_t)(gbt_hal_now_ms(&P) - 0xFFFFFFFFu), 3U);   /* wrap-safe difference */
    gbt_hal_event(&P, 0x1234U, -5, 7);
    gbt_hal_event(&P, 0x0042U, 1, 2);
    CHECK_EQ_U(P.event_count, 2U);
    CHECK_EQ_U(P.last_event_code, 0x0042U);
    CHECK_EQ_U((uint32_t)P.last_event_b, 2U);
    CHECK(gbt_port_g474_deinit(&P) == GBT_HAL_OK);
    CHECK_EQ_U(gbt_hal_now_ms(&P), 0U);                    /* uninitialised */
    gbt_hal_event(&P, 1U, 0, 0);
    CHECK_EQ_U(P.event_count, 2U);
    {
        gbt_can_frame_t f;
        gbt_can_status_t s;
        f = txf(ID_CHM, 1U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_E_NOT_READY);
        CHECK(gbt_hal_can_receive(&P, &f) == GBT_HAL_E_NOT_READY);
        CHECK(gbt_hal_can_get_status(&P, &s) == GBT_HAL_E_NOT_READY);
    }
}

/* ---- ND-1 lifecycle and output sink ---- */
static gbt_output_cmd_t g_applied;
static gbt_hal_status_t g_reentry;

static gbt_hal_status_t sink_apply(void *ctx, const gbt_output_cmd_t *cmd)
{
    gbt_can_frame_t f = txf(ID_CHM, 1U, (uint8_t)GBT_CAN_FLAG_EXT, 0U);
    (void)ctx;
    g_applied = *cmd;
    g_reentry = gbt_hal_can_send(&P, &f);                  /* forbidden re-entry */
    return GBT_HAL_OK;
}

static gbt_hal_status_t sink_feedback(void *ctx, gbt_output_fb_t *fb)
{
    (void)ctx;
    memset(fb, 0, sizeof *fb);
    fb->valid = 1U;
    return GBT_HAL_OK;
}

static void test_lifecycle_sink(void)
{
    gbt_output_sink_t sink;
    gbt_output_cmd_t cmd;
    gbt_output_fb_t fb;
    setup();
    sink.apply = sink_apply;
    sink.feedback = sink_feedback;
    sink.ctx = NULL;
    cmd.seq = 7U;
    cmd.enabled = 0U;
    cmd.voltage_mv = 0U;
    cmd.current_ma = 0U;
    CHECK(gbt_hal_claim(&P) == GBT_HAL_E_NOT_READY);
    CHECK(gbt_hal_output_apply(&P, &cmd) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_bind_output_sink(&P, &sink) == GBT_HAL_OK);
    CHECK(gbt_hal_claim(&P) == GBT_HAL_OK);
    CHECK(gbt_port_g474_deinit(&P) == GBT_HAL_E_STATE);
    CHECK(gbt_hal_output_apply(&P, &cmd) == GBT_HAL_OK);
    CHECK_EQ_U(g_applied.seq, 7U);
    CHECK_EQ_U(P.accepted_seq, 7U);
    CHECK(g_reentry == GBT_HAL_E_STATE);
    CHECK(gbt_hal_output_feedback(&P, &fb) == GBT_HAL_OK);
    CHECK_EQ_U(fb.valid, 1U);
    CHECK(gbt_hal_release(&P) == GBT_HAL_OK);
    CHECK(gbt_hal_unbind_output_sink(&P) == GBT_HAL_OK);
    CHECK(gbt_port_g474_deinit(&P) == GBT_HAL_OK);
}

/* ---- STEP 07-C2.4: TX confirmation from TXBTO (driver acceptance != bus completion) ---- */
static void test_tx_confirmation(void)
{
    gbt_can_frame_t f = txf(ID_CHM, 8U, (uint8_t)GBT_CAN_FLAG_EXT, 3U);
    gbt_port_g474_diag_t d;
    unsigned i;
    setup();
    d = diag();
    CHECK_EQ_U(d.tx_confirmed, 0U);
    CHECK_EQ_U(d.tx_pending, 0U);
    /* accepted into the TX FIFO, nothing transmitted yet (e.g. no ACK on the bus) */
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    (void)status();
    d = diag();
    CHECK_EQ_U(d.tx_frames, 2U);
    CHECK_EQ_U(d.tx_confirmed, 0U);                        /* acceptance is NOT confirmation */
    CHECK_EQ_U(d.tx_pending, 2U);
    CHECK_EQ_U(H.LatestTxFifoQRequest, 2U);                /* buffer 1 */
    fake_g4_tx_complete(1U);                               /* buffer 0 transmitted (ACKed) */
    (void)status();
    d = diag();
    CHECK_EQ_U(d.tx_confirmed, 1U);
    CHECK_EQ_U(d.tx_pending, 1U);
    fake_g4_tx_complete(1U);
    (void)status();
    d = diag();
    CHECK_EQ_U(d.tx_confirmed, 2U);
    CHECK_EQ_U(d.tx_pending, 0U);
    /* buffers are reused cyclically: completed buffers are counted before reuse */
    for (i = 0U; i < 10U; i++) {
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
        fake_g4_tx_complete(1U);
    }
    (void)status();
    d = diag();
    CHECK_EQ_U(d.tx_frames, 12U);
    CHECK_EQ_U(d.tx_confirmed, 12U);
    CHECK_EQ_U(d.tx_pending, 0U);
    CHECK_EQ_U(d.tx_unconfirmed_reuse, 0U);
    /* send-time poll alone (no status call) also counts before reuse */
    for (i = 0U; i < 6U; i++) {
        CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
        fake_g4_tx_complete(1U);
    }
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    d = diag();
    CHECK_EQ_U(d.tx_confirmed, 18U);
    CHECK_EQ_U(d.tx_pending, 1U);
    CHECK_EQ_U(d.tx_unconfirmed_reuse, 0U);
    fake_g4_tx_complete(1U);
    (void)status();
    /* a request that ends WITHOUT transmission is never confirmed; reuse of its buffer is counted */
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    fake_g4_tx_cancel(1U);
    (void)status();
    d = diag();
    CHECK_EQ_U(d.tx_confirmed, 19U);
    CHECK_EQ_U(d.tx_pending, 1U);                          /* still unconfirmed */
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    fake_g4_tx_complete(1U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);
    fake_g4_tx_complete(1U);
    CHECK(gbt_hal_can_send(&P, &f) == GBT_HAL_OK);         /* buffer of the cancelled request */
    fake_g4_tx_complete(1U);
    (void)status();
    d = diag();
    CHECK_EQ_U(d.tx_unconfirmed_reuse, 1U);
    CHECK_EQ_U(d.tx_confirmed, 22U);
    CHECK_EQ_U(d.tx_pending, 0U);
    CHECK_EQ_U(d.tx_frames, 23U);                          /* 22 confirmed + 1 never transmitted */
    CHECK_EQ_U(H.ErrorCode, HAL_FDCAN_ERROR_NONE);
}

int main(void)
{
    test_init();
    test_filter_acceptance();
    test_rx_valid_dlc();
    test_rx_invalid();
    test_rx_drain_and_loss();
    test_rx_queue_overflow();
    test_tx();
    test_tx_full_busoff_error();
    test_status();
    test_time_and_event();
    test_lifecycle_sink();
    test_tx_confirmation();              /* STEP 07-C2.4 */
    return TEST_SUMMARY("test_port_g474");
}
