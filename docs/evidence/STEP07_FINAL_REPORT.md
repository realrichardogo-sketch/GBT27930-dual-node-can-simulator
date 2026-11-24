# STEP 07 Final Report: Project 1, GB/T 27930-2023 System A Dual-Node CAN Simulator

| Field | Value |
|---|---|
| Date | 2026-11-24 |
| Phase | STEP 07, final closeout |
| **Final verdict** | **CONDITIONAL PASS** (owner scope decision, 2026-11-24) |
| Release source (WSL) | `~/projects/step07_fi04_working/STEP07_C2.4_DUAL_NODE_TP_TEST_20261118_172749/` |
| Hardware-tested export (Windows) | `D:\Download\STEP07_C2.4_DUAL_NODE_TP_TEST_20261118_172749_FI04_IMPL_EXPORT_20261123_193906` |
| Archive snapshot | `~/project_backups/STEP07_FINAL_CLOSEOUT_20261124_213047/` |
| Companion document | `STEP07_FINAL_HANDOVER.md` |

**Status.** The owner decided to close STEP 07 as CONDITIONAL PASS:
- No further testing, debugging or non-essential fixes are authorised.
- That decision is a scope decision. It does not claim that every original acceptance criterion passed.
- The unmet criteria are listed in §11. They are not rewritten and not marked PASS.

**Not claimed:**
- Project 1 is not standard-certified and not production-ready.
- It is a lean portfolio simulator of a *core subset* of GB/T 27930-2023 System A.
- Both nodes share one reading of the standard. It has not been checked against real chargers or vehicles.

## Evidence labels

| Label | Meaning |
|---|---|
| **IV** | Independently verified in WSL from files present in the repository or working copy (hashes, logs, tests re-run on 2026-11-23, file inspection on 2026-11-24). |
| **OR** | Owner-reported. Observed by the owner on hardware or on Windows; no artifact is archived in WSL. |
| **HR** | Historical result. Valid for an earlier source revision; not re-run on the release source. |
| **NV** | Not verified. No evidence available. |

---

## 1. Project objective and approved lean scope

**Objective.** A dual-node CAN simulator of a core subset of GB/T 27930-2023 System A:
- a charger node (STM32G474RE, FDCAN) and a vehicle node (STM32F103ZET6, bxCAN);
- classical CAN 2.0B, 29-bit identifiers, 250 kbit/s;
- a shared portable C99 protocol core that also runs in Linux SIL.

**Scope authority.** `docs/PROJECT_PLAN.md` (in `~/projects/gbt27930-sim-lean`; archived copy under
`history/`). Its principle is: "SIL carries the proof, and hardware shows one clean session and one
timing measurement."

Out of scope (cut by the plan):
- reconnection;
- pause;
- the vehicle-initiated stop on hardware;
- the 21-criterion acceptance set.

The original **"Done when"** checks, reproduced verbatim:
1. A full session completes on both boards with no error messages.
2. Captured identifiers and bytes match the tables by hand check, for example CHM = 0x1826F456.
3. BRM, BCP and BCS transfer over the transport protocol on hardware.
4. BCL and CCS timeouts each produce their error message and a safe state in SIL.
5. The output command reaches 0 within 10 ms, in SIL and on the logic analyzer.
6. One command builds all three targets and runs the tests.

Their status is in §11.

## 2. Chronological STEP 07 history

