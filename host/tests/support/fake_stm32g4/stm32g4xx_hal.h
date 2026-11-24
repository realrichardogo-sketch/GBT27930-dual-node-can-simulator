/*
 * tests/support/fake_stm32g4/stm32g4xx_hal.h - HOST TEST FAKE of the subset
 * of the STM32G4 HAL (driver 1.2.4) used by ports/stm32/g474/port_g474.c.
 *
 * Not vendor code. Type names, field names, prototypes and constant VALUES
 * are copied from the reference project's
 *   Drivers/STM32G4xx_HAL_Driver/Inc/stm32g4xx_hal_fdcan.h, stm32g4xx_hal_def.h
 *   Drivers/CMSIS/Device/ST/STM32G4xx/Include/stm32g474xx.h (FDCAN_IE_* bits)
 * and checked by tests/stm32/check_g4_constants.c, which is compiled against
 * this fake (host CTest) and against the real headers (tools/check_cross_g474.sh).
 * FDCAN_HandleTypeDef is reduced to the fields the fake needs.
 * Behaviour of the fake functions: tests/support/fake_stm32g4/fake_stm32g4.c.
 */
#ifndef FAKE_STM32G4XX_HAL_H
#define FAKE_STM32G4XX_HAL_H

#include <stdint.h>

typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef enum {
    HAL_FDCAN_STATE_RESET = 0x00U,
    HAL_FDCAN_STATE_READY = 0x01U,
    HAL_FDCAN_STATE_BUSY  = 0x02U,
    HAL_FDCAN_STATE_ERROR = 0x03U
} HAL_FDCAN_StateTypeDef;

/* STEP 07-C2.4: reduced register block (only the registers the port reads directly). */
typedef struct {
    volatile uint32_t TXBRP;          /* Tx Buffer Request Pending */
    volatile uint32_t TXBTO;          /* Tx Buffer Transmission Occurred */
} FDCAN_GlobalTypeDef;

typedef struct {
    FDCAN_GlobalTypeDef *Instance;
    volatile HAL_FDCAN_StateTypeDef State;
    uint32_t LatestTxFifoQRequest;    /* as the real handle: 1 << PutIndex of the last request */
    volatile uint32_t ErrorCode;
} FDCAN_HandleTypeDef;

typedef struct {
    uint32_t IdType;
    uint32_t FilterIndex;
    uint32_t FilterType;
    uint32_t FilterConfig;
    uint32_t FilterID1;
    uint32_t FilterID2;
} FDCAN_FilterTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t TxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t TxEventFifoControl;
    uint32_t MessageMarker;
} FDCAN_TxHeaderTypeDef;

typedef struct {
    uint32_t Identifier;
    uint32_t IdType;
    uint32_t RxFrameType;
    uint32_t DataLength;
    uint32_t ErrorStateIndicator;
    uint32_t BitRateSwitch;
    uint32_t FDFormat;
    uint32_t RxTimestamp;
    uint32_t FilterIndex;
    uint32_t IsFilterMatchingFrame;
} FDCAN_RxHeaderTypeDef;

typedef struct {
    uint32_t LastErrorCode;
    uint32_t DataLastErrorCode;
    uint32_t Activity;
    uint32_t ErrorPassive;
    uint32_t Warning;
    uint32_t BusOff;
    uint32_t RxESIflag;
    uint32_t RxBRSflag;
    uint32_t RxFDFflag;
    uint32_t ProtocolException;
    uint32_t TDCvalue;
} FDCAN_ProtocolStatusTypeDef;

typedef struct {
    uint32_t TxErrorCnt;
    uint32_t RxErrorCnt;
    uint32_t RxErrorPassive;
    uint32_t ErrorLogging;
} FDCAN_ErrorCountersTypeDef;

#define HAL_FDCAN_ERROR_NONE         ((uint32_t)0x00000000U)
#define HAL_FDCAN_ERROR_NOT_READY    ((uint32_t)0x00000004U)
#define HAL_FDCAN_ERROR_NOT_STARTED  ((uint32_t)0x00000008U)
#define HAL_FDCAN_ERROR_FIFO_EMPTY   ((uint32_t)0x00000100U)
#define HAL_FDCAN_ERROR_FIFO_FULL    ((uint32_t)0x00000200U)

