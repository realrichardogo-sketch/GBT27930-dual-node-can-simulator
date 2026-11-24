/*
 * tests/support/fake_stm32f1/fake_stm32f1.h - control interface of the host bxCAN fake
 * (see stm32f1xx_hal.h in this folder). Models, per STM32F1 HAL 1.1.10 and the bxCAN
 * register behaviour the port relies on:
 *  - filter bank 0, 32-bit ID/mask on the register words (EXID<<3 | IDE | RTR); a frame that
 *    matches no active bank is discarded by the controller
 *  - RX FIFO0 of 3 messages; RF0R.FMP0/FULL0/FOVR0; the output mailbox registers
 *    sFIFOMailBox[0].RIR/RDTR/RDLR/RDHR always describe the oldest message; RFLM = 0: a
 *    message arriving at a full FIFO overwrites the newest stored one and sets FOVR0
 *  - HAL_CAN_GetRxMessage: DLC >= 8 truncated to 8, 8 data bytes, FIFO release
 *  - three TX mailboxes; TSR.TMEx/RQCPx/TXOKx; RQCP/TXOK/ALST/TERR are write-1-to-clear
 *    (a TSR write by the port is applied at the next fake HAL call - every TSR access of the
 *    port is preceded by one); bus arbitration among pending mailboxes by identifier (lowest
 *    wins; equal identifiers: lowest mailbox number)
 *  - ESR: EWGF/EPVF/BOFF, TEC, REC
 *  - HAL_CAN_IRQHandler (RX0 line) order: FIFO0 overrun -> errorcode, clear FOVR0;
 *    message pending -> callback (re-entered while FMP0 != 0, as the level-triggered IRQ);
 *    then ErrorCode |= errorcode and HAL_CAN_ErrorCallback
 */
#ifndef FAKE_STM32F1_H
#define FAKE_STM32F1_H

#include <stdint.h>
#include "stm32f1xx_hal.h"

#define FAKE_F1_TXLOG_LEN 8192u

typedef struct {
    uint32_t id;        /* 29-bit or 11-bit */
    uint8_t  ide;       /* 1 = extended */
    uint8_t  rtr;
    uint8_t  dlc;       /* as written to TDTR (0..8 from the HAL) */
    uint8_t  data[8];
    uint8_t  mailbox;   /* 0..2 */
} fake_f1_tx_t;

typedef struct {
    CAN_TypeDef regs;
    CAN_HandleTypeDef *h;
    uint32_t tick;
    uint8_t  started;
    uint8_t  filter_active;
    uint32_t filter_id_word, filter_mask_word, filter_fifo, filter_mode, filter_scale, filter_bank;
    char     order[16];          /* C ConfigFilter, N ActivateNotification, S Start */
    uint32_t order_len;
    /* RX FIFO0 */
    struct { uint32_t rir, rdtr, rdlr, rdhr; } fifo[3];
    uint32_t fifo_count;
    uint32_t rx_overwritten;     /* frames lost by overwrite (RFLM = 0) */
    uint32_t rx_filtered;
    /* TX mailboxes */
    uint8_t  mb_pending[3];
    fake_f1_tx_t mb[3];
    uint32_t published_tsr;
    fake_f1_tx_t txlog[FAKE_F1_TXLOG_LEN];
    uint32_t txlog_count, tx_read;
    /* fault injection */
    HAL_StatusTypeDef fail_filter, fail_notify, fail_start, fail_add, fail_getrx;
    uint32_t calls_add, calls_getrx;
} fake_f1_t;

extern fake_f1_t g_fake_f1;

/* Reset the fake; h->Instance points at the fake registers, state READY (after MX_CAN_Init). */
void fake_f1_reset(CAN_HandleTypeDef *h);
/* Frame on the bus: returns 1 stored, 0 rejected (filter / not started), 2 stored by
 * overwriting the newest FIFO message (overrun). raw_dlc 0..15 as on the wire. */
int  fake_f1_bus_rx(uint32_t id, uint8_t ide, uint8_t rtr, uint8_t raw_dlc, const uint8_t *data8);
/* HAL_CAN_IRQHandler model for the RX0 line. */
void fake_f1_irq(void);
/* Bus transmits the highest-priority pending mailbox; ok = 1 success (TXOK), 0 failure.
 * Returns 1 if a frame was transmitted, 0 if no mailbox was pending. */
int  fake_f1_bus_tx(uint8_t ok);
/* Next transmitted frame in bus order; 0 if none. */
int  fake_f1_tx_pop(fake_f1_tx_t *out);
/* ESR flags and counters. */
void fake_f1_set_esr(uint8_t bus_off, uint8_t passive, uint8_t warning, uint8_t tec, uint8_t rec);

#endif /* FAKE_STM32F1_H */
