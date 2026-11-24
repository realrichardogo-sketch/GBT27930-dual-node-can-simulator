/*
 * tests/stm32/check_f1_constants.c - compile-time proof that the STM32F1 HAL / CMSIS constants
 * used by ports/stm32/f103 have the expected values. Compiled against the host fake (CTest
 * test_port_f103) AND against the real F103 project headers (tools/check_cross_f103.sh):
 * both must compile, so fake == real. C99: negative array size on mismatch.
 */
#include "stm32f1xx_hal.h"

#define F1_CHECK(tag, expr) typedef char f1_check_##tag[(expr) ? 1 : -1]

F1_CHECK(hal_ok,         HAL_OK == 0);
F1_CHECK(hal_error,      HAL_ERROR == 1);
F1_CHECK(st_ready,       HAL_CAN_STATE_READY == 1);
F1_CHECK(st_listening,   HAL_CAN_STATE_LISTENING == 2);
F1_CHECK(disable,        DISABLE == 0);
F1_CHECK(err_none,       HAL_CAN_ERROR_NONE == 0x00000000UL);
F1_CHECK(err_fov0,       HAL_CAN_ERROR_RX_FOV0 == 0x00000200UL);
F1_CHECK(err_notinit,    HAL_CAN_ERROR_NOT_INITIALIZED == 0x00040000UL);
F1_CHECK(err_param,      HAL_CAN_ERROR_PARAM == 0x00200000UL);
F1_CHECK(id_std,         CAN_ID_STD == 0x0UL);
F1_CHECK(id_ext,         CAN_ID_EXT == 0x4UL);
F1_CHECK(rtr_data,       CAN_RTR_DATA == 0x0UL);
F1_CHECK(rtr_remote,     CAN_RTR_REMOTE == 0x2UL);
F1_CHECK(rx_fifo0,       CAN_RX_FIFO0 == 0x0UL);
F1_CHECK(filter_fifo0,   CAN_FILTER_FIFO0 == 0x0UL);
F1_CHECK(fmode_idmask,   CAN_FILTERMODE_IDMASK == 0x0UL);
F1_CHECK(fscale_32,      CAN_FILTERSCALE_32BIT == 0x1UL);
F1_CHECK(filter_enable,  CAN_FILTER_ENABLE == 0x1UL);
F1_CHECK(mb0,            CAN_TX_MAILBOX0 == 0x1UL);
F1_CHECK(mb1,            CAN_TX_MAILBOX1 == 0x2UL);
F1_CHECK(mb2,            CAN_TX_MAILBOX2 == 0x4UL);
F1_CHECK(it_fmp0,        CAN_IT_RX_FIFO0_MSG_PENDING == 0x2UL);
F1_CHECK(it_full0,       CAN_IT_RX_FIFO0_FULL == 0x4UL);
F1_CHECK(it_fov0,        CAN_IT_RX_FIFO0_OVERRUN == 0x8UL);
F1_CHECK(rqcp0,          CAN_TSR_RQCP0 == 0x00000001UL);
F1_CHECK(txok0,          CAN_TSR_TXOK0 == 0x00000002UL);
F1_CHECK(alst0,          CAN_TSR_ALST0 == 0x00000004UL);
F1_CHECK(terr0,          CAN_TSR_TERR0 == 0x00000008UL);
F1_CHECK(rqcp1,          CAN_TSR_RQCP1 == 0x00000100UL);
F1_CHECK(txok1,          CAN_TSR_TXOK1 == 0x00000200UL);
F1_CHECK(rqcp2,          CAN_TSR_RQCP2 == 0x00010000UL);
F1_CHECK(txok2,          CAN_TSR_TXOK2 == 0x00020000UL);
F1_CHECK(tme0,           CAN_TSR_TME0 == 0x04000000UL);
F1_CHECK(tme1,           CAN_TSR_TME1 == 0x08000000UL);
F1_CHECK(tme2,           CAN_TSR_TME2 == 0x10000000UL);
F1_CHECK(fmp0,           CAN_RF0R_FMP0 == 0x00000003UL);
F1_CHECK(full0,          CAN_RF0R_FULL0 == 0x00000008UL);
F1_CHECK(fovr0,          CAN_RF0R_FOVR0 == 0x00000010UL);
F1_CHECK(rdt_dlc,        CAN_RDT0R_DLC == 0x0000000FUL);
F1_CHECK(rdt_dlc_pos,    CAN_RDT0R_DLC_Pos == 0U);
F1_CHECK(ewgf,           CAN_ESR_EWGF == 0x00000001UL);
F1_CHECK(epvf,           CAN_ESR_EPVF == 0x00000002UL);
F1_CHECK(boff,           CAN_ESR_BOFF == 0x00000004UL);
F1_CHECK(tec,            CAN_ESR_TEC == 0x00FF0000UL);
F1_CHECK(tec_pos,        CAN_ESR_TEC_Pos == 16U);
F1_CHECK(rec,            CAN_ESR_REC == 0xFF000000UL);
F1_CHECK(rec_pos,        CAN_ESR_REC_Pos == 24U);
