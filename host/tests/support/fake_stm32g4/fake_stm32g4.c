/* tests/support/fake_stm32g4/fake_stm32g4.c - host FDCAN fake, see fake_stm32g4.h. */
#include "fake_stm32g4.h"

#include <stddef.h>
#include <string.h>

fake_g4_t g_fake_g4;

/* Same table as stm32g4xx_hal_fdcan.c v1.2.4 L246. */
static const uint8_t DLCtoBytes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20, 24, 32, 48, 64};

#define IR_RX_MASK  (FDCAN_IT_RX_FIFO0_NEW_MESSAGE | FDCAN_IT_RX_FIFO0_FULL | FDCAN_IT_RX_FIFO0_MESSAGE_LOST)
#define IR_ERR_MASK (FDCAN_IT_ERROR_PASSIVE | FDCAN_IT_ERROR_WARNING | FDCAN_IT_BUS_OFF)

static void order(char c)
{
    if (g_fake_g4.order_len + 1U < sizeof g_fake_g4.order) {
        g_fake_g4.order[g_fake_g4.order_len++] = c;
    }
}

void fake_g4_reset(FDCAN_HandleTypeDef *h)
{
    memset(&g_fake_g4, 0, sizeof g_fake_g4);
    g_fake_g4.h = h;
    h->Instance = &g_fake_g4.regs;
    h->LatestTxFifoQRequest = 0U;
    h->State = HAL_FDCAN_STATE_READY;
    h->ErrorCode = HAL_FDCAN_ERROR_NONE;
    g_fake_g4.anfs = FDCAN_ACCEPT_IN_RX_FIFO0;   /* RXGFC reset value (RM0440) */
    g_fake_g4.anfe = FDCAN_ACCEPT_IN_RX_FIFO0;
}

int fake_g4_bus_rx(uint32_t id, uint8_t ext, uint8_t remote, uint8_t fd, uint8_t brs,
                   uint32_t dlc_code, const uint8_t *data64)
{
    fake_g4_rx_elem_t *e;
    uint8_t accept;
    if (g_fake_g4.h == NULL || g_fake_g4.h->State != HAL_FDCAN_STATE_BUSY) {
        g_fake_g4.rx_filtered++;
        return 0;
    }
    if (remote != 0U && ((ext != 0U) ? g_fake_g4.rrfe : g_fake_g4.rrfs) == FDCAN_REJECT_REMOTE) {
        g_fake_g4.rx_filtered++;
        return 0;
    }
    if (ext != 0U && g_fake_g4.filter_set != 0U && g_fake_g4.filter.IdType == FDCAN_EXTENDED_ID
        && g_fake_g4.filter.FilterType == FDCAN_FILTER_MASK
        && g_fake_g4.filter.FilterConfig == FDCAN_FILTER_TO_RXFIFO0
        && (id & g_fake_g4.filter.FilterID2) == (g_fake_g4.filter.FilterID1 & g_fake_g4.filter.FilterID2)) {
        accept = 1U;
    } else {
        accept = (uint8_t)((((ext != 0U) ? g_fake_g4.anfe : g_fake_g4.anfs) == FDCAN_ACCEPT_IN_RX_FIFO0) ? 1U : 0U);
    }
    if (accept == 0U) {
        g_fake_g4.rx_filtered++;
        return 0;
    }
    if (g_fake_g4.rx_count >= FAKE_G4_RXFIFO_LEN) {          /* blocking mode: new frame lost */
        g_fake_g4.rx_lost_frames++;
        g_fake_g4.ir |= FDCAN_IT_RX_FIFO0_MESSAGE_LOST;
        return 2;
    }
    e = &g_fake_g4.rx[(g_fake_g4.rx_get + g_fake_g4.rx_count) % FAKE_G4_RXFIFO_LEN];
    memset(e, 0, sizeof *e);
    e->hdr.Identifier = (ext != 0U) ? (id & 0x1FFFFFFFU) : (id & 0x7FFU);
    e->hdr.IdType = (ext != 0U) ? FDCAN_EXTENDED_ID : FDCAN_STANDARD_ID;
    e->hdr.RxFrameType = (remote != 0U) ? FDCAN_REMOTE_FRAME : FDCAN_DATA_FRAME;
    e->hdr.DataLength = dlc_code & 0xFU;
    e->hdr.FDFormat = (fd != 0U) ? FDCAN_FD_CAN : FDCAN_CLASSIC_CAN;
    e->hdr.BitRateSwitch = (brs != 0U) ? FDCAN_BRS_ON : FDCAN_BRS_OFF;
    e->hdr.IsFilterMatchingFrame = 0U;
    memcpy(e->data, data64, sizeof e->data);
    g_fake_g4.rx_count++;
    g_fake_g4.ir |= FDCAN_IT_RX_FIFO0_NEW_MESSAGE;
    if (g_fake_g4.rx_count == FAKE_G4_RXFIFO_LEN) {
        g_fake_g4.ir |= FDCAN_IT_RX_FIFO0_FULL;
    }
    return 1;
}

