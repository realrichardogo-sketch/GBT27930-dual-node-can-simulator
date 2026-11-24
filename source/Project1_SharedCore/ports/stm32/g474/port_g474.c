/*
 * ports/stm32/g474/port_g474.c - see port_g474.h.
 *
 * STM32G4 HAL behaviour relied upon (HAL driver 1.2.4, verified in the
 * reference project, docs/STEP07C1_2A2_G474_RX_AUDIT.md):
 *  - HAL_FDCAN_IRQHandler clears the RX FIFO 0 flags BEFORE calling
 *    HAL_FDCAN_RxFifo0Callback(h, RxFifo0ITs): the callback drains the FIFO.
 *  - HAL_FDCAN_GetRxMessage copies DLCtoBytes[DLC] (<= 64) bytes and
 *    acknowledges the element; RxHeader.DataLength is the DLC code.
 *  - HAL_FDCAN_AddMessageToTxFifoQ returns HAL_ERROR on a full FIFO and then
 *    ORs HAL_FDCAN_ERROR_FIFO_FULL into hfdcan->ErrorCode, a field the ISR
 *    also read-modify-writes. This port never reads ErrorCode and checks the
 *    free level first so the normal path does not touch it.
 *  - Error frames are handled by the controller and never reach RX FIFO 0.
 *  - HAL_FDCAN_AddMessageToTxFifoQ writes the requested buffer as (1 << PutIndex) to TXBAR and
 *    to hfdcan->LatestTxFifoQRequest (HAL_FDCAN_GetLatestTxFifoQRequestBuffer). RM0440: a TXBTO
 *    bit is set when the transmission from that buffer occurred and is reset by the next TXBAR
 *    request of the same buffer. STEP 07-C2.4 reads TXBTO (side-effect free) in main context only;
 *    the TX interrupts stay disabled.
 */
#include "ports/stm32/g474/port_g474.h"

#include <stddef.h>
#include <string.h>

#define G474_NOTIFICATIONS (FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO0_MESSAGE_LOST | \
                            FDCAN_IT_BUS_OFF | FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ERROR_WARNING)

/* The single active instance; written in main context only while the
 * notifications are disabled, read by the callbacks. */
static gbt_hal_t *volatile s_g474 = NULL;

static void sat_inc(volatile uint32_t *c)
{
    uint32_t v = *c;
    if (v != 0xFFFFFFFFu) {
        *c = v + 1U;                       /* single writer per counter */
    }
}

/* DLC code <-> byte count, through the HAL constants (no arithmetic on codes). */
static uint8_t dlc_from_code(uint32_t code)
{
    switch (code) {
    case FDCAN_DLC_BYTES_0: return 0U;
    case FDCAN_DLC_BYTES_1: return 1U;
    case FDCAN_DLC_BYTES_2: return 2U;
    case FDCAN_DLC_BYTES_3: return 3U;
    case FDCAN_DLC_BYTES_4: return 4U;
    case FDCAN_DLC_BYTES_5: return 5U;
    case FDCAN_DLC_BYTES_6: return 6U;
    case FDCAN_DLC_BYTES_7: return 7U;
    case FDCAN_DLC_BYTES_8: return 8U;
    default:                return 0xFFU;   /* 9..15: not Classical 0..8 */
    }
}

static uint32_t dlc_to_code(uint8_t dlc)
{
    static const uint32_t codes[GBT_CAN_MAX_DLC + 1U] = {
        FDCAN_DLC_BYTES_0, FDCAN_DLC_BYTES_1, FDCAN_DLC_BYTES_2, FDCAN_DLC_BYTES_3, FDCAN_DLC_BYTES_4,
        FDCAN_DLC_BYTES_5, FDCAN_DLC_BYTES_6, FDCAN_DLC_BYTES_7, FDCAN_DLC_BYTES_8
    };
    return codes[dlc];                     /* dlc <= 8 checked by the caller */
}

/* ---- ISR side (producer) ---- */

