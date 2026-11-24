# Software Architecture — GB/T 27930-2023 System A Dual-Node CAN Simulator

| Field | Value |
|---|---|
| Step | STEP 07-A.1 architecture package |
| Status | **FROZEN — Version 1.0. STEP 07-A.1 OWNER APPROVED (AR-1 C-1/C-2, AR-2, AR-3, AR-4, C99 headers; gbt_result_t retained). ND-6 OPEN; ND-3 PROVISIONAL; ARMCC V5 to be verified in STEP 07-C.** |
| Date | 2026-11-16 |
| Baseline inputs | STEP 07-A Revision 3.2 decision package; B1-01..B1-15; STEP 07-A.1 directive |
| Interface files | `common/gbt_types.h`, `hal/hal.h`, `app/output_if.h`, `protocol/gbt_node.h` |

## 1. Layers

```
+-------------------------------------------------------------+
| Application / Integration layer (app/, tests/sil/)           |
|   charger_app  vehicle_app  battery_model  output_if         |
|   main loop: moves frames HAL <-> node; output via HAL sink   |
+------------------------------+------------------------------+
            | gbt_node API                 | gbt_hal API
            v                              v
+------------------------------+  +----------------------------+
| Protocol layer (protocol/)   |  | HAL interface (hal/hal.h)  |
|   pure C99, no HAL, no OS    |  |   ND-1: CAN TX/RX, clock,  |
|                              |  |   status, output sink,     |
|                              |  |   lifecycle                |
|   gbt_id  gbt_codec  gbt_tp  |  +-------------+--------------+
|   gbt_timer  gbt_charger_sm  |                | implemented by
|   gbt_vehicle_sm  gbt_safety |                v
|   gbt_node                   |  +----------------------------+
+--------------+---------------+  | Platform ports (ports/)    |
               |                  |   linux (SocketCAN)        |
               v                  |   stm32/g474 (FDCAN)       |
+------------------------------+  |   stm32/f103 (bxCAN)       |
| Common types (common/)       |<-|   tests/support/fake_hal   |
|   gbt_can_frame_t, output    |  +----------------------------+
|   command/feedback, results  |
+------------------------------+
```

| Layer | Responsibility | Must not |
|---|---|---|
| Application / Integration | Owns the main loop and all instances (node, HAL, output backend). Runs the ND-1 lifecycle (bind, claim, release, unbind), feeds the node, delivers every output command through `gbt_hal_output_apply()`, confirms the result to the node, supplies battery model, role, parameters and session restart. | Implement protocol rules; drive the physical output by any path other than the HAL sink. |
| Protocol (`protocol/`) | All GB/T 27930-2023 System A behaviour: identifiers, codecs, transport, timers, state machines, safety decisions, TX queue. Deterministic; driven only by `on_rx`, `tick` and application inputs. | Include `hal/hal.h`, `app/`, `ports/`, STM32 HAL, CMSIS, SocketCAN or OS headers; allocate memory; block; call the HAL. |
| HAL interface (`hal/hal.h`) | ND-1 contract (Revision 3.2 §4.3.1–4.3.3): CAN TX/RX, millisecond clock, controller status, diagnostic events, output sink, lifecycle UNINIT→READY→BOUND→CLAIMED, bind/unbind, claim/release. | Contain protocol logic. |
| Platform ports (`ports/`) | Implement `hal.h` for one platform: controller setup, filters, ISR, bounded RX queue, clock. | Include `app/` or `protocol/`; make protocol decisions. |
| Common (`common/`) | Shared plain-data types used across layers. | Include anything except C99 standard headers. |
| Verification (`tests/`) | Unit tests, fake HAL, deterministic dual-node SIL, SocketCAN SIL; hardware and logic-analyzer procedures. | Be linked into target firmware. |

## 2. Modules

### 2.1 Protocol modules

