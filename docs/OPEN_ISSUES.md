# Open Issues and Limitations

**Status:** STEP 07 is closed as **CONDITIONAL PASS**. The owner decided not to spend further effort on
non-essential issues. Accordingly:
- No item here is being worked on.
- Items marked *owner review* need a decision before anyone changes code.
- The review of 2026-11-24 found **no critical functional or safety defect**.

## 1. Critical defects

None found.

The review covered:
- the safety module and output path;
- the node runtime, TX gate and timers;
- TP buffer bounds;
- both ports' interrupt paths, TX ownership and error handling;
- the SPSC queue;
- the FI-04 instrumentation;
- both `main.c` application sections.

## 2. Items requiring owner review

| ID | Location | Finding | Impact | Evidence | Proposed minimal repair (needs approval) |
|---|---|---|---|---|---|
| OI-01 | `protocol/gbt_safety.c:85-89` (`gbt_safety_apply_bcl`) | A non-zero demand below the charger minimum (1 A) is raised to that minimum **after** the BCP clamp. If a vehicle's BCP maximum current were below the charger minimum, the commanded current would exceed the vehicle's own limit (SAF-2). | Low. Needs a BCP maximum below 1 A; never occurs with the project battery model. It is a safety calculation, so not changed. | code reading; the clamp order is visible in lines 70–89 | After raising to the minimum, re-apply `ma = min(ma, max_ma)`, or stop with a parameter-mismatch reason. Add a SIL case with BCP max < charger min. |
| OI-02 | `source/Project1_F103B_Receiver_CubeMX/MDK-ARM/Project1_F103.uvprojx` | The hardware-tested F103 image was built from the Windows copy of this file, which differs in ARMCC selection metadata and `Ir2Chk` 1 → 0. This repository holds the pre-build copy. | Rebuilding from this repository may not reproduce the tested F103 AXF (`3E8CC512…`). The source files are identical. | owner Windows check (102 manifest mismatches, 0 in `.c/.h/.s/.ioc/.sct`) | Copy the tested `.uvprojx` from the Windows export into `source/`, record its hash in `docs/PROVENANCE.md`, and re-run `check_keil_refs.sh`. |
| OI-03 | whole source tree | The STEP 07 working tree was not under version control; the older development repository held stale code. | Changes were not traceable. | review of 2026-11-24 | **Addressed by this repository:** its initial commit is the first versioned baseline of the release source. |
| OI-04 | `ports/stm32/g474/port_g474.c`, `ports/stm32/f103/port_f103.c`, `app/gbt_integration.c` | Bus-off is observed but neither inhibited nor recovered; F103 has AutoBusOff disabled. The output is disabled only when a protocol timeout fires, about 1 s later. Requirement **CAN-1 (MUST)** of `host/docs/sources/REQUIREMENTS_BASELINE.md` asks for output 0 within 10 ms, a latched fault and recovery. The lean plan (`docs/design/PROJECT_PLAN.md`, Cuts) moved CAN-1 to CAN-4 to a **stretch item**, so this is within the approved lean scope. | Within lean scope. Outside it, a missed MUST, with the safe state reached about 1 s late. | requirement text; lean plan | Keep as accepted lean-scope behaviour, or add a bus-off output inhibit (safety-logic change: approval, ARMCC rebuild, hardware re-test). |
| OI-11 | G474 `startup_stm32g474xx.s` / CubeMX stack size (0x400) | ARMCC-based worst-case estimate about 800 B of 1024 B. It includes one extended FP frame, because `_fp_init` executes `vmsr`, and three-level interrupt nesting (SysTick priority 0). This is a smaller margin than the F103 (about 652 B); no runtime measurement exists. | Low; 224 B margin by estimate. | `docs/VERIFICATION.md` §4.4 | Optional: raise the G474 stack to 0x800 in CubeMX, or port the F103 paint helper and measure. Either is a configuration change: approval and re-test. |
| OI-05 | G474 `main.c` (`apply_fail_tx_policy = OPTION_A`) | ND-6 (what to send after a refused output command) is an open decision; the board uses option A as a "test setting". | Defines the frames sent after a sink refusal. | `gbt_node.c` `gbt_node_tx_check`; `docs/design/PROVISIONAL_PARAMETERS.md` | Owner decision on ND-6. |
| OI-06 | comments in firmware sources | References to moved documents: G474 `main.c` ("ports/stm32/g474/MAIN_C_INTEGRATION.md in the WSL repository"); F103 `main.c` (`docs/WATCH_VARIABLES.md` and `WATCH_VARIABLES.md`); `ports/common/gbt_spsc.[ch]` (`docs/STEP07C1_SPSC_DESIGN.md`, now `docs/design/`). | Documentation only. Left unchanged so the firmware stays byte-identical. | grep | Correct them together with the next approved firmware change. |
| OI-07 | `protocol/gbt_node.c` `gbt_node_restart_session` | Public function, never called and not tested. The restart policy is "reset only". | Dead, untested code in the protocol core (removed by the linker). | grep: 0 callers in `source/` and `host/tests/` | Remove it, or add a SIL test when a restart policy is decided. |
| OI-08 | `protocol/gbt_safety.c` `gbt_safety_disable(n, reason)` | The `reason` argument is discarded; `gbt_node_peer_error` passes the TIMEOUT reason for a peer BEM/CEM. | Diagnostics clarity only. | code reading | Either drop the parameter or record it in diagnostics. |
| OI-09 | both `main.c` USER CODE sections | Three overlapping Watch layers: raw status/port structs, flat `g_hw_diag_*` (C2), structured `g_gbt_*` and acceptance mask (C2.4), plus `g_gbt_fi04`. About 400 lines per board. | Maintainability and RAM; the procedures depend on these names. | `main.c` | Keep for now. If simplified later, keep `WATCH_VARIABLES.md` and the procedures in step. |
| OI-10 | integration layer | The legacy `output_shutdowns` / `g_hw_diag_out_shutdown_count` also counts a shutdown the sink refused. FI-04 uses `g_gbt_fi04.shutdowns` (accepted and confirmed only). | Could be misread in reports. | FI-04 implementation report | Document (done), or align the counter in a later approved change. |

