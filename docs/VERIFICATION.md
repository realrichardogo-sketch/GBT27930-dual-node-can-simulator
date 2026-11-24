# Verification

This document records what was verified for the STEP 07 release of the simulator, how, and with which
evidence label. The firmware source in this repository is byte-identical to the hardware-tested
FI-04 source ([PROVENANCE.md](PROVENANCE.md)).

## Evidence labels

| Label | Meaning |
|---|---|
| **IV** | Independently verified in WSL from files and tests in this repository or its source (host tests re-run on 2026-11-25 from this repository; same results as the STEP 07 working edition on 2026-11-24). |
| **OR** | Owner-reported. Observed by the owner on hardware or Windows; no artifact archived in WSL. |
| **HR** | Historical result for an earlier source revision; not re-run on this source. |
| **NV** | Not verified. |

**Overall status: CONDITIONAL PASS** (owner decision, 2026-11-24). The original lean acceptance checks
(§7) are kept exactly as written. Unmet checks are not reclassified.

**The scope is software command acceptance.** No result in this document concerns physical current
decay; there is no power stage.

## 1. Unit and SIL tests (host, WSL)

**Run from this repository on 2026-11-25 (IV):**
- Command: `sh scripts/run_host_checks.sh`.
- Toolchain: WSL2, gcc, strict flags `-std=c99 -pedantic-errors -Wall -Wextra -Werror -Wconversion`.

| Group | Tests | Result |
|---|---|---|
| Protocol core units | `test_ring`, `test_outbox`, `test_timer`, `test_id` (33 checks), `test_codec` (122), `test_bits`, `test_codec_ext` (71), `test_tp` (343), `bootstrap_smoke` | PASS |
| SIL infrastructure and sessions | `test_hal_fake`, `test_sil_infra`, `test_sil_session` (**1087 checks, 0 failures**) | PASS |
| Ports against HAL fakes | `test_port_g474`, `test_port_g474_session` (**297/0**), `test_port_f103`, `test_port_f103_session` (**439/0**), `test_port_linux` | PASS |
| Sanitizers | `test_port_g474_asan`, `test_port_f103_asan`, `test_port_f103_session_asan` (ASan + UBSan) | PASS |
| Concurrency | `test_spsc`, `test_spsc_signal` (signal-driven producer stress) | PASS |
| Architecture and purity | `architecture_rules`, `protocol_no_writable_statics`, `source_manifest` | PASS |
| Cross compile (arm-none-eabi-gcc) | `cross_c99_cortex_m`, `cross_c99_spsc`, `cross_c99_g474`, `cross_c99_f103` (against the HAL headers of the two board projects in `source/`) | PASS |
| Two-process SocketCAN SIL | `linux_sil_vcan` | **NOT RUN** (no `vcan0`; exit 77) |

**Total:** 29 passed, 0 failed, 1 not run, out of 30. The build produced 0 warnings.

**SIL scenarios covered** (`host/tests/sil/test_sil_session.c`):
- normal session with vehicle stop (both processing orders)
- charger stop
- BCL timeout (TO-14)
- CCS timeout (BEM)
- out-of-range current
- missing TP DT
- minimum-voltage boundary (T-3c)
- RX software-queue overflow, RX hardware-FIFO overflow, overflow outside charging (T-5)
- driver refusal not counted
- start order
- FI-04: unit cases 1–11, repeated stop, sink refusal (FAIL at 21 ms, as designed), RX limit (INVALID), T1 freshness

**Trace equivalence (IV, 2026-11-24, run in the STEP 07 working edition).** A trace-equivalence script
compiled five dual-node scenarios against the release source and against the historical development
tree. That script is not published here because it needs the unpublished historical tree. Every CAN frame and the
final node state are identical:
- 1186 lines;
- sha256 `fcc0b360f774163a0d0f4568f430e0ebf94000d22f65d0a5b78e74e45f1c7a94`, the same value as on 2026-11-23.

