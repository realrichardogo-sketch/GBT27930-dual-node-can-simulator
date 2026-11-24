/*
 * tests/stm32/check_g4_constants.c - compile-time proof that the STM32G4 HAL
 * constants used by ports/stm32/g474 have the expected values. Compiled
 * against the host fake (CTest g474_fake_constants) AND against the real
 * reference-project headers (tools/check_cross_g474.sh): both must compile,
 * so fake == real for every value below. C99: negative array size on mismatch.
 */
#include "stm32g4xx_hal.h"

#define G4_CHECK(tag, expr) typedef char g4_check_##tag[(expr) ? 1 : -1]

G4_CHECK(hal_ok,        HAL_OK == 0);
G4_CHECK(hal_error,     HAL_ERROR == 1);
G4_CHECK(st_ready,      HAL_FDCAN_STATE_READY == 1);
G4_CHECK(st_busy,       HAL_FDCAN_STATE_BUSY == 2);
G4_CHECK(err_none,      HAL_FDCAN_ERROR_NONE == 0x00000000UL);
G4_CHECK(err_fifo_full, HAL_FDCAN_ERROR_FIFO_FULL == 0x00000200UL);
G4_CHECK(err_fifo_emp,  HAL_FDCAN_ERROR_FIFO_EMPTY == 0x00000100UL);
G4_CHECK(std_id,        FDCAN_STANDARD_ID == 0x00000000UL);
G4_CHECK(ext_id,        FDCAN_EXTENDED_ID == 0x40000000UL);
G4_CHECK(data_frame,    FDCAN_DATA_FRAME == 0x00000000UL);
G4_CHECK(remote_frame,  FDCAN_REMOTE_FRAME == 0x20000000UL);
G4_CHECK(dlc0,          FDCAN_DLC_BYTES_0 == 0UL);
G4_CHECK(dlc1,          FDCAN_DLC_BYTES_1 == 1UL);
G4_CHECK(dlc2,          FDCAN_DLC_BYTES_2 == 2UL);
G4_CHECK(dlc3,          FDCAN_DLC_BYTES_3 == 3UL);
G4_CHECK(dlc4,          FDCAN_DLC_BYTES_4 == 4UL);
G4_CHECK(dlc5,          FDCAN_DLC_BYTES_5 == 5UL);
G4_CHECK(dlc6,          FDCAN_DLC_BYTES_6 == 6UL);
G4_CHECK(dlc7,          FDCAN_DLC_BYTES_7 == 7UL);
G4_CHECK(dlc8,          FDCAN_DLC_BYTES_8 == 8UL);
G4_CHECK(esi_active,    FDCAN_ESI_ACTIVE == 0x00000000UL);
G4_CHECK(esi_passive,   FDCAN_ESI_PASSIVE == 0x80000000UL);
G4_CHECK(brs_off,       FDCAN_BRS_OFF == 0x00000000UL);
G4_CHECK(brs_on,        FDCAN_BRS_ON == 0x00100000UL);
G4_CHECK(classic,       FDCAN_CLASSIC_CAN == 0x00000000UL);
G4_CHECK(fd,            FDCAN_FD_CAN == 0x00200000UL);
G4_CHECK(no_tx_events,  FDCAN_NO_TX_EVENTS == 0x00000000UL);
G4_CHECK(f_range,       FDCAN_FILTER_RANGE == 0UL);
G4_CHECK(f_dual,        FDCAN_FILTER_DUAL == 1UL);
G4_CHECK(f_mask,        FDCAN_FILTER_MASK == 2UL);
G4_CHECK(f_fifo0,       FDCAN_FILTER_TO_RXFIFO0 == 1UL);
G4_CHECK(accept_fifo0,  FDCAN_ACCEPT_IN_RX_FIFO0 == 0UL);
G4_CHECK(reject,        FDCAN_REJECT == 2UL);
G4_CHECK(reject_remote, FDCAN_REJECT_REMOTE == 1UL);
G4_CHECK(rx_fifo0,      FDCAN_RX_FIFO0 == 0x00000040UL);
G4_CHECK(it_rf0n,       FDCAN_IT_RX_FIFO0_NEW_MESSAGE == 0x00000001UL);
G4_CHECK(it_rf0f,       FDCAN_IT_RX_FIFO0_FULL == 0x00000002UL);
G4_CHECK(it_rf0l,       FDCAN_IT_RX_FIFO0_MESSAGE_LOST == 0x00000004UL);
G4_CHECK(it_ep,         FDCAN_IT_ERROR_PASSIVE == 0x00020000UL);
G4_CHECK(it_ew,         FDCAN_IT_ERROR_WARNING == 0x00040000UL);
G4_CHECK(it_bo,         FDCAN_IT_BUS_OFF == 0x00080000UL);
