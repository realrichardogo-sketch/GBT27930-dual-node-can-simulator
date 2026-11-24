/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fdcan.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app/gbt_integration.h"   /* STEP 07-C1.2-B2: GB/T 27930 protocol integration */
#include "app/sim_plant.h"
#include "ports/stm32/g474/port_g474.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* STEP 07-C2.4 structured Keil Watch diagnostics. Every field is uint32_t; written only by
 * gbt_watch_update() (main loop, every 100 ms, never from an ISR); never read by the firmware.
 * Meanings and pass values: WATCH_VARIABLES.md of the STEP07_C2.4 package. */
typedef struct {
  uint32_t now_ms;                /* HAL_GetTick() at this refresh */
  uint32_t tp_role;               /* 1 = TP sender (vehicle F103 B), 2 = TP receiver (charger G474) */
  uint32_t tx_started;            /* send sessions opened (RTS queued) */
  uint32_t tx_completed;          /* send sessions completed (EOM_ACK accepted) */
  uint32_t rx_started;            /* receive sessions opened (RTS accepted, CTS queued) */
  uint32_t rx_completed;          /* receive sessions completed (last DT accepted, EOM_ACK queued) */
  uint32_t brm_completed;         /* completed sessions of this node's TP direction, per message */
  uint32_t bcp_completed;
  uint32_t bcs_completed;
  uint32_t other_completed;       /* any other PGN (expected 0) */
  uint32_t last_tx_pgn;           /* PGN / length of the current or last send session */
  uint32_t last_tx_len;
  uint32_t last_rx_pgn;           /* PGN / length of the current or last receive session */
  uint32_t last_rx_len;
  uint32_t tx_expected_packets;   /* DT packets of the current / last send session */
  uint32_t tx_actual_packets;     /* DTs queued in the current / last send session */
  uint32_t rx_expected_packets;   /* DT packets announced by the current / last RTS */
  uint32_t rx_actual_packets;     /* DTs accepted in the current / last receive session */
  uint32_t tx_next_seq;           /* next DT sequence number to send */
  uint32_t rx_next_seq;           /* next DT sequence number expected */
  uint32_t tx_state;              /* 0 IDLE, 2 wait CTS/EOM_ACK, 3 sending DT, 4 hold (CTS 0) */
  uint32_t rx_state;              /* 0 IDLE, 1 receiving DT */
  uint32_t aborts_sent;           /* Connection Abort frames queued by this node */
  uint32_t aborts_received;       /* received Connection Aborts that ended a local session */
  uint32_t transport_failures;    /* node: TP sessions ended by abort or timeout */
  uint32_t timeouts_tx;           /* send sessions ended by T3/T4 */
  uint32_t timeouts_rx;           /* receive sessions ended by T1/T2 */
  uint32_t last_end_ev;           /* 1 RX done, 2 RX aborted, 4 TX done, 8 TX aborted (0 none) */
  uint32_t sessions_ended;        /* session ends, both directions */
  uint32_t last_tx_duration_ms;   /* last completed send session: RTS queued -> EOM_ACK accepted */
  uint32_t max_tx_duration_ms;
  uint32_t last_rx_duration_ms;   /* last completed receive session: RTS accepted -> last DT accepted */
  uint32_t max_rx_duration_ms;
  uint32_t max_tx_dt_interval_ms; /* largest gap between consecutive DTs queued in one send session */
  uint32_t max_rx_dt_interval_ms; /* largest gap between consecutive DTs accepted in one receive session */
  uint32_t outbox_full;           /* TP frames that could not be queued */
  uint32_t step_changes;          /* protocol step changes seen by the integration loop */
} gbt_watch_tp_t;