| Module | Files | Responsibility | State in repository |
|---|---|---|---|
| gbt_types | `common/gbt_types.h` | Shared types: `gbt_can_frame_t`, frame flags/limits and validation, `gbt_output_cmd_t`, `gbt_output_fb_t`, `gbt_result_t` | Interface (this step) |
| gbt_id | `protocol/gbt_id.[ch]` | 29-bit identifier build/parse/classify; Lean message table (PGN, P, length, period, direction) | Implemented in STEP 07-B0 session; not yet wired into CMake |
| gbt_codec | `protocol/gbt_codec.[ch]` | Message encode/decode, scaling, range/value status | Byte-aligned subset implemented (B0); bit-field, BCP/CSD deferred |
| gbt_tp | `protocol/gbt_tp.[ch]` | Connection-mode multi-frame transport (BRM, BCP, BCS) | DEFER — gated by K-4 |
| gbt_timer | `protocol/gbt_timer.[ch]` | Wrap-safe elapsed time, arm/disarm | Primitives implemented (B0); expiry rule S-5 deferred |
| gbt_charger_sm | `protocol/gbt_charger_sm.[ch]` | Charger state machine (SM TC transitions, Lean subset) | DEFER |
| gbt_vehicle_sm | `protocol/gbt_vehicle_sm.[ch]` | Vehicle state machine (SM TV transitions, Lean subset) | DEFER |
| gbt_safety | `protocol/gbt_safety.[ch]` | PROT-7 setpoint rules, range checks, trigger detection, inhibit latch, output command | DEFER |
| gbt_node | `protocol/gbt_node.[ch]` | Node object, dispatch of RX to codec/TP/SM, tick scheduling, TX queue, output command, status | Interface (this step); implementation DEFER |
| support | `protocol/gbt_ring.[ch]`, `protocol/gbt_outbox.[ch]`, `protocol/gbt_provisional.h` | Single-context FIFO (adapted OLD ring buffer), 16-entry TX outbox, isolated PROVISIONAL constants | Implemented (B0) |

### 2.2 Application modules

| Module | Responsibility | State |
|---|---|---|
| `app/output_if.h` | Output BACKEND adapter (SAF-4): `output_if_make_sink()` builds the ND-1 `gbt_output_sink_t` for the selected backend. Not a second output path. | Interface (this step) |
| `app/charger_app.c` | Charger integration loop and parameters | DEFER |
| `app/vehicle_app.c` | Vehicle integration loop and parameters | DEFER |
| `app/battery_model.c` | Linear SOC ramp, fixed voltage/current demand (Lean Plan) | DEFER |
| `app/sim_plant.c` | Simulated output backend for SIL (implements `output_if.h`) | DEFER |

### 2.3 Platform modules

| Module | Responsibility | State |
|---|---|---|
| `ports/common/gbt_hal_lc.h` | Shared HAL instance states | Present |
| `ports/linux/` | SocketCAN port, poll-based, single-threaded | DEFER |
| `ports/stm32/g474/` | FDCAN1 port (PA12 TX / PA11 RX), ISR -> bounded RX queue | DEFER (STEP 07-C) |
| `ports/stm32/f103/` | bxCAN1 port (PB9 TX / PB8 RX, remap 2), ISR -> bounded RX queue | DEFER (STEP 07-C) |
| `tests/support/fake_hal.[ch]` | Fake HAL with simulated two-node bus and virtual clock | Present |

## 3. Dependency rules

```
app/ ---------> protocol/gbt_node.h ---> protocol/* ---> common/
  |                                                        ^
  +-----------> hal/hal.h ---------------------------------+
  |                ^
  |                | implements
  +-----------> ports/<p>/  ------> ports/common/ , common/
  +-----------> app/output_if.h ---> hal/hal.h ---> common/
tests/ may include everything; nothing includes tests/ except tests/.
```

| From \ May include | common | protocol | hal | ports | app | platform SDK |
|---|---|---|---|---|---|---|
| common | yes | — | — | — | — | — |
| protocol | yes | yes | **no** | **no** | **no** | **no** |
| hal | yes | **no** | yes | — | **no** | **no** |
| ports | yes | **no** | yes | yes | **no** | yes |
| app | yes | `gbt_node.h` (+ public protocol headers) | yes | own port header only | yes | via port only |

`app/output_if.h` includes `hal/hal.h` (for the sink type); this is app -> hal, which is permitted.

No circular dependencies: arrows point only downward (app -> protocol -> common; app -> hal -> common; ports -> hal -> common). The protocol and the HAL never see each other; the application connects them.

## 4. Data flow

### 4.1 Receive path