| Date (2026) | Step | Content | Outcome | Label | Evidence |
|---|---|---|---|---|---|
| 11-11 | Bootstrap | Repository and lean plan | Git commit `f81e259`, the only commit | IV | `git log` in `gbt27930-sim-lean` |
| ≤ 11-16 | 07-A / A.1 | Architecture packages Rev 1–3.2, decisions AR-1..AR-4, ND-1 HAL contract | `ARCHITECTURE.md` v1.0 frozen, owner-approved. The decision packages are not stored in the repository. | IV (document) / NV (packages) | `docs/ARCHITECTURE.md`, `docs/INDEX.md:114-118` |
| 11-16 | 07-B | Protocol core, codecs, TP, charger and vehicle state machines, safety and timeouts, deterministic SIL, SocketCAN two-process SIL, T-3c, T-5 | Closed as complete in software. vcan 4/4 and CTest 18/18 on the owner host. | HR | `docs/STEP07_IMPLEMENTATION_STATUS.md`, `docs/STEP07_LINUX_SIL_REPORT.md`, `docs/STEP07_TEST_RESULTS.md` |
| 11-16 | C0 | STM32 integration readiness review | CONDITIONALLY READY. No report file was saved. | NV (no file) | status document |
| 11-16 | C1.1 | ISR-safe SPSC RX queue `ports/common/gbt_spsc` | CONDITIONALLY COMPLETE (GCC only) | HR | `docs/STEP07C1_SPSC_DESIGN.md`, `docs/STEP07C1_IMPLEMENTATION_REPORT.md` |
| 11-16 | C1.1-FINAL | Full repository backup | 273 files, 84 directories, 0 symlinks. Re-check on 11-24: 271/272 manifest entries OK; `.git/index` differs (Git metadata only, likely an index stat refresh). | IV | `~/project_backups/gbt27930-sim-lean_STEP07_C1.1_FINAL_20261116_153204/` |
| 11-16 | C1.2-A2/B1/B2 | G474 FDCAN audit, adapter `port_g474`, Keil integration package | B1 software complete (25 CTest). B2 export verified with GCC only ("No ARMCC/Keil build"). Export preflight approval is not hardware validation. | HR | `docs/STEP07C1_2*_REPORT.md`, `export/STEP07-C1.2-B2_G474_R2/VERIFICATION_REPORT.md` |
| 11-16 | C2.1 | F103 bxCAN adapter `port_f103` | CONDITIONALLY PASS, no hardware. Owner ARMCC V5.06u7 build 0 errors, 0 warnings (Code 18456). | HR | `export/STEP07-C2.1-F103_EXPORT_step7/`, `evidence/STEP07-C2.1-F103/keil_results_20261116_223601_step7/` |
| 11-16/17 | C2.2-E | F103 stack-paint debugger script | Prepared. No watermark result recorded. | NV | `export/STEP07-C2.2-E-F103_STACK_PAINT_EXPORT_step7_R1/` |
| 11-17 | C2 bring-up | Owner integrated sources on Windows and ran both boards | Pre-C2.4 hardware failure diagnosed as start order: F103 B phase 7, step 42, last RX `0x081FF456` (CEM). Reproduced in SIL `scenario_start_order`. | OR (hardware) + IV (SIL) | `incoming/STEP07-C2_SOURCE_20261117/`, C2.4 `CHAT_HANDOVER.md`, `HARDWARE_TEST_PROCEDURE.md:12-17` |
| 11-18 | C2 TP observability / overnight | TP statistics, Watch structs, evlog | Software complete; "ARMCC NOT YET VERIFIED — HARDWARE NOT TESTED" | HR | `STEP07-C2_OVERNIGHT_20261118_023613/reports/FINAL_ACCEPTANCE_REPORT.md` |
| 11-18 12:34 | vcan rerun | Two-process SocketCAN SIL | `linux_sil_vcan` PASS; full CTest 30/30 (on C2, pre-C2.4 source) | HR | `step07_stm32_workspace/evidence/STEP07-C2-vcan-rerun_20261118_123421/` |
| 11-18 | C2.4 | Dual-node TP test package, acceptance mask bits 0–17, criteria A1–A13 | Owner ARMCC build 0 errors, 0 warnings (G474 Code 30604, F103 Code 21482). No hardware results recorded for A1–A13. | IV (build logs in tree) / NV (hardware) | `ACCEPTANCE_CRITERIA.md`, `source/*/MDK-ARM/*/*.build_log.htm` |
| 11-23 | FI-04 implementation | Shutdown response-time instrumentation in the shared core and a G474 Watch mirror | Software CONDITIONALLY APPROVED. Pre-change backup verified. | IV | `FI04_IMPLEMENTATION_REPORT.md`, `FI04_TEST_REPORT.md`, `evidence/fi04/` |
| 11-23 | FI-04 ARMCC build | Both targets rebuilt on Windows | 0 errors, 0 warnings; MAP and AXF hashes in §7 | OR | owner statement |
| 11-23/24 | FI-04 hardware | 10 independent trials | 10/10 PASS | OR | owner statement; one trial inspected in conversation |
| 11-24 | Audits and closeout | Read-only acceptance audit and closeout review; this report | CONDITIONAL PASS | — | this document |