typedef struct {
  uint32_t state;                 /* 1 error active, 2 warning, 3 error passive, 4 bus-off */
  uint32_t tec;
  uint32_t rec;
  uint32_t rx_frames;             /* frames accepted by the filter and queued by the RX ISR */
  uint32_t tx_frames;             /* frames accepted by the driver (TX FIFO element requested) - NOT bus proof */
  uint32_t tx_confirmed;          /* TXBTO observed: frame completed on the bus, i.e. acknowledged by another node */
  uint32_t tx_pending;            /* driver-accepted frames not confirmed yet */
  uint32_t tx_backpressure;       /* E_FULL refusals, frame kept and retried (not a loss) */
  uint32_t rx_overflow_sw;        /* frames lost: software RX queue full */
  uint32_t rx_overflow_hw;        /* RX FIFO 0 message-lost events */
  uint32_t rx_invalid;            /* frames rejected by the RX ISR (format / DLC / id / HAL read) */
  uint32_t hal_errors;            /* HAL error callbacks + HAL RX read errors */
  uint32_t tx_failures;           /* bus-off refusals + HAL TX errors + buffer reuse without confirmation + integration drops */
  uint32_t warning_events;        /* FDCAN status-change interrupts (entry AND exit) */
  uint32_t error_passive_events;
  uint32_t bus_off_events;
} gbt_watch_can_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* GB/T 27930 charger node (STEP 07-C1.2-B2). Static storage only. */
static gbt_hal_t            s_gbt_hal;      /* G474 FDCAN1 adapter instance */
static gbt_node_t           s_gbt_node;     /* protocol node (charger) */
static gbt_integ_t          s_gbt_integ;    /* integration loop: RX, tick, APPLY, SEND */
static output_if_backend_t  s_gbt_plant;    /* Project 1 output backend: software command only */
static gbt_charger_params_t s_gbt_cp;
static gbt_node_config_t    s_gbt_cfg;
static uint32_t             s_gbt_now;      /* non-volatile mirror of HAL_GetTick() for sim_plant */
static uint32_t             s_gbt_diag_ms;
/* Debugger-visible diagnostics, refreshed every 100 ms from the main loop (never from an ISR). */
volatile gbt_can_status_t     g_gbt_can_status;      /* state, TEC/REC, rx_overflow_sw/hw, rx_invalid, tx_error */
volatile gbt_port_g474_diag_t g_gbt_port_diag;       /* per-reason RX rejects, TX FIFO full, bus-off events, ... */
volatile uint8_t              g_gbt_phase;           /* GBT_NODE_PHASE_* */
volatile uint32_t             g_gbt_tx_dropped;      /* integration: frames discarded after a send error (ND-3) */
volatile uint32_t             g_gbt_tx_full_retries; /* integration: E_FULL retries */
volatile uint32_t             g_gbt_rx_overflow;     /* protocol: frames reported lost (T-5) */

/* ---- STEP 07-C2 flat Keil Watch mirrors (g_hw_diag_*) -------------------------------------
 * Plain global scalars: type the name in the Watch window, no file qualifier, no nesting.
 * Written ONLY by gbt_hw_diag_update(), called from gbt_app_diag() in the main loop every
 * 100 ms; never written by an ISR; never read by the firmware. Every value is a copy of an
 * existing source (port diag, HAL status, node status, TP statistics, integration record);
 * meanings and pass/fail values: docs/WATCH_VARIABLES.md. Node role: charger, TP RECEIVER. */
