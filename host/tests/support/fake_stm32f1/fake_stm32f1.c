/* tests/support/fake_stm32f1/fake_stm32f1.c - host bxCAN fake, see fake_stm32f1.h. */
#include "fake_stm32f1.h"

#include <stddef.h>
#include <string.h>

fake_f1_t g_fake_f1;

static const uint32_t TME[3]  = {CAN_TSR_TME0, CAN_TSR_TME1, CAN_TSR_TME2};
static const uint32_t RQCP[3] = {CAN_TSR_RQCP0, CAN_TSR_RQCP1, CAN_TSR_RQCP2};
static const uint32_t TXOK[3] = {CAN_TSR_TXOK0, CAN_TSR_TXOK1, CAN_TSR_TXOK2};
static const uint32_t ALST[3] = {CAN_TSR_ALST0, CAN_TSR_ALST1, CAN_TSR_ALST2};
static const uint32_t TERR[3] = {CAN_TSR_TERR0, CAN_TSR_TERR1, CAN_TSR_TERR2};

static void order(char c)
{
    if (g_fake_f1.order_len + 1U < sizeof g_fake_f1.order) {
        g_fake_f1.order[g_fake_f1.order_len++] = c;
    }
}

/* Apply a pending TSR write (write-1-to-clear) and republish TSR. */
static void sync_tsr(void)
{
    uint32_t r = g_fake_f1.regs.TSR;
    unsigned i;
    if (r != g_fake_f1.published_tsr) {
        uint32_t keep = g_fake_f1.published_tsr;
        for (i = 0U; i < 3U; i++) {
            if ((r & RQCP[i]) != 0U) {
                keep &= ~(RQCP[i] | TXOK[i] | ALST[i] | TERR[i]);
            }
        }
        g_fake_f1.published_tsr = keep;
    }
    g_fake_f1.regs.TSR = g_fake_f1.published_tsr;
}

static void publish_tsr(uint32_t v)
{
    g_fake_f1.published_tsr = v;
    g_fake_f1.regs.TSR = v;
}

static void publish_fifo(void)
{
    uint32_t rf = g_fake_f1.regs.RF0R & CAN_RF0R_FOVR0;
    rf |= g_fake_f1.fifo_count & CAN_RF0R_FMP0;
    if (g_fake_f1.fifo_count == 3U) {
        rf |= CAN_RF0R_FULL0;
    }
    g_fake_f1.regs.RF0R = rf;
    if (g_fake_f1.fifo_count != 0U) {
        g_fake_f1.regs.sFIFOMailBox[0].RIR = g_fake_f1.fifo[0].rir;
        g_fake_f1.regs.sFIFOMailBox[0].RDTR = g_fake_f1.fifo[0].rdtr;
        g_fake_f1.regs.sFIFOMailBox[0].RDLR = g_fake_f1.fifo[0].rdlr;
        g_fake_f1.regs.sFIFOMailBox[0].RDHR = g_fake_f1.fifo[0].rdhr;
    }
}

void fake_f1_reset(CAN_HandleTypeDef *h)
{
    memset(&g_fake_f1, 0, sizeof g_fake_f1);
    g_fake_f1.h = h;
    h->Instance = &g_fake_f1.regs;
    h->State = HAL_CAN_STATE_READY;
    h->ErrorCode = HAL_CAN_ERROR_NONE;
    publish_tsr(CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2);
}

