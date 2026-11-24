# Architecture

This document describes the software as delivered in `source/`. Its authority is the frozen
architecture [design/ARCHITECTURE.md](design/ARCHITECTURE.md) (v1.0, owner-approved in
STEP 07-A.1). Section numbers such as "§4.3" refer to that document.

That document's §8 ("repository structure") predates the STM32 integration. For file locations, use
[README.md](../README.md).

## 1. Layers

```
+----------------------------------------------------------------------------+
| Board application (USER CODE in Core/Src/main.c of each Keil project)       |
|   init: port, node config, sink, integration;  loop: gbt_integ_step();     |
|   every 100 ms: copy diagnostics into Keil Watch variables                  |
+----------------------------------------------------------------------------+
| Integration layer  app/gbt_integration.[ch]   (the ONLY caller of the HAL) |
|   lifecycle (bind sink, claim, release, unbind), one loop pass, event log,  |
|   FI-04 shutdown-timing measurement                                         |
|   app/sim_plant.[ch]  simulated output stage (the "sink")                   |
|   app/battery_model.[ch]  vehicle-side battery model (SOC, demand)          |
+-------------------------------+--------------------------------------------+
| Protocol core (pure C99)      |  HAL contract  hal/hal.h  (ND-1)           |
|  protocol/, common/           |   CAN send/receive, ms clock, output sink, |
|  no HAL include, no malloc,   |   CAN status, lifecycle                    |
|  no blocking, no writable     +--------------------------------------------+
|  statics                      |  Ports (implement hal.h)                   |
|                               |   ports/stm32/g474/port_g474.c (FDCAN)     |
|                               |   ports/stm32/f103/port_f103.c (bxCAN)     |
|                               |   host/ports/linux/port_linux.c (SocketCAN)|
|                               |   ports/common/gbt_spsc.c (ISR->main queue)|
|                               |   host/tests/support/fake_hal.c (SIL)      |
+-------------------------------+--------------------------------------------+
```

**Dependency rules (§3).** These are checked by CTest `architecture_rules`,
`protocol_no_writable_statics` and the `cross_c99_*` tests:
- `protocol/` and `common/` include nothing from `hal/`, `app/`, `ports/` or vendor headers.
- Only `app/gbt_integration.c` calls `gbt_hal_*` at runtime.
- Vendor STM32 headers appear only in `ports/stm32/*` and the board projects.

**One source tree for all targets.**
- `source/Project1_SharedCore/` is compiled by both Keil projects (include path
  `../../Project1_SharedCore`).
- The host harness compiles the same files through symlinks in `host/`.

## 2. Protocol core (`source/Project1_SharedCore/protocol/`, `common/`)

| Module | Responsibility |
|---|---|
| `gbt_id` | 29-bit identifier build/classify for the 18 Lean messages (priority, PGN, SA/DA) |
| `gbt_codec`, `gbt_codec_ext`, `gbt_bits` | message encode/decode, little-endian, 0.1 V / 0.1 A scaling, −400 A current offset, bit fields |
| `gbt_tp` | J1939-style transport: RTS/CTS/DT/EOM ACK/Abort, T1–T4 waits, 10 ms DT spacing, 64-byte buffer (largest Lean payload is BRM, 49 B) |
| `gbt_charger_sm`, `gbt_vehicle_sm` | Lean state machines (steps below) |
| `gbt_node` | node runtime: RX dispatch, timers TO-01..TO-25, scheduler of periodic messages, TP integration, TX gate, output state |
| `gbt_safety` | the only module that changes the output command (disable / zero current / BCL setpoint with local clamps) |
| `gbt_outbox`, `gbt_ring`, `gbt_timer` | bounded TX queue (16 frames), ring buffer, wrap-safe millisecond timers |
| `gbt_provisional.h` | values not confirmed by the standard ([design/PROVISIONAL_PARAMETERS.md](design/PROVISIONAL_PARAMETERS.md)) |

### State machines (step codes as shown in the Watch variable `g_hw_diag_gbt_step`)

