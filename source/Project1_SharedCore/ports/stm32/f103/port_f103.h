/*
 * ports/stm32/f103/port_f103.h - STM32F103 bxCAN (CAN1) implementation of hal/hal.h (ND-1).
 * STEP 07-C2.1. Audit basis: STEP 07-C2.0 F103 pre-implementation audit; STM32F1 HAL 1.1.10.
 *
 * Exactly one instance per executable (it owns the HAL CAN callbacks).
 * Classical CAN 2.0B, 29-bit identifiers. Vehicle node: accepts PDU1 frames addressed to
 * own_sa from peer_sa (filter bank 0), everything else is rejected by the controller.
 *
 * Owner decisions (final):
 *  D1  TX: exactly ONE transmit mailbox in flight. gbt_hal_can_send() returns GBT_HAL_E_FULL
 *      while the previous frame's mailbox has not completed; the caller keeps the frame and
 *      retries it first (ND-1 / app/gbt_integration single pending frame). TXFP and every
 *      other bxCAN setting stay as generated (TXFP = 0: identifier-priority arbitration
 *      between mailboxes would otherwise reorder frames).
 *  D2  RX: the original DLC is read from FIFO0 RDTR before HAL_CAN_GetRxMessage() (which
 *      truncates DLC >= 8 to 8); frames with DLC > 8 are released and counted, never queued.
 *
 * Contexts:
 *   USB_LP_CAN1_RX0 IRQ (HAL_CAN_IRQHandler -> HAL_CAN_RxFifo0MsgPendingCallback /
 *     HAL_CAN_ErrorCallback below): sole PRODUCER of the RX queue; writes ISR-owned counters.
 *   Main loop: every gbt_hal_* function; sole CONSUMER; owns the TX mailbox state and the
 *     TSR request-completed flags (no TX interrupt is enabled in the project).
 * The CAN SCE (status/error) interrupt is not enabled in the project: bus-off, error-passive
 * and warning are read from ESR in the main loop.
 *
 * Requires MX_CAN_Init() to have run and CAN1 NOT started (HAL state READY):
 * gbt_port_f103_init() configures the filter, enables the notifications and starts CAN1.
 * Must not include app/ or protocol/.
 */
#ifndef GBT_PORT_F103_H
#define GBT_PORT_F103_H

#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "hal/hal.h"
#include "ports/common/gbt_hal_lc.h"
#include "ports/common/gbt_spsc.h"

/* RX FIFO0 depth on bxCAN (3 mailboxes); frames drained per callback. */
#define GBT_PORT_F103_RX_DRAIN_MAX 3u

/* Diagnostics beyond gbt_can_status_t. ISR-owned: rx_*, hal_error_callbacks.
 * Main-owned: tx_*, bus_* polls. All saturating. */
typedef struct {
    uint32_t rx_frames;            /* accepted and queued */
    uint32_t rx_bad_dlc;           /* rejected: original RDTR DLC > 8 (D2) */
    uint32_t rx_remote;            /* rejected: RTR frame */
    uint32_t rx_std_id;            /* rejected: 11-bit identifier */
    uint32_t rx_hal_error;         /* HAL_CAN_GetRxMessage() != HAL_OK */
    uint32_t rx_fifo_overruns;     /* FOV0 events (HAL_CAN_ERROR_RX_FOV0), not frames */
    uint32_t hal_error_callbacks;  /* HAL_CAN_ErrorCallback() invocations */
    uint32_t tx_frames;            /* accepted by HAL_CAN_AddTxMessage */
    uint32_t tx_completed_ok;      /* mailbox completed with TXOK = 1 */
    uint32_t tx_completed_fail;    /* mailbox completed with TXOK = 0 (aborted/failed) */
    uint32_t tx_busy;              /* E_FULL returned: previous mailbox still in flight (D1) */
    uint32_t tx_bus_off;           /* E_BUS_OFF returned */
    uint32_t tx_hal_error;         /* AddTxMessage failed */
    uint32_t last_tx_hal_status;   /* HAL_StatusTypeDef of the last AddTxMessage call */
    uint32_t tx_in_flight;         /* 1 while a mailbox is pending */
    uint32_t tx_in_flight_ms;      /* age of the pending mailbox at the last poll */
    /* STEP 07-C2.4 error-state ENTRY events, main-owned. The SCE interrupt is not enabled, so they
     * are detected by polling ESR in gbt_hal_can_get_status() (called on every integration step,
     * i.e. every main-loop pass): a state lasting shorter than one pass can be missed. Each counts
     * transitions INTO the state or a worse one (warning < passive < bus-off). */
    uint32_t warning_events;
    uint32_t error_passive_events;
    uint32_t bus_off_events;
    uint32_t last_polled_state;    /* GBT_CAN_STATE_* of the last poll (0 = not polled yet) */
} gbt_port_f103_diag_t;