void fake_g4_irq(void)
{
    FDCAN_HandleTypeDef *h = g_fake_g4.h;
    uint32_t rx = g_fake_g4.ir & IR_RX_MASK & g_fake_g4.ie;
    uint32_t es = g_fake_g4.ir & IR_ERR_MASK & g_fake_g4.ie;
    if (h == NULL) {
        return;
    }
    if (rx != 0U) {
        g_fake_g4.ir &= ~rx;                                  /* cleared before the callback */
        HAL_FDCAN_RxFifo0Callback(h, rx);
    }
    if (es != 0U) {
        g_fake_g4.ir &= ~es;
        HAL_FDCAN_ErrorStatusCallback(h, es);
    }
    if (h->ErrorCode != HAL_FDCAN_ERROR_NONE) {
        HAL_FDCAN_ErrorCallback(h);
    }
}

static void tx_finish(uint32_t n, uint8_t occurred)
{
    while (n > 0U && g_fake_g4.tx_pending > 0U) {
        uint32_t bit = (uint32_t)1U << g_fake_g4.tx_order[0];
        uint32_t i;
        g_fake_g4.regs.TXBRP &= ~bit;
        if (occurred != 0U) {
            g_fake_g4.regs.TXBTO |= bit;
        }
        for (i = 1U; i < g_fake_g4.tx_pending; i++) {
            g_fake_g4.tx_order[i - 1U] = g_fake_g4.tx_order[i];
        }
        g_fake_g4.tx_pending--;
        n--;
    }
}

void fake_g4_tx_complete(uint32_t n)
{
    tx_finish(n, 1U);
}

void fake_g4_tx_cancel(uint32_t n)
{
    tx_finish(n, 0U);
}

int fake_g4_tx_pop(fake_g4_tx_elem_t *out)
{
    if (g_fake_g4.tx_read >= g_fake_g4.txlog_count) {
        return 0;
    }
    *out = g_fake_g4.txlog[g_fake_g4.tx_read++];
    return 1;
}

void fake_g4_set_bus_state(uint8_t bus_off, uint8_t passive, uint8_t warning, uint32_t tec, uint32_t rec)
{
    if ((g_fake_g4.psr.BusOff != 0U) != (bus_off != 0U)) {
        g_fake_g4.ir |= FDCAN_IT_BUS_OFF;
    }
    if ((g_fake_g4.psr.ErrorPassive != 0U) != (passive != 0U)) {
        g_fake_g4.ir |= FDCAN_IT_ERROR_PASSIVE;
    }
    if ((g_fake_g4.psr.Warning != 0U) != (warning != 0U)) {
        g_fake_g4.ir |= FDCAN_IT_ERROR_WARNING;
    }
    g_fake_g4.psr.BusOff = bus_off;
    g_fake_g4.psr.ErrorPassive = passive;
    g_fake_g4.psr.Warning = warning;
    g_fake_g4.ecr.TxErrorCnt = tec;
    g_fake_g4.ecr.RxErrorCnt = rec;
    g_fake_g4.ecr.RxErrorPassive = (rec >= 128U) ? 1U : 0U;
}

/* ---- HAL API (fake) ---- */

