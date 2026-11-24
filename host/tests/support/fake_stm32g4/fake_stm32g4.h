/*
 * tests/support/fake_stm32g4/fake_stm32g4.h - control interface of the host
 * FDCAN fake (see stm32g4xx_hal.h in this folder). Models, per the STM32G4
 * HAL 1.2.4 sources and RM0440 behaviour used by the port:
 *  - acceptance filtering (one extended mask filter + global filter; reset
 *    values = accept non-matching into FIFO0) applied by the "controller"
 *  - RX FIFO 0 of 3 elements, blocking mode: a 4th frame is lost (RF0L)
 *  - TX FIFO of 3 elements (buffers 0..2, cyclic put index), freed by fake_g4_tx_complete()
 *    (TXBRP cleared, TXBTO set: transmission occurred) or fake_g4_tx_cancel() (TXBRP cleared,
 *    TXBTO not set); a new request of a buffer clears its TXBTO bit (RM0440)
 *  - HAL_FDCAN_GetRxMessage copies DLCtoBytes[DLC] bytes (up to 64)
 *  - HAL_FDCAN_IRQHandler order: clear RX FIFO 0 flags, RxFifo0Callback,
 *    clear error-status flags, ErrorStatusCallback, ErrorCallback if ErrorCode != 0
 *  - HAL state checks (READY / BUSY) of the functions used
 */
#ifndef FAKE_STM32G4_H
#define FAKE_STM32G4_H

#include <stdint.h>
#include "stm32g4xx_hal.h"

#define FAKE_G4_RXFIFO_LEN 3u
#define FAKE_G4_TXFIFO_LEN 3u
#define FAKE_G4_TXLOG_LEN  4096u

typedef struct {
    FDCAN_RxHeaderTypeDef hdr;
    uint8_t               data[64];
} fake_g4_rx_elem_t;

typedef struct {
    FDCAN_TxHeaderTypeDef hdr;
    uint8_t               data[8];
} fake_g4_tx_elem_t;

typedef struct {
    FDCAN_HandleTypeDef *h;
    uint32_t tick;
    /* configuration captured from the port */
    uint8_t  filter_set;
    FDCAN_FilterTypeDef filter;
    uint8_t  global_set;
    uint32_t anfs, anfe, rrfs, rrfe;
    uint32_t ie;                    /* enabled notifications */
    uint32_t ir;                    /* pending interrupt flags */
    char     order[16];             /* call order: F filter, G global, N notify, S start */
    uint32_t order_len;
    /* RX FIFO 0 */
    fake_g4_rx_elem_t rx[FAKE_G4_RXFIFO_LEN];
    uint32_t rx_count, rx_get;
    uint32_t rx_lost_frames;        /* frames dropped by the "hardware" (FIFO full) */
    uint32_t rx_filtered;           /* frames rejected by the acceptance filter */
    /* TX FIFO */
    uint32_t tx_pending;
    uint32_t tx_put;                /* next buffer index (cyclic) */
    uint32_t tx_order[FAKE_G4_TXFIFO_LEN];   /* pending buffer indices, oldest first */
    FDCAN_GlobalTypeDef regs;       /* h->Instance points here */
    fake_g4_tx_elem_t txlog[FAKE_G4_TXLOG_LEN];
    uint32_t txlog_count;
    uint32_t tx_read;               /* next txlog entry for fake_g4_tx_pop() */
    /* status registers */
    FDCAN_ProtocolStatusTypeDef psr;
    FDCAN_ErrorCountersTypeDef  ecr;
    /* failure injection: HAL_OK unless set */
    HAL_StatusTypeDef fail_filter, fail_global, fail_notify, fail_start, fail_add, fail_psr, fail_ecr;
    /* call counters */
    uint32_t calls_add, calls_getrx, calls_psr;
} fake_g4_t;

extern fake_g4_t g_fake_g4;

/* Reset the fake and put *h in HAL_FDCAN_STATE_READY (as after MX_FDCAN1_Init). */
void fake_g4_reset(FDCAN_HandleTypeDef *h);
/* A frame on the bus. data: 64 bytes. Returns 1 stored, 0 rejected by filters
 * or controller not started, 2 lost (RX FIFO 0 full, RF0L set). */
int  fake_g4_bus_rx(uint32_t id, uint8_t ext, uint8_t remote, uint8_t fd, uint8_t brs,
                    uint32_t dlc_code, const uint8_t *data64);
/* HAL_FDCAN_IRQHandler model for the RX FIFO 0 and error-status groups. */
void fake_g4_irq(void);
/* Controller finished n pending TX elements (oldest first): transmission occurred. */
void fake_g4_tx_complete(uint32_t n);
/* n oldest pending TX elements end WITHOUT transmission (cancellation / reset model). */
void fake_g4_tx_cancel(uint32_t n);
/* Next transmitted element (in order); 0 if none. */
int  fake_g4_tx_pop(fake_g4_tx_elem_t *out);
/* Set PSR/ECR and raise the status-change flags that differ. */
void fake_g4_set_bus_state(uint8_t bus_off, uint8_t passive, uint8_t warning, uint32_t tec, uint32_t rec);

#endif /* FAKE_STM32G4_H */
