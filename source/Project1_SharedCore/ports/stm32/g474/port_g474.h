/*
 * ports/stm32/g474/port_g474.h - STM32G474 FDCAN1 implementation of hal/hal.h (ND-1).
 * STEP 07-C1.2-B1. Audit basis: docs/STEP07C1_2A2_G474_RX_AUDIT.md.
 *
 * Exactly one instance per executable (it owns the HAL FDCAN callbacks).
 * Classical CAN 2.0B only; 29-bit identifiers (11-bit frames are rejected by
 * the global filter and counted as invalid if they ever arrive).
 *
 * Contexts:
 *   FDCAN1_IT0 ISR (HAL_FDCAN_IRQHandler -> HAL_FDCAN_*Callback below):
 *     sole PRODUCER of the RX queue; writes only the ISR-owned counters.
 *   Main loop (integration layer): every gbt_hal_* function; sole CONSUMER
 *     of the RX queue; writes only the main-owned counters.
 * No protocol code runs in the ISR. Nothing blocks.
 *
 * Requires the CubeMX-generated MX_FDCAN1_Init() to have run and FDCAN1 NOT
 * to be started yet (HAL state READY): gbt_port_g474_init() configures the
 * filters, enables the notifications and starts the controller itself.
 *
 * Must not include app/ or protocol/. STM32 HAL headers are used only in
 * ports/stm32/.
 */
#ifndef GBT_PORT_G474_H
#define GBT_PORT_G474_H

#include <stdint.h>
#include "stm32g4xx_hal.h"
#include "hal/hal.h"
#include "ports/common/gbt_hal_lc.h"
#include "ports/common/gbt_spsc.h"

/* HAL_FDCAN_GetRxMessage() copies DLCtoBytes[DLC] bytes (up to 64) without
 * checking the FD flag (stm32g4xx_hal_fdcan.c v1.2.4 L246, L2338): the
 * scratch buffer must hold 64 bytes. */
#define GBT_PORT_G474_RX_SCRATCH 64u
/* RX FIFO 0 depth on STM32G4 (SRAMCAN_RF0_NBR); frames drained per callback. */
#define GBT_PORT_G474_RX_DRAIN_MAX 3u

/* Acceptance: J1939 PDU1 identifiers with PS = own address and SA = peer
 * address, i.e. bits 15..0 = (own_sa << 8) | peer_sa. */
#define GBT_PORT_G474_FILTER_MASK 0x0000FFFFu

/* Diagnostics beyond gbt_can_status_t. ISR-owned: rx_*, *_events,
 * hal_error_callbacks. Main-owned: tx_*. All saturating. */
typedef struct {
    uint32_t rx_frames;            /* accepted and queued */
    uint32_t rx_fd;                /* rejected: FD format or BRS */
    uint32_t rx_remote;            /* rejected: remote frame */
    uint32_t rx_bad_dlc;           /* rejected: DLC code outside 0..8 */
    uint32_t rx_std_id;            /* rejected: 11-bit identifier */
    uint32_t rx_hal_error;         /* HAL_FDCAN_GetRxMessage() != HAL_OK */
    uint32_t rx_fifo_loss_events;  /* RF0L notifications (events, not frames) */
    uint32_t bus_off_events;       /* BO status-change notifications */
    uint32_t error_passive_events; /* EP status-change notifications */
    uint32_t warning_events;       /* EW status-change notifications */
    uint32_t hal_error_callbacks;  /* HAL_FDCAN_ErrorCallback() invocations */
    uint32_t tx_frames;            /* accepted by HAL_FDCAN_AddMessageToTxFifoQ */
    uint32_t tx_fifo_full;         /* E_FULL returned (no free TX FIFO element) */
    uint32_t tx_bus_off;           /* E_BUS_OFF returned */
    uint32_t tx_hal_error;         /* AddMessageToTxFifoQ / status read failed */
    uint32_t last_tx_hal_status;   /* HAL_StatusTypeDef of the last AddMessage call */
    /* STEP 07-C2.4 TX confirmation (main-owned, polled in gbt_hal_can_send / gbt_hal_can_get_status).
     * tx_frames only proves that a TX FIFO element was requested. A request is CONFIRMED when the
     * controller sets its TXBTO bit (Tx Buffer Transmission Occurred): the frame completed on the bus,
     * which in Classical CAN requires an ACK from at least one other node (with the frozen
     * two-active-node topology: the F103 B controller). Automatic retransmission is enabled
     * (fdcan.c), so an unacknowledged frame stays pending and is never confirmed. */
    uint32_t tx_confirmed;         /* requests whose TXBTO bit was observed */
    uint32_t tx_pending;           /* requests not (yet) confirmed, 0..3 */
    uint32_t tx_unconfirmed_reuse; /* buffer requested again although its previous request was never
                                      seen confirmed (only after a cancellation / controller reset; 0 expected) */
} gbt_port_g474_diag_t;

struct gbt_hal {
    uint8_t               lifecycle;      /* GBT_HAL_LC_* (main) */
    FDCAN_HandleTypeDef  *hfdcan;
    uint32_t              filter_id;      /* (own_sa << 8) | peer_sa */
    gbt_spsc_t            rxq;            /* ISR -> main */
    uint8_t               rx_scratch[GBT_PORT_G474_RX_SCRATCH];   /* ISR-owned */
    volatile uint32_t     rx_overflow_hw; /* ISR-owned: = rx_fifo_loss_events */
    volatile uint32_t     rx_invalid;     /* ISR-owned: sum of rx_fd..rx_hal_error */
    volatile gbt_port_g474_diag_t diag;   /* ownership per field, see above */
    uint32_t              tx_error;       /* main-owned: tx_bus_off + tx_hal_error */
    uint32_t              tx_wait_mask;   /* main-owned: TX buffers requested and not yet confirmed (bit n = buffer n) */
    uint32_t              accepted_seq;   /* main */
    uint8_t               in_sink;        /* main */
    uint32_t              event_count;    /* main */
    uint16_t              last_event_code;
    int32_t               last_event_a;
    int32_t               last_event_b;
    gbt_output_sink_t     sink;           /* main */
};

typedef struct {
    FDCAN_HandleTypeDef *hfdcan;          /* &hfdcan1 after MX_FDCAN1_Init() */
    uint8_t              own_sa;          /* 0x56 charger */
    uint8_t              peer_sa;         /* 0xF4 vehicle */
} gbt_port_g474_cfg_t;

/*
 * UNINIT -> READY. Order: RX queue init, extended mask filter (index 0),
 * global filter (reject non-matching and remote), notifications (RX FIFO 0
 * new message / message lost, bus-off, error passive, warning), FDCAN start.
 * E_PARAM: bad argument; E_STATE: not UNINIT or another instance active;
 * E_IO: a HAL call failed (FDCAN may be partly configured; not started
 * unless the failing call was HAL_FDCAN_Start itself).
 */
gbt_hal_status_t gbt_port_g474_init(gbt_hal_t *h, const gbt_port_g474_cfg_t *cfg);
/* READY -> UNINIT: deactivate notifications, stop FDCAN. */
gbt_hal_status_t gbt_port_g474_deinit(gbt_hal_t *h);
/* Main-context snapshot of the diagnostic counters. */
gbt_hal_status_t gbt_port_g474_get_diag(const gbt_hal_t *h, gbt_port_g474_diag_t *out);

#endif /* GBT_PORT_G474_H */