## 3. Software architecture (IV)

| Layer | Location (release source) | Content |
|---|---|---|
| Protocol core (pure C99, no writable statics) | `source/Project1_SharedCore/protocol/`, `common/` | identifiers, codecs (18 Lean messages + BCP/BCS payloads), bit utility, TP (RTS/CTS/DT/EOM/Abort, T1–T4), charger and vehicle state machines, node, safety/timeouts, outbox, ring, timer |
| HAL contract (ND-1) | `source/Project1_SharedCore/hal/hal.h` | CAN RX/TX, clock, output apply/feedback |
| Integration | `source/Project1_SharedCore/app/` | `gbt_integration` (loop: overflow → RX ≤ 16 → tick → APPLY → SEND; event log; FI-04), `sim_plant`, `battery_model` |
| Ports | `source/Project1_SharedCore/ports/` | `stm32/g474/port_g474` (FDCAN), `stm32/f103/port_f103` (bxCAN), `common/gbt_spsc` (ISR → main RX queue); Linux SocketCAN port in `regression/gbt27930-sim-lean/ports/linux/` |
| Board projects | `source/Project1_G474/`, `source/Project1_F103B_Receiver_CubeMX/` | CubeMX + Keil MDK. Application code only inside USER CODE blocks of `main.c`. |
| Host harness | `regression/gbt27930-sim-lean/` | CMake, unit/SIL/port tests, fakes. The 41 shared-core files listed in `SYMLINKS.txt` are byte-identical copies of `source/`. |

Rules enforced by CTest:
- `architecture_rules`
- `protocol_no_writable_statics`
- `source_manifest`
- `cross_c99_cortex_m`, `cross_c99_spsc`, `cross_c99_g474`, `cross_c99_f103`

All pass on the release source (IV, 2026-11-23).

## 4. STM32 hardware integration

| Item | Value | Label |
|---|---|---|
| Charger | NUCLEO-G474RE, FDCAN1 classic mode, 250 kbit/s (170 MHz / 40 / 17), sample point 88.2 %, SA 0x56, TP receiver | IV (source, `HARDWARE_TOPOLOGY.md`) |
| Vehicle | F103 B (STM32F103ZET6), bxCAN1 remap 2 (PB8 RX / PB9 TX), 250 kbit/s (36 MHz / 9 / 16), sample point 87.5 %, SA 0xF4, TP sender | IV (source) |
| F103 A | **Passive transceiver carrier, not a CAN node.** G474 PA11/PA12 are wired to its PD1050S. Its MCU must not drive TXD/RXD. | IV (document) |
| Bus | CANH–CANH, CANL–CANL, common GND, two 120 Ω terminations | IV (document) / OR (wiring) |
| Filters | G474: 1 extended filter → RX FIFO 0, everything else rejected. F103: bank 0, 32-bit ID/mask → FIFO 0 | IV (source) |
| Interrupts | G474 `FDCAN1_IT0` (RX FIFO 0, lost, bus-off, error passive, warning). F103 `USB_LP_CAN1_RX0` priority 5, SysTick priority 15. TX completion polled in main. | IV (source) |
| Debug | Two ST-LINKs (0483:374E, 0483:3752); serial-number selection issue resolved; Keil Watch and peripheral register views (FDCAN PSR/ECR/TXFQS/IR/TXBRP/TXBTO; bxCAN MCR/MSR/BTR/ESR/RF0R/TSR) | OR |
| Open wiring checks | F103 A passive method and back-powering; PD1050S pin 8 (S/STB) tied to GND on both boards | NV (owner checks, non-blocking: the bus works) |