1. **CAN RX interrupt** (port): read at most a fixed number of frames per invocation from the controller FIFO (bound defined per MCU in STEP 07-C), validate Classical/extended format, push each into the port's bounded RX queue. On a full queue the frame is dropped and `rx_overflow_sw` is incremented; hardware FIFO loss increments `rx_overflow_hw`. The ISR never calls protocol or application code.
2. **Platform RX queue**: bounded, single-producer (ISR) / single-consumer (main loop).
3. **Main-loop RX polling** (integration): `gbt_hal_can_get_status()` -> report any counter increase with `gbt_node_on_rx_overflow()`; then `gbt_hal_can_receive()` repeatedly, bounded per loop.
4. **`gbt_node_on_rx()`**: classify the identifier (`gbt_id`); unknown or unexpected frames are counted and ignored (GB/T §5.5).
5. **Codec and TP dispatch**: single-frame messages go to `gbt_codec`; TP.CM/TP.DT go to `gbt_tp`, which delivers a complete payload to `gbt_codec` only after full reassembly.
6. **State machine**: the role's state machine (`gbt_charger_sm` or `gbt_vehicle_sm`) consumes the decoded message.
7. **Safety evaluation**: `gbt_safety` evaluates triggers and computes the output command (§4.3).

### 4.2 Transmit path

8. **TX frame generation**: state machines and periodic scheduling (in `gbt_node_tick`) encode frames and append them to the node outbox (16 entries; overflow counted).
9. **TX queue extraction**: integration calls `gbt_node_pop_tx()` subject to the TX gate (§4.4) and the one-pending-frame rule (§4.5).
10. **Platform CAN transmission**: `gbt_hal_can_send()`; result handling per the TX ownership contract (§4.5).

### 4.3 Output path and safety inhibit (single authoritative path)

There is exactly ONE path to the physical output:

```
gbt_node (protocol)        integration (app)                    HAL (ND-1)          backend
out_cmd {seq,...}  --get--> gbt_hal_output_apply(h, &cmd) -----> sink.apply(ctx) --> output_if backend
                   <-confirm-- gbt_node_confirm_output(seq, st == GBT_HAL_OK)
```

No other function drives the output: the earlier `safe_output_hook` and
`output_if_force_safe()` proposals are withdrawn (AR-2).

Safety sequence for a trigger detected inside `gbt_node_on_rx` /
`gbt_node_on_rx_overflow` / `gbt_node_tick`:

1. `gbt_safety` latches the inhibit and sets `out_cmd = {seq+1, enabled 0, 0 mV, 0 mA}`; output state -> PENDING.
2. The state machine may queue CST/CEM; while PENDING the TX gate holds every frame (§4.4).
3. The node call returns. The integration layer immediately (before handling the next frame or calling tick) performs APPLY: `gbt_node_get_output` -> `gbt_hal_output_apply` -> sink, synchronously in the same loop iteration.
4. `gbt_node_confirm_output(seq, accepted)`: ACCEPTED opens the gate per §4.4; refused -> FAILED (ND-6 OPEN).
5. Only then can SEND hand frames to `gbt_hal_can_send()`.

The inhibit never waits for CAN transmission; at most one node call plus the
APPLY step lie between detection and sink acceptance. The 10 ms metric ends
at the `sink.apply` OK return (K-6B, Revision 3.2).

`gbt_node_confirm_output` confirms exactly one thing: whether the sink
returned GBT_HAL_OK for the command with that `seq` (software acceptance, not
a physical measurement). It is called once per new command, immediately after
`gbt_hal_output_apply`, and before the next `pop_tx`/`tx_check`. A stale `seq`
is ignored and counted.

### 4.4 TX gate policy

Every Lean message has a fixed TX class:

| Class | Messages | Purpose |
|---|---|---|
| NORMAL | CHM, BHM, CRM, BRM, BCP, CML, BRO, CRO, BCL, BCS, CCS, BSM, TP.CM, TP.DT | Handshake, configuration, ordinary charging |
| STOP | BST, CST, BSD, CSD | Stop sequence (§11.3.8–11.4.2) |
| ERROR | BEM, CEM | Timeout error messages (§11.5) |

| Output state | NORMAL | STOP | ERROR | Basis |
|---|---|---|---|---|
| ACCEPTED | send (as scheduled by the state machine) | send | send | — |
| PENDING (new command not yet confirmed) | hold | hold | hold | SAF-1 "execute before transmission" (BL L204); lasts only until the APPLY in the same loop iteration, so it never delays safety communication beyond the sink call |
| FAILED (sink refused) | discard (never sent) | **ND-6 OPEN** | **ND-6 OPEN** | Option A: hold until a later command is accepted. Option B: send, recorded as SAF-1 deviation. Selected only through `apply_fail_tx_policy`; UNSET is a configuration error |