/* A. CAN controller / port (G474 FDCAN1) */
volatile uint32_t g_hw_diag_can_tx_queued;       /* frames accepted into the FDCAN TX FIFO (NOT bus-confirmed) */
volatile uint32_t g_hw_diag_can_tx_backpressure; /* TX FIFO full -> E_FULL, frame kept and retried */
volatile uint32_t g_hw_diag_can_tx_error;        /* HAL tx_error: bus-off refusals + HAL TX errors */
volatile uint32_t g_hw_diag_can_tx_dropped;      /* integration: frames discarded after a send error */
volatile uint32_t g_hw_diag_can_rx_count;        /* frames accepted by the filter and queued by the RX ISR */
volatile uint32_t g_hw_diag_can_rx_invalid;      /* frames rejected by the RX ISR (FD/remote/DLC/std id/HAL) */
volatile uint32_t g_hw_diag_can_rx_overflow_sw;  /* frames lost: software RX queue full */
volatile uint32_t g_hw_diag_can_rx_overflow_hw;  /* RX FIFO 0 message-lost events */
volatile uint32_t g_hw_diag_can_tec;             /* transmit error counter */
volatile uint32_t g_hw_diag_can_rec;             /* receive error counter */
volatile uint8_t  g_hw_diag_can_state;           /* GBT_CAN_STATE_*: 1 active, 2 warning, 3 passive, 4 bus-off */
volatile uint8_t  g_hw_diag_can_bus_off;         /* 1 while the controller is bus-off */
volatile uint32_t g_hw_diag_can_bus_off_events;  /* FDCAN bus-off status-change notifications */
volatile uint32_t g_hw_diag_can_err_passive_events;
volatile uint32_t g_hw_diag_can_warning_events;
/* B. Last frame popped from the RX queue by the integration loop (main context) */
volatile uint8_t  g_hw_diag_last_rx_valid;       /* 0 = none yet, 1 = fields below hold a frame */
volatile uint32_t g_hw_diag_last_rx_id;
volatile uint8_t  g_hw_diag_last_rx_ide;         /* 1 = 29-bit identifier */
volatile uint8_t  g_hw_diag_last_rx_dlc;
volatile uint8_t  g_hw_diag_last_rx_data0;
volatile uint8_t  g_hw_diag_last_rx_data1;
volatile uint8_t  g_hw_diag_last_rx_data2;
volatile uint8_t  g_hw_diag_last_rx_data3;
volatile uint8_t  g_hw_diag_last_rx_data4;
volatile uint8_t  g_hw_diag_last_rx_data5;
volatile uint8_t  g_hw_diag_last_rx_data6;
volatile uint8_t  g_hw_diag_last_rx_data7;
volatile uint32_t g_hw_diag_rx_seq;              /* frames popped and passed to the protocol node */
/* C. Transport protocol (gbt_tp_stats_t of this node + live session fields) */
volatile uint32_t g_hw_diag_tp_rts_tx;
volatile uint32_t g_hw_diag_tp_rts_rx;
volatile uint32_t g_hw_diag_tp_cts_tx;
volatile uint32_t g_hw_diag_tp_cts_rx;
volatile uint32_t g_hw_diag_tp_dt_tx;
volatile uint32_t g_hw_diag_tp_dt_rx_ok;
volatile uint32_t g_hw_diag_tp_eom_tx;
volatile uint32_t g_hw_diag_tp_eom_rx;
volatile uint32_t g_hw_diag_tp_tx_started;
volatile uint32_t g_hw_diag_tp_rx_started;
volatile uint32_t g_hw_diag_tp_tx_complete;
volatile uint32_t g_hw_diag_tp_rx_complete;
volatile uint32_t g_hw_diag_tp_abort_tx;
volatile uint32_t g_hw_diag_tp_abort_rx;
volatile uint32_t g_hw_diag_tp_timeout_tx;
volatile uint32_t g_hw_diag_tp_timeout_rx;
volatile uint32_t g_hw_diag_tp_outbox_full;      /* TP frames that could not be queued (outbox full) */
volatile uint32_t g_hw_diag_tx_hal_accepted;       /* frames accepted by the CAN driver (gbt_hal_can_send OK) */
volatile uint32_t g_hw_diag_tx_hal_accepted_tp_cm; /* ... of which TP.CM (G474: CTS + EOM [+ aborts]) */
volatile uint32_t g_hw_diag_tx_hal_accepted_tp_dt; /* ... of which TP.DT (G474: expected 0) */
volatile uint8_t  g_hw_diag_tp_last_abort_reason_tx;
volatile uint8_t  g_hw_diag_tp_last_abort_reason_rx;
volatile uint8_t  g_hw_diag_tp_last_end_ev;      /* GBT_TP_EV_*: 1 RX ok, 2 RX abort, 4 TX ok, 8 TX abort */
volatile uint32_t g_hw_diag_tp_last_rx_complete_pgn;
volatile uint32_t g_hw_diag_tp_last_rx_complete_len;
volatile uint32_t g_hw_diag_tp_last_tx_complete_pgn;
volatile uint32_t g_hw_diag_tp_last_tx_complete_len;
volatile uint8_t  g_hw_diag_tp_active_dir;       /* 0 idle, 1 send session open, 2 receive session open, 3 both */
volatile uint8_t  g_hw_diag_tp_rx_state;         /* GBT_TP_IDLE 0 / GBT_TP_RX_DATA 1 */
volatile uint32_t g_hw_diag_tp_rx_pgn;           /* PGN of the current / last receive session */
volatile uint8_t  g_hw_diag_tp_rx_packets;       /* expected packets of the current / last receive session */
volatile uint8_t  g_hw_diag_tp_rx_next_seq;      /* next expected sequence number */
volatile uint8_t  g_hw_diag_tp_tx_state;         /* 0 idle, 2 wait CTS/EOM, 3 sending, 4 hold */
volatile uint32_t g_hw_diag_tp_tx_pgn;
volatile uint8_t  g_hw_diag_tp_tx_packets;
volatile uint8_t  g_hw_diag_tp_tx_next_seq;
/* D. Protocol node */
volatile uint8_t  g_hw_diag_gbt_phase;           /* GBT_NODE_PHASE_* (5 = END) */
volatile uint8_t  g_hw_diag_gbt_step;            /* protocol step code (charger END = 41) */
volatile uint8_t  g_hw_diag_gbt_stop_reason;
volatile uint32_t g_hw_diag_gbt_rx_frames;       /* frames processed by gbt_node_on_rx */
volatile uint32_t g_hw_diag_gbt_rx_ignored;
volatile uint32_t g_hw_diag_gbt_rx_invalid;
volatile uint32_t g_hw_diag_gbt_rx_overflow;
volatile uint32_t g_hw_diag_gbt_tx_frames;       /* frames popped from the node outbox for sending */
volatile uint32_t g_hw_diag_gbt_tx_discarded;
volatile uint32_t g_hw_diag_gbt_outbox_overflow;
volatile uint32_t g_hw_diag_gbt_timeouts;        /* application timeouts (BEM/CEM path) */
volatile uint32_t g_hw_diag_gbt_transport_failures;
volatile uint32_t g_hw_diag_gbt_output_failures;
volatile uint32_t g_hw_diag_gbt_api_errors;
volatile uint32_t g_hw_diag_gbt_error_count;     /* sum: rx_invalid + rx_overflow + timeouts + transport_failures
                                                    + controller_failures + physical_failures + output_failures
                                                    + tx_discarded + outbox_overflow + api_errors (saturating) */