uint32_t HAL_GetTick(void)
{
    return g_fake_g4.tick;
}

HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan, const FDCAN_FilterTypeDef *sFilterConfig)
{
    order('F');
    if (g_fake_g4.fail_filter != HAL_OK) {
        return g_fake_g4.fail_filter;
    }
    if (hfdcan->State != HAL_FDCAN_STATE_READY && hfdcan->State != HAL_FDCAN_STATE_BUSY) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_NOT_READY;
        return HAL_ERROR;
    }
    g_fake_g4.filter = *sFilterConfig;
    g_fake_g4.filter_set = 1U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan, uint32_t NonMatchingStd,
                                               uint32_t NonMatchingExt, uint32_t RejectRemoteStd,
                                               uint32_t RejectRemoteExt)
{
    order('G');
    if (g_fake_g4.fail_global != HAL_OK) {
        return g_fake_g4.fail_global;
    }
    if (hfdcan->State != HAL_FDCAN_STATE_READY) {             /* READY only (HAL L1392) */
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_NOT_READY;
        return HAL_ERROR;
    }
    g_fake_g4.anfs = NonMatchingStd;
    g_fake_g4.anfe = NonMatchingExt;
    g_fake_g4.rrfs = RejectRemoteStd;
    g_fake_g4.rrfe = RejectRemoteExt;
    g_fake_g4.global_set = 1U;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan)
{
    order('S');
    if (g_fake_g4.fail_start != HAL_OK) {
        return g_fake_g4.fail_start;
    }
    if (hfdcan->State != HAL_FDCAN_STATE_READY) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_NOT_READY;
        return HAL_ERROR;
    }
    hfdcan->State = HAL_FDCAN_STATE_BUSY;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_Stop(FDCAN_HandleTypeDef *hfdcan)
{
    if (hfdcan->State != HAL_FDCAN_STATE_BUSY) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_NOT_STARTED;
        return HAL_ERROR;
    }
    hfdcan->State = HAL_FDCAN_STATE_READY;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan, const FDCAN_TxHeaderTypeDef *pTxHeader,
                                                const uint8_t *pTxData)
{
    fake_g4_tx_elem_t *e;
    uint32_t n;
    uint32_t i;
    g_fake_g4.calls_add++;
    if (g_fake_g4.fail_add != HAL_OK) {
        return g_fake_g4.fail_add;
    }
    if (hfdcan->State != HAL_FDCAN_STATE_BUSY) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_NOT_STARTED;
        return HAL_ERROR;
    }
    if (g_fake_g4.tx_pending >= FAKE_G4_TXFIFO_LEN) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_FIFO_FULL;       /* as HAL L2136 */
        return HAL_ERROR;
    }
    if (g_fake_g4.txlog_count >= FAKE_G4_TXLOG_LEN) {
        return HAL_ERROR;                                      /* test log exhausted */
    }
    e = &g_fake_g4.txlog[g_fake_g4.txlog_count++];
    memset(e, 0, sizeof *e);
    e->hdr = *pTxHeader;
    n = DLCtoBytes[pTxHeader->DataLength & 0xFU];
    for (i = 0U; i < n; i += 4U) {                             /* word reads, as HAL L3524-3529 */
        uint32_t w = ((uint32_t)pTxData[i + 3U] << 24U) | ((uint32_t)pTxData[i + 2U] << 16U)
                   | ((uint32_t)pTxData[i + 1U] << 8U) | (uint32_t)pTxData[i];
        if (i + 3U < sizeof e->data) {
            e->data[i] = (uint8_t)(w & 0xFFU);
            e->data[i + 1U] = (uint8_t)((w >> 8U) & 0xFFU);
            e->data[i + 2U] = (uint8_t)((w >> 16U) & 0xFFU);
            e->data[i + 3U] = (uint8_t)((w >> 24U) & 0xFFU);
        }
    }
    {                                                          /* buffer model (STEP 07-C2.4) */
        uint32_t idx = g_fake_g4.tx_put;
        g_fake_g4.tx_put = (g_fake_g4.tx_put + 1U) % FAKE_G4_TXFIFO_LEN;
        g_fake_g4.tx_order[g_fake_g4.tx_pending] = idx;
        g_fake_g4.regs.TXBRP |= (uint32_t)1U << idx;
        g_fake_g4.regs.TXBTO &= ~((uint32_t)1U << idx);       /* reset by the new request */
        hfdcan->LatestTxFifoQRequest = (uint32_t)1U << idx;
    }
    g_fake_g4.tx_pending++;
    return HAL_OK;
}

