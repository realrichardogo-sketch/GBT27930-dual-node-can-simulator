/*
 * tests/support/fake_stm32f1/stm32f1xx_hal.h - HOST TEST FAKE of the subset of the STM32F1 HAL
 * (driver 1.1.10, CMSIS stm32f103xe.h) used by ports/stm32/f103/port_f103.c.
 *
 * Not vendor code. Type names, field names, prototypes and constant VALUES are copied from
 *   Drivers/STM32F1xx_HAL_Driver/Inc/stm32f1xx_hal_can.h, stm32f1xx_hal_def.h,
 *   Drivers/CMSIS/Device/ST/STM32F1xx/Include/stm32f103xe.h
 * of the official F103 project, and checked by tests/stm32/check_f1_constants.c (compiled
 * against this fake and against the real headers by tools/check_cross_f103.sh).
 * CAN_TypeDef keeps the register NAMES the port uses (TSR, ESR, RF0R, IER, sFIFOMailBox[].RDTR);
 * reserved gaps are omitted. Behaviour: tests/support/fake_stm32f1/fake_stm32f1.c.
 */
#ifndef FAKE_STM32F1XX_HAL_H
#define FAKE_STM32F1XX_HAL_H

#include <stdint.h>

typedef enum { HAL_OK = 0x00U, HAL_ERROR = 0x01U, HAL_BUSY = 0x02U, HAL_TIMEOUT = 0x03U } HAL_StatusTypeDef;
typedef enum { DISABLE = 0U, ENABLE = !DISABLE } FunctionalState;

typedef enum {
    HAL_CAN_STATE_RESET = 0x00U, HAL_CAN_STATE_READY = 0x01U, HAL_CAN_STATE_LISTENING = 0x02U,
    HAL_CAN_STATE_SLEEP_PENDING = 0x03U, HAL_CAN_STATE_SLEEP_ACTIVE = 0x04U, HAL_CAN_STATE_ERROR = 0x05U
} HAL_CAN_StateTypeDef;

typedef struct {
    volatile uint32_t RIR;
    volatile uint32_t RDTR;
    volatile uint32_t RDLR;
    volatile uint32_t RDHR;
} CAN_FIFOMailBox_TypeDef;

typedef struct {
    volatile uint32_t MCR;
    volatile uint32_t MSR;
    volatile uint32_t TSR;
    volatile uint32_t RF0R;
    volatile uint32_t RF1R;
    volatile uint32_t IER;
    volatile uint32_t ESR;
    volatile uint32_t BTR;
    CAN_FIFOMailBox_TypeDef sFIFOMailBox[2];
} CAN_TypeDef;

typedef struct {
    CAN_TypeDef *Instance;
    volatile HAL_CAN_StateTypeDef State;
    volatile uint32_t ErrorCode;
} CAN_HandleTypeDef;

typedef struct {
    uint32_t FilterIdHigh;
    uint32_t FilterIdLow;
    uint32_t FilterMaskIdHigh;
    uint32_t FilterMaskIdLow;
    uint32_t FilterFIFOAssignment;
    uint32_t FilterBank;
    uint32_t FilterMode;
    uint32_t FilterScale;
    uint32_t FilterActivation;
    uint32_t SlaveStartFilterBank;
} CAN_FilterTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    FunctionalState TransmitGlobalTime;
} CAN_TxHeaderTypeDef;

typedef struct {
    uint32_t StdId;
    uint32_t ExtId;
    uint32_t IDE;
    uint32_t RTR;
    uint32_t DLC;
    uint32_t Timestamp;
    uint32_t FilterMatchIndex;
} CAN_RxHeaderTypeDef;

/* HAL CAN constants (stm32f1xx_hal_can.h) */
#define HAL_CAN_ERROR_NONE           (0x00000000U)
#define HAL_CAN_ERROR_RX_FOV0        (0x00000200U)
#define HAL_CAN_ERROR_NOT_INITIALIZED (0x00040000U)
#define HAL_CAN_ERROR_PARAM          (0x00200000U)
#define CAN_ID_STD                   (0x00000000U)
#define CAN_ID_EXT                   (0x00000004U)
#define CAN_RTR_DATA                 (0x00000000U)
#define CAN_RTR_REMOTE               (0x00000002U)
#define CAN_RX_FIFO0                 (0x00000000U)
#define CAN_FILTER_FIFO0             (0x00000000U)
#define CAN_FILTERMODE_IDMASK        (0x00000000U)
#define CAN_FILTERSCALE_32BIT        (0x00000001U)
#define CAN_FILTER_ENABLE            (0x00000001U)
#define CAN_TX_MAILBOX0              (0x00000001U)
#define CAN_TX_MAILBOX1              (0x00000002U)
#define CAN_TX_MAILBOX2              (0x00000004U)