**Mutation check (IV for the 2026-11-23 source, which is byte-identical).** The FI-04 tests detect
both a 9 → 10 ms limit change and a stale T1. Log:
[evidence/fi04/post_mutation_check.log](evidence/fi04/post_mutation_check.log).

**Two-process vcan SIL:**
- Last PASS on 2026-11-18 on the C2 (pre-C2.4) source (HR): `linux_sil_vcan` and full CTest 30/30.
- Not run on the release source (NV).

## 2. Board source checks (WSL, IV)

| Check | Result |
|---|---|
| `scripts/gcc_board_compile.sh`: every `.c` of both Keil projects with arm-none-eabi-gcc at the Keil optimisation level (G474 -O0, F103 -O3), `-Werror` for application and shared core | PASS, G474 43 files, F103 36 files. **GCC only, not ARMCC.** |
| `scripts/check_keil_refs.sh`: every file and include directory referenced by both `.uvprojx` exists | PASS (G474 44 files / 7 include dirs; F103 38 / 6) |
| `scripts/check_user_code.sh` (G474 `main.c` FI-04 change vs pre-FI-04) | PASS on 2026-11-23: 5 lines, all inside USER CODE ([evidence/fi04/post_user_code_check_g474.log](evidence/fi04/post_user_code_check_g474.log)) |

## 3. ARMCC builds (Keil MDK, ARMCC V5.06 update 7 build 960, Windows)

| Build | G474 Code / RO / RW / ZI | F103 Code / RO / RW / ZI | Errors / warnings | Label |
|---|---|---|---|---|
| 2026-11-18 (pre-FI-04) | 30604 / 1076 / 372 / 5940 | 21482 / 814 / 336 / 5728 | 0 / 0 | HR (logs in the STEP 07 archive, not published) |
| **2026-11-23 FI-04, hardware-tested** | **31336 / 1076 / 372 / 6116** | **21914 / 814 / 336 / 5816** | **0 / 0** | **OR** |

**Consistency (IV, arithmetic).** ZI grew by +176 B on G474 and +88 B on F103. This equals the FI-04
design: an 88 B structure inside the integration object, plus the 88 B G474 Watch copy.

**Facts from the FI-04 build (OR):**
- G474 MAP: `g_gbt_fi04 = 0x200011a8, Data 88, main.o(.bss)`.
- F103 MAP: `RW_IRAM1` at 0x20000000, size 0x1808, maximum 0x10000; no RAM2 region.
- F103 `.htm`: maximum stack 432 B + Unknown.
- AXF SHA-256:
  - G474: `5A097A1F43E644FA70E18C70F0BBDC76C294A6ECDCF3DD6E756C9BFDF31763A4`
  - F103: `3E8CC5120BF7EAA423A3776826DFE315029A25554C3EA11BF7F93807024D3A05`

**Traceability gaps** (detail in [PROVENANCE.md](PROVENANCE.md)):
- These outputs exist only on Windows.
- Flash content was never read back.
- The tested F103 `.uvprojx` differs from the copy in this repository.

## 4. Hardware evidence

### 4.1 Dual-node CAN and TP

| Behaviour | Evidence | Label |
|---|---|---|
| G474 → F103 B frames | Before C2.4, F103 B received CEM `0x081FF456` (start-order diagnosis) | OR |
| F103 B → G474 frames, handshake, BRM and BCP over TP | The G474 reached charging step 10 in every FI-04 trial. Steps 4 and 5 wait for BRM and BCP over TP, so both must have completed. | OR (implied) |
| BCS over TP; direct TP counters and BRM snapshot (criteria A5–A9) | not recorded | NV |
| Full normal session to END on both boards (A1–A4) | not recorded | NV |
| CAN error counters, overflow counters (A10) | not recorded for the FI-04 trials. FI-04 `invalid_count = 0` shows no RX overflow or RX-limit pass before the event. | NV / partial OR |
| Identifier capture (hand check) | no capture | NV |

### 4.2 FI-04: communication loss during charging (10/10 PASS, OR)