int fake_f1_bus_rx(uint32_t id, uint8_t ide, uint8_t rtr, uint8_t raw_dlc, const uint8_t *data8)
{
    uint32_t word;
    uint32_t slot;
    int ret = 1;
    if (g_fake_f1.started == 0U) {
        g_fake_f1.rx_filtered++;
        return 0;
    }
    word = (ide != 0U) ? (((id & 0x1FFFFFFFU) << 3) | 0x4U) : ((id & 0x7FFU) << 21);
    word |= (rtr != 0U) ? 0x2U : 0x0U;
    if (g_fake_f1.filter_active == 0U || g_fake_f1.filter_mode != CAN_FILTERMODE_IDMASK
        || g_fake_f1.filter_scale != CAN_FILTERSCALE_32BIT || g_fake_f1.filter_fifo != CAN_FILTER_FIFO0
        || (word & g_fake_f1.filter_mask_word) != (g_fake_f1.filter_id_word & g_fake_f1.filter_mask_word)) {
        g_fake_f1.rx_filtered++;                 /* no matching bank: discarded */
        return 0;
    }
    if (g_fake_f1.fifo_count == 3U) {            /* RFLM = 0: newest message overwritten */
        slot = 2U;
        g_fake_f1.regs.RF0R |= CAN_RF0R_FOVR0;
        g_fake_f1.rx_overwritten++;
        ret = 2;
    } else {
        slot = g_fake_f1.fifo_count++;
    }
    g_fake_f1.fifo[slot].rir = (ide != 0U) ? (((id & 0x1FFFFFFFU) << 3) | 0x4U) : ((id & 0x7FFU) << 21);
    g_fake_f1.fifo[slot].rir |= (rtr != 0U) ? 0x2U : 0x0U;
    g_fake_f1.fifo[slot].rdtr = (uint32_t)raw_dlc & CAN_RDT0R_DLC;
    g_fake_f1.fifo[slot].rdlr = (uint32_t)data8[0] | ((uint32_t)data8[1] << 8) | ((uint32_t)data8[2] << 16)
                              | ((uint32_t)data8[3] << 24);
    g_fake_f1.fifo[slot].rdhr = (uint32_t)data8[4] | ((uint32_t)data8[5] << 8) | ((uint32_t)data8[6] << 16)
                              | ((uint32_t)data8[7] << 24);
    publish_fifo();
    return ret;
}

void fake_f1_irq(void)
{
    CAN_HandleTypeDef *h = g_fake_f1.h;
    uint32_t errorcode = HAL_CAN_ERROR_NONE;
    unsigned guard;
    if (h == NULL) {
        return;
    }
    sync_tsr();
    if ((g_fake_f1.regs.IER & CAN_IT_RX_FIFO0_OVERRUN) != 0U && (g_fake_f1.regs.RF0R & CAN_RF0R_FOVR0) != 0U) {
        errorcode |= HAL_CAN_ERROR_RX_FOV0;
        g_fake_f1.regs.RF0R &= ~(uint32_t)CAN_RF0R_FOVR0;
    }
    if ((g_fake_f1.regs.IER & CAN_IT_RX_FIFO0_MSG_PENDING) != 0U) {
        for (guard = 0U; guard < 4U && (g_fake_f1.regs.RF0R & CAN_RF0R_FMP0) != 0U; guard++) {
            HAL_CAN_RxFifo0MsgPendingCallback(h);   /* level-triggered IRQ re-entry */
        }
    }
    if (errorcode != HAL_CAN_ERROR_NONE) {
        h->ErrorCode |= errorcode;
        HAL_CAN_ErrorCallback(h);
    }
}

int fake_f1_bus_tx(uint8_t ok)
{
    unsigned i;
    int best = -1;
    uint32_t v;
    sync_tsr();
    for (i = 0U; i < 3U; i++) {
        if (g_fake_f1.mb_pending[i] != 0U
            && (best < 0 || g_fake_f1.mb[i].id < g_fake_f1.mb[(unsigned)best].id)) {
            best = (int)i;                       /* lowest identifier wins; ties: lowest mailbox */
        }
    }
    if (best < 0) {
        return 0;
    }
    i = (unsigned)best;
    g_fake_f1.mb_pending[i] = 0U;
    v = g_fake_f1.published_tsr | TME[i] | RQCP[i];
    if (ok != 0U) {
        v |= TXOK[i];
        if (g_fake_f1.txlog_count < FAKE_F1_TXLOG_LEN) {
            g_fake_f1.txlog[g_fake_f1.txlog_count++] = g_fake_f1.mb[i];
        }
    } else {
        v = (v & ~TXOK[i]) | TERR[i];
    }
    publish_tsr(v);
    return 1;
}

int fake_f1_tx_pop(fake_f1_tx_t *out)
{
    if (g_fake_f1.tx_read >= g_fake_f1.txlog_count) {
        return 0;
    }
    *out = g_fake_f1.txlog[g_fake_f1.tx_read++];
    return 1;
}