/* CMSIS register bits (stm32f103xe.h) */
#define CAN_IER_FMPIE0   (0x00000002UL)
#define CAN_IER_FFIE0    (0x00000004UL)
#define CAN_IER_FOVIE0   (0x00000008UL)
#define CAN_IT_RX_FIFO0_MSG_PENDING ((uint32_t)CAN_IER_FMPIE0)
#define CAN_IT_RX_FIFO0_FULL        ((uint32_t)CAN_IER_FFIE0)
#define CAN_IT_RX_FIFO0_OVERRUN     ((uint32_t)CAN_IER_FOVIE0)
#define CAN_TSR_RQCP0    (0x00000001UL)
#define CAN_TSR_TXOK0    (0x00000002UL)
#define CAN_TSR_ALST0    (0x00000004UL)
#define CAN_TSR_TERR0    (0x00000008UL)
#define CAN_TSR_RQCP1    (0x00000100UL)
#define CAN_TSR_TXOK1    (0x00000200UL)
#define CAN_TSR_ALST1    (0x00000400UL)
#define CAN_TSR_TERR1    (0x00000800UL)
#define CAN_TSR_RQCP2    (0x00010000UL)
#define CAN_TSR_TXOK2    (0x00020000UL)
#define CAN_TSR_ALST2    (0x00040000UL)
#define CAN_TSR_TERR2    (0x00080000UL)
#define CAN_TSR_TME0     (0x04000000UL)
#define CAN_TSR_TME1     (0x08000000UL)
#define CAN_TSR_TME2     (0x10000000UL)
#define CAN_RF0R_FMP0    (0x00000003UL)
#define CAN_RF0R_FULL0   (0x00000008UL)
#define CAN_RF0R_FOVR0   (0x00000010UL)
#define CAN_RDT0R_DLC_Pos (0U)
#define CAN_RDT0R_DLC    (0x0000000FUL)
#define CAN_ESR_EWGF     (0x00000001UL)
#define CAN_ESR_EPVF     (0x00000002UL)
#define CAN_ESR_BOFF     (0x00000004UL)
#define CAN_ESR_TEC_Pos  (16U)
#define CAN_ESR_TEC      (0x00FF0000UL)
#define CAN_ESR_REC_Pos  (24U)
#define CAN_ESR_REC      (0xFF000000UL)

uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_CAN_ConfigFilter(CAN_HandleTypeDef *hcan, const CAN_FilterTypeDef *sFilterConfig);
HAL_StatusTypeDef HAL_CAN_Start(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_Stop(CAN_HandleTypeDef *hcan);
HAL_StatusTypeDef HAL_CAN_AddTxMessage(CAN_HandleTypeDef *hcan, const CAN_TxHeaderTypeDef *pHeader,
                                       const uint8_t aData[], uint32_t *pTxMailbox);
uint32_t HAL_CAN_GetTxMailboxesFreeLevel(const CAN_HandleTypeDef *hcan);
uint32_t HAL_CAN_IsTxMessagePending(const CAN_HandleTypeDef *hcan, uint32_t TxMailboxes);
HAL_StatusTypeDef HAL_CAN_GetRxMessage(CAN_HandleTypeDef *hcan, uint32_t RxFifo,
                                       CAN_RxHeaderTypeDef *pHeader, uint8_t aData[]);
uint32_t HAL_CAN_GetRxFifoFillLevel(const CAN_HandleTypeDef *hcan, uint32_t RxFifo);
HAL_StatusTypeDef HAL_CAN_ActivateNotification(CAN_HandleTypeDef *hcan, uint32_t ActiveITs);
HAL_StatusTypeDef HAL_CAN_DeactivateNotification(CAN_HandleTypeDef *hcan, uint32_t InactiveITs);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan);

#endif /* FAKE_STM32F1XX_HAL_H */