The gate does not invent protocol behaviour: which frames exist is decided by
the state machines (Revision 3.2 K-6A/K-6B); the gate only orders them
relative to the output command.

### 4.5 TX frame ownership

| Event | Rule |
|---|---|
| `gbt_node_pop_tx()` returns true | The frame is copied to the caller and removed from the node. The integration layer owns it; the node never re-sends it. |
| Pending slot | The integration layer holds at most one popped frame and does not pop again while it holds one. FIFO outbox + one pending slot + order-preserving HAL = order preserved. |
| Before each send attempt | `gbt_node_tx_check(frame)`: SEND -> send; HOLD -> keep, retry next loop; DISCARD -> drop (gate counter). |
| `GBT_HAL_OK` | Frame handed to the controller; pending slot cleared; may be discarded by the caller. |
| `GBT_HAL_E_FULL` | Not accepted; keep it pending and retry it first next loop. No duplicate: the HAL did not accept it and the node no longer has it. |
| `GBT_HAL_E_BUS_OFF` | Not accepted; discard and count (integration counter). No bus-off recovery (Lean stretch). Periodic messages regenerate at their next period. |
| `GBT_HAL_E_IO` / `E_NOT_READY` / `E_STATE` / `E_PARAM` | Not accepted; discard and count. (ND-3, PROVISIONAL.) |

## 5. Execution model

| Aspect | Rule |
|---|---|
| Protocol processing | Single-threaded, main context only, one `gbt_node_t` per node, no re-entrancy |
| ISR | Port-only; bounded frame count per invocation; pushes into a bounded queue; increments saturating counters; never calls protocol or application |
| Queues | Port RX queue (ISR -> main, SPSC; design gated by G-SPSC); node outbox (16, single context); integration holds at most one pending TX frame (§4.5) |
| Memory | Static only; `gbt_node_t`, HAL instances and queues are caller-allocated; no malloc/calloc/free anywhere in protocol/ |
| Blocking | None: no delays, no busy waits, no waiting for TX completion |
| Time | 32-bit millisecond clock from `gbt_hal_now_ms()`, passed into every node call; wrap-safe `(uint32_t)(now - start)` comparisons |
| Loop period | `gbt_node_tick()` at least every 1 ms (PROVISIONAL budget for the 10 ms requirement) |

## 6. Platform integration

The same `common/` and `protocol/` sources are compiled unchanged for every target; only the port and the integration layer differ.

| Target | Compiler | Port | Integration | Build |
|---|---|---|---|---|
| Linux host / SIL | gcc `-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion` | `tests/support/fake_hal` (Mode A) or `ports/linux` SocketCAN (Mode B) | `tests/sil`, `app/` | CMake + CTest (automated) |
| STM32G474 (charger) | Keil ARMCC V5.06u7 `--c99` (release); arm-none-eabi-gcc (verification) | `ports/stm32/g474` (FDCAN1, Cube HAL types allowed here only) | CubeMX `main.c` USER CODE calls the integration loop | Keil project groups (Owner approval required) |
| STM32F103 (vehicle) | same | `ports/stm32/f103` (bxCAN1) | same | same |

C99 portability rules for `common/` and `protocol/`: no C11 features, no GNU extensions, no `static inline` or attributes in public headers (only ordinary declarations), no floating point, no VLAs, no recursion, no enums or bit-fields in wire layouts, unsigned elapsed-time arithmetic, explicit casts before shifts. Public headers use `<stdint.h>`, `<stddef.h>` and (in `gbt_node.h`) `<stdbool.h>`. ARMCC V5 compatibility is NOT claimed until a Keil build has been performed (STEP 07-C).

## 7. Verification architecture