volatile uint32_t g_hw_diag_gbt_err_fields;      /* last BEM/CEM bytes 0..3, byte 0 in bits 7..0 */
/* E. Output command / safety */
volatile uint32_t g_hw_diag_out_seq;             /* seq of the last command delivered to the output sink */
volatile uint8_t  g_hw_diag_out_enabled;         /* 1 = last delivered command enabled the (software) output */
volatile uint32_t g_hw_diag_out_voltage_mv;
volatile uint32_t g_hw_diag_out_current_ma;
volatile uint8_t  g_hw_diag_out_apply_status;    /* gbt_hal_status_t of the sink (0 = accepted) */
volatile uint8_t  g_hw_diag_out_state;           /* GBT_NODE_OUT_*: 0 accepted, 1 pending, 2 failed */
volatile uint8_t  g_hw_diag_out_inhibit_latched; /* 1 after DISABLE_OUTPUT latched the inhibit */
volatile uint32_t g_hw_diag_out_shutdown_count;  /* delivered commands with enabled 1 -> 0 */
volatile uint32_t g_hw_diag_update_count;        /* number of mirror refreshes (liveness) */
/* ---- STEP 07-C2.4 structured Keil Watch diagnostics (written only by gbt_watch_update()) ---- */
volatile gbt_watch_tp_t  g_gbt_tp_diag;            /* TP sessions, packets, timing (this node) */
volatile gbt_watch_can_t g_gbt_can_diag;           /* CAN controller / driver */
/* BRM payload as received and reassembled by this node (49 bytes, PGN 512). Copied from the TP layer's frozen
 * snapshot; state: 0 none, 1 captured (session not completed yet), 2 frozen (never changes again). */
volatile uint8_t  g_gbt_brm_rx_snapshot[49];
volatile uint32_t g_gbt_brm_rx_snapshot_state;
volatile uint32_t g_gbt_brm_rx_snapshot_len;
volatile uint32_t g_gbt_brm_rx_snapshot_captures;
/* Acceptance summary, re-evaluated every 100 ms: bit set = criterion currently NOT met
 * (bit list: ACCEPTANCE_CRITERIA.md). g_gbt_accept_pass = 1 only with fail mask 0. */
volatile uint32_t g_gbt_accept_fail;
volatile uint32_t g_gbt_accept_pass;
/* 128-entry RAM event log; single writer: the integration loop (main context). Full -> frozen. */
gbt_evlog_t g_gbt_evlog;
/* STEP 07 FI-04: copy of the integration layer's shutdown response-time record (refreshed every
 * 100 ms by gbt_app_diag; fields: Project1_SharedCore/app/gbt_integration.h gbt_fi04_t). Software
 * command acceptance by the ND-1 sink only - not a physical power-stage measurement. */
volatile gbt_fi04_t g_gbt_fi04;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* STEP 07-C2.4: the 100 ms diagnostics refresh is kept out of main()'s stack frame (ARMCC 5 and GCC
 * both support this attribute), so its locals are not stacked on top of the protocol call chain. */
#define GBT_DIAG_NOINLINE __attribute__((noinline))
static void gbt_app_init(void);
static GBT_DIAG_NOINLINE void gbt_app_diag(uint32_t now);
static void gbt_hw_diag_update(const gbt_node_status_t *ns);   /* STEP 07-C2 */
static void gbt_watch_update(const gbt_node_status_t *ns);     /* STEP 07-C2.4 */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_FDCAN1_Init();
  
	/* USER CODE BEGIN 2 */
  /* STEP 07-C1.2-B2: the former periodic TX demo and its HAL_FDCAN_Start() are replaced.
   * gbt_port_g474_init() configures the filters, enables the notifications and starts FDCAN1. */
  gbt_app_init();
	/* USER CODE END 2 */


  /* Initialize led */
  BSP_LED_Init(LED_GREEN);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* One non-blocking pass: RX overflow -> RX (<= 16 frames from the ISR queue) -> protocol tick
     * -> output APPLY -> SEND (E_FULL retried next pass). No delay, no wait, no Error_Handler on TX. */
    s_gbt_now = HAL_GetTick();
    gbt_integ_step(&s_gbt_integ);
    gbt_app_diag(s_gbt_now);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/* STEP 07-C1.2-B2 - see ports/stm32/g474/MAIN_C_INTEGRATION.md in the WSL repository. */