#define FDCAN_STANDARD_ID            ((uint32_t)0x00000000U)
#define FDCAN_EXTENDED_ID            ((uint32_t)0x40000000U)
#define FDCAN_DATA_FRAME             ((uint32_t)0x00000000U)
#define FDCAN_REMOTE_FRAME           ((uint32_t)0x20000000U)
#define FDCAN_DLC_BYTES_0            ((uint32_t)0x00000000U)
#define FDCAN_DLC_BYTES_1            ((uint32_t)0x00000001U)
#define FDCAN_DLC_BYTES_2            ((uint32_t)0x00000002U)
#define FDCAN_DLC_BYTES_3            ((uint32_t)0x00000003U)
#define FDCAN_DLC_BYTES_4            ((uint32_t)0x00000004U)
#define FDCAN_DLC_BYTES_5            ((uint32_t)0x00000005U)
#define FDCAN_DLC_BYTES_6            ((uint32_t)0x00000006U)
#define FDCAN_DLC_BYTES_7            ((uint32_t)0x00000007U)
#define FDCAN_DLC_BYTES_8            ((uint32_t)0x00000008U)
#define FDCAN_ESI_ACTIVE             ((uint32_t)0x00000000U)
#define FDCAN_ESI_PASSIVE            ((uint32_t)0x80000000U)
#define FDCAN_BRS_OFF                ((uint32_t)0x00000000U)
#define FDCAN_BRS_ON                 ((uint32_t)0x00100000U)
#define FDCAN_CLASSIC_CAN            ((uint32_t)0x00000000U)
#define FDCAN_FD_CAN                 ((uint32_t)0x00200000U)
#define FDCAN_NO_TX_EVENTS           ((uint32_t)0x00000000U)
#define FDCAN_FILTER_RANGE           ((uint32_t)0x00000000U)
#define FDCAN_FILTER_DUAL            ((uint32_t)0x00000001U)
#define FDCAN_FILTER_MASK            ((uint32_t)0x00000002U)
#define FDCAN_FILTER_TO_RXFIFO0      ((uint32_t)0x00000001U)
#define FDCAN_ACCEPT_IN_RX_FIFO0     ((uint32_t)0x00000000U)
#define FDCAN_REJECT                 ((uint32_t)0x00000002U)
#define FDCAN_REJECT_REMOTE          ((uint32_t)0x00000001U)
#define FDCAN_RX_FIFO0               ((uint32_t)0x00000040U)
/* FDCAN_IT_* = FDCAN_IE_* bit values from stm32g474xx.h (same positions in IR) */
#define FDCAN_IT_RX_FIFO0_NEW_MESSAGE  ((uint32_t)0x00000001U)
#define FDCAN_IT_RX_FIFO0_FULL         ((uint32_t)0x00000002U)
#define FDCAN_IT_RX_FIFO0_MESSAGE_LOST ((uint32_t)0x00000004U)
#define FDCAN_IT_ERROR_PASSIVE         ((uint32_t)0x00020000U)
#define FDCAN_IT_ERROR_WARNING         ((uint32_t)0x00040000U)
#define FDCAN_IT_BUS_OFF               ((uint32_t)0x00080000U)

uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_FDCAN_ConfigFilter(FDCAN_HandleTypeDef *hfdcan, const FDCAN_FilterTypeDef *sFilterConfig);
HAL_StatusTypeDef HAL_FDCAN_ConfigGlobalFilter(FDCAN_HandleTypeDef *hfdcan, uint32_t NonMatchingStd,
                                               uint32_t NonMatchingExt, uint32_t RejectRemoteStd,
                                               uint32_t RejectRemoteExt);
HAL_StatusTypeDef HAL_FDCAN_Start(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_Stop(FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_AddMessageToTxFifoQ(FDCAN_HandleTypeDef *hfdcan, const FDCAN_TxHeaderTypeDef *pTxHeader,
                                                const uint8_t *pTxData);
HAL_StatusTypeDef HAL_FDCAN_GetRxMessage(FDCAN_HandleTypeDef *hfdcan, uint32_t RxLocation,
                                         FDCAN_RxHeaderTypeDef *pRxHeader, uint8_t *pRxData);
HAL_StatusTypeDef HAL_FDCAN_GetProtocolStatus(const FDCAN_HandleTypeDef *hfdcan,
                                              FDCAN_ProtocolStatusTypeDef *ProtocolStatus);
HAL_StatusTypeDef HAL_FDCAN_GetErrorCounters(const FDCAN_HandleTypeDef *hfdcan,
                                             FDCAN_ErrorCountersTypeDef *ErrorCounters);
uint32_t HAL_FDCAN_GetRxFifoFillLevel(const FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo);
uint32_t HAL_FDCAN_GetTxFifoFreeLevel(const FDCAN_HandleTypeDef *hfdcan);
uint32_t HAL_FDCAN_GetLatestTxFifoQRequestBuffer(const FDCAN_HandleTypeDef *hfdcan);
HAL_StatusTypeDef HAL_FDCAN_ActivateNotification(FDCAN_HandleTypeDef *hfdcan, uint32_t ActiveITs,
                                                 uint32_t BufferIndexes);
HAL_StatusTypeDef HAL_FDCAN_DeactivateNotification(FDCAN_HandleTypeDef *hfdcan, uint32_t InactiveITs);
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs);
void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan);
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs);

#endif /* FAKE_STM32G4XX_HAL_H */
