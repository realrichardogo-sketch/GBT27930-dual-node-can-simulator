/*
 * ports/stm32/f103/port_f103.c - see port_f103.h.
 *
 * STM32F1 HAL 1.1.10 behaviour relied upon (verified in stm32f1xx_hal_can.c):
 *  - HAL_CAN_IRQHandler (RX0 line): FIFO0 overrun -> errorcode |= HAL_CAN_ERROR_RX_FOV0 and
 *    FOV0 cleared; message pending -> HAL_CAN_RxFifo0MsgPendingCallback while FMP0 != 0;
 *    at the end hcan->ErrorCode |= errorcode and HAL_CAN_ErrorCallback().
 *  - HAL_CAN_GetRxMessage: ErrorCode |= PARAM on an empty FIFO; DLC >= 8 truncated to 8;
 *    always writes 8 data bytes; releases the FIFO entry (RFOM0).
 *  - HAL_CAN_AddTxMessage: uses the mailbox given by TSR.CODE; no free mailbox ->
 *    ErrorCode |= PARAM, HAL_ERROR; *pTxMailbox = 1 << mailbox; always writes 8 data bytes.
 * hcan->ErrorCode is read-modify-written by the ISR. The main context never takes a HAL path
 * that writes it (FIFO fill level and mailbox free level are checked first); only the ISR
 * clears the FOV0 bit after counting it.
 */
#include "ports/stm32/f103/port_f103.h"

#include <stddef.h>
#include <string.h>

#define F103_NOTIFICATIONS (CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_RX_FIFO0_OVERRUN)
#define F103_ALL_MAILBOXES 3U

/* The single active instance; written in main context only while notifications are off. */
static gbt_hal_t *volatile s_f103 = NULL;

static void sat_inc(volatile uint32_t *c)
{
    uint32_t v = *c;
    if (v != 0xFFFFFFFFu) {
        *c = v + 1U;                       /* single writer per counter */
    }
}

/* ---- ISR side (producer) ---- */

static void rx_invalid(gbt_hal_t *h, volatile uint32_t *reason)
{
    sat_inc(reason);
    sat_inc(&h->rx_invalid);
}

/* Read one FIFO0 element; 0 = stop draining (HAL error). */
static uint8_t rx_one(gbt_hal_t *h)
{
    CAN_RxHeaderTypeDef hdr;
    uint8_t data[8];
    gbt_can_frame_t f;
    uint32_t raw_dlc;
    unsigned b;

    /* D2: original DLC from the FIFO0 output mailbox before the HAL truncates it */
    raw_dlc = (h->hcan->Instance->sFIFOMailBox[CAN_RX_FIFO0].RDTR & CAN_RDT0R_DLC) >> CAN_RDT0R_DLC_Pos;
    memset(&hdr, 0, sizeof hdr);
    memset(data, 0, sizeof data);
    if (HAL_CAN_GetRxMessage(h->hcan, CAN_RX_FIFO0, &hdr, data) != HAL_OK) {   /* also releases */
        rx_invalid(h, &h->diag.rx_hal_error);
        return 0U;
    }
    if (raw_dlc > GBT_CAN_MAX_DLC) {
        rx_invalid(h, &h->diag.rx_bad_dlc);
        return 1U;
    }
    if (hdr.RTR != CAN_RTR_DATA) {
        rx_invalid(h, &h->diag.rx_remote);
        return 1U;
    }
    if (hdr.IDE != CAN_ID_EXT) {
        rx_invalid(h, &h->diag.rx_std_id);
        return 1U;
    }
    f.id = hdr.ExtId & GBT_CAN_MAX_EXT_ID;
    f.dlc = (uint8_t)raw_dlc;
    f.flags = (uint8_t)GBT_CAN_FLAG_EXT;
    for (b = 0U; b < GBT_CAN_MAX_DLC; b++) {
        f.data[b] = (b < raw_dlc) ? data[b] : 0U;   /* bytes beyond DLC are not payload */
    }
    if (gbt_spsc_push(&h->rxq, &f) == GBT_SPSC_OK) {
        sat_inc(&h->diag.rx_frames);
    }                                      /* E_FULL: counted by the queue (rx_overflow_sw) */
    return 1U;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    gbt_hal_t *h = s_f103;
    unsigned n;

    if (h == NULL || hcan != h->hcan) {
        return;
    }
    for (n = 0U; n < GBT_PORT_F103_RX_DRAIN_MAX; n++) {              /* bounded */
        if (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) == 0U) {
            break;
        }
        if (rx_one(h) == 0U) {
            break;
        }
    }
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    gbt_hal_t *h = s_f103;
    uint32_t ec;

    if (h == NULL || hcan != h->hcan) {
        return;
    }
    sat_inc(&h->diag.hal_error_callbacks);
    ec = hcan->ErrorCode;
    if ((ec & HAL_CAN_ERROR_RX_FOV0) != 0U) {
        sat_inc(&h->rx_overflow_hw);
        sat_inc(&h->diag.rx_fifo_overruns);
        hcan->ErrorCode = ec & ~HAL_CAN_ERROR_RX_FOV0;   /* only the ISR writes ErrorCode */
    }
}