## 5. CAN and Transport Protocol implementation

| Item | Implementation | SIL evidence | Hardware evidence |
|---|---|---|---|
| CAN identifiers (18 Lean) | `protocol/gbt_id.c`; formula per GB/T Table 1 / J1939-21 (VERIFIED_GBT, derived) | `test_id` 33 checks (IV) | Single ID observed: CEM `0x081FF456` received by F103 B (OR). **No capture.** |
| Codecs | `gbt_codec.c`, `gbt_codec_ext.c`, `gbt_bits.c` | `test_codec` 122, `test_codec_ext` 71, `test_bits` (IV) | — |
| Negative-current encoding | 0.1 A/bit, −400 A offset (raw 0x0000 = −400.0 A) | `test_codec.c:180-205` (IV) | — |
| TP (RTS/CTS/DT/EOM/Abort, T1–T4, 10 ms DT spacing) | `gbt_tp.c` | `test_tp` 343 checks; `scenario_missing_tp_dt` (IV) | BRM and BCP over TP are implied by reaching step 10 (OR). BCS and direct TP counters: **NV**. |
| Handshake, configuration, charging | charger/vehicle state machines | normal-session scenarios (IV) | Step 10 reached with output enabled in every FI-04 trial (OR) |
| Normal termination (step 41) | BST/CST | `scenario_normal_vehicle_stop`, `scenario_charger_stop` (IV) | **NV** |
| Timeout termination (10 → 40 → 42) | TO-xx timers → safety → C-TO → EXIT | `scenario_bcl_timeout`, `scenario_ccs_timeout` (IV) | FI-04 10/10 (OR) |
| BEM/CEM | sent on timeouts | BEM/CEM counted in SIL scenarios (IV) | CEM `err_fields` not reported for FI-04 (NV) |

**Provisional parameters.** Several parameters are provisional, as recorded in `docs/PROVISIONAL_PARAMETERS.md`:
- K-3 data-field bit numbering;
- K-4 TP timers T1–T4, priority 7 and control bytes (SECONDARY);
- K-5 BCP and CSD units and offset;
- UR2 current endpoints.

They are simulator assumptions, not confirmed standard values.

## 6. Unit and SIL evidence

| Run | Source | Environment | Result | Label |
|---|---|---|---|---|
| **FI-04 post-change, 2026-11-23** | **release source** (post-change manifest still matches on 2026-11-24) | WSL2, host gcc `-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion` | **29/30 PASS, 1 NOT RUN (`linux_sil_vcan`: no `vcan0`), 0 warnings** | **IV** |
| FI-04 pre-change, 2026-11-23 | C2.4 | same | 29/30, vcan not run | IV |
| vcan rerun, 2026-11-18 | C2 (pre-C2.4) | owner host | 30/30, including `linux_sil_vcan` | HR |
| STEP 07-B final, 2026-11-16 | 07-B | owner host | 18/18 | HR |

**Release-source details** (logs in `evidence/fi04/` of the release source):
- Check counts:
  - `test_sil_session` 1087/0 failures;
  - `test_port_g474_session` 297/0;
  - `test_port_f103_session` 439/0, including the ASan/UBSan variant.
- Unit tests: ring, outbox, timer, id, codec, bits, codec_ext, tp, hal_fake, port_linux, spsc, spsc_signal, port_g474 (+ASan), port_f103 (+ASan).
- SIL scenarios:
  - normal vehicle stop
  - charger stop
  - BCL timeout
  - CCS timeout
  - current out of range
  - missing TP DT
  - voltage-minimum boundary
  - RX queue overflow
  - RX hardware overflow
  - overflow when not charging
  - driver refusal
  - start order
  - five FI-04 scenarios
