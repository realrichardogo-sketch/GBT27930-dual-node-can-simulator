# Project Journey: STEP 00 to STEP 07

This record traces how Project 1 was planned, implemented, verified and closed out. It summarises
existing records only, and the source of every statement is named. Records that are not part of this
repository are marked *(archived)*: the owner keeps them in the STEP 07 archive and the earlier
development workspaces, or they are *handover* records from earlier project conversations.

**Final verdict: STEP 07 closed as CONDITIONAL PASS (owner scope decision, 2026-11-24).**

## How to read the evidence

Each statement carries a **record type** and, for results, an **evidence label**.

| Record type | Meaning |
|---|---|
| **[F] Formal Lean STEP record** | Step report, build report, manifest or commit produced inside the Lean project (from 2026-11-11) |
| **[H] Historical handover record** | Summary written in a project conversation handover or an earlier project's documents. It describes work but is not itself a test artifact. |
| **[OR] Owner-reported test evidence** | Result observed by the owner on hardware or Windows. No artifact (log, capture, screenshot) is archived with it. |
| **[P] Planned / not verified** | Defined or planned activity with no execution evidence available |

The evidence labels are as in [VERIFICATION.md](VERIFICATION.md):
- **IV**: independently verified from files or re-run tests.
- **OR**: owner-reported.
- **HR**: historical result for an earlier source revision.
- **NV**: not verified.

None of the [H] or [OR] results below were re-verified for this document.

---

## Roadmap (Lean project)