void fake_f1_set_esr(uint8_t bus_off, uint8_t passive, uint8_t warning, uint8_t tec, uint8_t rec)
{
    uint32_t e = 0U;
    e |= (bus_off != 0U) ? CAN_ESR_BOFF : 0U;
    e |= (passive != 0U) ? CAN_ESR_EPVF : 0U;
    e |= (warning != 0U) ? CAN_ESR_EWGF : 0U;
    e |= ((uint32_t)tec << CAN_ESR_TEC_Pos) | ((uint32_t)rec << CAN_ESR_REC_Pos);
    g_fake_f1.regs.ESR = e;
}

/* ---- HAL API (fake) ---- */

uint32_t HAL_GetTick(void)
{
    sync_tsr();
    return g_fake_f1.tick;
}

HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, const CAN_FilterTypeDef *f)
{
    sync_tsr();
    order('C');
    if (g_fake_f1.fail_filter != HAL_OK) {
        return g_fake_f1.fail_filter;
    }
    if (hcan->State != HAL_CAN_STATE_READY && hcan->State != HAL_CAN_STATE_LISTENING) {
        hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;
        return HAL_ERROR;
    }
    g_fake_f1.filter_id_word = ((f->FilterIdHigh & 0xFFFFU) << 16) | (f->FilterIdLow & 0xFFFFU);
    g_fake_f1.filter_mask_word = ((f->FilterMaskIdHigh & 0xFFFFU) << 16) | (f->FilterMaskIdLow & 0xFFFFU);
    g_fake_f1.filter_fifo = f->FilterFIFOAssignment;
    g_fake_f1.filter_mode = f->FilterMode;
    g_fake_f1.filter_scale = f->FilterScale;
    g_fake_f1.filter_bank = f->FilterBank;
    g_fake_f1.filter_active = (f->FilterActivation == CAN_FILTER_ENABLE) ? 1U : 0U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan)
{
    sync_tsr();
    order('S');
    if (g_fake_f1.fail_start != HAL_OK) {
        return g_fake_f1.fail_start;
    }
    if (hcan->State != HAL_CAN_STATE_READY) {
        hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;
        return HAL_ERROR;
    }
    hcan->State = HAL_CAN_STATE_LISTENING;
    g_fake_f1.started = 1U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_Stop(CAN_HandleTypeDef *hcan)
{
    sync_tsr();
    if (hcan->State != HAL_CAN_STATE_LISTENING) {
        hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;
        return HAL_ERROR;
    }
    hcan->State = HAL_CAN_STATE_READY;
    g_fake_f1.started = 0U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *p,
                                       const uint8_t aData[], uint32_t *pTxMailbox)
{
    unsigned i;
    sync_tsr();
    g_fake_f1.calls_add++;
    if (g_fake_f1.fail_add != HAL_OK) {
        return g_fake_f1.fail_add;
    }
    if (hcan->State != HAL_CAN_STATE_READY && hcan->State != HAL_CAN_STATE_LISTENING) {
        hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;
        return HAL_ERROR;
    }
    for (i = 0U; i < 3U; i++) {                  /* TSR.CODE: lowest empty mailbox */
        if ((g_fake_f1.published_tsr & TME[i]) != 0U) {
            break;
        }
    }
    if (i == 3U) {
        hcan->ErrorCode |= HAL_CAN_ERROR_PARAM;  /* as HAL: no free mailbox */
        return HAL_ERROR;
    }
    memset(&g_fake_f1.mb[i], 0, sizeof g_fake_f1.mb[i]);
    g_fake_f1.mb[i].ide = (p->IDE == CAN_ID_EXT) ? 1U : 0U;
    g_fake_f1.mb[i].id = (p->IDE == CAN_ID_EXT) ? p->ExtId : p->StdId;
    g_fake_f1.mb[i].rtr = (p->RTR == CAN_RTR_REMOTE) ? 1U : 0U;
    g_fake_f1.mb[i].dlc = (uint8_t)(p->DLC & 0xFU);
    memcpy(g_fake_f1.mb[i].data, aData, 8U);     /* HAL always writes TDLR/TDHR (8 bytes) */
    g_fake_f1.mb[i].mailbox = (uint8_t)i;
    g_fake_f1.mb_pending[i] = 1U;
    publish_tsr(g_fake_f1.published_tsr & ~TME[i]);
    *pTxMailbox = (uint32_t)1U << i;
    return HAL_OK;
}

uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef *hcan)
{
    uint32_t n = 0U;
    unsigned i;
    (void)hcan;
    sync_tsr();
    for (i = 0U; i < 3U; i++) {
        n += ((g_fake_f1.published_tsr & TME[i]) != 0U) ? 1U : 0U;
    }
    return n;
}