static void rx_invalid(gbt_hal_t *h, volatile uint32_t *reason)
{
    sat_inc(reason);
    sat_inc(&h->rx_invalid);
}

/* Read one element; 0 = stop draining (HAL error). */
static uint8_t rx_one(gbt_hal_t *h)
{
    FDCAN_RxHeaderTypeDef hdr;
    gbt_can_frame_t f;
    uint8_t dlc;
    unsigned b;

    memset(&hdr, 0, sizeof hdr);
    if (HAL_FDCAN_GetRxMessage(h->hfdcan, FDCAN_RX_FIFO0, &hdr, h->rx_scratch) != HAL_OK) {
        rx_invalid(h, &h->diag.rx_hal_error);
        return 0U;
    }
    if (hdr.FDFormat != FDCAN_CLASSIC_CAN || hdr.BitRateSwitch != FDCAN_BRS_OFF) {
        rx_invalid(h, &h->diag.rx_fd);
        return 1U;
    }
    if (hdr.RxFrameType != FDCAN_DATA_FRAME) {
        rx_invalid(h, &h->diag.rx_remote);
        return 1U;
    }
    dlc = dlc_from_code(hdr.DataLength);
    if (dlc > GBT_CAN_MAX_DLC) {
        rx_invalid(h, &h->diag.rx_bad_dlc);
        return 1U;
    }
    if (hdr.IdType != FDCAN_EXTENDED_ID) {
        rx_invalid(h, &h->diag.rx_std_id);
        return 1U;
    }
    f.id = hdr.Identifier & GBT_CAN_MAX_EXT_ID;
    f.dlc = dlc;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    for (b = 0U; b < GBT_CAN_MAX_DLC; b++) {
        /* HAL copied only dlc bytes; the rest of the scratch is stale */
        f.data[b] = (b < dlc) ? h->rx_scratch[b] : 0U;
    }
    if (gbt_spsc_push(&h->rxq, &f) == GBT_SPSC_OK) {
        sat_inc(&h->diag.rx_frames);
    }                                      /* E_FULL: counted by the queue (rx_overflow_sw) */
    return 1U;
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    gbt_hal_t *h = s_g474;
    unsigned n;

    if (h == NULL || hfdcan != h->hfdcan) {
        return;
    }
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_MESSAGE_LOST) != 0U) {
        sat_inc(&h->rx_overflow_hw);
        sat_inc(&h->diag.rx_fifo_loss_events);
    }
    for (n = 0U; n < GBT_PORT_G474_RX_DRAIN_MAX; n++) {             /* bounded */
        if (HAL_FDCAN_GetRxFifoFillLevel(hfdcan, FDCAN_RX_FIFO0) == 0U) {
            break;
        }
        if (rx_one(h) == 0U) {
            break;
        }
    }
}

void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    gbt_hal_t *h = s_g474;
    if (h == NULL || hfdcan != h->hfdcan) {
        return;
    }
    /* Status-change events only; the current state is read from PSR in
     * gbt_hal_can_get_status() / gbt_hal_can_send(). */
    if ((ErrorStatusITs & FDCAN_IT_BUS_OFF) != 0U) {
        sat_inc(&h->diag.bus_off_events);
    }
    if ((ErrorStatusITs & FDCAN_IT_ERROR_PASSIVE) != 0U) {
        sat_inc(&h->diag.error_passive_events);
    }
    if ((ErrorStatusITs & FDCAN_IT_ERROR_WARNING) != 0U) {
        sat_inc(&h->diag.warning_events);
    }
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
{
    gbt_hal_t *h = s_g474;
    if (h == NULL || hfdcan != h->hfdcan) {
        return;
    }
    /* Counted only. hfdcan->ErrorCode is shared with thread context and is
     * neither read for decisions nor cleared here. */
    sat_inc(&h->diag.hal_error_callbacks);
}

/* ---- TX confirmation (STEP 07-C2.4, main context) ---- */