static void gbt_app_init(void)
{
  gbt_port_g474_cfg_t pc;
  gbt_output_sink_t sink;

  pc.hfdcan = &hfdcan1;
  pc.own_sa = 0x56U;                                  /* charger (GB/T 27930 Table 2) */
  pc.peer_sa = 0xF4U;                                 /* vehicle */
  if (gbt_port_g474_init(&s_gbt_hal, &pc) != GBT_HAL_OK)
  {
    Error_Handler();                                  /* initialisation only */
  }
  s_gbt_cp.max_voltage_mv = 750000U;                  /* project simulation values (as Linux SIL) */
  s_gbt_cp.min_voltage_mv = 200000U;
  s_gbt_cp.max_current_ma = 250000U;
  s_gbt_cp.min_current_ma = 1000U;
  s_gbt_cp.charger_number = 0x01020304U;
  s_gbt_cfg.role = GBT_NODE_ROLE_CHARGER;
  s_gbt_cfg.charger = &s_gbt_cp;
  s_gbt_cfg.vehicle = NULL;
  s_gbt_cfg.t_handle_ms = 1000U;                      /* K-6A PROVISIONAL */
  s_gbt_cfg.selfcheck_delay_ms = 0U;                  /* S-8 PROVISIONAL */
  s_gbt_cfg.apply_fail_tx_policy = GBT_NODE_FAILTX_OPTION_A;   /* ND-6 OPEN: test setting */
  s_gbt_now = HAL_GetTick();
  sim_plant_init(&s_gbt_plant, &s_gbt_now);
  if (output_if_make_sink(&s_gbt_plant, &sink) != GBT_HAL_OK
      || gbt_integ_attach(&s_gbt_integ, &s_gbt_node, &s_gbt_hal, &s_gbt_cfg, &sink) != GBT_HAL_OK)
  {
    Error_Handler();                                  /* initialisation only */
  }
  gbt_integ_set_evlog(&s_gbt_integ, &g_gbt_evlog);    /* STEP 07-C2.4: event log on */
  gbt_node_set_connected(&s_gbt_node, 1U, s_gbt_now);
  s_gbt_diag_ms = s_gbt_now;
}

/* Diagnostic snapshot for the debugger; non-blocking, main context only. */
static GBT_DIAG_NOINLINE void gbt_app_diag(uint32_t now)
{
  gbt_can_status_t cs;
  gbt_port_g474_diag_t pd;
  gbt_node_status_t ns;

  if ((uint32_t)(now - s_gbt_diag_ms) < 100U)
  {
    return;
  }
  s_gbt_diag_ms = now;
  if (gbt_hal_can_get_status(&s_gbt_hal, &cs) == GBT_HAL_OK)
  {
    g_gbt_can_status = cs;
  }
  if (gbt_port_g474_get_diag(&s_gbt_hal, &pd) == GBT_HAL_OK)
  {
    g_gbt_port_diag = pd;
  }
  gbt_node_get_status(&s_gbt_node, &ns);
  g_gbt_phase = ns.phase;
  g_gbt_rx_overflow = ns.diag.rx_overflow;
  g_gbt_tx_dropped = s_gbt_integ.tx_dropped;
  g_gbt_tx_full_retries = s_gbt_integ.tx_full_retries;
  gbt_hw_diag_update(&ns);                            /* STEP 07-C2 flat Watch mirrors */
  gbt_watch_update(&ns);                              /* STEP 07-C2.4 structured Watch */
  g_gbt_fi04 = s_gbt_integ.fi04;                      /* STEP 07 FI-04 Watch copy */
}

/* ---- STEP 07-C2: flat Keil Watch mirrors (main context only, bounded, no HAL calls) ---- */
static uint32_t gbt_hw_diag_sat_add(uint32_t a, uint32_t b)
{
  return (a > 0xFFFFFFFFu - b) ? 0xFFFFFFFFu : (a + b);
}

/* Copies existing diagnostic sources into the g_hw_diag_* scalars. Called only from
 * gbt_app_diag() after it refreshed g_gbt_can_status / g_gbt_port_diag and *ns.
 * Reads s_gbt_node.tp (complete type from protocol/gbt_node.h) for diagnostics only. */