/* ---- TX completion (main context; TSR request-completed flags are main-owned) ---- */

static void tx_poll(gbt_hal_t *h)
{
    uint32_t tsr;
    uint32_t rqcp;
    uint32_t txok;

    if (h->tx_mailbox == 0U) {
        h->diag.tx_in_flight = 0U;
        return;
    }
    switch (h->tx_mailbox) {
    case CAN_TX_MAILBOX0: rqcp = CAN_TSR_RQCP0; txok = CAN_TSR_TXOK0; break;
    case CAN_TX_MAILBOX1: rqcp = CAN_TSR_RQCP1; txok = CAN_TSR_TXOK1; break;
    default:              rqcp = CAN_TSR_RQCP2; txok = CAN_TSR_TXOK2; break;
    }
    if (HAL_CAN_IsTxMessagePending(h->hcan, h->tx_mailbox) != 0U) {   /* TME clear */
        h->diag.tx_in_flight = 1U;                                     /* HW retransmits */
        h->diag.tx_in_flight_ms = (uint32_t)(HAL_GetTick() - h->tx_start_ms);
        return;
    }
    tsr = h->hcan->Instance->TSR;
    if ((tsr & rqcp) != 0U && (tsr & txok) != 0U) {
        sat_inc(&h->diag.tx_completed_ok);
    } else {                               /* aborted / failed, or completion not recorded */
        sat_inc(&h->diag.tx_completed_fail);
        sat_inc(&h->tx_error);
    }
    h->hcan->Instance->TSR = rqcp;         /* rc_w1: clears RQCP, TXOK, ALST, TERR of this mailbox */
    h->tx_mailbox = 0U;
    h->diag.tx_in_flight = 0U;
    h->diag.tx_in_flight_ms = 0U;
}

/* ---- port lifecycle ---- */