uint32_t HAL_FDCAN_GetLatestTxFifoQRequestBuffer(const FDCAN_HandleTypeDef *hfdcan)
{
    return hfdcan->LatestTxFifoQRequest;
}

HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation,
                                         FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData)
{
    fake_g4_rx_elem_t *e;
    uint32_t n;
    uint32_t i;
    g_fake_g4.calls_getrx++;
    if (hfdcan->State != HAL_FDCAN_STATE_BUSY || RxLocation != FDCAN_RX_FIFO0) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_NOT_STARTED;
        return HAL_ERROR;
    }
    if (g_fake_g4.rx_count == 0U) {
        hfdcan->ErrorCode |= HAL_FDCAN_ERROR_FIFO_EMPTY;
        return HAL_ERROR;
    }
    e = &g_fake_g4.rx[g_fake_g4.rx_get];
    *pRxHeader = e->hdr;
    n = DLCtoBytes[e->hdr.DataLength & 0xFU];                  /* as HAL L2338: no FD check */
    for (i = 0U; i < n; i++) {
        pRxData[i] = e->data[i];
    }
    g_fake_g4.rx_get = (g_fake_g4.rx_get + 1U) % FAKE_G4_RXFIFO_LEN;   /* acknowledge */
    g_fake_g4.rx_count--;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_GetProtocolStatus(const FDCAN_HandleTypeDef *hfdcan,
                                              FDCAN_ProtocolStatusTypeDef *ProtocolStatus)
{
    (void)hfdcan;
    g_fake_g4.calls_psr++;
    if (g_fake_g4.fail_psr != HAL_OK) {
        return g_fake_g4.fail_psr;
    }
    *ProtocolStatus = g_fake_g4.psr;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_GetErrorCounters(const FDCAN_HandleTypeDef *hfdcan,
                                             FDCAN_ErrorCountersTypeDef *ErrorCounters)
{
    (void)hfdcan;
    if (g_fake_g4.fail_ecr != HAL_OK) {
        return g_fake_g4.fail_ecr;
    }
    *ErrorCounters = g_fake_g4.ecr;
    return HAL_OK;
}

uint32_t HAL_FDCAN_GetRxFifoFillLevel(const FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo)
{
    (void)hfdcan;
    return (RxFifo == FDCAN_RX_FIFO0) ? g_fake_g4.rx_count : 0U;
}

uint32_t HAL_FDCAN_GetTxFifoFreeLevel(const FDCAN_HandleTypeDef *hfdcan)
{
    (void)hfdcan;
    return FAKE_G4_TXFIFO_LEN - g_fake_g4.tx_pending;
}

HAL_StatusTypeDef HAL_FDCAN_ActivateNotification(FDCAN_HandleTypeDef *hfdcan, uint32_t ActiveITs,
                                                 uint32_t BufferIndexes)
{
    (void)BufferIndexes;
    order('N');
    if (g_fake_g4.fail_notify != HAL_OK) {
        return g_fake_g4.fail_notify;
    }
    if (hfdcan->State != HAL_FDCAN_STATE_READY && hfdcan->State != HAL_FDCAN_STATE_BUSY) {
        return HAL_ERROR;
    }
    g_fake_g4.ie |= ActiveITs;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FDCAN_DeactivateNotification(FDCAN_HandleTypeDef *hfdcan, uint32_t InactiveITs)
{
    if (hfdcan->State != HAL_FDCAN_STATE_READY && hfdcan->State != HAL_FDCAN_STATE_BUSY) {
        return HAL_ERROR;
    }
    g_fake_g4.ie &= ~InactiveITs;
    return HAL_OK;
}