static void gbt_hw_diag_update(const gbt_node_status_t *ns)
{
  const gbt_tp_t *tp = &s_gbt_node.tp;
  const gbt_tp_stats_t *ts = &s_gbt_node.tp.stats;
  const gbt_node_diag_t *nd = &ns->diag;
  uint32_t e;

  /* A. CAN controller / port */
  g_hw_diag_can_tx_queued = g_gbt_port_diag.tx_frames;
  g_hw_diag_can_tx_backpressure = g_gbt_port_diag.tx_fifo_full;
  g_hw_diag_can_tx_error = g_gbt_can_status.tx_error;
  g_hw_diag_can_tx_dropped = s_gbt_integ.tx_dropped;
  g_hw_diag_can_rx_count = g_gbt_port_diag.rx_frames;
  g_hw_diag_can_rx_invalid = g_gbt_can_status.rx_invalid;
  g_hw_diag_can_rx_overflow_sw = g_gbt_can_status.rx_overflow_sw;
  g_hw_diag_can_rx_overflow_hw = g_gbt_can_status.rx_overflow_hw;
  g_hw_diag_can_tec = g_gbt_can_status.tec;
  g_hw_diag_can_rec = g_gbt_can_status.rec;
  g_hw_diag_can_state = g_gbt_can_status.state;
  g_hw_diag_can_bus_off = (uint8_t)((g_gbt_can_status.state == GBT_CAN_STATE_BUS_OFF) ? 1U : 0U);
  g_hw_diag_can_bus_off_events = g_gbt_port_diag.bus_off_events;
  g_hw_diag_can_err_passive_events = g_gbt_port_diag.error_passive_events;
  g_hw_diag_can_warning_events = g_gbt_port_diag.warning_events;

  /* B. Last frame popped by the integration loop */
  g_hw_diag_last_rx_valid = s_gbt_integ.last_rx_valid;
  g_hw_diag_last_rx_id = s_gbt_integ.last_rx.id;
  g_hw_diag_last_rx_ide = (uint8_t)(((s_gbt_integ.last_rx.flags & GBT_CAN_FLAG_EXT) != 0U) ? 1U : 0U);
  g_hw_diag_last_rx_dlc = s_gbt_integ.last_rx.dlc;
  g_hw_diag_last_rx_data0 = s_gbt_integ.last_rx.data[0];
  g_hw_diag_last_rx_data1 = s_gbt_integ.last_rx.data[1];
  g_hw_diag_last_rx_data2 = s_gbt_integ.last_rx.data[2];
  g_hw_diag_last_rx_data3 = s_gbt_integ.last_rx.data[3];
  g_hw_diag_last_rx_data4 = s_gbt_integ.last_rx.data[4];
  g_hw_diag_last_rx_data5 = s_gbt_integ.last_rx.data[5];
  g_hw_diag_last_rx_data6 = s_gbt_integ.last_rx.data[6];
  g_hw_diag_last_rx_data7 = s_gbt_integ.last_rx.data[7];
  g_hw_diag_rx_seq = s_gbt_integ.rx_seq;

  /* C. Transport protocol */
  g_hw_diag_tp_rts_tx = ts->rts_tx;
  g_hw_diag_tp_rts_rx = ts->rts_rx;
  g_hw_diag_tp_cts_tx = ts->cts_tx;
  g_hw_diag_tp_cts_rx = ts->cts_rx;
  g_hw_diag_tp_dt_tx = ts->dt_tx;
  g_hw_diag_tp_dt_rx_ok = ts->dt_rx_ok;
  g_hw_diag_tp_eom_tx = ts->eom_tx;
  g_hw_diag_tp_eom_rx = ts->eom_rx;
  g_hw_diag_tp_tx_started = ts->tx_started;
  g_hw_diag_tp_rx_started = ts->rx_started;
  g_hw_diag_tp_tx_complete = ts->tx_complete;
  g_hw_diag_tp_rx_complete = ts->rx_complete;
  g_hw_diag_tp_abort_tx = ts->abort_tx;
  g_hw_diag_tp_abort_rx = ts->abort_rx;
  g_hw_diag_tp_timeout_tx = ts->timeout_tx;
  g_hw_diag_tp_timeout_rx = ts->timeout_rx;
  g_hw_diag_tp_outbox_full = tp->outbox_full;
  g_hw_diag_tx_hal_accepted = s_gbt_integ.tx_hal_accepted;
  g_hw_diag_tx_hal_accepted_tp_cm = s_gbt_integ.tx_hal_accepted_tp_cm;
  g_hw_diag_tx_hal_accepted_tp_dt = s_gbt_integ.tx_hal_accepted_tp_dt;
  g_hw_diag_tp_last_abort_reason_tx = ts->last_abort_reason_tx;
  g_hw_diag_tp_last_abort_reason_rx = ts->last_abort_reason_rx;
  g_hw_diag_tp_last_end_ev = ts->last_end_ev;
  g_hw_diag_tp_last_rx_complete_pgn = ts->last_rx_complete_pgn;
  g_hw_diag_tp_last_rx_complete_len = ts->last_rx_complete_len;
  g_hw_diag_tp_last_tx_complete_pgn = ts->last_tx_complete_pgn;
  g_hw_diag_tp_last_tx_complete_len = ts->last_tx_complete_len;
  g_hw_diag_tp_active_dir = (uint8_t)(((tp->tx.state != GBT_TP_IDLE) ? 1U : 0U)
                                      | ((tp->rx.state != GBT_TP_IDLE) ? 2U : 0U));
  g_hw_diag_tp_rx_state = tp->rx.state;
  g_hw_diag_tp_rx_pgn = tp->rx.pgn;
  g_hw_diag_tp_rx_packets = tp->rx.packets;
  g_hw_diag_tp_rx_next_seq = tp->rx.next_seq;
  g_hw_diag_tp_tx_state = tp->tx.state;
  g_hw_diag_tp_tx_pgn = tp->tx.pgn;
  g_hw_diag_tp_tx_packets = tp->tx.packets;
  g_hw_diag_tp_tx_next_seq = tp->tx.next_seq;

  /* D. Protocol node */
  g_hw_diag_gbt_phase = ns->phase;
  g_hw_diag_gbt_step = ns->step;
  g_hw_diag_gbt_stop_reason = ns->stop_reason;
  g_hw_diag_gbt_rx_frames = nd->rx_frames;
  g_hw_diag_gbt_rx_ignored = nd->rx_ignored;
  g_hw_diag_gbt_rx_invalid = nd->rx_invalid;
  g_hw_diag_gbt_rx_overflow = nd->rx_overflow;
  g_hw_diag_gbt_tx_frames = nd->tx_frames;
  g_hw_diag_gbt_tx_discarded = nd->tx_discarded;
  g_hw_diag_gbt_outbox_overflow = nd->outbox_overflow;
  g_hw_diag_gbt_timeouts = nd->timeouts;
  g_hw_diag_gbt_transport_failures = nd->transport_failures;
  g_hw_diag_gbt_output_failures = nd->output_failures;
  g_hw_diag_gbt_api_errors = nd->api_errors;
  e = gbt_hw_diag_sat_add(nd->rx_invalid, nd->rx_overflow);
  e = gbt_hw_diag_sat_add(e, nd->timeouts);
  e = gbt_hw_diag_sat_add(e, nd->transport_failures);
  e = gbt_hw_diag_sat_add(e, nd->controller_failures);
  e = gbt_hw_diag_sat_add(e, nd->physical_failures);
  e = gbt_hw_diag_sat_add(e, nd->output_failures);
  e = gbt_hw_diag_sat_add(e, nd->tx_discarded);
  e = gbt_hw_diag_sat_add(e, nd->outbox_overflow);
  g_hw_diag_gbt_error_count = gbt_hw_diag_sat_add(e, nd->api_errors);
  g_hw_diag_gbt_err_fields = (uint32_t)ns->err_fields[0] | ((uint32_t)ns->err_fields[1] << 8)
                             | ((uint32_t)ns->err_fields[2] << 16) | ((uint32_t)ns->err_fields[3] << 24);

  /* E. Output command / safety (last command delivered to the sink by APPLY) */
  g_hw_diag_out_seq = s_gbt_integ.last_cmd.seq;
  g_hw_diag_out_enabled = s_gbt_integ.last_cmd.enabled;
  g_hw_diag_out_voltage_mv = s_gbt_integ.last_cmd.voltage_mv;
  g_hw_diag_out_current_ma = s_gbt_integ.last_cmd.current_ma;
  g_hw_diag_out_apply_status = (uint8_t)s_gbt_integ.last_apply_status;
  g_hw_diag_out_state = ns->output_state;
  g_hw_diag_out_inhibit_latched = ns->inhibit_latched;
  g_hw_diag_out_shutdown_count = s_gbt_integ.output_shutdowns;
  g_hw_diag_update_count = g_hw_diag_update_count + 1U;
}
/* ---- STEP 07-C2.4: structured Keil Watch diagnostics (main context only, bounded, no HAL calls).
 * Called only from gbt_app_diag() after gbt_hw_diag_update(). Reads s_gbt_node.tp and s_gbt_integ
 * (complete types) for diagnostics only. ---- */