- Trace equivalence: every CAN frame of five dual-node scenarios from the original sim-lean, the pre-change tree and the post-change tree is identical (1186 lines, sha256 `fcc0b360f774163a0d0f4568f430e0ebf94000d22f65d0a5b78e74e45f1c7a94`). FI-04 did not change protocol behaviour. (IV)
- GCC board compile-only of every `.c` in both Keil projects: PASS. This is GCC, not ARMCC. (IV)

## 7. ARMCC build evidence

**Toolchain:** Keil MDK, ARMCC V5.06 update 7 (build 960), on Windows.

| Build | G474 Code / RO / RW / ZI | F103 Code / RO / RW / ZI | Errors / warnings | Label |
|---|---|---|---|---|
| 2026-11-18 (pre-FI-04; outputs in the WSL tree) | 30604 / 1076 / 372 / 5940 | 21482 / 814 / 336 / 5728 | 0 / 0 both | IV (logs) / HR (superseded) |
| **2026-11-23 (FI-04, hardware-tested)** | **31336 / 1076 / 372 / 6116** | **21914 / 814 / 336 / 5816** | **0 / 0 both** | **OR** |

**Consistency check (IV, arithmetic against the archived 11-18 logs):**
- ZI grew by +176 B on G474 and +88 B on F103. This exactly matches the FI-04 design: `gbt_fi04_t` is 88 B inside `gbt_integ_t`, plus the 88 B G474 Watch mirror.
- Code grew by +732 B and +432 B. RO and RW are unchanged.

**FI-04 build facts (OR, from the Windows MAP, `.htm` and logs):**
- G474 MAP: `g_gbt_fi04 = 0x200011a8, Data 88, main.o(.bss)`.
- F103 MAP: `RW_IRAM1` at `0x20000000`, size `0x1808`, maximum `0x10000`; no RAM2 execution region.
- F103 `.htm`: `Maximum Stack Usage = 432 bytes + Unknown`.
- AXF SHA-256:
  - G474: `5A097A1F43E644FA70E18C70F0BBDC76C294A6ECDCF3DD6E756C9BFDF31763A4`
  - F103: `3E8CC5120BF7EAA423A3776826DFE315029A25554C3EA11BF7F93807024D3A05`

The 11-23 logs, MAP, `.htm` and AXF files are on Windows only. **They are not in WSL and not in the archive snapshot.**

## 8. FI-04 ten-trial hardware results

**What FI-04 measures:**
- T0 is the start of the integration pass before the one that handled the timeout.
- T1 is a fresh clock read after the sink accepted the disabled output command and the node confirmed that sequence.
- The clock is 1 ms (`HAL_GetTick`).
- PASS if elapsed ≤ 9 ms. FAIL if ≥ 10 ms. An invalid reference (overflow, RX limit, RX error, status error, no previous pass) is never a PASS.
- **This is software output-command acceptance and confirmation, not physical current decay.**

**Procedure performed by the owner, per trial:**
1. Reset and start both nodes.
2. Establish a session.
3. Wait for G474 step 0x0A with the output enabled.
4. Hold F103 B RESET for about 3 s.
5. Let the G474 detect the timeout.
6. Halt the G474 and inspect the diagnostics.
7. Record the result.
8. Restart both nodes.

This procedure is a valid variant of `FI04_HARDWARE_TEST_PROCEDURE.md` §3, because the silence lasts longer than TO-14 (1000 ms).

**Result: 10/10 trials PASS (OR).**

Final G474 diagnostics, reported as identical for every trial (OR):