static uint32_t popcount3(uint32_t m)
{
    return (m & 1U) + ((m >> 1) & 1U) + ((m >> 2) & 1U);
}

/* Moves every requested buffer whose TXBTO bit is set from tx_wait_mask to tx_confirmed.
 * Called before any new request (so a completed buffer is counted before it can be reused). */
static void tx_poll(gbt_hal_t *h)
{
    uint32_t done;
    if (h->tx_wait_mask != 0U) {
        done = h->tx_wait_mask & h->hfdcan->Instance->TXBTO;
        while (done != 0U) {
            uint32_t bit = done & (~done + 1U);        /* lowest set bit */
            sat_inc(&h->diag.tx_confirmed);
            h->tx_wait_mask &= ~bit;
            done &= ~bit;
        }
    }
    h->diag.tx_pending = popcount3(h->tx_wait_mask);
}

/* ---- port lifecycle ---- */

gbt_hal_status_t gbt_port_g474_init(gbt_hal_t *h, const gbt_port_g474_cfg_t *cfg)
{
    FDCAN_FilterTypeDef fc;

    if (h == NULL || cfg == NULL || cfg->hfdcan == NULL || cfg->own_sa == cfg->peer_sa) {
        return GBT_HAL_E_PARAM;
    }
    if (h->lifecycle != GBT_HAL_LC_UNINIT || s_g474 != NULL) {
        return GBT_HAL_E_STATE;
    }
    memset(h, 0, sizeof *h);
    h->hfdcan = cfg->hfdcan;
    h->filter_id = ((uint32_t)cfg->own_sa << 8) | (uint32_t)cfg->peer_sa;
    if (gbt_spsc_init(&h->rxq) != GBT_SPSC_OK) {                   /* before any RX interrupt */
        return GBT_HAL_E_IO;
    }

    memset(&fc, 0, sizeof fc);
    fc.IdType = FDCAN_EXTENDED_ID;
    fc.FilterIndex = 0U;                                           /* < ExtFiltersNbr (1) */
    fc.FilterType = FDCAN_FILTER_MASK;
    fc.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    fc.FilterID1 = h->filter_id;
    fc.FilterID2 = GBT_PORT_G474_FILTER_MASK;
    if (HAL_FDCAN_ConfigFilter(h->hfdcan, &fc) != HAL_OK) {
        return GBT_HAL_E_IO;
    }
    if (HAL_FDCAN_ConfigGlobalFilter(h->hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_REJECT_REMOTE,
                                     FDCAN_REJECT_REMOTE) != HAL_OK) {
        return GBT_HAL_E_IO;
    }
    s_g474 = h;                                                    /* before the first callback */
    if (HAL_FDCAN_ActivateNotification(h->hfdcan, G474_NOTIFICATIONS, 0U) != HAL_OK) {
        s_g474 = NULL;
        return GBT_HAL_E_IO;
    }
    if (HAL_FDCAN_Start(h->hfdcan) != HAL_OK) {
        (void)HAL_FDCAN_DeactivateNotification(h->hfdcan, G474_NOTIFICATIONS);
        s_g474 = NULL;
        return GBT_HAL_E_IO;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_READY;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_port_g474_deinit(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_READY || s_g474 != h) {
        return GBT_HAL_E_STATE;
    }
    if (HAL_FDCAN_DeactivateNotification(h->hfdcan, G474_NOTIFICATIONS) != HAL_OK
        || HAL_FDCAN_Stop(h->hfdcan) != HAL_OK) {
        return GBT_HAL_E_IO;
    }
    s_g474 = NULL;
    h->lifecycle = (uint8_t)GBT_HAL_LC_UNINIT;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_port_g474_get_diag(const gbt_hal_t *h, gbt_port_g474_diag_t *out)
{
    if (h == NULL || out == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    *out = h->diag;                        /* word-wise snapshot; counters may advance */
    return GBT_HAL_OK;
}

/* ---- ND-1 lifecycle (same contract as every port) ---- */

gbt_hal_status_t gbt_hal_bind_output_sink(gbt_hal_t *h, const gbt_output_sink_t *sink)
{
    if (h == NULL || sink == NULL || sink->apply == NULL || sink->feedback == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    if (h->lifecycle != GBT_HAL_LC_READY) {
        return GBT_HAL_E_STATE;
    }
    h->sink = *sink;
    h->lifecycle = (uint8_t)GBT_HAL_LC_BOUND;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_unbind_output_sink(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_BOUND) {
        return GBT_HAL_E_STATE;
    }
    memset(&h->sink, 0, sizeof h->sink);
    h->lifecycle = (uint8_t)GBT_HAL_LC_READY;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_claim(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT || h->lifecycle == GBT_HAL_LC_READY) {
        return GBT_HAL_E_NOT_READY;
    }
    if (h->lifecycle != GBT_HAL_LC_BOUND) {
        return GBT_HAL_E_STATE;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_CLAIMED;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_release(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_BOUND;
    return GBT_HAL_OK;
}

/* ---- runtime (main context) ---- */

gbt_hal_status_t gbt_hal_can_send(gbt_hal_t *h, const gbt_can_frame_t *f)
{
    FDCAN_TxHeaderTypeDef th;
    FDCAN_ProtocolStatusTypeDef ps;
    HAL_StatusTypeDef hs;

    if (h == NULL || gbt_can_frame_is_valid(f) == 0U) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    memset(&ps, 0, sizeof ps);
    if (HAL_FDCAN_GetProtocolStatus(h->hfdcan, &ps) != HAL_OK) {    /* PSR read resets LEC */
        sat_inc(&h->diag.tx_hal_error);
        sat_inc(&h->tx_error);
        return GBT_HAL_E_IO;
    }
    if (ps.BusOff != 0U) {
        sat_inc(&h->diag.tx_bus_off);
        sat_inc(&h->tx_error);
        return GBT_HAL_E_BUS_OFF;
    }
    tx_poll(h);                                                    /* STEP 07-C2.4 */
    if (HAL_FDCAN_GetTxFifoFreeLevel(h->hfdcan) == 0U) {
        sat_inc(&h->diag.tx_fifo_full);
        return GBT_HAL_E_FULL;                                     /* caller keeps and retries */
    }
    memset(&th, 0, sizeof th);
    th.Identifier = f->id;
    th.IdType = ((f->flags & GBT_CAN_FLAG_EXT) != 0U) ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    th.TxFrameType = FDCAN_DATA_FRAME;
    th.DataLength = dlc_to_code(f->dlc);
    th.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    th.BitRateSwitch = FDCAN_BRS_OFF;
    th.FDFormat = FDCAN_CLASSIC_CAN;
    th.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    th.MessageMarker = 0U;
    /* f->data has 8 bytes: the HAL reads whole words up to DLCtoBytes[dlc] <= 8 */
    hs = HAL_FDCAN_AddMessageToTxFifoQ(h->hfdcan, &th, f->data);
    h->diag.last_tx_hal_status = (uint32_t)hs;
    if (hs != HAL_OK) {
        sat_inc(&h->diag.tx_hal_error);
        sat_inc(&h->tx_error);
        return GBT_HAL_E_IO;
    }
    sat_inc(&h->diag.tx_frames);
    {                                                              /* STEP 07-C2.4 */
        uint32_t bit = HAL_FDCAN_GetLatestTxFifoQRequestBuffer(h->hfdcan) & 0x7U;
        if ((h->tx_wait_mask & bit) != 0U) {
            sat_inc(&h->diag.tx_unconfirmed_reuse);
        }
        h->tx_wait_mask |= bit;
        h->diag.tx_pending = popcount3(h->tx_wait_mask);
    }
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_hal_can_receive(gbt_hal_t *h, gbt_can_frame_t *out)
{
    if (h == NULL || out == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    return (gbt_spsc_pop(&h->rxq, out) == GBT_SPSC_OK) ? GBT_HAL_OK : GBT_HAL_E_EMPTY;
}

uint32_t gbt_hal_now_ms(gbt_hal_t *h)
{
    if (h == NULL || h->lifecycle == GBT_HAL_LC_UNINIT) {
        return 0U;
    }
    return HAL_GetTick();                  /* 1 ms SysTick count, wraps at 2^32 */
}

gbt_hal_status_t gbt_hal_output_apply(gbt_hal_t *h, const gbt_output_cmd_t *cmd)
{
    gbt_hal_status_t st;
    if (h == NULL || cmd == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    if (h->sink.apply == NULL) {
        return GBT_HAL_E_NOT_READY;
    }
    h->in_sink = 1U;
    st = h->sink.apply(h->sink.ctx, cmd);
    h->in_sink = 0U;
    if (st == GBT_HAL_OK) {
        h->accepted_seq = cmd->seq;        /* no timing-marker GPIO yet (pin not assigned) */
    }
    return st;
}

gbt_hal_status_t gbt_hal_output_feedback(gbt_hal_t *h, gbt_output_fb_t *fb)
{
    gbt_hal_status_t st;
    if (h == NULL || fb == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_CLAIMED) {
        return GBT_HAL_E_STATE;
    }
    if (h->sink.feedback == NULL) {
        return GBT_HAL_E_NOT_READY;
    }
    h->in_sink = 1U;
    st = h->sink.feedback(h->sink.ctx, fb);
    h->in_sink = 0U;
    return st;
}

gbt_hal_status_t gbt_hal_can_get_status(gbt_hal_t *h, gbt_can_status_t *s)
{
    FDCAN_ProtocolStatusTypeDef ps;
    FDCAN_ErrorCountersTypeDef ec;

    if (h == NULL || s == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    tx_poll(h);                                                    /* STEP 07-C2.4 */
    memset(s, 0, sizeof *s);
    memset(&ps, 0, sizeof ps);
    memset(&ec, 0, sizeof ec);
    if (HAL_FDCAN_GetProtocolStatus(h->hfdcan, &ps) != HAL_OK) {
        s->state = (uint8_t)GBT_CAN_STATE_UNKNOWN;
    } else if (ps.BusOff != 0U) {
        s->state = (uint8_t)GBT_CAN_STATE_BUS_OFF;
    } else if (ps.ErrorPassive != 0U) {
        s->state = (uint8_t)GBT_CAN_STATE_ERROR_PASSIVE;
    } else if (ps.Warning != 0U) {
        s->state = (uint8_t)GBT_CAN_STATE_ERROR_WARNING;
    } else {
        s->state = (uint8_t)GBT_CAN_STATE_ERROR_ACTIVE;
    }
    if (HAL_FDCAN_GetErrorCounters(h->hfdcan, &ec) == HAL_OK) {   /* ECR read clears CEL */
        s->tec = (uint8_t)(ec.TxErrorCnt & 0xFFU);                 /* TEC: 8 bits */
        s->rec = (uint8_t)(ec.RxErrorCnt & 0x7FU);                 /* REC: 7 bits */
        s->tec_rec_valid = 1U;
    }
    s->rx_overflow_sw = gbt_spsc_dropped(&h->rxq);
    s->rx_overflow_hw = h->rx_overflow_hw;
    s->rx_invalid = h->rx_invalid;
    s->tx_error = h->tx_error;
    return GBT_HAL_OK;
}

void gbt_hal_event(gbt_hal_t *h, uint16_t code, int32_t a, int32_t b)
{
    if (h == NULL || h->lifecycle == GBT_HAL_LC_UNINIT || h->in_sink != 0U) {
        return;
    }
    sat_inc(&h->event_count);
    h->last_event_code = code;             /* bounded, non-blocking: no UART, no printf */
    h->last_event_a = a;
    h->last_event_b = b;
}