| Level | Mechanism | What it proves |
|---|---|---|
| Unit tests | One C99 executable per module under `tests/unit/`, run by CTest | Module behaviour: golden vectors, boundaries, rollover, error paths |
| Fake HAL | `tests/support/fake_hal` — in-memory bus between two instances, virtual clock, TX-status and overflow injection | HAL-contract behaviour without hardware |
| Deterministic SIL (Mode A) | `tests/sil/gbt_sil` — two nodes, two fake HALs, shared 1 ms virtual clock, fixed and swappable step order | Full sessions, timeouts, fault scenarios, 10 ms software-command timing — reproducibly |
| SocketCAN SIL (Mode B) | Two processes on `vcan0` via `ports/linux`, candump capture | Same protocol sources over a real socket stack |
| Hardware | G474 + F103 with the official CubeMX/Keil projects | Real controllers, full session, BRM/BCP/BCS transport |
| Logic analyzer | CAN_RX decode + software output-command marker GPIO | Quantitative trigger-to-command latency on hardware (software command only, not physical power) |

Architecture checks automated in CTest (STEP 07-B): header include rules (§3), forbidden tokens in `protocol/`/`common/`, no writable static data in protocol objects (`nm`), C99 cross-compile of `protocol/` with arm-none-eabi-gcc.

## 8. Repository structure (current, adapted)

```
common/        gbt_types.h gbt_types.c                       NEW (07-A.1)
hal/           hal.h                                         NEW (07-A.1, replaces B0 draft)
app/           output_if.h                                   NEW (07-A.1)
               charger_app.c vehicle_app.c battery_model.c sim_plant.c   DEFER
protocol/      gbt_node.h                                    NEW (07-A.1)
               gbt_id.[ch] gbt_codec.[ch] gbt_timer.[ch]
               gbt_ring.[ch] gbt_outbox.[ch] gbt_provisional.h   NEW (B0 session, reused)
               gbt_node.c gbt_tp.[ch] gbt_charger_sm.[ch]
               gbt_vehicle_sm.[ch] gbt_safety.[ch]           DEFER
ports/common/  gbt_hal_lc.h                                  NEW
ports/linux/   ports/stm32/{g474,f103}/                      DEFER (.gitkeep kept)
tests/unit/    bootstrap_smoke.c                             KEEP
               test_{ring,outbox,timer,id,codec,hal_fake}.c  NEW (B0 session)
tests/support/ test_assert.h fake_hal.[ch]                   NEW
tests/sil/     gbt_sil.[ch]                                  NEW (infrastructure only)
docs/          ARCHITECTURE.md                               NEW
               PROJECT_PLAN.md PROJECT_BRIEF.md              KEEP
               CHAT_HANDOVER.md                              KEEP (historical; to be updated)
               sources/ (8 imported docs + manifest)         NEW (K-9a)
firmware/      stm32 bring-up projects                       KEEP (historical evidence)
build/         historical build outputs and hardware logs    KEEP (gitignored)
CMakeLists.txt README.md                                     KEEP (to be extended in 07-B)
```

## 9. ND-1 compatibility (AR-1)

ND-1 (B1-05; Revision 3.2 §4.3.1–4.3.3) is preserved. `hal/hal.h` contains the
approved status set, CAN frame/status interfaces, output command/feedback
interfaces, output sink, lifecycle UNINIT -> READY -> BOUND -> CLAIMED,
bind/unbind, claim/release, lifetime and re-entrancy rules, and the
one-concrete-HAL-per-executable rule. API names, parameters and return codes
are unchanged.

Two compatibility points, both forced by the STEP 07-A.1 rule "the protocol
core must not include hal.h or call the HAL", are listed for Owner review:

| # | Revision 3.2 text | STEP 07-A.1 form | Effect on ND-1 |
|---|---|---|---|
| C-1 | `gbt_can_frame_t`, `gbt_output_cmd_t`, `gbt_output_fb_t` declared in `hal/hal.h` | Declared in `common/gbt_types.h` with identical fields; `hal/hal.h` includes it, so every ND-1 user still sees them via `hal/hal.h` | Location only; types unchanged |
| C-2 | `gbt_hal_claim` / `gbt_hal_release` "called only by `gbt_node_init` / `gbt_node_deinit`"; protocol runtime calls `gbt_hal_output_apply` and `gbt_hal_can_send` | The integration layer performs claim/release when it attaches/detaches a node, and performs every runtime HAL call (attach/detach sequence in `protocol/gbt_node.h`) | Caller changes from protocol to integration; lifecycle, states and rules unchanged. **Requires Owner acknowledgement.** |

`app/output_if.h` does not replace the sink: it only builds the
`gbt_output_sink_t` that is bound with `gbt_hal_bind_output_sink()`.