static void gbt_watch_update(const gbt_node_status_t *ns)
{
  const gbt_tp_t *tp = &s_gbt_node.tp;
  const gbt_tp_stats_t *ts = &s_gbt_node.tp.stats;
  const gbt_node_diag_t *nd = &ns->diag;
  volatile gbt_watch_tp_t *t = &g_gbt_tp_diag;
  volatile gbt_watch_can_t *c = &g_gbt_can_diag;
  uint32_t e;
  uint32_t f;
  uint32_t i;
  uint32_t n;

  /* A. TP */
  t->now_ms = s_gbt_now;
  t->tp_role = 2U;
  t->tx_started = ts->tx_started;
  t->tx_completed = ts->tx_complete;
  t->rx_started = ts->rx_started;
  t->rx_completed = ts->rx_complete;
  t->brm_completed = s_gbt_integ.tp_rx_complete_msg[0];
  t->bcp_completed = s_gbt_integ.tp_rx_complete_msg[1];
  t->bcs_completed = s_gbt_integ.tp_rx_complete_msg[2];
  t->other_completed = s_gbt_integ.tp_rx_complete_msg[3];
  t->last_tx_pgn = tp->tx.pgn;
  t->last_tx_len = tp->tx.len;
  t->last_rx_pgn = tp->rx.pgn;
  t->last_rx_len = tp->rx.len;
  t->tx_expected_packets = tp->tx.packets;
  t->tx_actual_packets = ts->tx_sess_dt;
  t->rx_expected_packets = tp->rx.packets;
  t->rx_actual_packets = ts->rx_sess_dt;
  t->tx_next_seq = tp->tx.next_seq;
  t->rx_next_seq = tp->rx.next_seq;
  t->tx_state = tp->tx.state;
  t->rx_state = tp->rx.state;
  t->aborts_sent = ts->abort_tx;
  t->aborts_received = ts->abort_rx;
  t->transport_failures = nd->transport_failures;
  t->timeouts_tx = ts->timeout_tx;
  t->timeouts_rx = ts->timeout_rx;
  t->last_end_ev = ts->last_end_ev;
  t->sessions_ended = ts->sessions_ended;
  t->last_tx_duration_ms = ts->last_tx_duration_ms;
  t->max_tx_duration_ms = ts->max_tx_duration_ms;
  t->last_rx_duration_ms = ts->last_rx_duration_ms;
  t->max_rx_duration_ms = ts->max_rx_duration_ms;
  t->max_tx_dt_interval_ms = ts->max_tx_dt_interval_ms;
  t->max_rx_dt_interval_ms = ts->max_rx_dt_interval_ms;
  t->outbox_full = tp->outbox_full;
  t->step_changes = s_gbt_integ.step_changes;

  /* B. CAN controller / driver */
  c->state = g_gbt_can_status.state;
  c->tec = g_gbt_can_status.tec;
  c->rec = g_gbt_can_status.rec;
  c->rx_frames = g_gbt_port_diag.rx_frames;
  c->tx_frames = g_gbt_port_diag.tx_frames;
  c->tx_confirmed = g_gbt_port_diag.tx_confirmed;
  c->tx_pending = g_gbt_port_diag.tx_pending;
  c->tx_backpressure = g_gbt_port_diag.tx_fifo_full;
  c->rx_overflow_sw = g_gbt_can_status.rx_overflow_sw;
  c->rx_overflow_hw = g_gbt_can_status.rx_overflow_hw;
  c->rx_invalid = g_gbt_can_status.rx_invalid;
  c->hal_errors = gbt_hw_diag_sat_add(g_gbt_port_diag.hal_error_callbacks, g_gbt_port_diag.rx_hal_error);
  e = gbt_hw_diag_sat_add(g_gbt_port_diag.tx_bus_off, g_gbt_port_diag.tx_hal_error);
  e = gbt_hw_diag_sat_add(e, g_gbt_port_diag.tx_unconfirmed_reuse);
  c->tx_failures = gbt_hw_diag_sat_add(e, s_gbt_integ.tx_dropped);
  c->warning_events = g_gbt_port_diag.warning_events;
  c->error_passive_events = g_gbt_port_diag.error_passive_events;
  c->bus_off_events = g_gbt_port_diag.bus_off_events;

  /* C. BRM snapshot (the TP copy is frozen after completion, so this copy is stable) */
  n = tp->snap_rx.len;
  if (n > sizeof g_gbt_brm_rx_snapshot)
  {
    n = sizeof g_gbt_brm_rx_snapshot;
  }
  for (i = 0U; i < sizeof g_gbt_brm_rx_snapshot; i++)
  {
    g_gbt_brm_rx_snapshot[i] = (i < n && tp->snap_rx.state != GBT_TP_SNAP_NONE) ? tp->snap_rx.buf[i] : 0U;
  }
  g_gbt_brm_rx_snapshot_state = tp->snap_rx.state;
  g_gbt_brm_rx_snapshot_len = tp->snap_rx.len;
  g_gbt_brm_rx_snapshot_captures = tp->snap_rx.captures;

  /* D. Acceptance summary (bit set = criterion not met; list in ACCEPTANCE_CRITERIA.md) */
  f = 0U;
  if (ns->step != 41U) { f |= 1U << 0; }                                         /* END step reached */
  if (t->brm_completed == 0U) { f |= 1U << 1; }
  if (t->bcp_completed == 0U) { f |= 1U << 2; }
  if (t->bcs_completed == 0U) { f |= 1U << 3; }
  if (t->aborts_sent != 0U || t->aborts_received != 0U) { f |= 1U << 4; }
  if (t->timeouts_tx != 0U || t->timeouts_rx != 0U) { f |= 1U << 5; }
  if (t->transport_failures != 0U) { f |= 1U << 6; }
  if (t->tx_state != 0U || t->rx_state != 0U) { f |= 1U << 7; }                 /* both TP sides IDLE */
  if (c->rx_overflow_sw != 0U || c->rx_overflow_hw != 0U || nd->rx_overflow != 0U) { f |= 1U << 8; }
  if (c->rx_invalid != 0U) { f |= 1U << 9; }
  if (nd->rx_invalid != 0U || nd->outbox_overflow != 0U || nd->tx_discarded != 0U || t->outbox_full != 0U)
  {
    f |= 1U << 10;                                                             /* unexpected discard */
  }
  if (c->tx_failures != 0U) { f |= 1U << 11; }
  if (c->state != 1U || c->error_passive_events != 0U || c->bus_off_events != 0U) { f |= 1U << 12; }
  if (g_gbt_brm_rx_snapshot_state != GBT_TP_SNAP_FROZEN || g_gbt_brm_rx_snapshot_len != 49U) { f |= 1U << 13; }
  if (nd->timeouts != 0U || nd->output_failures != 0U || nd->api_errors != 0U) { f |= 1U << 14; }  /* safety / app */
  if (c->hal_errors != 0U) { f |= 1U << 15; }
  if (c->tx_confirmed + c->tx_pending != c->tx_frames) { f |= 1U << 16; }      /* accepted = confirmed + pending */
  if (t->other_completed != 0U) { f |= 1U << 17; }
  g_gbt_accept_fail = f;
  g_gbt_accept_pass = (f == 0U) ? 1U : 0U;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