| Variable | Value |
|---|---|
| `g_hw_diag_gbt_step` | 0x2A (42, EXIT) |
| `g_hw_diag_gbt_timeouts` | 1 |
| `g_hw_diag_out_enabled` | 0 |
| `g_hw_diag_out_inhibit_latched` | 1 |
| `g_hw_diag_out_apply_status` | 0 |
| `g_hw_diag_out_state` | 0 (no pending command) |
| `g_hw_diag_out_shutdown_count` | 1 |
| `g_gbt_fi04.pass_count` / `fail_count` / `invalid_count` | 1 / 0 / 0 |
| `g_gbt_fi04.max_valid_ms` | 1 |

Inspected trial, event log and FI-04 internals (OR, reviewed in conversation):
- Event 43 at 2413 ms: step 0x0A → 0x28.
- Event 44 at 3413 ms: step 0x28 → 0x2A.
- The 1000 ms gap equals the designed C-TO → EXIT delay.
- `apply_status = GBT_HAL_OK`, `ref_flags = 0`; accepted and confirmed sequence numbers matched.

**Internal consistency (IV, design-level reasoning on the reported values):**
- Step 42 together with `timeouts = 1` is reachable only through the timeout path; a normal BST stop ends at step 41.
- `pass_count = 1` is recorded only when all of these hold: `ref_flags = 0`, sink OK, node-confirmed sequence, and elapsed ≤ 9 ms (`gbt_integration.c`, close logic).

**Archive status:**
- One trial's screenshots and event log were reviewed in conversation.
- **No per-trial artifacts for the ten trials are archived in WSL (NV).**
- Per-trial `elapsed_ms`, `t0_ms`/`t1_ms` and CAN error counters were not reported.

## 9. Safety and shutdown-timing evidence

| Item | Evidence | Label |
|---|---|---|
| BCL timeout → DISABLE_OUTPUT, BEM/CEM, safe state | `scenario_bcl_timeout` | IV |
| CCS timeout → safe state | `scenario_ccs_timeout` | IV |
| Out-of-range current, voltage minimum → disable | `scenario_current_out_of_range`, `scenario_voltage_min_boundary` | IV |
| TX gating and output sequencing unchanged by FI-04 | trace equivalence, unchanged `protocol/` | IV |
| FI-04 logic: 9 ms PASS / 10 ms FAIL boundary, invalid-reference handling, wraparound, no duplicates, sink refusal, sequence mismatch, T1 freshness | 11 required unit cases plus integration scenarios; mutants (limit 9 → 10; stale T1) both killed | IV |
| SIL shutdown timing | elapsed 1 ms in all valid scenarios | IV |
| Hardware timeout → output inhibited and latched | FI-04 10/10 | OR |
| Hardware software-command timing | `max_valid_ms = 1` (limit 9) | OR |
| Logic-analyzer timing (plan check 5) | not performed; replaced by FI-04 software timing (owner-accepted deviation D3) | NV |
| Physical power-stage current decay | no power stage exists | NV (out of scope) |

**F103 stack (IV, analysis of the 2026-11-18 ARMCC `.htm`; the 11-23 FI-04 `.htm` reports the same 432 B, OR):**
- `Stack_Size` is 0x400 (1024 B); the A13 budget is 768 B.
- The "Unknown" part consists of:
  - four function-pointer targets of 0–12 B, all leaves, none on the 432 B chain;
  - default fault handlers that branch to themselves (reported as "cycles", not recursion);
  - pre-`main` C-library and startup stubs.
- No hand-written assembly is on the runtime path.
- Interrupt nesting: CAN RX0 (144 B chain) can preempt SysTick, with 32 B exception frames each.
- Estimated worst case: 432 + 12 + 32 + 0 + 32 + 144 ≈ **652 B of 1024 B**.
- This is a bounded static estimate. **No runtime watermark was measured (NV).**

## 10. Source-integrity and firmware-traceability findings