**Procedure per trial:**
1. Reset and start both nodes.
2. Wait for G474 step 10 with the output enabled.
3. Hold F103 B RESET for about 3 s.
4. Wait for the timeout.
5. Halt the G474 and read the diagnostics.
6. Restart both nodes.

The full procedure is in [hardware/FI04_HARDWARE_TEST_PROCEDURE.md](hardware/FI04_HARDWARE_TEST_PROCEDURE.md).
The owner's 3 s hold is a valid variant, because the silence lasts longer than TO-14 (1000 ms).

Final values reported for every trial:

| Variable | Value |
|---|---|
| `g_hw_diag_gbt_step` / `_timeouts` | 0x2A / 1 |
| `g_hw_diag_out_enabled` / `_inhibit_latched` / `_apply_status` / `_state` / `_shutdown_count` | 0 / 1 / 0 / 0 / 1 |
| `g_gbt_fi04.pass_count` / `fail_count` / `invalid_count` | 1 / 0 / 0 |
| `g_gbt_fi04.max_valid_ms` | **1** (limit 9) |

Inspected trial:
- Event log: 2413 ms step 10 → 40; 3413 ms step 40 → 42. The gap equals the 1000 ms handling time.
- `ref_flags = 0`, `apply_status = GBT_HAL_OK`; accepted and confirmed sequence numbers matched.

**Consistency (IV, design reasoning):**
- Step 42 with `timeouts = 1` is reachable only through the timeout path; a normal stop ends at 41.
- `pass_count = 1` requires `ref_flags = 0`, sink OK, a confirmed sequence and elapsed ≤ 9 ms.

**Not archived (NV):** per-trial screenshots, logs, `elapsed_ms`, `t0_ms`/`t1_ms`, CAN counters.

### 4.3 Safety behaviour

| Item | SIL | Hardware |
|---|---|---|
| BCL timeout → output disabled, CEM, safe state | IV | OR (FI-04) |
| CCS timeout → BEM, safe state | IV | NV |
| Out-of-range current, minimum voltage → stop | IV | NV |
| Output command accepted before any further frame (TX gate) | IV (tests, trace) | OR (sequence consistent) |
| Shutdown command timing | IV (1 ms in SIL) | OR (1 ms, software tick) |
| Logic-analyzer timing | — | NV (replaced by software timing; accepted deviation) |
| Physical current decay | — | out of scope |

### 4.4 Stack margins

Both boards have `Stack_Size` 0x400 (1024 B). The estimates are static, from the ARMCC call graphs
(`.htm`) of the 2026-11-18 builds. FI-04 adds about 8 B in `apply()`, which is not on either maximum chain
(GCC call-graph comparison). **No runtime watermark exists for either board (NV).**

**F103 B** (A13 budget 768 B):
- ARMCC: 432 B + Unknown (2026-11-18 IV; 2026-11-23 OR).
- What the Unknown consists of:
  - four function-pointer targets of 0–12 B, all leaves, none on the 432 B chain;
  - default fault handlers that branch to themselves (reported as "cycles");
  - pre-`main` library stubs;
  - the assembly helper `stack_paint_f103.s` (`$Sub$$main`), which runs once before `main` and by design uses no stack.
- Interrupt nesting: CAN RX (144 B chain + 32 B frame) can preempt SysTick (32 B frame). The F103 (Cortex-M3) has no FPU, so all frames are basic.
- **Estimate: about 652 B of 1024 B.**
- The firmware already paints the stack, so a debugger scan after a session gives the runtime value without any code change ([hardware/HARDWARE_TEST_PROCEDURE.md](hardware/HARDWARE_TEST_PROCEDURE.md) §8).

**G474** (IV, analysis performed during the STEP 07 review on the 2026-11-18 ARMCC image and `.htm`):
- ARMCC main chain: 392 B + Unknown. The Unknown is the same kind as on the F103 (function pointers ≤ 12 B, self-looping fault handlers, library stubs).
- **Floating point.** The disassembly (`arm-none-eabi-objdump -d Project1_G474.axf`) contains exactly one VFP instruction: `vmsr fpscr` in the C-library startup `_fp_init`, before `main`.
  - This sets CONTROL.FPCA in thread mode, so an exception that preempts the main loop reserves the **extended 104 B frame**.
  - The ISRs contain no FP instructions, so nested exceptions use basic 32 B frames.