gbt_hal_status_t gbt_port_f103_init(gbt_hal_t *h, const gbt_port_f103_cfg_t *cfg)
{
    CAN_FilterTypeDef fc;
    uint32_t id_word;
    uint32_t mask_word;

    if (h == NULL || cfg == NULL || cfg->hcan == NULL || cfg->own_sa == cfg->peer_sa) {
        return GBT_HAL_E_PARAM;
    }
    if (h->lifecycle != GBT_HAL_LC_UNINIT || s_f103 != NULL) {
        return GBT_HAL_E_STATE;
    }
    memset(h, 0, sizeof *h);
    h->hcan = cfg->hcan;
    h->filter_id = ((uint32_t)cfg->own_sa << 8) | (uint32_t)cfg->peer_sa;
    if (gbt_spsc_init(&h->rxq) != GBT_SPSC_OK) {                   /* before any RX interrupt */
        return GBT_HAL_E_IO;
    }

    id_word = GBT_PORT_F103_FILTER_ID(h->filter_id);               /* IDE = 1, RTR = 0 */
    mask_word = GBT_PORT_F103_FILTER_MASK(GBT_PORT_F103_ADDR_MASK); /* PS, SA, IDE, RTR */
    memset(&fc, 0, sizeof fc);
    fc.FilterIdHigh = id_word >> 16;
    fc.FilterIdLow = id_word & 0xFFFFU;
    fc.FilterMaskIdHigh = mask_word >> 16;
    fc.FilterMaskIdLow = mask_word & 0xFFFFU;
    fc.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    fc.FilterBank = 0U;
    fc.FilterMode = CAN_FILTERMODE_IDMASK;
    fc.FilterScale = CAN_FILTERSCALE_32BIT;
    fc.FilterActivation = CAN_FILTER_ENABLE;
    fc.SlaveStartFilterBank = 14U;                                 /* ignored: no CAN2 on F103xE */
    if (HAL_CAN_ConfigFilter(h->hcan, &fc) != HAL_OK) {
        return GBT_HAL_E_IO;
    }
    s_f103 = h;                                                    /* before the first callback */
    if (HAL_CAN_ActivateNotification(h->hcan, F103_NOTIFICATIONS) != HAL_OK) {
        s_f103 = NULL;
        return GBT_HAL_E_IO;
    }
    if (HAL_CAN_Start(h->hcan) != HAL_OK) {                        /* bounded HAL init wait */
        (void)HAL_CAN_DeactivateNotification(h->hcan, F103_NOTIFICATIONS);
        s_f103 = NULL;
        return GBT_HAL_E_IO;
    }
    h->lifecycle = (uint8_t)GBT_HAL_LC_READY;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_port_f103_deinit(gbt_hal_t *h)
{
    if (h == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U || h->lifecycle != GBT_HAL_LC_READY || s_f103 != h) {
        return GBT_HAL_E_STATE;
    }
    if (HAL_CAN_DeactivateNotification(h->hcan, F103_NOTIFICATIONS) != HAL_OK
        || HAL_CAN_Stop(h->hcan) != HAL_OK) {
        return GBT_HAL_E_IO;
    }
    s_f103 = NULL;
    h->lifecycle = (uint8_t)GBT_HAL_LC_UNINIT;
    return GBT_HAL_OK;
}

gbt_hal_status_t gbt_port_f103_get_diag(gbt_hal_t *h, gbt_port_f103_diag_t *out)
{
    if (h == NULL || out == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    tx_poll(h);
    *out = h->diag;                        /* word-wise snapshot; ISR counters may advance */
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
    CAN_TxHeaderTypeDef th;
    HAL_StatusTypeDef hs;
    uint32_t mailbox = 0U;

    if (h == NULL || gbt_can_frame_is_valid(f) == 0U) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    if ((h->hcan->Instance->ESR & CAN_ESR_BOFF) != 0U) {          /* unsafe: inhibit TX */
        sat_inc(&h->diag.tx_bus_off);
        sat_inc(&h->tx_error);
        return GBT_HAL_E_BUS_OFF;
    }
    tx_poll(h);
    /* D1: one mailbox in flight. Also refuse if any mailbox is unexpectedly busy, so
     * AddTxMessage never takes its "no free mailbox" path (which writes ErrorCode). */
    if (h->tx_mailbox != 0U || HAL_CAN_GetTxMailboxesFreeLevel(h->hcan) != F103_ALL_MAILBOXES) {
        sat_inc(&h->diag.tx_busy);
        return GBT_HAL_E_FULL;                                     /* caller keeps and retries */
    }
    memset(&th, 0, sizeof th);
    if ((f->flags & GBT_CAN_FLAG_EXT) != 0U) {
        th.ExtId = f->id;
        th.IDE = CAN_ID_EXT;
    } else {
        th.StdId = f->id;
        th.IDE = CAN_ID_STD;
    }
    th.RTR = CAN_RTR_DATA;
    th.DLC = f->dlc;
    th.TransmitGlobalTime = DISABLE;
    hs = HAL_CAN_AddTxMessage(h->hcan, &th, f->data, &mailbox);   /* writes 8 bytes: data[8] */
    h->diag.last_tx_hal_status = (uint32_t)hs;
    if (hs != HAL_OK) {
        sat_inc(&h->diag.tx_hal_error);
        sat_inc(&h->tx_error);
        return GBT_HAL_E_IO;
    }
    h->tx_mailbox = mailbox;
    h->tx_start_ms = HAL_GetTick();
    h->diag.tx_in_flight = 1U;
    sat_inc(&h->diag.tx_frames);
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
        h->accepted_seq = cmd->seq;
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
    uint32_t esr;

    if (h == NULL || s == NULL) {
        return GBT_HAL_E_PARAM;
    }
    if (h->in_sink != 0U) {
        return GBT_HAL_E_STATE;
    }
    if (h->lifecycle == GBT_HAL_LC_UNINIT) {
        return GBT_HAL_E_NOT_READY;
    }
    tx_poll(h);
    memset(s, 0, sizeof *s);
    esr = h->hcan->Instance->ESR;          /* polled: the SCE interrupt is not enabled */
    if ((esr & CAN_ESR_BOFF) != 0U) {
        s->state = (uint8_t)GBT_CAN_STATE_BUS_OFF;
    } else if ((esr & CAN_ESR_EPVF) != 0U) {
        s->state = (uint8_t)GBT_CAN_STATE_ERROR_PASSIVE;
    } else if ((esr & CAN_ESR_EWGF) != 0U) {
        s->state = (uint8_t)GBT_CAN_STATE_ERROR_WARNING;
    } else {
        s->state = (uint8_t)GBT_CAN_STATE_ERROR_ACTIVE;
    }
    {                                      /* STEP 07-C2.4: polled entry events */
        uint32_t prev = h->diag.last_polled_state;
        uint32_t now_st = s->state;
        if (now_st >= GBT_CAN_STATE_ERROR_WARNING && (prev < GBT_CAN_STATE_ERROR_WARNING)) {
            sat_inc(&h->diag.warning_events);
        }
        if (now_st >= GBT_CAN_STATE_ERROR_PASSIVE && (prev < GBT_CAN_STATE_ERROR_PASSIVE)) {
            sat_inc(&h->diag.error_passive_events);
        }
        if (now_st == GBT_CAN_STATE_BUS_OFF && prev != GBT_CAN_STATE_BUS_OFF) {
            sat_inc(&h->diag.bus_off_events);
        }
        h->diag.last_polled_state = now_st;
    }
    s->tec = (uint8_t)((esr & CAN_ESR_TEC) >> CAN_ESR_TEC_Pos);
    s->rec = (uint8_t)((esr & CAN_ESR_REC) >> CAN_ESR_REC_Pos);
    s->tec_rec_valid = 1U;
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
    h->last_event_code = code;             /* bounded, non-blocking */
    h->last_event_a = a;
    h->last_event_b = b;
}