| Finding | Label |
|---|---|
| The release source matches `FI04_POST_CHANGE_SHA256SUMS.txt` (2802 files; manifest sha256 `2ba663a394b7536a268eb131ae56a372be046cf2a1c4985679dae81f4d2daf8c`), all OK on 2026-11-24, no newer files | IV |
| Original C2.4 manifest `SHA256SUMS.txt` `bd1ce980bb8a2fc27b95a3c2de53e49cb055c5bbc6e338dc25391d1aa5a0758b` unchanged; original WSL export untouched | IV |
| Pre-FI-04 backup `~/project_backups/STEP07_FI04_PRE_IMPL_20261123_185208/` verified identical to the pre-change tree | IV |
| Windows export against the post-change manifest: 2802 checked, 2700 PASS, 102 MISMATCH, **0 mismatches in `.c/.h/.s/.ioc/.sct`** | OR |
| The 102 mismatches are Keil-generated outputs (the manifest holds the 11-18 outputs; Windows holds the 11-23 rebuild) and project/debug metadata | OR |
| **F103 `.uvprojx` on Windows differs from WSL:** ARMCC selection metadata, and `Ir2Chk` 1 → 0 (second internal RAM region disabled). The F103 MAP shows a single `RW_IRAM1` region within its 64 KB maximum and no RAM2 execution region. The WSL release source therefore does **not** carry the exact project configuration that produced the tested F103 image. | OR (difference) / IV (WSL side unchanged) |
| Three key files on Windows (`gbt_integration.c` `a9807df1…`, `gbt_integration.h` `73a47194…`, G474 `main.c` `0efebe6f…`) equal the WSL release source | OR (Windows) / IV (WSL) |
| AXF fingerprints recorded (§7). **Flash content was not read back and compared**, so the flashed images are identified only by the build that was flashed. | OR / NV (Flash identity) |
| Git: `~/projects/gbt27930-sim-lean` has only bootstrap commit `f81e259`. Its working tree is stale (11-16 code: no C2/C2.4/FI-04) and is **not the release source**. The release source is not under version control (commits require separate owner approval). | IV |
| C1.1 backup: 271/272 entries OK; `.git/index` changed (metadata only) | IV |

## 11. Remaining unverified acceptance criteria

The original lean "Done when" checks are **not** rewritten:

| # | Original check | Status | Label |
|---|---|---|---|
| 1 | Full session completes on both boards with no error messages | **NOT VERIFIED.** No archived hardware session reaching END (step 41) on both boards. Hardware reached step 10 (OR). | NV |
| 2 | Captured identifiers and bytes match the tables by hand check | **NOT VERIFIED on hardware.** No capture. IDs verified in SIL; one ID (CEM `0x081FF456`) observed on hardware. | NV (hardware) / IV (SIL) |
| 3 | BRM, BCP and BCS transfer over TP on hardware | **PARTIAL.** BRM and BCP implied by reaching step 10 (OR). BCS and direct TP readouts (A5–A9) not verified. | OR (implied) / NV |
| 4 | BCL and CCS timeouts produce their error message and a safe state in SIL | **MET** | IV |
| 5 | Output command reaches 0 within 10 ms, in SIL and on the logic analyzer | SIL **MET** (IV). Hardware: software timing 1 ms (OR). **Logic-analyzer measurement NOT performed** (deviation D3). | IV / OR / NV |
| 6 | One command builds all three targets and runs the tests | **NOT MET as written.** CMake builds and tests the host; Keil builds the boards separately on Windows. | IV (separate commands) |

C2.4 hardware criteria A1–A13 (`ACCEPTANCE_CRITERIA.md`): no recorded results (NV). A13 is the F103 stack watermark.

## 12. Accepted risks and deviations

The owner explicitly accepted the following on 2026-11-24, to be documented instead of worked on:

| ID | Accepted limitation |
|---|---|
| L1 | No independent Flash readback comparison |
| L2 | No complete ten-trial FI-04 evidence archive in WSL |
| L3 | No direct archived hardware evidence of a full normal session ending on both boards |
| L4 | Incomplete direct hardware TP readouts, including BCS |
| L5 | No complete hardware identifier capture |
| L6 | No F103 runtime stack-watermark measurement (static estimate about 652 B of 1024 B only) |
| L7 | Release source has not run the two-process vcan test (last pass on C2 source, 11-18) |
| L8 | Some protocol parameters remain PROVISIONAL (`docs/PROVISIONAL_PARAMETERS.md`) |
| L9 | Software-command timing used instead of the planned logic-analyzer measurement |
| L10 | No physical power-stage current-decay measurement |

Further documented residual items, identified by the audits:

| ID | Item |
|---|---|
| R1 | The WSL F103 `.uvprojx` differs from the Windows tested copy (`Ir2Chk`, ARMCC selection); the tested Keil outputs exist only on Windows |
| R2 | The release source is not committed to Git; the named repository is stale |
| R3 | The legacy `g_hw_diag_out_shutdown_count` also counts refused sink calls; FI-04 uses `g_gbt_fi04` |
| R4 | Open wiring checks: F103 A passive method, PD1050S pin 8 |
| R5 | Restart after END/EXIT is by reset only |

Deviations from the plan:

| ID | Deviation |
|---|---|
| D1 | Check 1 accepted without archived evidence (L3) |
| D2 | Checks 2 and 3 partially evidenced (L4, L5) |
| D3 | Check 5 hardware method changed from logic analyzer to software tick (L9) |
| D4 | Check 6 split into two build systems |

**No critical safety or data-integrity problem was found** during closeout.

## 13. Final verdict

**CONDITIONAL PASS.**

Basis for the verdict:
- Core software objectives are met and independently verified.
- Hardware communication through to an active charging session, timeout detection, safe output shutdown and the FI-04 timing measurement are demonstrated by owner-reported evidence that is internally consistent.
- Plan checks 1, 2, 3 (partly), 5 (hardware method) and 6 are not met as written. They are accepted by the owner as documented limitations (§12) and are **not** marked PASS.

## 14. Demonstrated portfolio capabilities

- **Portable embedded protocol stack.** A pure C99 implementation of a GB/T 27930-2023 System A core subset: codecs, J1939-style TP, two state machines, safety and timeouts. One source tree runs on Linux and on two different STM32 families. (IV)
- **Hardware adapters for two CAN controllers.** STM32G4 FDCAN (classic mode) and STM32F1 bxCAN, with an ISR-safe SPSC receive queue, explicit overflow accounting and TX gating. (IV source; OR hardware)
- **Dual-node hardware bring-up.** The two boards communicate bidirectionally at 250 kbit/s and establish a charging session. Debugging used Keil Watch and peripheral registers, including diagnosis of a start-order failure that was reproduced in SIL. (OR + IV)
- **Layered verification.** Unit tests, deterministic dual-node SIL, port-level SIL against HAL fakes, sanitizers, architecture and purity checks, cross-compilation, frame-level trace equivalence and mutation checks. (IV)
- **Fault injection and safety timing.** Vehicle communication loss led to timeout, latched output inhibit and a CEM/EXIT sequence in 10/10 hardware trials, with software command acceptance in 1 ms against a 9 ms limit. (OR; logic IV)
- **Evidence discipline.** Hash manifests, verified backups, explicit evidence labels and disclosed limitations. (IV)

## 15. Future work (optional or deferred, not authorised)

None of these items is part of STEP 07. All are **OPTIONAL / DEFERRED**:
- Commit the release source to version control and tag it (needs owner approval).
- Bring the tested F103 `.uvprojx` and the 11-23 Keil outputs from Windows into the archive.
- One recorded normal session (A1–A13), including the F103 stack watermark.
- One USB-CAN capture for an identifier hand check.
- Two-process vcan run on the release source.
- Logic-analyzer GPIO timing; DWT sub-millisecond timing.
- Resolve PROVISIONAL parameters against the J1939-21:2006 text.
- Restart policy after END/EXIT.