uint32_t HAL_CAN_IsTxMessagePending(const CAN_HandleTypeDef *hcan, uint32_t TxMailboxes)
{
    uint32_t tme = TxMailboxes << 26;            /* CAN_TSR_TME0_Pos = 26 */
    (void)hcan;
    sync_tsr();
    return ((g_fake_f1.published_tsr & tme) != tme) ? 1U : 0U;
}

HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t RxFifo,
                                       CAN_RxHeaderTypeDef *pHeader, uint8_t aData[])
{
    uint32_t rir;
    uint32_t dlc;
    unsigned i;
    sync_tsr();
    g_fake_f1.calls_getrx++;
    if (g_fake_f1.fail_getrx != HAL_OK) {
        return g_fake_f1.fail_getrx;
    }
    if (hcan->State != HAL_CAN_STATE_READY && hcan->State != HAL_CAN_STATE_LISTENING) {
        hcan->ErrorCode |= HAL_CAN_ERROR_NOT_INITIALIZED;
        return HAL_ERROR;
    }
    if (RxFifo != CAN_RX_FIFO0 || g_fake_f1.fifo_count == 0U) {
        hcan->ErrorCode |= HAL_CAN_ERROR_PARAM;
        return HAL_ERROR;
    }
    rir = g_fake_f1.fifo[0].rir;
    pHeader->IDE = rir & 0x4U;
    if (pHeader->IDE == CAN_ID_STD) {
        pHeader->StdId = rir >> 21;
    } else {
        pHeader->ExtId = rir >> 3;
    }
    pHeader->RTR = rir & 0x2U;
    dlc = g_fake_f1.fifo[0].rdtr & CAN_RDT0R_DLC;
    pHeader->DLC = (dlc >= 8U) ? 8U : dlc;       /* HAL 1.1.10 truncation */
    for (i = 0U; i < 4U; i++) {
        aData[i] = (uint8_t)((g_fake_f1.fifo[0].rdlr >> (8U * i)) & 0xFFU);
        aData[4U + i] = (uint8_t)((g_fake_f1.fifo[0].rdhr >> (8U * i)) & 0xFFU);
    }
    for (i = 0U; i + 1U < g_fake_f1.fifo_count; i++) {   /* RFOM0: release */
        g_fake_f1.fifo[i] = g_fake_f1.fifo[i + 1U];
    }
    g_fake_f1.fifo_count--;
    publish_fifo();
    return HAL_OK;
}

uint32_t HAL_CAN_GetRxFifoFillLevel(const CAN_HandleTypeDef *hcan, uint32_t RxFifo)
{
    (void)hcan;
    sync_tsr();
    return (RxFifo == CAN_RX_FIFO0) ? g_fake_f1.fifo_count : 0U;
}

HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t ActiveITs)
{
    sync_tsr();
    order('N');
    if (g_fake_f1.fail_notify != HAL_OK) {
        return g_fake_f1.fail_notify;
    }
    if (hcan->State != HAL_CAN_STATE_READY && hcan->State != HAL_CAN_STATE_LISTENING) {
        return HAL_ERROR;
    }
    g_fake_f1.regs.IER |= ActiveITs;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_CAN_DeactivateNotification(CAN_HandleTypeDef *hcan, uint32_t InactiveITs)
{
    sync_tsr();
    if (hcan->State != HAL_CAN_STATE_READY && hcan->State != HAL_CAN_STATE_LISTENING) {
        return HAL_ERROR;
    }
    g_fake_f1.regs.IER &= ~InactiveITs;
    return HAL_OK;
}