| Charger step | Meaning | Vehicle step | Meaning |
|---|---|---|---|
| 2 | CHM, wait BHM | 22 | wait CHM |
| 3 | self-check | 23 | BHM, wait CRM 00 |
| 4 | CRM 00, wait BRM (TP) | 25 | BRM (TP), wait CRM AA |
| 5 | CRM AA, wait BCP (TP) | 26 | BCP (TP), wait CML |
| 7 | CML, wait BRO | 29 | BRO AA, wait CRO |
| 8 / 9 | CRO 00 / CRO AA, wait BCL | 30 | BCL + BCS (TP), wait CCS |
| 10 (11) | charging (11 = inhibited by SPN3096) | 31 | charging |
| 12 / 13 | CST, wait BST / BSD | 32 / 33 | BST / BSD, wait CSD |
| 40 | timeout: send CEM | 40 | timeout: send BEM |
| 41 | END (normal) | 41 | END |
| 42 | EXIT (after timeout handling, peer error, shutdown) | 42 | EXIT |

Typical paths:
- **Normal session (SIL):** charger 2→3→4→5→7→8→9→10→13→41; vehicle 22→23→25→26→29→30→31→32→33→41.
- **Communication loss (FI-04):** charger 10 → 40 (CEM) → 42 after the handling time
  `t_handle_ms` = 1000 ms (K-6A, PROVISIONAL).

Restart after END/EXIT is by reset only. `gbt_node_restart_session()` exists in the core but is
not called by any application or test (OPEN_ISSUES OI-07).

## 3. Integration loop (`app/gbt_integration.c`, `gbt_integ_step`)

The board's `while (1)` calls `gbt_integ_step()` once per pass. Each pass is non-blocking:

1. Read the clock once (`gbt_hal_now_ms`, 1 ms `HAL_GetTick`, wrap-safe arithmetic).
2. **Overflow:** read CAN status. New software-queue or hardware-FIFO losses go to
   `gbt_node_on_rx_overflow()`. While charging, the charger stops on overflow (T-5).
3. **RX:** pop at most `GBT_INTEG_RX_PER_STEP` = 16 frames from the ISR queue → `gbt_node_on_rx()`.
4. **Tick:** `gbt_node_tick()` handles TP timers, protocol timeouts, state actions and the scheduler.
5. **APPLY:** if the node's output command has a new sequence number, deliver it to the sink with
   `gbt_hal_output_apply()`, then confirm it to the node (`gbt_node_confirm_output`).
6. **SEND:** pop frames that pass the TX gate and send them. On `GBT_HAL_E_FULL` the frame is
   retried first in the next pass.

## 4. Safety shutdown path (§4.3, §4.4)

```
timeout / peer BEM-CEM / overflow / stop request / sink refusal while charging
        |
        v
gbt_safety_disable(): inhibit_latched = 1; out_cmd = {enabled 0, 0 mV, 0 mA}, seq+1;
                      out_state = PENDING  -->  TX gate HOLDS every frame (SAF-1)
        |
        v  (same integration pass, step 5)
gbt_hal_output_apply(cmd) --> port --> sink (sim_plant) accepts: OK
        |
        v
gbt_node_confirm_output(seq, accepted) --> out_state = ACCEPTED --> TX gate opens
        |
        v  (step 6)
CEM / CST / ... are sent only now: output first, then the message
```

Details of the shutdown path:
- **Latching.** The inhibit stays latched for the session. `gbt_safety_apply_bcl()` ignores demands while it is latched.
- **Sink refusal.** If the sink refuses the command:
  - while charging, the charger stops (ND-7, PROVISIONAL);
  - otherwise the output state becomes FAILED, and the configured policy decides:
    - option A (configured on the G474): hold stop/error frames;
    - option B: send them;
    - in both cases, normal frames are discarded (ND-6, open decision).
- **Physical output.** The "output stage" on the boards is the software model `sim_plant`. There is
  no power stage and no GPIO enable line. A shutdown therefore means that the software output
  command was accepted and confirmed. It does not mean current decay.

### FI-04 measurement (charger)

`gbt_fi04_*` in `gbt_integration.c` and `gbt_fi04_t` in `gbt_integration.h`:
- **T0** is the start of the integration pass *before* the one that handled the fault.
- **T1** is a fresh clock read right after the sink accepted the disabled command and the node
  confirmed exactly that sequence.
- **Counted events:** accepted and confirmed enabled→disabled transitions only, with no duplicates.
- **Verdict:**
  - PASS if T1 − T0 ≤ 9 ms; FAIL if ≥ 10 ms;
  - INVALID if the reference pass is untrustworthy: no previous pass, RX limit reached, RX error,
    overflow, or status read failure. INVALID is never PASS.
- **Watch view:** the G474 copies the structure into `g_gbt_fi04` every 100 ms.

## 5. Ports and concurrency