- Priorities: SysTick 0, FDCAN 5, button EXTI 15, so three-level nesting is possible.
- ISR depths: FDCAN 180 B, EXTI 40 B, SysTick 8 B.
- Worst case: 392 + 12 + 104 + 40 + (32 + 180) + (32 + 8) ≈ **800 B of 1024 B** (about 224 B margin).
- For the ARMCC build, this replaces earlier GCC -O0 figures (856–1072 B). The margin is smaller than on the F103. There is no paint helper for the G474.

## 5. Original lean acceptance checks (unchanged wording, from [design/PROJECT_PLAN.md](design/PROJECT_PLAN.md))

| # | Check | Status |
|---|---|---|
| 1 | A full session completes on both boards with no error messages. | **NOT VERIFIED** (hardware reached step 10, OR; no END recorded) |
| 2 | Captured identifiers and bytes match the tables by hand check, for example CHM = 0x1826F456. | **NOT VERIFIED on hardware** (IV in SIL) |
| 3 | BRM, BCP and BCS transfer over the transport protocol on hardware. | **PARTIAL** (BRM and BCP implied, OR; BCS NV) |
| 4 | BCL and CCS timeouts each produce their error message and a safe state in SIL. | **MET** (IV) |
| 5 | The output command reaches 0 within 10 ms, in SIL and on the logic analyzer. | SIL **MET** (IV); hardware 1 ms by software timing (OR); **logic analyzer not used** (accepted deviation) |
| 6 | One command builds all three targets and runs the tests. | **NOT MET as written.** `scripts/run_host_checks.sh` builds and tests the host and GCC-compiles the boards; the ARMCC build is a separate Windows step. |

## 6. How to reproduce

### WSL / Linux

```sh
cd gbt27930-dual-node-can-simulator
sh scripts/run_host_checks.sh                  # all host checks -> build/host_checks/
```

**Manual equivalent of the host checks:**
1. Run `cmake -S host -B build/host`.
2. Run `cmake --build build/host`.
3. Run `cd build/host && ctest --output-on-failure`.
4. Run `sh scripts/gcc_board_compile.sh source build/board`.

**Prerequisites:** gcc, cmake, and `arm-none-eabi-gcc` for the cross checks. Without
`arm-none-eabi-gcc`, those tests are not registered.

**Two-process vcan test.** It needs root once:
```sh
sudo ip link add vcan0 type vcan && sudo ip link set vcan0 up
```
Then re-run ctest.

### Windows (ARMCC)

1. Clone or copy the repository on Windows. Without Git symlink support, the 41 links under `host/`
   are checked out as small text files. That does not matter for Keil, which uses `source/` only.
2. Open `source\Project1_G474\MDK-ARM\Project1_G474.uvprojx` and `source\Project1_F103B_Receiver_CubeMX\MDK-ARM\Project1_F103.uvprojx` and run *Rebuild all*.
   Or use:
   `powershell -NoProfile -ExecutionPolicy Bypass -File scripts\keil_rebuild.ps1 -UV4 "<path>\UV4.exe"`
   (build only, logs to `evidence\armcc\`).
3. **For the tested F103 configuration,** use the `.uvprojx` from the Windows hardware-tested export
   (`Ir2Chk` 1 → 0, ARMCC selection). It is not in this repository; see [OPEN_ISSUES.md](OPEN_ISSUES.md) OI-02.

### Hardware

Hardware procedures are in [hardware/](hardware):
- session: `HARDWARE_TEST_PROCEDURE.md`
- FI-04: `FI04_HARDWARE_TEST_PROCEDURE.md`
- Watch variables: `WATCH_VARIABLES.md`
- criteria: `ACCEPTANCE_CRITERIA.md`

Observe the start order: F103 B must be in step 22 before the G474 runs.