| Step | Period (2026) | Focus | Verdict as recorded | Record type | Evidence in this repository |
|---|---|---|---|---|---|
| — | 11-04 – 11-10 | *Historical context, not a Lean step:* earlier PHASE 00–10 project (requirements, architecture), see [Part A](#part-a-historical-context-not-lean-steps) | PHASE 01 and PHASE 02.2 CONDITIONAL PASS | [H] / archived documents | [`host/docs/sources/`](../host/docs/sources) (hash-pinned) |
| STEP 00 | 11-11 | Lean plan; repository bootstrap; toolchain and debug-probe setup; G474 LED firmware on hardware | Bootstrap complete (Git `f81e259`); LED observed (OR) | [F] + [H] + [OR] | [`design/PROJECT_PLAN.md`](design/PROJECT_PLAN.md) |
| STEP 01 – 03 | — | **No formal Lean STEP report available.** Hardware preparation of this period is described only in handover records ([Part A.2](#a2-hardware-preparation-sequence)). | cannot be established | — | — |
| STEP 04 | 11-12 – 11-13 (archive folder dates) | STM32 bring-up: LED, FDCAN loopback, G474 ↔ F103 request/response builds, programming preparation | Builds and static review PASS; physical CAN not tested in this step | [F] (archived) | — |
| STEP 05 | — | **No formal Lean STEP report available** | cannot be established | — | — |
| STEP 06 | ≤ 11-16 | Board-to-board CAN on the CubeMX/Keil projects: 06.2 G474 → F103, 06.3 F103 → G474 | 06.2 PASS (OR); 06.3 PASS on hardware (OR, work log), ARMCC 0 / 0 (OR) | [F] (archived) + [H] + [OR] | — |
| STEP 07 | 11-16 – 11-24 | Protocol core, SIL, STM32 ports, dual-node TP on hardware, FI-04 fault injection, closeout | **CONDITIONAL PASS** | [F] + [OR] | [`VERIFICATION.md`](VERIFICATION.md), [`evidence/`](evidence) |

---

## Part A: Historical context (not Lean steps)

This part is background. It must **not** be read as Lean STEP 01–07 content: the earlier project
structure was superseded by the Lean Plan of 2026-11-11, and its phases do not map to Lean steps.

### A.1 Earlier project roadmap: PHASE 00–10 (2026-11-04 – 11-10)

The earlier project (spec v2.0) restarted from a clean repository on 2026-11-04 and used phases PHASE 00–10. The recorded content of the phases:

| Phase | Content |
|---|---|
| PHASE 01 | Requirements and standard analysis |
| PHASE 02 | System architecture |
| PHASE 03 | Development environment |
| PHASE 04 | Protocol database |
| PHASE 05 | Protocol core |
| PHASE 06 – 10 | Application, SIL, hardware integration, release |

Recorded outcomes:

| Item | Result | Record type |
|---|---|---|
| Standard source registration (GB/T 27930-2023 PDF by SHA-256) | 2026-11-08 | archived document |
| Protocol tables, state machines, timeout table TO-01 … TO-25 (PHASE 01.2) | Draft, accepted for working use; values read from page renderings of the standard | archived document; copies in [`host/docs/sources/`](../host/docs/sources) |
| **B1** Requirements Baseline v0.3.3; **B2** Verification Test Definition v0.1.6 | Accepted | archived documents; copies in [`host/docs/sources/`](../host/docs/sources) |
| PHASE 01 exit | **CONDITIONAL PASS** (owner, 2026-11-09) | archived approval record |
| PHASE 02.2 system architecture exit | **CONDITIONAL PASS — exit approved** (owner, 2026-11-09) | archived approval record |
| PHASE 02.3 interface design plan | Draft submitted for owner review (2026-11-10) | archived document |
| Linux SocketCAN HAL prototype (tags `L04`, `L05-Step1…5`) | Built and tested in that repository. The "L05 Steps" are not Lean STEPs. | archived repository |

**What carried over into the Lean project:**
- The eight requirement and protocol documents in [`host/docs/sources/`](../host/docs/sources),
  imported byte-identical and checked by CTest `source_manifest`.
- The decision to keep open questions open (provisional parameters) instead of filling them from
  older editions.

The PHASE 02 architecture and the HAL prototype were replaced by the Lean architecture v1.0.

### A.2 Hardware preparation sequence

The handover records define a hardware work sequence for the two boards. Its link to Lean STEP 01–05
cannot be established from the available records. Status per sub-step:

| Sub-step | What is recorded | Record type | Status |
|---|---|---|---|
| 1. Pre-power inspection | Single termination measured **121.1 Ω** (the archived handover records ≈ 120 Ω, F103 isolated). About **60 Ω** with two terminations in parallel. GND continuity reported PASS. | [H] + [OR] | measurements owner-reported |
| 2. Board power-up | Archived handover: supply rails 3.29 V; idle bus CANH/CANL 2.12 V / 2.12 V, 0 V differential. Consistent with a recessive bus; does not prove frame exchange. | [H] + [OR] | measurements owner-reported |
| 3. Minimal CAN firmware | G474 FDCAN internal loopback reached `TEST_PASSED` (owner-reported). STEP 04 normal-mode programs built (see STEP 04). | [OR] + [F] | loopback owner-reported; normal mode not tested in STEP 04 |
| 4. Standard CAN frame | Test definition: ID `0x123`, DLC 4, `11 22 33 44`. Linux vcan standard-frame test recorded in handover. | [H] / [P] | **hardware execution of this definition not evidenced** |
| 5. 29-bit extended CAN frame | Test definition: ID `0x1826F456`, DLC 8, `11 22 33 44 55 66 77 88`. Linux vcan extended-frame test recorded in handover. | [H] / [P] | **hardware execution of this definition not evidenced.** Extended frames on hardware were later shown in STEP 06 and STEP 07 (owner-reported). |
| 6. Hardware acceptance | No acceptance record for this sequence | [P] | not verified |

The archived handover also records ST-LINK/OpenOCD/GDB recovery after a Windows/WSL restart, the
G474 PA11/PA12 AF9 audit, and an SN65HVD230 transceiver module on the G474 side at that time. That
module was later replaced by the PD1050S on board F103 A.

---

## Part B: Lean STEP records

### STEP 00: lean plan and bootstrap (2026-11-11)

- **Objective:** Fix a deliverable scope for a 14-day release, create a minimal buildable repository,
  and confirm the toolchain and debug path on real hardware.
- **What was implemented:**
  - The lean plan, which replaced the spec v2.0 release scope.
  - Lean repository initialisation with a CMake host build (C11 at bootstrap; the architecture later
    fixed C99) and the `bootstrap_smoke` CTest.
  - WSL and ARM toolchain setup.
  - A G474 bare-metal LED program.
  - OpenOCD/GDB connection with MCU register inspection.
  - No CAN application firmware yet.
- **Key decisions:**
  - "SIL carries the proof, and hardware shows one clean session and one timing measurement."
  - Cuts: code generator, reconnection and pause, full TP policy, bus-off recovery (made a stretch
    item), most fault scenarios.
  - Six "Done when" checks replaced the 21-criterion acceptance set.
- **Evidence:**

  | Item | Source | Type |
  |---|---|---|
  | Plan text | [`design/PROJECT_PLAN.md`](design/PROJECT_PLAN.md) | [F] IV |
  | Bootstrap commit `f81e259` (13 files, `C_STANDARD 11`, `bootstrap_smoke` test) | archived repository | [F] IV |
  | CTest 1/1 passed | historical handover | [H] |
  | LED firmware programmed and physically observed; OpenOCD/GDB register inspection | historical handover | [OR] |

- **Result / verdict:** Bootstrap complete.
- **Open issues / limitations:**
  - `f81e259` stayed the only commit of the development repository; all later work was untracked
    there.
  - This was later addressed by this repository (OI-03 in [OPEN_ISSUES.md](OPEN_ISSUES.md)).

### STEP 01 – STEP 03

**No formal Lean STEP report, review or acceptance record exists for STEP 01, 02 or 03 in the
available records.** The STEP 00 brief states only that STEP 01 required review and owner approval.

Hardware preparation recorded in handovers (Part A.2) may belong to this period, but its exact
assignment to these steps, and their completion, **cannot be established**.

**Recommended evidence to add:** the formal step records, if they exist.

### STEP 04: STM32 hardware bring-up

- **Objective:** Prove that both microcontrollers, their clocks and their CAN controllers work before
  any protocol code reaches the boards. The plan names two different CAN controllers as the main
  schedule risk.
- **What was implemented** (standalone GCC/CMake programs, not the later CubeMX projects):
  - G474 LED2 blink.
  - G474 FDCAN internal loopback ("G474-01").
  - G474 FDCAN normal-mode initiator, which sends 11-bit `0x123` and expects `0x124`.
  - Register-level F103 bxCAN responder.
- **Reviews and preparation:**
  - 04-A: F103 hardware and schematic audit.
  - 04-A2: SN65HVD230 transceiver RS-pin check.
  - 04-C: source review.
  - 04-D: flashing preflight.
  - 04-E: programming plan, SWD identification of the F103, gated procedure runs.
- **Key decisions:**
  - Loopback first, then normal mode.
  - Both nodes at 250 kbit/s with 1 + 13 + 2 time quanta.
  - Flashing only after a separate owner authorisation (gated 04-E procedure).
- **Evidence:** [F] build reports, a 22-file STEP 04-B manifest and 13 STEP 04-E log folders
  *(archived, about 1.2 GB)*.
- **Result / verdict:**
  - Build PASS and static review PASS (HR).
  - The loopback `TEST_PASSED` is owner-reported.
  - The normal-mode exchange was recorded as **physical CAN NOT TESTED** in this step.
- **Open issues / limitations:**
  - Internal loopback proves neither the transceiver nor the receive path.
  - These programs are historical and do not use the protocol core.

### STEP 05

**No formal Lean STEP report, review or acceptance record exists for STEP 05 in the available
records.** Its content and completion cannot be established.

**Recommended evidence to add:** the formal step record, if it exists.

### STEP 06: board-to-board CAN on the CubeMX / Keil projects

- **Objective:** Establish CAN communication in both directions between the two boards, using the
  CubeMX/Keil projects that later carry the protocol.
- **What was implemented:**
  - 06.2: G474 FDCAN1 transmitter → F103 B bxCAN receiver. The STEP 06.3 report describes the
    reference G474 source as sending extended ID `0x123`, DLC 8, payload `A5 5A 01 02 03 04` plus a
    16-bit sequence counter.
  - 06.3: reversed roles.
    - F103 B bxCAN transmitter of extended ID `0x124`, DLC 8, payload `B1 5A 02 03 04 05 06 07`.
    - G474 FDCAN receiver with an exact-match extended filter, a global reject filter and RX
      diagnostics.
    - The physical path runs through the onboard PD1050S of board F103 A (acting as the G474's
      transceiver) to the F103 B transceiver.
    - Changes stayed inside CubeMX `USER CODE` blocks.
- **Key decisions:**
  - No CubeMX regeneration.
  - All reference files hashed before and after the edits (unchanged).
  - G474 at 170 MHz, F103 at 72 MHz (HSE), both at 250 kbit/s.
- **Evidence:**

  | Item | Source | Type |
  |---|---|---|
  | 06.2: 346 frames received with zero RX errors | cited in the STEP 06.3 report | [OR] |
  | 06.3 implementation, static review, GCC build 0 errors / 0 warnings | STEP 06.3 implementation, build and static-review reports *(archived)*, dated 2026-11-16 | [F] IV (documents) |
  | 06.3 hardware run: all eight received bytes matched, TX/RX error counters stayed 0, about 5 minutes of continuous communication | historical work log dated 2026-11-15 | [H] + [OR] |
  | 06.3 ARMCC builds of both projects: 0 errors, 0 warnings | historical work log | [OR] |

- **Result / verdict:**
  - 06.2 PASS (OR).
  - 06.3: implemented; hardware communication reported successful by the owner (OR, not
    re-verified).
- **Open issues / limitations:**
  - **Record conflict:** the archived 06.3 report (dated 11-16, written in WSL) states "NOT hardware
    verified" and contains GCC results only. The hardware run and the ARMCC results come only from the
    owner's work log (dated 11-15). The order of these records cannot be reconciled from the
    available files. Both are kept as they are.
  - No CAN capture or screenshot of the 06.3 run is archived.
  - In the protocol-level sense, bidirectional communication on the final source was first shown in
    STEP 07: the G474 reached charging, which requires BRM and BCP from the F103.

### STEP 07: protocol stack, dual-node integration, fault injection and closeout

- **Objective:** Run the GB/T 27930 lean protocol subset on both boards from one portable C99 code
  base, prove it in SIL first, then on hardware, and measure the safe-state shutdown.
- **Key decisions:**
  - Architecture v1.0 frozen and owner-approved (07-A.1). The HAL contract `hal.h` (ND-1) is the only
    hardware boundary, and `gbt_integration` is the only HAL caller.
  - The protocol core has no dynamic memory, no blocking calls and no writable statics. CTest
    enforces these rules.
  - TX gate: the output command is accepted before any further frame is sent.
  - ISR → main single-producer/single-consumer queue; all protocol work runs in the main loop.
  - Software-tick timing replaced the logic-analyzer measurement (accepted deviation).
  - Closeout as a scope decision. Unmet checks are kept visible, not rewritten.

| Sub-step | Date | Content | Outcome | Label |
|---|---|---|---|---|
| 07-A / A.1 | ≤ 11-16 | Architecture packages Rev 1–3.2, AR-1 … AR-4 | [`design/ARCHITECTURE.md`](design/ARCHITECTURE.md) v1.0 frozen | IV (document) |
| 07-B | 11-16 | Codecs, 29-bit IDs, TP, both state machines, safety and timeouts, deterministic SIL, two-process SocketCAN SIL | Complete in software. vcan 4/4, CTest 18/18 on the owner host | HR |
| C0 / C1.1 | 11-16 | STM32 readiness review; ISR-safe SPSC queue ([design](design/STEP07C1_SPSC_DESIGN.md)) | Conditionally ready / conditionally complete (GCC only) | HR |
| C1.2 | 11-16 | G474 FDCAN audit, adapter `port_g474`, Keil integration package | Software complete; GCC only | HR |
| C2.1 | 11-16 | F103 bxCAN adapter `port_f103` | ARMCC 0 errors, 0 warnings (owner); no hardware | HR |
| C2.2-E | 11-16/17 | F103 stack-paint helper | Prepared; no watermark recorded | NV |
| C2 bring-up | 11-17 | Both boards running the stack | First failure diagnosed as start order (F103 B in EXIT after receiving CEM `0x081FF456`); reproduced in SIL | OR + IV |
| C2 observability | 11-18 | TP statistics, Watch structures, event log; vcan rerun | Full CTest 30/30 including `linux_sil_vcan` | HR |
| C2.4 | 11-18 | Dual-node TP test package; acceptance mask bits 0–17, criteria A1–A13 | ARMCC 0 errors, 0 warnings on both boards | IV (build logs) |
| FI-04 | 11-23 | Shutdown response-time instrumentation; ARMCC rebuild; 10 hardware trials | Software approved; ARMCC 0 / 0; **10/10 PASS** | IV / OR / OR |
| Closeout | 11-24 | Acceptance audit, final report, archive snapshot | **CONDITIONAL PASS** | — |
| Publication edition | 11-24/25 | Reorganised edition and this repository; firmware byte-identical | Host tests re-run: 29 passed, 1 not run | IV |

- **What was implemented:** see the table above. The architecture is in [ARCHITECTURE.md](ARCHITECTURE.md).
- **Evidence:**
  - [VERIFICATION.md](VERIFICATION.md);
  - FI-04 records and logs in [evidence/](evidence);
  - the closeout report [evidence/STEP07_FINAL_REPORT.md](evidence/STEP07_FINAL_REPORT.md);
  - source equivalence in [PROVENANCE.md](PROVENANCE.md).
- **Result / verdict:** **CONDITIONAL PASS.**
- **Open issues / limitations:** summarised below; the full list is in [OPEN_ISSUES.md](OPEN_ISSUES.md).

---

## Final summary

### ARMCC builds (Keil MDK, ARMCC V5.06 update 7, build 960)

| Build | G474 Code / RO / RW / ZI | F103 Code / RO / RW / ZI | Errors / warnings | Label |
|---|---|---|---|---|
| C2.4, 2026-11-18 | 30604 / 1076 / 372 / 5940 | 21482 / 814 / 336 / 5728 | 0 / 0 | IV (build logs; archived) |
| **FI-04, 2026-11-23 (hardware-tested)** | **31336 / 1076 / 372 / 6116** | **21914 / 814 / 336 / 5816** | **0 / 0** | OR |

The ZI growth (+176 B on the G474, +88 B on the F103) matches the FI-04 design arithmetic (IV).

The AXF SHA-256 values of the tested build are listed in [PROVENANCE.md](PROVENANCE.md). No Flash readback
was made.

### FI-04 hardware validation: communication loss during charging (10 runs)

- **Procedure, per run:**
  1. Start both boards and wait for charger step 10 with the output enabled.
  2. Hold F103 B in reset for about 3 s.
  3. Wait for the BCL timeout TO-14 (1000 ms).
  4. Halt the G474 and read its Watch variables.
- **Result: 10/10 PASS (OR).** Every run reported:
  - step `0x2A` (EXIT) with `timeouts = 1`;
  - output disabled, inhibit latched, one accepted shutdown;
  - `pass_count = 1`, `fail_count = 0`, `invalid_count = 0`;
  - **maximum shutdown-command confirmation 1 ms** (limit 9 ms), measured with the 1 ms software
    tick.
- **Inspected run:** event log step 10 → 40 at 2413 ms and 40 → 42 at 3413 ms. The 1000 ms gap is
  the handling time. Reference flags were 0 and the sink status OK.
- **Not archived:** per-run screenshots, logs and timing values (NV). No logic-analyzer or
  oscilloscope measurement exists.

### Final conclusion: CONDITIONAL PASS

The owner closed STEP 07 as **CONDITIONAL PASS** on 2026-11-24. This is a scope decision, not a claim
that every original check passed. The six lean "Done when" checks stand as follows:

| # | Check | Status |
|---|---|---|
| 1 | Full session on both boards without error messages | not verified (no session to END recorded) |
| 2 | Identifiers and bytes match the tables by hand check | not verified on hardware (verified in SIL) |
| 3 | BRM, BCP and BCS over TP on hardware | partial (BRM and BCP implied; BCS not recorded) |
| 4 | BCL and CCS timeouts give error message and safe state in SIL | **met** |
| 5 | Output command to 0 within 10 ms, in SIL and on a logic analyzer | **met in SIL**; hardware 1 ms by software timing; logic analyzer not used (accepted substitution) |
| 6 | One command builds all three targets and runs the tests | not met as written (ARMCC is a separate Windows step) |

**Main remaining limitations:**
- Bus-off is not handled within 10 ms (lean-scope stretch item).
- The tested F103 `.uvprojx` exists only on Windows (OI-02).
- Stack margins are static estimates only.
- Several protocol parameters are provisional.
- There is no power stage.

The project is a portfolio simulator of a core subset of GB/T 27930-2023 System A. It is not
standard-certified and not production-ready.

## Recommended evidence to add

No screenshots are included yet. The suggested set is listed in
[assets/journey/README.md](assets/journey/README.md). Each image should be an original capture from the
owner's bench or Windows build and should be labelled with its date and source revision.