## 3. Accepted limitations (owner decision 2026-11-24; must stay visible)

| ID | Limitation |
|---|---|
| L1 | No Flash readback comparison; the flashed images are identified only by the owner-reported AXF hashes. |
| L2 | No archive of the ten FI-04 trials in WSL (owner-reported; one trial inspected). |
| L3 | No recorded hardware session ending in END on both boards (lean check 1). |
| L4 | Incomplete hardware TP readouts, including BCS (lean check 3). |
| L5 | No hardware identifier capture (lean check 2). |
| L6 | No runtime stack watermark on either board. Static ARMCC-based estimates: F103 about 652 B, G474 about 800 B, of 1024 B each. The F103 firmware already paints its stack. |
| L7 | Two-process vcan test not run on the release source. |
| L8 | Provisional protocol parameters (`docs/design/PROVISIONAL_PARAMETERS.md`). |
| L9 | Software-tick timing used instead of the planned logic-analyzer measurement (lean check 5). |
| L10 | No physical power-stage current measurement (no power stage). |
| L11 | Lean check 6: the ARMCC build is a separate Windows step. |
| L12 | Open wiring checks: F103 A passive method and back-powering; PD1050S pin 8 on both boards (`docs/hardware/HARDWARE_TOPOLOGY.md` §2). |

## 4. Deferred work (only with new owner authorisation)

- Resolve OI-02 (tested F103 `.uvprojx`). This is the cheapest remaining traceability step.
- One recorded normal session (criteria A1–A13), including the F103 stack watermark scan (§8 of the hardware procedure). This closes L3, L4 and L6 without code changes.
- Two-process vcan run on this source (L7).
- USB-CAN capture for the identifier hand check (L5).
- Restart policy after END/EXIT; decisions on ND-6 and ND-7.

## 5. Optional improvements

- Logic-analyzer GPIO timing; DWT sub-millisecond timing.
- Resolve PROVISIONAL parameters against the J1939-21:2006 text.
- Remove the G474 user-button EXTI remnant (`BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI)`, priority 15, empty callback) and the unused CMSIS DSP/NN/RTOS vendor folders via CubeMX.
- Make trace equivalence independent of the historical sim-lean repository (store a reference trace).
- Long-duration CAN stress test; bus-off recovery (OI-04).