| | G474 (`port_g474.c`) | F103 (`port_f103.c`) |
|---|---|---|
| Controller | FDCAN1, classic frames, normal mode, auto-retransmission | bxCAN1, normal mode, auto-retransmission, no automatic bus-off recovery |
| RX filter | one extended filter (DA 0x56, SA 0xF4) → FIFO 0; all other frames rejected | bank 0, 32-bit ID/mask (DA 0xF4, SA 0x56) → FIFO 0 |
| RX interrupt | `FDCAN1_IT0` priority 5: drains FIFO 0 (bounded) into the SPSC queue; counts FIFO message loss | `USB_LP_CAN1_RX0` priority 5: drains FIFO 0 (bounded); FIFO overrun via the error callback |
| Validation in the ISR | extended ID, data frame, DLC ≤ 8; invalid frames counted (`rx_invalid`) | same, using the raw DLC from the mailbox register |
| TX | main context only; TX FIFO request, completion polled (`TXBTO`) | main context only; one mailbox in flight, completion polled (`TSR.RQCPx/TXOKx`) |
| Error state | from PSR (state polled; bus-off and passive events counted) | from ESR (polled; SCE interrupt not enabled) |
| Bus-off | observed, **not recovered** (reset needed); protocol timeouts then force the safe state | same |

**SPSC queue (`ports/common/gbt_spsc.c`)**, design in [design/STEP07C1_SPSC_DESIGN.md](design/STEP07C1_SPSC_DESIGN.md):
- 16 slots; the ISR is the producer and main is the consumer.
- Each index is a 32-bit word written by exactly one side, and the volatile slot is accessed before the index is published.
- Correct for a single core without data cache (Cortex-M3 / M4). A barrier hook exists for other targets.

**Interrupt priorities** (lower number = higher priority):

| Board | Priorities |
|---|---|
| G474 | SysTick **0** (`TICK_INT_PRIORITY 0UL`, `stm32g4xx_hal_conf.h`), `FDCAN1_IT0` 5 (`fdcan.c`), user-button EXTI 15 (a CubeMX/BSP remnant with an empty callback) |
| F103 | `USB_LP_CAN1_RX0` 5 (`can.c`), SysTick 15 |

On the G474, three-level nesting is therefore possible: button → FDCAN → SysTick. This is included in
the stack estimate in [VERIFICATION.md §4.4](VERIFICATION.md#44-stack-margins).

## 6. Board responsibilities

| | G474 charger | F103 B vehicle |
|---|---|---|
| Node config | charger: 750 V / 200 V, 250 A / 1 A (simulation values), `t_handle_ms` 1000, ND-6 option A | vehicle: battery model SOC 500 → 520 ‰ at 10 ‰/s (about 2 s of charging) |
| TP role | receiver (BRM, BCP, BCS) | sender |
| Diagnostics (Watch) | `g_gbt_can_status`, `g_gbt_port_diag`, flat `g_hw_diag_*`, structured `g_gbt_*` (TP, BRM snapshot, acceptance mask), `g_gbt_evlog`, `g_gbt_fi04` | same set except `g_gbt_fi04`; plus the stack-paint area |
| Special | — | `MDK-ARM/stack_paint_f103.s` paints the main stack with 0xA5A5A5A5 before `main` (`$Sub$$main`) so a debugger can measure the stack high-water mark |

Watch variable meanings and pass values are in [hardware/WATCH_VARIABLES.md](hardware/WATCH_VARIABLES.md).

## 7. CAN and TP data flow (normal session, simplified)

```
Vehicle (F103 B)                       bus 250 kbit/s                  Charger (G474)
    <------------------------------------------------------------- CHM  (step 2)
BHM ------------------------------------------------------------------->
    <------------------------------------------------------------- CRM 00
BRM 49 B: RTS ---------------------------------------------------------->
          <------------------------------------------------------- CTS (7 packets)
          DT 1..7 ------------------------------------------------------>
          <------------------------------------------------------- EOM ACK
    <------------------------------------------------------------- CRM AA
BCP 13 B via TP (same RTS/CTS/DT/EOM pattern, 2 packets) --------------->
    <------------------------------------------------------------- CML
BRO AA ----------------------------------------------------------------->
    <------------------------------------------------------------- CRO AA
BCL (single frame) + BCS 9 B via TP, periodically------------------------->
    <------------------------------------------------------------- CCS
BST / BSD -------------------------------------------------------------->  (stop, END)
    <------------------------------------------------------------- CST / CSD
```

The frame-level sequence and expected bytes are in [hardware/TP_SESSION_FLOW.md](hardware/TP_SESSION_FLOW.md).