struct gbt_hal {
    uint8_t               lifecycle;      /* GBT_HAL_LC_* (main) */
    CAN_HandleTypeDef    *hcan;
    uint32_t              filter_id;      /* (own_sa << 8) | peer_sa */
    gbt_spsc_t            rxq;            /* ISR -> main */
    volatile uint32_t     rx_overflow_hw; /* ISR-owned: = rx_fifo_overruns */
    volatile uint32_t     rx_invalid;     /* ISR-owned: sum of rx_bad_dlc..rx_hal_error */
    volatile gbt_port_f103_diag_t diag;   /* ownership per field, see above */
    uint32_t              tx_error;       /* main-owned: tx_bus_off + tx_hal_error + tx_completed_fail */
    uint32_t              tx_mailbox;     /* main: CAN_TX_MAILBOXx of the frame in flight, 0 = none */
    uint32_t              tx_start_ms;    /* main: HAL_GetTick() at submission */
    uint32_t              accepted_seq;   /* main */
    uint8_t               in_sink;        /* main */
    uint32_t              event_count;    /* main */
    uint16_t              last_event_code;
    int32_t               last_event_a;
    int32_t               last_event_b;
    gbt_output_sink_t     sink;           /* main */
};

typedef struct {
    CAN_HandleTypeDef *hcan;              /* &hcan after MX_CAN_Init() */
    uint8_t            own_sa;            /* 0xF4 vehicle */
    uint8_t            peer_sa;           /* 0x56 charger */
} gbt_port_f103_cfg_t;

/* bxCAN 32-bit ID/mask filter words for a 29-bit identifier pattern (exposed for tests):
 * register layout EXID[28:0] at bits 31..3, IDE bit 2, RTR bit 1. */
#define GBT_PORT_F103_FILTER_ID(id29)   ((((uint32_t)(id29) & 0x1FFFFFFFu) << 3) | 0x4u)
#define GBT_PORT_F103_FILTER_MASK(m29)  ((((uint32_t)(m29) & 0x1FFFFFFFu) << 3) | 0x4u | 0x2u)
#define GBT_PORT_F103_ADDR_MASK         0x0000FFFFu   /* PS (bits 15..8) and SA (bits 7..0) */

/*
 * UNINIT -> READY. Order: RX queue init, filter bank 0 (32-bit ID/mask, IDE = 1, RTR = 0,
 * ID bits 15..0 = (own_sa << 8) | peer_sa) to FIFO0, notifications (FIFO0 message pending,
 * FIFO0 overrun), HAL_CAN_Start (bounded HAL init wait, initialisation only).
 * E_PARAM bad argument; E_STATE not UNINIT or another instance active; E_IO a HAL call failed.
 */
gbt_hal_status_t gbt_port_f103_init(gbt_hal_t *h, const gbt_port_f103_cfg_t *cfg);
/* READY -> UNINIT: deactivate notifications, stop CAN1. */
gbt_hal_status_t gbt_port_f103_deinit(gbt_hal_t *h);
/* Main-context snapshot of the diagnostic counters (also polls TX completion). */
gbt_hal_status_t gbt_port_f103_get_diag(gbt_hal_t *h, gbt_port_f103_diag_t *out);

#endif /* GBT_PORT_F103_H */
