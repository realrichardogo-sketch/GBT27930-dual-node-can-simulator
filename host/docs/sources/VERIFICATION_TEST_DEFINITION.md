# Verification Test Definition

## §1 Document Control

| Field | Value |
|---|---|
| Project | GB/T 27930-2023 System A Charger–BMS Simulator |
| Project ID | P1 |
| Document ID | P1-B2-001 |
| Version | 0.1.6 |
| Status | OWNER-ACCEPTED |
| Acceptance Date | 2026-11-09 (v0.1.6) |
| Phase | PHASE 01.4-B — B2 |
| Date | 2026-11-09 |
| Requirements Baseline | `docs/REQUIREMENTS_BASELINE.md` — last formally owner-accepted baseline version: v0.3.0 (B1 accepted, 2026-11-09). Current pending revision: v0.3.3 (DRAFT — PENDING PROJECT OWNER REVIEW); accepted by the Project Owner for current PHASE 01 working use (F-03 instruction, 2026-11-09) but not formally accepted as a new baseline version. Final PHASE 01 freeze remains pending the PHASE 01 Exit Review. |
| Decision Inputs | `docs/phase01/01.4/OWNER_DECISION_RECORD.md` (D-05–D-10); `docs/phase01/OWNER_DECISION_SUPPLEMENT_01.md` (ODS-01–ODS-07); `docs/phase01/01.4/OWNER_DECISION_CLARIFICATION_01.md` (ODC-01); B2 implementation authorization of 2026-11-09 (AC14-02 treatment) |
| Standard Evidence | `docs/phase01/01.2_PROTOCOL_TABLES.md`, `docs/phase01/01.2_STATE_MACHINES.md`, `docs/phase01/01.2_TIMEOUT_TABLE.md` (DRAFT); `docs/reference/STANDARD_SOURCE_REGISTER.md` |
| Normative Authority | GB/T 27930-2023, System A |

Revision history: §10.

## §2 Scope and Verification Principles

### 2.1 Purpose and scope

This document defines how each of the 21 acceptance-criteria rows of the requirements baseline (18 top-level AC identifiers; AC-9 is shown as AC-9a to AC-9d) is to be verified, and traces every test to the requirements it covers. It defines tests; it does not execute them.

### 2.2 Non-claims

- No test in this document has been executed. Every test has Definition Status DEFINED and Execution Status NOT_EXECUTED.
- No requirement is verified by this document, and no protocol-compliance claim is made.
- The repository contains no protocol or application implementation and no STM32 target port. The existing Linux HAL tests (L04/L05) verify HAL behaviour only and are not protocol-level coverage.
- A software output command of 0 A is not evidence of zero physical current; physical output and power-stage protection belong to Project 2.
- Equipment availability is recorded as stated in §4; no model or availability is assumed and no purchase is implied.

### 2.3 Source hierarchy and value classes

Sources, in order: GB/T 27930-2023; referenced external documents; Project Owner decisions; Spec v2.0 project decisions (reconstructed, not approved) as carried by the baseline; test-design choices made in this document. Every numerical value in a pass/fail criterion carries one class:

| Class | Meaning | May be an acceptance value? |
|---|---|---|
| [STD] | GB/T 27930-2023 | Yes |
| [DOC] | Approved project document (requirements baseline) | Yes |
| [OWNER] | Explicit Project Owner decision | Yes |
| [REC] | Recommended harness or stimulus parameter | No |
| [OPEN] | Not established | No — the dependent part of the verdict is INCONCLUSIVE |
| [EXTERNAL] | Value from an external document not yet available | No, until verified |

### 2.4 Verification terminology

- **Gating test**: a MUST-level test whose result decides the MUST pass/fail of its AC row.
- **Non-gating test** (ID suffix `-S`): a SHOULD, STRETCH or observational check; its result is recorded and never blocks a MUST result.
- **INCONCLUSIVE**: a future verdict used when a criterion depends on an OPEN or EXTERNAL item, or when the measurement cannot discriminate the condition under test (§6.2).
- **PROVISIONAL**: as defined in the baseline §2.1.

## §3 Test ID and Status Conventions

- Format `VT-AC<nn>[A-D]-<kk>` for gating tests and `VT-AC<nn>[A-D]-S<kk>` for non-gating tests (e.g. `VT-AC05-01`, `VT-AC09A-01`, `VT-AC09B-S01`).
- Each test belongs to exactly one AC row. Requirement traceability runs through that row: a test covers only requirements in the row's baseline Covers.
- IDs are unique and never reused. The conversation-only labels TC-VER-001 to TC-VER-055 of the PHASE 01.4-A audit are not carried forward.
- Status vocabulary: Definition Status DEFINED; Execution Status NOT_EXECUTED. No other status is used in this document.
- MUST/SHOULD convention (D-09, ODS-02, owner-approved): each MUST row has at least one gating test; the SHOULD-only rows AC-9b, AC-9c, AC-9d and AC-18 have non-gating tests only; in mixed rows SHOULD checks are `-S` tests and never gate the MUST result.

## §4 Test Environments E1–E7

| ID | Environment | Purpose | Current infrastructure | Prerequisites before execution |
|---|---|---|---|---|
| E1 | Host unit tests / CTest | Pure logic, encoding, queues, state-machine units | CTest host infrastructure EXISTS (L04/L05) | protocol/app modules under test |
| E2 | Linux SIL / SocketCAN / vcan | Integration, CAN communication, simulated protocol | vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED | SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation |
| E3 | Static analysis / generator / lint | Structural and source consistency | Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED | YAML, generator, lint tool, target build configurations |
| E4 | Physical CAN hardware | Actual bus communication and capture | USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED | confirmed USB-CAN adapter, capture and decode tooling, two physical nodes |
| E5 | STM32 target boards | MCU-specific implementation verification | STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32) | available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up |
| E6 | Logic analyzer / GPIO timing | Independent timing evidence | logic analyzer planned (charter), availability UNCONFIRMED; GPIO timing hooks NOT IMPLEMENTED | confirmed logic analyzer, GPIO instrumentation in firmware |
| E7 | External-standard-dependent | Verification requiring external standards | referenced external document NOT AVAILABLE in the project source set | the referenced external document, verified and registered |

Equipment availability (recorded as known; no purchase initiated):

| Item | Status |
|---|---|
| Linux host with vcan / SocketCAN | Available (used by the L05 integration tests) |
| USB-CAN adapter | Planned (charter); model and availability UNCONFIRMED |
| Logic analyzer | Planned (charter); availability UNCONFIRMED |
| STM32G474RE board (charger node) | Named in Spec v2.0; board availability UNCONFIRMED; target port NOT IMPLEMENTED |
| STM32F103ZE board (vehicle node) | Named in Spec v2.0; board availability UNCONFIRMED; target port NOT IMPLEMENTED |
| ARM cross toolchain | UNCONFIRMED |
| GPIO timing instrumentation | Planned; firmware hooks NOT IMPLEMENTED |

**Executable today: no test in this document**, because no protocol or application implementation exists. E1 and E2 tests become executable once the implementation and the SIL harness exist; E4–E6 tests also need confirmed hardware and target ports; parts depending on E7 need the external documents.

## §5 Test Definitions

Each test lists the 15 mandatory fields. Field 6 states the environment and the execution readiness. Every test: Definition Status DEFINED; Execution Status NOT_EXECUTED.

### AC-1 — Verification Level MUST

Baseline Covers: PROT-1, PROT-11, PROT-2, PROT-6, STD-1. Gating tests: VT-AC01-01, VT-AC01-02, VT-AC01-03, VT-AC01-04. Non-gating tests: none.

#### VT-AC01-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC01-01 |
| 2 | Related AC ID | AC-1 |
| 3 | Covered Requirement IDs | STD-1, PROT-1, PROT-2 |
| 4 | Test Objective | Demonstrate ten complete System A sessions between two independent physical nodes, from handshake to statistics, without BEM or CEM. |
| 5 | Verification Method | Physical CAN test with bus capture and decoded-log review. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Charger node and vehicle node flashed with the build under test; both nodes on one independent CAN bus at 250 kbit/s; capture running before power-up; optional messages BMV/BMT/BSP not sent. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED); bus termination per COM-1. |
| 9 | Input Conditions | Simulated plug-in and auxiliary-power events on the charger; vehicle battery model configured so that charging ends at its target SOC. |
| 10 | Execution Procedure | 1. Start capture. 2. Trigger the plug-in and auxiliary-power events. 3. Let the session run to CSD/BSD completion. 4. Stop capture, decode, archive. 5. Repeat to 10 runs with a power cycle between runs. |
| 11 | Expected Results | Each run passes through handshake, identification, configuration, charging and end phases in the order of Figure A.1 as reconstructed in 01.2_STATE_MACHINES.md (DRAFT); no BEM or CEM frame appears. |
| 12 | Pass/Fail Criteria | Pass if 10 of 10 runs [DOC AC-1] complete with statistics exchanged (BSD and CSD observed) and contain zero BEM/CEM frames. Fail if any run lacks a phase, ends early or contains BEM/CEM. |
| 13 | Evidence to Collect | Raw capture file per run; decoded message sequence per run; run log with build identifier; summary table of the 10 runs. |
| 14 | External Dependencies | NONE for the pass criterion. Physical-layer values (SAE J1939-11) EXTERNAL; self-check content (GB/T 18487.1-2023 B.4.3) simulated. |
| 15 | Known Limitations | Order is compared with a DRAFT state-machine reconstruction. Both nodes share code, so a shared misreading is not detected here (see VT-AC17-01). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC01-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC01-02 |
| 2 | Related AC ID | AC-1 |
| 3 | Covered Requirement IDs | PROT-1 |
| 4 | Test Objective | Show that the charger sends its first CHM within 1 s of the auxiliary-power event. |
| 5 | Verification Method | Physical CAN capture timed against an auxiliary-power event marker. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware); E5 (STM32 target boards); E6 (Logic analyzer / GPIO timing). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). E6: logic analyzer planned (charter), availability UNCONFIRMED; GPIO timing hooks NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up; confirmed logic analyzer, GPIO instrumentation in firmware. |
| 7 | Preconditions | As VT-AC01-01; a GPIO marker on the charger toggles when the simulated auxiliary-power event is applied. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED); Logic analyzer with GPIO probes (planned; availability UNCONFIRMED) sharing a time base with the CAN signal. |
| 9 | Input Conditions | Auxiliary-power event applied 10 times (one per VT-AC01-01 run). |
| 10 | Execution Procedure | 1. Record the GPIO marker and CAN bus on the logic analyzer. 2. For each run, measure from the marker edge to the start of the first CHM frame. 3. Tabulate. |
| 11 | Expected Results | First CHM begins within 1 s of the marker in every run. |
| 12 | Pass/Fail Criteria | Pass if every measured interval is at most 1 s [STD §11.1.1, Figure A.1 note a]. Fail otherwise. INCONCLUSIVE if the marker and CAN capture do not share a time base. |
| 13 | Evidence to Collect | Logic-analyzer traces with marker and first CHM; measurement table. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The auxiliary-power event is simulated; the measurement covers the software response to the simulated event. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC01-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC01-03 |
| 2 | Related AC ID | AC-1 |
| 3 | Covered Requirement IDs | PROT-6 |
| 4 | Test Objective | Show that the status-dependent waits are separate waits with their own identities: CRM 0x00 (TO-02) and CRM 0xAA (TO-04) at the vehicle, BRO / BRO 0xAA (TO-07 / TO-08) at the charger, and CRO / CRO 0xAA (TO-10 / TO-11) at the vehicle. For the CRM pair, the distinct expiry behaviour of TO-02 and TO-04 is also verified. |
| 5 | Verification Method | SIL state-hold test with wait-identity logging; for the CRM pair (cases a1 and a2), time-controlled expiry of each wait; confirmed on the hardware session log. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E4 (Physical CAN hardware). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Nodes log the armed wait by TO-ID; the peer simulator can hold a status value; for cases a1 and a2 the harness controls time through the tick entry point (DES-6). |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); for confirmation USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | (a1) TO-02 case: the charger simulator sends CHM; the vehicle (V03) sends BHM; the charger never sends CRM. (a2) TO-04 case: the charger simulator sends CRM 0x00 and holds it, never sending CRM 0xAA; the vehicle (V05) sends BRM. (b) BRO 0x00 held (charger waits for BRO 0xAA). (c) CRO 0x00 held (vehicle waits for CRO 0xAA). |
| 10 | Execution Procedure | 1. Case a1: run to V03; record the armed TO-ID; advance time past 30 s from the first BHM; record the transition and the BEM content. 2. Case a2: run to V05; record that CRM 0x00 disarmed TO-02 and that TO-04 is armed; advance time past 5 s from the first BRM; record the transition and the BEM content. 3. Cases b and c: run to the hold point; record which TO-IDs are armed and which are satisfied by the held message; release the hold and record the transition. |
| 11 | Expected Results | (a1) TO-02 armed in V03; on expiry the vehicle enters timeout handling (TV08) with BEM SPN3901 = 01. (a2) CRM 0x00 disarms TO-02; TO-04 armed in V05; on expiry the vehicle enters timeout handling (TV11) with BEM SPN3902 = 01. (b), (c) Receiving the 0x00 value satisfies only the any-value wait (TO-07 for BRO, TO-10 for CRO) and not the 0xAA wait (TO-08, TO-11), which stays armed until 0xAA arrives. |
| 12 | Pass/Fail Criteria | Pass if: (a1) the log shows TO-02 (not TO-04) armed and its expiry produces BEM SPN3901 = 01 without SPN3902 set to 01; (a2) the log shows TO-02 disarmed by CRM 0x00 and TO-04 armed, and its expiry produces BEM SPN3902 = 01 without SPN3901 set to 01; (b), (c) the log shows distinct TO-IDs for the any-value and 0xAA waits and only the 0xAA message disarms the 0xAA wait [STD Figure A.1 p.48–50; TO-02 30 s, TO-04 5 s, TV08, TV11 and BEM fields per 01.2_TIMEOUT_TABLE.md and 01.2_STATE_MACHINES.md]. Expiry times in a1 and a2 are recorded against 30 s and 5 s; no acceptance tolerance is applied (tolerance OPEN). Fail if one wait identity is used for both CRM values, if the wrong BEM field is set, or if one wait is used for both values in case b or c. |
| 13 | Evidence to Collect | SIL logs per case with armed and disarmed TO-IDs; for cases a1 and a2 the virtual timestamps and the BEM frames; decoded hardware-session log excerpt. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Expiry times in cases a1 and a2 are recorded, not assessed against a tolerance (OPEN); expiry timing in cases b and c is not assessed (see §6). TO-09 (BRO continued, a PROT-6 canonical row) is not exercised by this test. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC01-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC01-04 |
| 2 | Related AC ID | AC-1 |
| 3 | Covered Requirement IDs | PROT-11 |
| 4 | Test Objective | Show that absent BMV, BMT and BSP never cause an error or timeout, and that BSP is neither sent nor processed (public-site profile). |
| 5 | Verification Method | SIL session without optional messages plus BSP injection; confirmed on hardware capture. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E4 (Physical CAN hardware). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Vehicle configured not to send BMV, BMT or BSP; injector able to send a well-formed BSP frame. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); for confirmation USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | (a) Full session with no BMV/BMT/BSP. (b) A BSP frame injected to the charger during charging. |
| 10 | Execution Procedure | 1. Run (a) and record timeouts, errors and BEM/CEM. 2. Run (b) and record the charger state, counters and outputs before and after the BSP frame. 3. Inspect captures for any BSP sent by either node. |
| 11 | Expected Results | No timeout, error or BEM/CEM caused by the absent optional messages; the injected BSP changes no state or output; no node transmits BSP. |
| 12 | Pass/Fail Criteria | Pass if all three expectations hold [STD §10.3; STD Table 23 note with OWNER D-01 public site]. Fail otherwise. |
| 13 | Evidence to Collect | SIL logs and counters; capture showing no BSP transmission. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Sending BMV/BMT (STRETCH) is not covered. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-2 — Verification Level MUST

Baseline Covers: COM-1, COM-2, COM-3. Gating tests: VT-AC02-01, VT-AC02-02, VT-AC02-03. Non-gating tests: none.

#### VT-AC02-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC02-01 |
| 2 | Related AC ID | AC-2 |
| 3 | Covered Requirement IDs | COM-1 |
| 4 | Test Objective | Show that every frame on the bus is a Classic CAN 2.0B extended frame at 250 kbit/s with at most 8 data bytes and no CAN FD frame. |
| 5 | Verification Method | Physical CAN capture with frame-format analysis. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | VT-AC01-01 session running; capture tool configured to report frame format, bit rate and FD flags. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | One complete session. |
| 10 | Execution Procedure | 1. Capture the full session. 2. Filter for standard-ID, remote, FD or BRS frames and DLC > 8. 3. Confirm the bit rate reported by the capture tool. |
| 11 | Expected Results | All frames extended (29-bit), Classic CAN, DLC ≤ 8, 250 kbit/s; zero FD frames. |
| 12 | Pass/Fail Criteria | Pass if zero non-conforming frames are found [STD §5.1, §6, §7.1, Table 1; DOC COM-1 no FD]. Fail on any non-conforming frame. |
| 13 | Evidence to Collect | Capture file; filter output; tool configuration record. |
| 14 | External Dependencies | Identifier bit assignment per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | Detection of FD frames depends on the adapter capability (UNCONFIRMED). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC02-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC02-02 |
| 2 | Related AC ID | AC-2 |
| 3 | Covered Requirement IDs | COM-2, COM-3 |
| 4 | Test Objective | Show that every transmitted identifier equals the identifier-generator output and the derived identifier of 01.2_PROTOCOL_TABLES.md §2 for its message, with SA 0x56 (charger) or 0xF4 (vehicle). |
| 5 | Verification Method | Capture comparison against a reference identifier table. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Reference table of the 22 derived identifiers (01.2 §2; derived values, U-P1). |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | One complete session plus forced stop and error paths so that BST, CST, BEM and CEM appear. |
| 10 | Execution Procedure | 1. Capture. 2. For each distinct identifier, decode P, EDP, DP, PF, PS, SA. 3. Compare with the reference table and the generator output. |
| 11 | Expected Results | Every identifier matches its reference value; EDP = 0, DP = 0; PS = destination address; SA fixed per node. |
| 12 | Pass/Fail Criteria | Pass if all observed identifiers match [STD Table 1, §7.4, §7.6 Table 2]. Fail on any mismatch. |
| 13 | Evidence to Collect | Capture; identifier comparison table. |
| 14 | External Dependencies | Bit assignment of the 29-bit identifier per SAE J1939-21 [EXTERNAL] (derived identifiers, U-P1). |
| 15 | Known Limitations | Messages not exercised in the session (e.g. optional BMV/BMT) are compared only against the generator output. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC02-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC02-03 |
| 2 | Related AC ID | AC-2 |
| 3 | Covered Requirement IDs | COM-1 |
| 4 | Test Objective | Confirm by inspection that the charger–vehicle communication uses an independent CAN bus and the bench wiring of COM-1. |
| 5 | Verification Method | Inspection of the bench set-up. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Bench assembled for VT-AC01-01. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); multimeter for the termination check (availability UNCONFIRMED). |
| 9 | Input Conditions | NOT APPLICABLE (inspection). |
| 10 | Execution Procedure | 1. Confirm no other node or bus is connected. 2. Confirm twisted-pair wiring. 3. Measure the termination resistance between CAN_H and CAN_L with power off. |
| 11 | Expected Results | Independent two-node bus; twisted pair; termination at each end. |
| 12 | Pass/Fail Criteria | Pass if the bus is independent [STD §6] and the wiring matches COM-1 [DOC: twisted pair, 120 Ω termination at each end]. Fail otherwise. |
| 13 | Evidence to Collect | Photographs; inspection record; resistance reading. |
| 14 | External Dependencies | Physical-layer values per SAE J1939-11:2006 [EXTERNAL, unverified]. |
| 15 | Known Limitations | Inspection only; not an electrical conformance test. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-3 — Verification Level MUST

Baseline Covers: COM-4, TP-2. Gating tests: VT-AC03-01, VT-AC03-02. Non-gating tests: none.

#### VT-AC03-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC03-01 |
| 2 | Related AC ID | AC-3 |
| 3 | Covered Requirement IDs | COM-4 |
| 4 | Test Objective | Measure the transmission intervals of every periodic message in each phase against Tables 3–7. |
| 5 | Verification Method | SIL state-hold timing measurement. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session able to hold each phase steady (the peer withholds the next transition message). |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); independent vcan receive socket with kernel timestamps. |
| 9 | Input Conditions | Hold of each phase: handshake, identification, configuration, charging, end. |
| 10 | Execution Procedure | 1. Hold each phase. 2. Timestamp every frame of each periodic message on an independent socket. 3. Compute intervals per message. 4. Compare with the Tables 3–7 period. |
| 11 | Expected Results | Intervals close to the nominal period of each message. |
| 12 | Pass/Fail Criteria | Pass if every interval is within ±10% of its Tables 3–7 period [STD period; DOC COM-4 tolerance] under the interval metric to be approved (NF-03 OPEN). Until the metric is approved the verdict is INCONCLUSIVE. Fail if any interval is outside ±10% under the approved metric. |
| 13 | Evidence to Collect | Timestamped frame logs; interval statistics per message; host load record. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | SIL timing on a non-real-time host is not target timing; sufficiency of SIL-only evidence for COM-4 is OPEN (NF-03). The phase-hold duration is not specified by the baseline [OPEN]. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC03-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC03-02 |
| 2 | Related AC ID | AC-3 |
| 3 | Covered Requirement IDs | TP-2 |
| 4 | Test Objective | Measure the period of each multi-packet message as the period of the whole packet. |
| 5 | Verification Method | SIL timing measurement of transport sessions. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the configuration and charging phases. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); independent vcan receive socket. |
| 9 | Input Conditions | BCP (period 500 ms), BRM (250 ms) and BCS (250 ms) sent via the transport protocol. |
| 10 | Execution Procedure | 1. Timestamp each transport session start (RTS). 2. Compute the interval between consecutive session starts per message. 3. Compare with the Tables 3–5 period. |
| 11 | Expected Results | Session-start interval equals the message period; data frames inside a session are not counted as periods. |
| 12 | Pass/Fail Criteria | Pass if the whole-packet period is within ±10% [DOC COM-4] of the table period [STD Tables 3–5; §7.5], under the metric of VT-AC03-01 (NF-03 OPEN; INCONCLUSIVE until approved). Fail otherwise. |
| 13 | Evidence to Collect | Timestamped transport logs; period statistics. |
| 14 | External Dependencies | Transport-protocol framing per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | As VT-AC03-01. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-4 — Verification Level MUST

Baseline Covers: TP-1, TP-2, TP-3, TP-4, TP-5, TP-7. Gating tests: VT-AC04-01, VT-AC04-02, VT-AC04-03, VT-AC04-04, VT-AC04-05. Non-gating tests: VT-AC04-S01.

#### VT-AC04-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC04-01 |
| 2 | Related AC ID | AC-4 |
| 3 | Covered Requirement IDs | TP-1, TP-2 |
| 4 | Test Objective | Show that BRM, BCP and BCS transfer correctly on hardware and that data frames are spaced 10 ms apart. |
| 5 | Verification Method | Physical CAN capture of transport sessions. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Hardware session as VT-AC01-01. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | BRM (49 bytes), BCP (13 bytes), BCS (9 bytes) during a normal session. |
| 10 | Execution Procedure | 1. Capture. 2. Reassemble each transport session and compare with the sender payload log. 3. Measure the spacing between consecutive data frames. |
| 11 | Expected Results | Every session completes; reassembled payload equals the sent payload; data-frame spacing nominally 10 ms. |
| 12 | Pass/Fail Criteria | Pass if every transfer completes with an identical payload [STD §7.5]. Data-frame spacing is measured against 10 ms [STD §7.5]; the spacing tolerance is not established [OPEN], so the spacing result is recorded and INCONCLUSIVE until a tolerance is approved. Fail if a transfer does not complete or the payload differs. |
| 13 | Evidence to Collect | Capture; reassembly comparison; spacing statistics. |
| 14 | External Dependencies | Transport procedure, control bytes and packet numbering per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | Spacing tolerance OPEN. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC04-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC04-02 |
| 2 | Related AC ID | AC-4 |
| 3 | Covered Requirement IDs | TP-1, TP-3 |
| 4 | Test Objective | Verify the transport-protocol procedures RTS, CTS window, DT sequencing, EOM ACK, abort and timers T1–T3 in unit tests. |
| 5 | Verification Method | Host unit tests of the transport module with a scripted peer. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E7 (External-standard-dependent). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E7: referenced external document NOT AVAILABLE in the project source set. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; the referenced external document, verified and registered. |
| 7 | Preconditions | Transport module built for host; scripted peer; transport parameters from the YAML marked EXTERNAL. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Scripted sequences: normal transfer; CTS with a window smaller than the message; abort from the peer; missing CTS, DT and EOM ACK to drive T1–T3. |
| 10 | Execution Procedure | 1. Run each scripted sequence. 2. Record emitted frames and session state. 3. Compare with the expected procedure. |
| 11 | Expected Results | Procedures follow SAE J1939-21; timer expiries end the session as specified there. |
| 12 | Pass/Fail Criteria | Pass if each sequence matches the expected procedure using the EXTERNAL parameter values [EXTERNAL SAE J1939-21]. Until those values are verified, timer-related results are INCONCLUSIVE. Fail on a procedural deviation. |
| 13 | Evidence to Collect | Unit-test report; frame traces per sequence. |
| 14 | External Dependencies | SAE J1939-21:2006 §5.4.7, §5.10: PGNs, control bytes, T1–T3 values [EXTERNAL]. |
| 15 | Known Limitations | T4 is not included (AC-4 lists T1–T3). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC04-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC04-03 |
| 2 | Related AC ID | AC-4 |
| 3 | Covered Requirement IDs | TP-5 |
| 4 | Test Objective | Show that a sequence gap in data frames aborts the transfer. |
| 5 | Verification Method | Host unit test with a scripted sender. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Transport receiver active for an announced BRM transfer. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Data frames with sequence numbers 1, 2, 4. |
| 10 | Execution Procedure | 1. Announce the transfer. 2. Send frames 1, 2, 4. 3. Record receiver output and buffer state. |
| 11 | Expected Results | The receiver aborts on the gap and discards the partial message. |
| 12 | Pass/Fail Criteria | Pass if an abort is issued and no partial message is delivered [DOC TP-5]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report; emitted abort frame. |
| 14 | External Dependencies | Abort reason code per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC04-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC04-04 |
| 2 | Related AC ID | AC-4 |
| 3 | Covered Requirement IDs | TP-4 |
| 4 | Test Objective | Show that an oversized optional message is refused, the 64-byte buffer is never overrun, and only one session per direction per peer is accepted. |
| 5 | Verification Method | Host unit test with a scripted sender. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Receive buffer configured to 64 bytes [DOC TP-4]. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | (a) RTS for a 512-byte BMV. (b) RTS for a 49-byte BRM. (c) A second RTS from the same peer in the same direction while (b) is active. |
| 10 | Execution Procedure | 1. Send (a) and record the response. 2. Send (b) and complete it. 3. During (b), send (c). 4. Check buffer bounds with a guard pattern. |
| 11 | Expected Results | (a) refused; (b) completes; (c) not accepted as a second session; guard bytes intact. |
| 12 | Pass/Fail Criteria | Pass if all expectations hold [STD §10.3; DOC TP-4]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report; guard-pattern check result. |
| 14 | External Dependencies | Refusal encoding per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | The 512-byte BMV size is derived from Table 21 (U-P6). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC04-05

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC04-05 |
| 2 | Related AC ID | AC-4 |
| 3 | Covered Requirement IDs | TP-7 |
| 4 | Test Objective | Show that after a failed transfer the sender restarts at the message next period without a retry loop. |
| 5 | Verification Method | SIL timing test with a forced transfer failure. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Vehicle sending BCP (500 ms period) via transport. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | The charger simulator aborts one BCP transfer. |
| 10 | Execution Procedure | 1. Force the abort. 2. Timestamp the next RTS for BCP. 3. Check for any RTS between the abort and the next period. |
| 11 | Expected Results | No RTS until the next scheduled BCP period. |
| 12 | Pass/Fail Criteria | Pass if the next RTS occurs at the next scheduled period and no intermediate RTS is sent [DOC TP-7]. Fail on any immediate retry. |
| 13 | Evidence to Collect | Timestamped SIL log. |
| 14 | External Dependencies | Abort semantics per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | Period tolerance as VT-AC03-02 (NF-03 OPEN). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC04-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC04-S01 |
| 2 | Related AC ID | AC-4 |
| 3 | Covered Requirement IDs | TP-5 |
| 4 | Test Objective | Check the SHOULD transport policies: duplicate data frame ignored; RTS during an active session rejected with abort; invalid CTS window aborts. |
| 5 | Verification Method | Host unit tests (non-gating SHOULD check). |
| 6 | Test Level / Environment | Non-gating. E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Transport module on host. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | (a) Duplicate DT frame. (b) RTS during an active session. (c) CTS with an invalid window. |
| 10 | Execution Procedure | Run each case and record the response. |
| 11 | Expected Results | (a) ignored; (b) abort; (c) abort. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check: result recorded as conforming or non-conforming to the DOC TP-5 policies; it does not affect the AC-4 MUST result. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | Consistency of these policies with SAE J1939-21 [EXTERNAL, unverified]. |
| 15 | Known Limitations | Project policies, not standard requirements. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-5 — Verification Level MUST

Baseline Covers: CAN-5, ERR-1, ERR-2, ERR-4, ERR-5, PROT-12, TP-6, TST-2. Gating tests: VT-AC05-01, VT-AC05-02, VT-AC05-03, VT-AC05-04, VT-AC05-05, VT-AC05-06, VT-AC05-07, VT-AC05-08. Non-gating tests: VT-AC05-S01. TP-6 (in the AC-5 Covers) is tested in AC-14 (VT-AC14-05), as the AC-5 procedure states.

#### VT-AC05-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-01 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | TST-2, ERR-1 |
| 4 | Test Objective | Verify the BCL reception timeout TO-14 at its boundary in both C10 (charging) and C10P (charging pause). |
| 5 | Verification Method | Time-controlled SIL boundary test (virtual time through the deterministic tick entry). Real-time confirmation on E5/E6: optional; not defined in B2. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E1 (Host unit tests / CTest). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; protocol/app modules under test. |
| 7 | Preconditions | Charger in C10; then repeated in C10P (entered by BSM SPN3096 = 00 with SPN3090–3095 all 00). |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); time-controlled harness driving the protocol tick (DES-6). |
| 9 | Input Conditions | Last BCL delivered at t0; no further BCL. Case A: advance to t0 + T − 1 ms. Case B: advance to t0 + T + 1 ms. T = 1 s [STD Figure A.1 p.51; §11.3.1]. |
| 10 | Execution Procedure | 1. Reach C10. 2. Deliver the last BCL at t0 and stop BCL. 3. Case A: advance time to t0 + T − 1 ms and record state. 4. Case B: reset, repeat, advance to t0 + T + 1 ms and record state and transmitted frames. 5. Repeat steps 2–4 in C10P. |
| 11 | Expected Results | Case A: no timeout. Case B: timeout handling entered (Figure A.2): current messages stopped, CEM sent with CEM SPN3925 = 01. |
| 12 | Pass/Fail Criteria | Pass if, in both C10 and C10P, Case A shows no timeout and Case B shows the timeout with CEM SPN3925 = 01 and the timeout-handling state [STD duration 1 s [STD Figure A.1 p.51; §11.3.1]; OWNER D-05/ODS-01 set; DOC TST-1 T±1 ms boundary; CEM field by name, 01.2 U-T2]. Fail otherwise. INCONCLUSIVE if the time base cannot distinguish T − 1 ms from T + 1 ms (§6.2). |
| 13 | Evidence to Collect | Harness log with virtual timestamps; transmitted-frame log; state log. Real-time confirmation evidence (§6.2) applies only if an optional E5/E6 confirmation is defined later; it is not defined in B2. |
| 14 | External Dependencies | NONE for the boundary. Timeout-handling completion time GB/T 18487.1-2023 B.3.7.3 [EXTERNAL] is not assessed here. |
| 15 | Known Limitations | Virtual-time result verifies the logic boundary only. Real-time confirmation on E5/E6 is optional and not defined in B2 (§6.2). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-02 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | TST-2, ERR-1 |
| 4 | Test Objective | Verify the BCS reception timeout TO-15 at its boundary in both C10 (charging) and C10P (charging pause). |
| 5 | Verification Method | Time-controlled SIL boundary test (virtual time through the deterministic tick entry). Real-time confirmation on E5/E6: optional; not defined in B2. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E1 (Host unit tests / CTest). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; protocol/app modules under test. |
| 7 | Preconditions | Charger in C10; then repeated in C10P (entered by BSM SPN3096 = 00 with SPN3090–3095 all 00). |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); time-controlled harness driving the protocol tick (DES-6). |
| 9 | Input Conditions | Last BCS delivered at t0; no further BCS. Case A: advance to t0 + T − 1 ms. Case B: advance to t0 + T + 1 ms. T = 5 s [STD Figure A.1 p.51; §11.3.2]. |
| 10 | Execution Procedure | 1. Reach C10. 2. Deliver the last BCS at t0 and stop BCS. 3. Case A: advance time to t0 + T − 1 ms and record state. 4. Case B: reset, repeat, advance to t0 + T + 1 ms and record state and transmitted frames. 5. Repeat steps 2–4 in C10P. |
| 11 | Expected Results | Case A: no timeout. Case B: timeout handling entered (Figure A.2): current messages stopped, CEM sent with CEM SPN3924 = 01. |
| 12 | Pass/Fail Criteria | Pass if, in both C10 and C10P, Case A shows no timeout and Case B shows the timeout with CEM SPN3924 = 01 and the timeout-handling state [STD duration 5 s [STD Figure A.1 p.51; §11.3.2]; OWNER D-05/ODS-01 set; DOC TST-1 T±1 ms boundary; CEM field by name, 01.2 U-T2]. Fail otherwise. INCONCLUSIVE if the time base cannot distinguish T − 1 ms from T + 1 ms (§6.2). |
| 13 | Evidence to Collect | Harness log with virtual timestamps; transmitted-frame log; state log. Real-time confirmation evidence (§6.2) applies only if an optional E5/E6 confirmation is defined later; it is not defined in B2. |
| 14 | External Dependencies | NONE for the boundary. Timeout-handling completion time GB/T 18487.1-2023 B.3.7.3 [EXTERNAL] is not assessed here. |
| 15 | Known Limitations | As VT-AC05-01. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-03 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | TST-2, ERR-1 |
| 4 | Test Objective | Verify the BSM reception timeout TO-16 at its boundary in both C10 (charging) and C10P (charging pause). |
| 5 | Verification Method | Time-controlled SIL boundary test (virtual time through the deterministic tick entry). Real-time confirmation on E5/E6: optional; not defined in B2. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E1 (Host unit tests / CTest). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; protocol/app modules under test. |
| 7 | Preconditions | Charger in C10; then repeated in C10P (entered by BSM SPN3096 = 00 with SPN3090–3095 all 00). At least one BSM has been received before t0 (TO-16 is armed only after the first BSM, note h). |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); time-controlled harness driving the protocol tick (DES-6). |
| 9 | Input Conditions | Last BSM delivered at t0; no further BSM. Case A: advance to t0 + T − 1 ms. Case B: advance to t0 + T + 1 ms. T = 5 s [STD Figure A.1 p.51; note h; §11.3.4]. |
| 10 | Execution Procedure | 1. Reach C10. 2. Deliver the last BSM at t0 and stop BSM. 3. Case A: advance time to t0 + T − 1 ms and record state. 4. Case B: reset, repeat, advance to t0 + T + 1 ms and record state and transmitted frames. 5. Repeat steps 2–4 in C10P. |
| 11 | Expected Results | Case A: no timeout. Case B: timeout handling entered (Figure A.2): current messages stopped, CEM sent with CEM SPN3928 = 01. |
| 12 | Pass/Fail Criteria | Pass if, in both C10 and C10P, Case A shows no timeout and Case B shows the timeout with CEM SPN3928 = 01 and the timeout-handling state [STD duration 5 s [STD Figure A.1 p.51; note h; §11.3.4]; OWNER D-05/ODS-01 set; DOC TST-1 T±1 ms boundary; CEM field by name, 01.2 U-T2]. Fail otherwise. INCONCLUSIVE if the time base cannot distinguish T − 1 ms from T + 1 ms (§6.2). |
| 13 | Evidence to Collect | Harness log with virtual timestamps; transmitted-frame log; state log. Real-time confirmation evidence (§6.2) applies only if an optional E5/E6 confirmation is defined later; it is not defined in B2. |
| 14 | External Dependencies | NONE for the boundary. Timeout-handling completion time GB/T 18487.1-2023 B.3.7.3 [EXTERNAL] is not assessed here. |
| 15 | Known Limitations | Only detection and CEM SPN3928 are asserted. Behaviour after the timeout (reconnection or re-plugging) is NOT asserted: OQ-S1 OPEN. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-04 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | ERR-2, ERR-4 |
| 4 | Test Objective | Verify the mandatory timeout response: the node stops its current messages and sends its error message periodically, and the event is recorded in the timeout failure class. |
| 5 | Verification Method | SIL test with timestamped frame capture. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | (a) Charger in C10. (b) Vehicle in V10 waiting for the first CCS. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); independent vcan receive socket. |
| 9 | Input Conditions | (a) BCL withheld until TO-14 expires. (b) First CCS withheld until TO-13 expires. |
| 10 | Execution Procedure | 1. Trigger (a); capture for at least 10 error-message periods after expiry ([REC] observation window). 2. Trigger (b) likewise. 3. Inspect transmitted messages, error-message intervals and diagnostic counters. |
| 11 | Expected Results | After expiry the node sends no further current-phase messages; CEM (charger) or BEM (vehicle) is sent every 250 ms; the event is counted as an application timeout. |
| 12 | Pass/Fail Criteria | Pass if current-phase messages stop, error messages repeat at 250 ms [STD Table 7] within ±10% [DOC COM-4] under the approved interval metric (NF-03 OPEN; the interval part is INCONCLUSIVE until approved), and only the timeout counter increments [DOC ERR-4]. Fail if current messages continue or the error message is absent. |
| 13 | Evidence to Collect | Timestamped captures; diagnostic counter dumps. |
| 14 | External Dependencies | Timeout-handling completion per GB/T 18487.1-2023 B.3.7.3 [EXTERNAL] (see VT-AC05-06). |
| 15 | Known Limitations | Interval metric OPEN (NF-03). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-05

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-05 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | PROT-12 |
| 4 | Test Objective | Verify the TO-01 compatibility path: with no BHM the charger continues after 10 s from the first CHM, generates no timeout error response, sets no CEM field and proceeds to CRM. |
| 5 | Verification Method | Time-controlled SIL test. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Charger in C02 sending CHM; vehicle simulator silent (never sends BHM). |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); time-controlled harness. |
| 9 | Input Conditions | No BHM. |
| 10 | Execution Procedure | 1. Start the charger. 2. Advance time past 10 s from the first CHM. 3. Record state, transmitted frames and error fields until the first CRM. |
| 11 | Expected Results | The charger continues on the compatibility path (self-check per the compatible lower version, simulated), sends no CEM and no timeout response, and then sends CRM (SPN2560 = 0x00). |
| 12 | Pass/Fail Criteria | Pass if no CEM frame and no timeout state occur and CRM 0x00 follows the continuation [STD Figure A.1 p.47, note c; OWNER D-05]. The 10 s duration is [STD]. The time at which continuation occurs is recorded; no acceptance tolerance is applied (tolerance OPEN). Fail if CEM, error handling or a timeout state occurs. |
| 13 | Evidence to Collect | Harness log with timestamps; transmitted-frame log. |
| 14 | External Dependencies | Lower-version self-check method [EXTERNAL] (simulated). |
| 15 | Known Limitations | Continuation timing is recorded, not assessed against a tolerance (OPEN). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-06

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-06 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | ERR-2, ERR-5 |
| 4 | Test Objective | Verify the TO-25 timeout-handling path: entry into handling and exit to EXIT when the handling-deadline parameter expires, at C-TO, C-BE, V-TO and V-CE. |
| 5 | Verification Method | SIL test with a configurable handling-deadline parameter. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E7 (External-standard-dependent). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E7: referenced external document NOT AVAILABLE in the project source set. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; the referenced external document, verified and registered. |
| 7 | Preconditions | Handling deadline implemented as a single configuration parameter marked EXTERNAL. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Entry by: own timeout (C-TO, V-TO); received BEM (C-BE); received CEM (V-CE). Handling held incomplete. |
| 10 | Execution Procedure | 1. Set the parameter to a harness value. 2. For each entry point, enter handling and keep it incomplete. 3. Record the state when the parameter expires. |
| 11 | Expected Results | At each entry point the node enters handling and moves to EXIT when the parameter expires (TC37, TC45, TV31, TV37 in 01.2_STATE_MACHINES.md). |
| 12 | Pass/Fail Criteria | Pass if each entry point reaches handling and then EXIT at parameter expiry [STD Figures A.2–A.3]. The deadline value is [EXTERNAL] GB/T 18487.1-2023 B.3.7.3 and OPEN; the ERR-5 1 s placeholder may be used only as a [REC] harness parameter and is not an acceptance limit. No pass/fail is assessed on the duration. Fail if an entry point does not reach handling or EXIT. |
| 13 | Evidence to Collect | SIL logs per entry point with the parameter value recorded. |
| 14 | External Dependencies | GB/T 18487.1-2023 B.3.7.3: value and start condition [EXTERNAL, OPEN]. |
| 15 | Known Limitations | Duration conformance cannot be verified until the external value is available (U-T1). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-07

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-07 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | CAN-5 |
| 4 | Test Objective | Verify detection of peer loss without bus-off through application waits in steps that have an applicable wait. |
| 5 | Verification Method | SIL peer-silence injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Simulated peer able to stop transmitting at a chosen step. For the V12 case the stop is vehicle-initiated (the vehicle has sent its first BST because of its own stop condition): TO-20 starts at the first BST sent in a vehicle-initiated stop (TV26); in a charger-initiated stop TO-20 is not armed and the V12 case does not apply. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Peer stops transmitting in C04 (TO-03), C10 (TO-14), V05 (TO-04) and V12 during a vehicle-initiated stop (TO-20). |
| 10 | Execution Procedure | 1. For each step, silence the peer; for the V12 case, first trigger a vehicle-initiated stop and silence the charger after the first BST sent by the vehicle. 2. Record the expiring TO-ID and the resulting state. |
| 11 | Expected Results | The applicable wait expires and timeout handling is entered. |
| 12 | Pass/Fail Criteria | Pass if, in each case, the applicable TO expires and handling is entered [DOC CAN-5; STD Figure A.1]. Durations are not assessed against a tolerance here. Fail if peer loss goes undetected in a step with an applicable wait. |
| 13 | Evidence to Collect | SIL logs per step. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Steps without an applicable wait (e.g. V02, V07–V08) are OPEN (RQ-05) and not tested. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-08

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-08 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | ERR-1 |
| 4 | Test Objective | Confirm by inspection that every wait TO-01 to TO-25 is mapped in the implementation to its duration, start/reset condition and error handling, and that only waits with a Tables 28–29 field set that field. |
| 5 | Verification Method | Inspection of the implementation and the generated timeout table against 01.2_TIMEOUT_TABLE.md. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Implementation and generated tables available. |
| 8 | Required Equipment and Fixtures | Review checklist; generated timeout table. |
| 9 | Input Conditions | NOT APPLICABLE (inspection). |
| 10 | Execution Procedure | 1. For each TO-ID, compare duration, start/reset condition and error handling with 01.2_TIMEOUT_TABLE.md. 2. Confirm that TO-01, TO-23, TO-24 and TO-25 set no BEM/CEM field. |
| 11 | Expected Results | All 25 TO-IDs mapped; field setting only where a field exists. |
| 12 | Pass/Fail Criteria | Pass if all 25 rows match [STD §9, Tables 28–29; DOC ERR-1; field mapping by name, 01.2 U-T2]. Fail on any missing or mismatched row. |
| 13 | Evidence to Collect | Completed inspection checklist. |
| 14 | External Dependencies | TO-25 value [EXTERNAL]. |
| 15 | Known Limitations | The by-name field mapping is a project interpretation (U-T2). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC05-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC05-S01 |
| 2 | Related AC ID | AC-5 |
| 3 | Covered Requirement IDs | TST-2 |
| 4 | Test Objective | Observe expiry of the timeouts outside the safety-critical set (TO-02 to TO-13, TO-18 to TO-21) against their standard durations. |
| 5 | Verification Method | Time-controlled SIL observation (non-gating SHOULD check). |
| 6 | Test Level / Environment | Non-gating. E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Per TO-ID, the node in the step where the wait is armed. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Awaited message withheld per TO-ID. |
| 10 | Execution Procedure | Per TO-ID: withhold the message; record expiry time and error field. |
| 11 | Expected Results | Expiry near the standard duration with the mapped error field. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check [DOC TST-2]: expiry time and field recorded; no acceptance tolerance is defined [OPEN]. Does not affect the AC-5 MUST result. |
| 13 | Evidence to Collect | SIL log per TO-ID. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | TO-17 excluded (OPEN); TO-22 to TO-24 in VT-AC18-S03; TO-01 in VT-AC05-05; TO-25 in VT-AC05-06. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-6 — Verification Level MUST

Baseline Covers: CAN-1, SAF-1, TST-4. Gating tests: VT-AC06-01, VT-AC06-02, VT-AC06-03. Non-gating tests: none.

#### VT-AC06-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC06-01 |
| 2 | Related AC ID | AC-6 |
| 3 | Covered Requirement IDs | SAF-1, CAN-1 |
| 4 | Test Objective | Measure, in SIL, the time from detection of each SAF-1 trigger to the output command becoming disabled with 0 A, and confirm it is set before any transmission. |
| 5 | Verification Method | SIL test with per-tick sampling of the output command. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Charger in C10 with a non-zero output command; output-command sampler recording every tick. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); output-command sampling hook. |
| 9 | Input Conditions | One run per trigger: stop request (BST received), charger fault, application timeout (TO-14), injected bus-off, RX overflow while charging. |
| 10 | Execution Procedure | 1. Establish charging. 2. Inject the trigger and record the detection timestamp. 3. Record the first sample showing disabled and 0 A. 4. Record the time of the first frame transmitted after detection. |
| 11 | Expected Results | Output command disabled and 0 A within 10 ms of detection, before any transmission caused by the event. |
| 12 | Pass/Fail Criteria | Pass if every trigger meets 10 ms [DOC SAF-1, CAN-1] and the command is set before the first post-detection transmission. Fail otherwise. INCONCLUSIVE if the sampling resolution cannot resolve the 10 ms bound. |
| 13 | Evidence to Collect | Command sample log; detection and transmission timestamps per trigger. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | "While charging" for the RX-overflow trigger is defined in PHASE 02 (RQ-04). Proves the software command only, not physical current (Project 2). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC06-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC06-02 |
| 2 | Related AC ID | AC-6 |
| 3 | Covered Requirement IDs | SAF-1, CAN-1 |
| 4 | Test Objective | Measure the same responses on target hardware with independent GPIO timing. |
| 5 | Verification Method | Target-board test with logic-analyzer timing. |
| 6 | Test Level / Environment | Gating (MUST). E5 (STM32 target boards); E6 (Logic analyzer / GPIO timing). Infrastructure — E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). E6: logic analyzer planned (charter), availability UNCONFIRMED; GPIO timing hooks NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up; confirmed logic analyzer, GPIO instrumentation in firmware. |
| 7 | Preconditions | G474 firmware with a GPIO toggled at detection and another when the output command reaches disabled/0 A. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); Logic analyzer with GPIO probes (planned; availability UNCONFIRMED). |
| 9 | Input Conditions | Triggers as VT-AC06-01 where reproducible on hardware: stop request (BST received), charger fault, application timeout (TO-14), injected bus-off, RX overflow while charging. |
| 10 | Execution Procedure | 1. Capture both GPIO lines. 2. Inject each trigger. 3. Measure the edge-to-edge time. |
| 11 | Expected Results | Edge-to-edge time within 10 ms. |
| 12 | Pass/Fail Criteria | Pass if every measured time is at most 10 ms [DOC SAF-1, CAN-1]. Fail otherwise. |
| 13 | Evidence to Collect | Logic-analyzer traces; measurement table. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | GPIO hooks NOT IMPLEMENTED; logic analyzer UNCONFIRMED. Software command only. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC06-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC06-03 |
| 2 | Related AC ID | AC-6 |
| 3 | Covered Requirement IDs | TST-4 |
| 4 | Test Objective | Show that logging never blocks safety timing by repeating VT-AC06-01 with the log queue saturated. |
| 5 | Verification Method | SIL test under forced log-queue saturation. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Log producer able to fill the log queue; log-drop counter readable. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | VT-AC06-01 triggers while the log queue is full. |
| 10 | Execution Procedure | 1. Saturate the log queue. 2. Run each VT-AC06-01 trigger. 3. Record timing and the log-drop counter. |
| 11 | Expected Results | Same 10 ms result as VT-AC06-01; the log-drop counter increases. |
| 12 | Pass/Fail Criteria | Pass if every trigger still meets 10 ms [DOC SAF-1] and the drop counter shows dropped entries [DOC TST-4]. No new threshold is introduced. Fail otherwise. |
| 13 | Evidence to Collect | Command sample log; log-drop counter before and after. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | hal_log NOT IMPLEMENTED. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-7 — Verification Level MUST

Baseline Covers: COM-6, ERR-3, ERR-6, PROT-3, PROT-4, PROT-7, SAF-2, SAF-3, SAF-5. Gating tests: VT-AC07-01, VT-AC07-02, VT-AC07-03, VT-AC07-04, VT-AC07-05, VT-AC07-06, VT-AC07-07, VT-AC07-08, VT-AC07-09, VT-AC07-10. Non-gating tests: none.

#### VT-AC07-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-01 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | PROT-7 |
| 4 | Test Objective | Verify the §11.3.1 CV and CC output rules. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | BCL in CV mode and in CC mode with non-zero demands inside the CML range. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | CV: output voltage meets the voltage demand, output current does not exceed the current demand. CC: output current meets the current demand, output voltage does not exceed the voltage demand. |
| 12 | Pass/Fail Criteria | Pass if each case matches [STD §11.3.1] with current comparisons in non-negative magnitude [OWNER D-06]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Zero-current demand is excluded while OPEN (OQ-12(a)). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-02 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | PROT-7 |
| 4 | Test Objective | Verify the current-demand rules against the CML maximum and the charger minimum current. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Non-zero current demand (magnitude) above the CML maximum, within the range, and below the charger minimum. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | Above: output at maximum capability. Within: requested current. Below minimum: output at minimum capability. |
| 12 | Pass/Fail Criteria | Pass if each case matches [STD §11.3.1; OWNER D-06 magnitude comparison]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Zero-current demand is excluded while OPEN (OQ-12(a)). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-03 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | PROT-7 |
| 4 | Test Objective | Verify the voltage rules: CC-mode voltage demand above the CML maximum, and voltage demand below the charger minimum. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | (a) CC mode, voltage demand above the CML maximum voltage. (b) Voltage demand below the charger minimum voltage. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | (a) The charger uses its own maximum voltage as the voltage limit. (b) The charger sends CST. |
| 12 | Pass/Fail Criteria | Pass if (a) and (b) match [STD §11.3.1]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | CV-mode behaviour above the CML maximum is not stated by the standard and is not tested. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-04 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | SAF-2 |
| 4 | Test Objective | Verify the SAF-2 clamps. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Demands exceeding, in turn: BCP SPN2817 maximum current, BCP SPN2819 maximum total voltage, the local current limit, the local voltage limit. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | The output command is clamped to the most restrictive applicable limit in each case. |
| 12 | Pass/Fail Criteria | Pass if each clamp binds as specified [DOC SAF-2; STD §11.2.1 permits SPN2819]. Fail otherwise. SPN2817 unit conflict OPEN (OQ-1): the SPN2817 case uses a PROVISIONAL interpretation and is INCONCLUSIVE until OQ-1 is decided. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | OQ-1 OPEN. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-05

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-05 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | SAF-3 |
| 4 | Test Objective | Verify that malformed and not-credible requests give 0 A while the session continues until its wait expires. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | (a) BCL with a wrong DLC. (b) BCL fields in the not-credible state. (c) Stale BCL. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | (a), (b): output command 0 A; the session continues until the applicable wait (TO-14) expires. (c): see limitation. |
| 12 | Pass/Fail Criteria | Pass if (a) and (b) match [DOC SAF-3; STD §8.9]. (c) is INCONCLUSIVE: the staleness threshold is OPEN (RQ-02). Fail if (a) or (b) produces non-zero output. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | RQ-02 OPEN (staleness threshold). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-06

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-06 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | ERR-6 |
| 4 | Test Objective | Verify that a received charging current outside −400 A to 0 A ends charging with the ERR-6 fault reasons. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | (a) BCL current demand raw value above 4000 (positive current) at the charger. (b) CCS current raw value above 4000 at the vehicle. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | (a) The charger exits charging and sends CST with SPN3522 bits 11–12 = 01. (b) The vehicle exits and sends BST with SPN3512 bits 15–16 = 01. |
| 12 | Pass/Fail Criteria | Pass if exit occurs with the specified fault bits [STD §5.4 with OWNER D-01; DOC ERR-6]. The accompanying SPN3521/SPN3511 value is OPEN (RQ-08) and not asserted. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | RQ-08 OPEN. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-07

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-07 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | SAF-5, PROT-4 |
| 4 | Test Objective | Verify that only fresh feedback appears in CCS and that stale or invalid feedback drives the output to 0. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Feedback with the valid flag set and age 100 ms; age 101 ms; valid flag cleared. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | 100 ms: reported in CCS. 101 ms and invalid: not reported in CCS; output command 0; the charger enters its local stop handling. |
| 12 | Pass/Fail Criteria | Pass if the 100 ms case is reported and the 101 ms and invalid cases are not, with output 0 [DOC SAF-5: fresh if at most 100 ms old; 101 ms is the first stale value at the 1 ms tick of DES-5]. The CST stop reason is OPEN (RQ-03) and not asserted. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | RQ-03 OPEN; C_STOP_LOCAL mapping in PHASE 02. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-08

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-08 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | ERR-3 |
| 4 | Test Objective | Verify that invalid data never refreshes a wait timer or affects output. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Charger in C10; valid BCL stops; frames with the BCL identifier but a wrong DLC continue to arrive periodically. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | TO-14 still expires 1 s after the last valid BCL; output unaffected by the invalid frames; invalid frames counted. |
| 12 | Pass/Fail Criteria | Pass if the TO-14 expiry time is unchanged by the invalid frames [DOC ERR-3; STD TO-14 1 s] and the invalid-frame counter increments. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The invalid-frame repetition interval is a [REC] stimulus parameter, not an acceptance value. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-09

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-09 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | PROT-3 |
| 4 | Test Objective | Verify that the configurable battery model drives BCL and that BCL demand is not limited by CML. |
| 5 | Verification Method | Host unit tests of the charger application logic, confirmed in SIL. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Two battery configurations; CML maximum current set below the battery need. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | BCL demand follows the battery model for each configuration and exceeds the CML maximum when the battery needs more. |
| 12 | Pass/Fail Criteria | Pass if BCL demand equals the model need and is not capped at the CML value [STD §11.3.1 last sentence; DOC PROT-3]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Vehicle-side test; zero-demand case excluded (OQ-12(a)). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC07-10

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC07-10 |
| 2 | Related AC ID | AC-7 |
| 3 | Covered Requirement IDs | COM-6 |
| 4 | Test Objective | Verify signed CAN current encoding and the single conversion to the non-negative application magnitude in mA. |
| 5 | Verification Method | Host unit tests of the codec. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Application logic built with the simulated backend; CML values configured. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists); Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Raw values 0 and 4000 and intermediate values; application magnitudes for encoding. |
| 10 | Execution Procedure | 1. Apply each input case. 2. Record the output command (voltage, current magnitude, enabled), the BCL produced (vehicle cases) and any transmitted CST/BST. 3. Compare with the expected result. |
| 11 | Expected Results | Decoding: physical = 0.1 A × raw − 400 A, then magnitude in mA; encoding is the inverse; the conversion is applied once. |
| 12 | Pass/Fail Criteria | Pass if all round trips match [STD §5.4, §5.6; OWNER D-02, D-06] and no double conversion occurs. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report per case; SIL log for the confirmation run. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Bit numbering within bytes per SAE J1939 [EXTERNAL] (U-P2). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-8 — Verification Level MUST

Baseline Covers: PROT-10, PROT-3, PROT-5, PROT-8, PROT-9, TST-2. Gating tests: VT-AC08-01, VT-AC08-02, VT-AC08-03, VT-AC08-04, VT-AC08-05, VT-AC08-06, VT-AC08-07, VT-AC08-08. Non-gating tests: VT-AC08-S01.

#### VT-AC08-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-01 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-5, PROT-3, TST-2 |
| 4 | Test Objective | Verify the vehicle-first stop procedure. |
| 5 | Verification Method | SIL scenario test, confirmed on hardware capture. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E4 (Physical CAN hardware). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); for confirmation STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED) and USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | Vehicle battery model reaches its target SOC during charging. |
| 10 | Execution Procedure | 1. Charge until the target SOC. 2. Capture until CSD/BSD complete. 3. Measure BST/CST intervals and count the SPN3521 stop reasons. |
| 11 | Expected Results | Vehicle sends BST every 10 ms; on CST the vehicle stops BST and sends BSD; the charger sends CST then CSD. |
| 12 | Pass/Fail Criteria | Pass if the sequence matches [STD Figure A.1 notes k, l; Table B.1 (informative)] with BST/CST at 10 ms [STD Table 5] within ±10% [DOC COM-4; metric NF-03 OPEN, interval part INCONCLUSIVE until approved] and exactly one SPN3521 reason [STD §11.3.9]. Fail otherwise. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL and hardware). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Interval metric OPEN (NF-03). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-02 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-5, TST-2 |
| 4 | Test Objective | Verify the charger-first stop procedure. |
| 5 | Verification Method | SIL scenario test, confirmed on hardware capture. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E4 (Physical CAN hardware). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); for confirmation STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED) and USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | Charger stop condition (operator stop) during charging. |
| 10 | Execution Procedure | 1. Trigger the charger stop. 2. Count the BST frames sent by the vehicle before BSD. 3. Capture to completion. |
| 11 | Expected Results | Charger sends CST every 10 ms; the vehicle sends 5 to 10 BST frames, then BSD; CSD follows. |
| 12 | Pass/Fail Criteria | Pass if the vehicle sends between 5 and 10 BST frames before BSD [STD note k; Table B.1] and the rest of the sequence matches. Fail otherwise. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL and hardware). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-03 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-8, TST-2 |
| 4 | Test Objective | Verify the charger-side parameter mismatch. |
| 5 | Verification Method | SIL scenario test, confirmed on hardware capture. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E4 (Physical CAN hardware). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); for confirmation STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED) and USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | BCP SPN2822 (taken as the battery present voltage) outside the CML voltage range. |
| 10 | Execution Procedure | 1. Configure the mismatch. 2. Capture the configuration phase. 3. Record the order of CML and CST and the CST reason. |
| 11 | Expected Results | The charger sends CML and then CST with the charging-parameter mismatch reason. |
| 12 | Pass/Fail Criteria | Pass if CML precedes CST and CST carries SPN3523 bits 5–6 = 01 [STD §10.2; Figure A.1]. Fail otherwise. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL and hardware). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | SPN2822 unit conflict OPEN (OQ-3): the test uses the voltage interpretation of the table note (PROVISIONAL). Subsequent end-phase behaviour is not asserted (OQ-S8). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-04 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-8, TST-2 |
| 4 | Test Objective | Verify the vehicle-side parameter mismatch. |
| 5 | Verification Method | SIL scenario test, confirmed on hardware capture. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E4 (Physical CAN hardware). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); for confirmation STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED) and USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | A CML voltage range that does not cover the battery rated voltage. |
| 10 | Execution Procedure | 1. Configure the mismatch. 2. Capture. 3. Record the BST reason. |
| 11 | Expected Results | The vehicle sends BST with the charging-parameter mismatch reason. |
| 12 | Pass/Fail Criteria | Pass if BST carries SPN3513 bits 5–6 = 01 [STD §10.2; DOC PROT-8 project rule]. Fail otherwise. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL and hardware). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The matching criterion is a project rule; further criteria are EXTERNAL (GB/T 18487.1, note d). The charger reaction to this BST outside charging is OPEN (OQ-S10). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-05

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-05 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-9, TST-2 |
| 4 | Test Objective | Verify that an abnormal BSM status or a received BST stops the charger with CST. |
| 5 | Verification Method | SIL scenario test, confirmed on hardware capture. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E4 (Physical CAN hardware). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; confirmed USB-CAN adapter, capture and decode tooling, two physical nodes. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); for confirmation STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED) and USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | Charging; in separate runs: each of SPN3090–3095 set abnormal; BST received. |
| 10 | Execution Procedure | 1. Inject each condition. 2. Record CST and its reason. |
| 11 | Expected Results | The charger sends CST in each case. |
| 12 | Pass/Fail Criteria | Pass if CST is sent in every case [STD §11.3.4]. Fail otherwise. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL and hardware). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Behaviour during pause is not asserted (OQ-S11). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-06

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-06 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-9 |
| 4 | Test Objective | Verify that no BSM timeout occurs before the first BSM. |
| 5 | Verification Method | SIL scenario test. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Charging entered; the vehicle never sends BSM. |
| 10 | Execution Procedure | 1. Enter C10 with BCL and BCS present but no BSM. 2. Hold for longer than 5 s. 3. Record any TO-16 expiry or CEM SPN3928. |
| 11 | Expected Results | No BSM timeout and no CEM SPN3928. |
| 12 | Pass/Fail Criteria | Pass if no TO-16 expiry occurs [STD Figure A.1 note h; §11.3.4]. Fail otherwise. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The hold duration beyond 5 s is a [REC] stimulus parameter. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-07

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-07 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-10 |
| 4 | Test Objective | Verify the PROT-10 functional output inhibit on BSM SPN3096 = 00 and record the latency. |
| 5 | Verification Method | SIL scenario test. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Charging with a non-zero output command; valid BSM with SPN3096 = 00 and SPN3090–3095 all 00. |
| 10 | Execution Procedure | 1. Inject the BSM. 2. Sample the output command every tick. 3. Record the time from BSM reception to disabled/0 A. |
| 11 | Expected Results | The output command enters the safe no-output condition (disabled, 0 A). |
| 12 | Pass/Fail Criteria | Pass if the command reaches disabled and 0 A [OWNER PROT-10 MUST, D-08]. The observed latency is recorded as evidence only; no numeric limit applies and the SAF-1 10 ms limit is not applied [OWNER D-08]. A harness watchdog may end the run; its duration is a [REC] test parameter, not a product requirement. Fail if the command does not reach disabled/0 A before the watchdog ends the run. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Software command only; not physical shutdown (Project 2). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-08

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-08 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-5 |
| 4 | Test Objective | Verify the D-04 no-output condition on a peer stop message outside the charging phase. |
| 5 | Verification Method | SIL scenario test. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | (a) BST injected at the charger in C04 to C09. (b) CST injected at the vehicle in V03 to V09. |
| 10 | Execution Procedure | 1. Inject in each step. 2. Sample the charger output command and the vehicle current request. |
| 11 | Expected Results | The output command and the current request never become non-zero after the injection. |
| 12 | Pass/Fail Criteria | Pass if no non-zero output or request occurs in any case [OWNER D-04]. The protocol response sequence is not asserted (OQ-S10). Fail otherwise. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | "Active charging phase" is defined in PHASE 02 (RQ-04); only steps clearly before charging are used. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC08-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC08-S01 |
| 2 | Related AC ID | AC-8 |
| 3 | Covered Requirement IDs | PROT-8 |
| 4 | Test Objective | Check that a vehicle that updates BCP sends at least two updated BCP frames before BRO. |
| 5 | Verification Method | SIL scenario test. Non-gating SHOULD check. |
| 6 | Test Level / Environment | Non-gating. E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Both nodes in the state stated in the input conditions; capture running. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Vehicle configured to update a BCP parameter during configuration. |
| 10 | Execution Procedure | 1. Trigger the update. 2. Count the updated BCP frames before BRO 0x00. |
| 11 | Expected Results | At least two updated BCP frames. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check [STD note f "宜"; DOC PROT-8 SHOULD]: result recorded; it does not affect the AC-8 MUST result. |
| 13 | Evidence to Collect | Timestamped captures and state logs (SIL). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-9a — Verification Level MUST

Baseline Covers: CAN-1, CAN-2, CAN-4, ERR-5. Gating tests: VT-AC09A-01, VT-AC09A-02, VT-AC09A-03. Non-gating tests: none.

#### VT-AC09A-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC09A-01 |
| 2 | Related AC ID | AC-9a |
| 3 | Covered Requirement IDs | CAN-1, ERR-5 |
| 4 | Test Objective | Verify the immediate response to an injected bus-off while charging: output/request to 0 within 10 ms, nothing transmitted (including BEM/CEM), fault latched, controller state logged. |
| 5 | Verification Method | Software bus-off injection through the HAL test seam, per port. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E5 (STM32 target boards). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Charging in progress; HAL built with HAL_TESTING; hal_test_inject_bus_off available. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); later per port STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED). |
| 9 | Input Conditions | One injected bus-off event. |
| 10 | Execution Procedure | 1. Inject bus-off. 2. Sample the output command every tick. 3. Capture the bus. 4. Read the fault latch and the log. |
| 11 | Expected Results | Output command or current request 0 within 10 ms; no frame transmitted while bus-off; fault latched; controller state logged. |
| 12 | Pass/Fail Criteria | Pass if all four hold [DOC CAN-1 10 ms; DOC ERR-5 no BEM/CEM while bus-off]. Fail otherwise. |
| 13 | Evidence to Collect | Command samples; capture; latch and log dumps; per-port record (Linux, G474, F103). |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | hal_test_inject_bus_off NOT IMPLEMENTED. Injected status carries no real TEC/REC values; whether injected values suffice for CAN-1 is OPEN (VG-07). Proves application logic only. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC09A-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC09A-02 |
| 2 | Related AC ID | AC-9a |
| 3 | Covered Requirement IDs | CAN-2 |
| 4 | Test Objective | Verify that after a successful recovery the old session is not resumed and a new session starts from CHM. |
| 5 | Verification Method | Software bus-off injection followed by injected recovery completion. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E5 (STM32 target boards). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | As VT-AC09A-01. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); later per port STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED). |
| 9 | Input Conditions | Bus-off during charging, then a recovery-complete event within the first attempt. |
| 10 | Execution Procedure | 1. Inject bus-off. 2. Signal recovery completion. 3. Capture the next frames from both nodes. |
| 11 | Expected Results | No frame of the old session (CCS, BCL, BCS) after recovery; the charger starts again with CHM. |
| 12 | Pass/Fail Criteria | Pass if the first charger frame after recovery is CHM and no old-session frame appears [DOC CAN-1, CAN-2]. Fail otherwise. |
| 13 | Evidence to Collect | Capture; state log. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The recovery-complete event per port is not yet defined (RQ-01, PHASE 02). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC09A-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC09A-03 |
| 2 | Related AC ID | AC-9a |
| 3 | Covered Requirement IDs | CAN-4, CAN-1 |
| 4 | Test Objective | Verify the recovery-attempt policy: 1 s per attempt, at most 3 attempts, FAULT_LOCAL after the third failure, operator restart required. |
| 5 | Verification Method | Software bus-off injection with suppressed recovery completion. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E5 (STM32 target boards). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | As VT-AC09A-01; recovery-complete events suppressed by the harness. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); later per port STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED). |
| 9 | Input Conditions | Bus-off during charging; no recovery-complete event in any attempt; later an operator restart command. |
| 10 | Execution Procedure | 1. Inject bus-off. 2. Record each attempt start and its expiry. 3. After the third expiry, record the state. 4. Confirm no further attempt and no transmission. 5. Issue an operator restart and record the state. |
| 11 | Expected Results | Three attempts, each failing after 1 s without completion; FAULT_LOCAL after the third; no automatic exit; operator restart leaves FAULT_LOCAL. |
| 12 | Pass/Fail Criteria | Pass if exactly three attempts occur, each failing after 1 s [OWNER D-07; DOC CAN-1], the node is then in FAULT_LOCAL [DOC CAN-4] and leaves it only on operator restart. The attempt-duration measurement tolerance is not established [OPEN]. Fail otherwise. Not executable until the attempt-start event, recovery-complete event, inter-attempt interval and operator-restart mechanism are defined (RQ-01, PHASE 02). |
| 13 | Evidence to Collect | State log with attempt timestamps; capture. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | RQ-01 OPEN items; FAULT_LOCAL is a legacy state name mapped in PHASE 02. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-9b — Verification Level SHOULD

Baseline Covers: CAN-3. Gating tests: none (SHOULD-only row). Non-gating tests: VT-AC09B-S01. SHOULD-only row: non-gating test only.

#### VT-AC09B-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC09B-S01 |
| 2 | Related AC ID | AC-9b |
| 3 | Covered Requirement IDs | CAN-3 |
| 4 | Test Objective | Observe a hardware bus-off, if reproducible, with TEC, REC and controller-state transitions logged. |
| 5 | Verification Method | Hardware fault (CAN_H shorted to CAN_L while the node transmits). Non-gating SHOULD test. |
| 6 | Test Level / Environment | Non-gating. E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Transceiver datasheet checked for safe short-circuit tolerance; node transmitting. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); switch for the CAN_H–CAN_L short; USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | Short applied until bus-off is reported, or until a stop time chosen per the datasheet check ([REC] parameter). |
| 10 | Execution Procedure | 1. Apply the short. 2. Record TEC, REC and state transitions. 3. Remove the short and observe the AC-9a behaviour. |
| 11 | Expected Results | Error-warning and error-passive logged with TEC/REC; bus-off entered. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD test [DOC CAN-3]: result recorded as observed, not reproducible, or not conforming. Objective pass/fail conditions are OPEN. |
| 13 | Evidence to Collect | Board logs; capture. |
| 14 | External Dependencies | Transceiver behaviour per its datasheet [EXTERNAL]. |
| 15 | Known Limitations | May not be reproducible with the selected transceiver. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-9c — Verification Level SHOULD

Baseline Covers: CAN-5. Gating tests: none (SHOULD-only row). Non-gating tests: VT-AC09C-S01. SHOULD-only row: non-gating test only.

#### VT-AC09C-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC09C-S01 |
| 2 | Related AC ID | AC-9c |
| 3 | Covered Requirement IDs | CAN-5 |
| 4 | Test Objective | Observe the system response when CAN_H is disconnected during charging (supplementary physical evidence for CAN-5). |
| 5 | Verification Method | Hardware disconnection test. Non-gating SHOULD test. |
| 6 | Test Level / Environment | Non-gating. E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Hardware charging session in progress. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED); switch in CAN_H. |
| 9 | Input Conditions | CAN_H opened during charging. |
| 10 | Execution Procedure | 1. Open CAN_H. 2. Record logs from both nodes and the output command. 3. Restore CAN_H and observe whether any node resumes without a new session. |
| 11 | Expected Results | Expected result OPEN (to be defined); observations are recorded against SAF-1 and CAN-2. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD test: observations recorded; no pass/fail criterion is defined [OPEN]. |
| 13 | Evidence to Collect | Board logs; capture. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Physical behaviour depends on the transceiver; the CAN-5 MUST gate is AC-5 (VT-AC05-07). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-9d — Verification Level SHOULD

Baseline Covers: CAN-5. Gating tests: none (SHOULD-only row). Non-gating tests: VT-AC09D-S01. SHOULD-only row: non-gating test only.

#### VT-AC09D-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC09D-S01 |
| 2 | Related AC ID | AC-9d |
| 3 | Covered Requirement IDs | CAN-5 |
| 4 | Test Objective | Observe the system response when CAN_H and CAN_L are both disconnected during charging (supplementary physical evidence for CAN-5). |
| 5 | Verification Method | Hardware disconnection test. Non-gating SHOULD test. |
| 6 | Test Level / Environment | Non-gating. E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Hardware charging session in progress. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED); switches in CAN_H and CAN_L. |
| 9 | Input Conditions | CAN_H and CAN_L opened during charging. |
| 10 | Execution Procedure | 1. Open both lines. 2. Record the expiring waits by TO-ID and the output command. 3. Restore and observe. |
| 11 | Expected Results | Charging-phase waits at the charger (e.g. TO-14) expire and the output command reaches disabled/0 A; the full "safe shutdown" result is OPEN (to be defined). |
| 12 | Pass/Fail Criteria | Non-gating SHOULD test: observations recorded; no pass/fail criterion is defined [OPEN]. The Spec §30 W6/W15 references are UNMAPPED and not used. |
| 13 | Evidence to Collect | Board logs; capture. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The CAN-5 MUST gate is AC-5. TO-17 (vehicle CCS during charging) is OPEN and not assessed. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-10 — Verification Level MUST

Baseline Covers: DES-1, DES-2, STD-1. Gating tests: VT-AC10-01, VT-AC10-02, VT-AC10-03. Non-gating tests: none.

#### VT-AC10-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC10-01 |
| 2 | Related AC ID | AC-10 |
| 3 | Covered Requirement IDs | DES-2 |
| 4 | Test Objective | Build the same protocol sources for G474, F103 and Linux. |
| 5 | Verification Method | Build of three targets from one source tree. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint); E5 (STM32 target boards). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Build configurations for the three targets. |
| 8 | Required Equipment and Fixtures | Linux host toolchain (exists); ARM cross toolchain (UNCONFIRMED). |
| 9 | Input Conditions | Clean checkout. |
| 10 | Execution Procedure | 1. Build each target. 2. Record the protocol source list per target. |
| 11 | Expected Results | All three builds succeed from identical protocol sources. |
| 12 | Pass/Fail Criteria | Pass if all three builds succeed and the protocol source lists are identical [DOC DES-2]. Fail otherwise. |
| 13 | Evidence to Collect | Build logs; source-list comparison. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | STM32 ports NOT IMPLEMENTED. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC10-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC10-02 |
| 2 | Related AC ID | AC-10 |
| 3 | Covered Requirement IDs | DES-1, DES-2 |
| 4 | Test Objective | Check by script that protocol/ includes no hardware headers, reaches hardware only through hal/hal.h, and contains no platform-specific conditional compilation. |
| 5 | Verification Method | Static source check. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Protocol sources present. |
| 8 | Required Equipment and Fixtures | Static check script (NOT IMPLEMENTED). |
| 9 | Input Conditions | protocol/ source tree. |
| 10 | Execution Procedure | 1. List all includes in protocol/. 2. Flag any include outside protocol/, hal/hal.h and standard C headers. 3. Flag platform-specific conditional-compilation directives. |
| 11 | Expected Results | No hardware header; no platform-specific conditional compilation. |
| 12 | Pass/Fail Criteria | Pass if zero findings [DOC DES-1, DES-2]. Fail on any finding. |
| 13 | Evidence to Collect | Script output. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The allow-list of permitted standard headers is defined with the script. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC10-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC10-03 |
| 2 | Related AC ID | AC-10 |
| 3 | Covered Requirement IDs | STD-1 |
| 4 | Test Objective | Confirm by inspection that System B, the 50 kbit/s option and the adapter node are not implemented. |
| 5 | Verification Method | Inspection. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Implementation and YAML present. |
| 8 | Required Equipment and Fixtures | Review checklist. |
| 9 | Input Conditions | NOT APPLICABLE (inspection). |
| 10 | Execution Procedure | 1. Search sources and YAML for System B messages, a 50 kbit/s configuration and an adapter address. 2. Record findings. |
| 11 | Expected Results | None present. |
| 12 | Pass/Fail Criteria | Pass if none present [DOC STD-1]. Fail otherwise. |
| 13 | Evidence to Collect | Inspection record. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Inspection only. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-11 — Verification Level MUST

Baseline Covers: DES-3, DES-5, DES-6, PROT-2, TST-1, TST-6. Gating tests: VT-AC11-01, VT-AC11-02, VT-AC11-03, VT-AC11-04, VT-AC11-05, VT-AC11-06. Non-gating tests: VT-AC11-S01.

#### VT-AC11-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC11-01 |
| 2 | Related AC ID | AC-11 |
| 3 | Covered Requirement IDs | TST-1 |
| 4 | Test Objective | Confirm that every TST-1 item has at least one passing unit test. |
| 5 | Verification Method | Traceability review of the unit-test inventory against the TST-1 list. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Unit-test build of the module under test. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | TST-1 item list; unit-test inventory. |
| 10 | Execution Procedure | 1. Map each TST-1 item to unit tests. 2. Record unmapped items. 3. Attach the test report. |
| 11 | Expected Results | Every TST-1 item mapped to a passing test. |
| 12 | Pass/Fail Criteria | Pass if no TST-1 item is unmapped and all mapped tests pass [DOC TST-1]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | T1–T3 items depend on EXTERNAL values (VT-AC04-02). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC11-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC11-02 |
| 2 | Related AC ID | AC-11 |
| 3 | Covered Requirement IDs | TST-1 |
| 4 | Test Objective | Verify TO-14, TO-15 and TO-16 at T−1 ms and T+1 ms in unit tests of the timer and state-machine logic. |
| 5 | Verification Method | Unit tests with virtual time. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Unit-test build of the module under test. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Virtual time advanced to T − 1 ms and T + 1 ms for each of TO-14 (1 s), TO-15 (5 s) and TO-16 (5 s, armed after the first BSM) in C10 and C10P. |
| 10 | Execution Procedure | Per timeout and state: arm, advance, record. |
| 11 | Expected Results | No expiry at T − 1 ms; expiry at T + 1 ms. |
| 12 | Pass/Fail Criteria | Pass if all 12 cases behave as expected [STD durations; OWNER D-05/ODS-01; DOC TST-1]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Virtual time verifies logic only (§6.2). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC11-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC11-03 |
| 2 | Related AC ID | AC-11 |
| 3 | Covered Requirement IDs | DES-5 |
| 4 | Test Objective | Verify wrap-safe 32-bit millisecond timer comparisons. |
| 5 | Verification Method | Unit tests at the 32-bit wrap. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Unit-test build of the module under test. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Timers armed shortly before 0xFFFFFFFF and expiring after the wrap. |
| 10 | Execution Procedure | Arm, advance across the wrap, record. |
| 11 | Expected Results | Expiry at the correct elapsed time across the wrap. |
| 12 | Pass/Fail Criteria | Pass if every case expires at the correct elapsed time [DOC DES-5]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC11-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC11-04 |
| 2 | Related AC ID | AC-11 |
| 3 | Covered Requirement IDs | DES-6 |
| 4 | Test Objective | Verify deterministic processing through the received-frame and tick entry points. |
| 5 | Verification Method | Replay test. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Unit-test build of the module under test. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | A recorded script of frames and ticks. |
| 10 | Execution Procedure | 1. Replay the script twice. 2. Compare the outputs. |
| 11 | Expected Results | Identical output traces. |
| 12 | Pass/Fail Criteria | Pass if the traces are identical [DOC DES-6]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC11-05

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC11-05 |
| 2 | Related AC ID | AC-11 |
| 3 | Covered Requirement IDs | DES-3 |
| 4 | Test Objective | Verify bounded queues at their configured capacities. |
| 5 | Verification Method | Unit tests filling each queue past capacity. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Unit-test build of the module under test. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Queue fills beyond capacity. |
| 10 | Execution Procedure | Fill each queue; record overflow handling and counters. |
| 11 | Expected Results | Overflow counted; no write beyond capacity. |
| 12 | Pass/Fail Criteria | Pass if overflow is counted and bounds hold at the configured capacities [DOC DES-3]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The capacities are specified in Spec v2.0 §9 [DOC, Spec only]; the baseline says "specified capacities". |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC11-06

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC11-06 |
| 2 | Related AC ID | AC-11 |
| 3 | Covered Requirement IDs | PROT-2 |
| 4 | Test Objective | Verify that each state machine runs independently, driven only by scripted peer frames. |
| 5 | Verification Method | Unit tests of each machine alone. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Unit-test build of the module under test. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Scripted peer frames for a normal session. |
| 10 | Execution Procedure | 1. Run the charger machine alone. 2. Run the vehicle machine alone. 3. Inspect for shared state. |
| 11 | Expected Results | Each machine completes its sequence without the other; no shared session-state variable. |
| 12 | Pass/Fail Criteria | Pass if both complete alone and no shared state is found [DOC PROT-2]. Fail otherwise. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC11-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC11-S01 |
| 2 | Related AC ID | AC-11 |
| 3 | Covered Requirement IDs | TST-6 |
| 4 | Test Objective | Report line and branch coverage of protocol/. |
| 5 | Verification Method | Coverage measurement (non-gating SHOULD check). |
| 6 | Test Level / Environment | Non-gating. E1 (Host unit tests / CTest). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test. |
| 7 | Preconditions | Unit-test build of the module under test. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Unit-test run with coverage instrumentation. |
| 10 | Execution Procedure | Run; generate the report. |
| 11 | Expected Results | Coverage report produced. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check: coverage reported against the 80% line-coverage target [DOC TST-6]; it does not block release or the AC-11 MUST result. |
| 13 | Evidence to Collect | Unit-test report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-12 — Verification Level MUST

Baseline Covers: COM-5, COM-7, DES-3, DES-4. Gating tests: VT-AC12-01, VT-AC12-02, VT-AC12-03. Non-gating tests: VT-AC12-S01.

#### VT-AC12-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC12-01 |
| 2 | Related AC ID | AC-12 |
| 3 | Covered Requirement IDs | DES-4 |
| 4 | Test Objective | Show that all targets build with zero warnings under -Wall -Wextra -Werror. |
| 5 | Verification Method | Build log review. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Build configurations with -Wall -Wextra -Werror. |
| 8 | Required Equipment and Fixtures | Toolchains (Linux exists; ARM UNCONFIRMED). |
| 9 | Input Conditions | Clean checkout. |
| 10 | Execution Procedure | Build all targets; collect warnings. |
| 11 | Expected Results | Zero warnings. |
| 12 | Pass/Fail Criteria | Pass if zero warnings on every target [DOC DES-4]. Fail otherwise. |
| 13 | Evidence to Collect | Build logs. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | STM32 targets NOT IMPLEMENTED. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC12-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC12-02 |
| 2 | Related AC ID | AC-12 |
| 3 | Covered Requirement IDs | DES-3 |
| 4 | Test Objective | Show that no dynamic memory allocation is used. |
| 5 | Verification Method | Static symbol and source check. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Linked images available. |
| 8 | Required Equipment and Fixtures | Symbol inspection tool (host toolchain). |
| 9 | Input Conditions | Linked images per target. |
| 10 | Execution Procedure | 1. Search the link maps and symbol tables for allocation functions. 2. Search the sources for allocation calls. |
| 11 | Expected Results | No allocation function referenced. |
| 12 | Pass/Fail Criteria | Pass if none is found [DOC DES-3]. Fail otherwise. |
| 13 | Evidence to Collect | Symbol listing; search output. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC12-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC12-03 |
| 2 | Related AC ID | AC-12 |
| 3 | Covered Requirement IDs | COM-5, COM-7 |
| 4 | Test Objective | Show that no protocol constant appears outside generated files and that no bare CTS identifier exists. |
| 5 | Verification Method | Lint. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Lint tool with the reviewed allow-list. |
| 8 | Required Equipment and Fixtures | Lint tool (NOT IMPLEMENTED). |
| 9 | Input Conditions | Source tree. |
| 10 | Execution Procedure | Run the lint; review findings against the allow-list. |
| 11 | Expected Results | No findings. |
| 12 | Pass/Fail Criteria | Pass if zero findings [DOC COM-5, COM-7]. Fail otherwise. |
| 13 | Evidence to Collect | Lint report. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Lint rules depend on the generated tables (YAML NOT IMPLEMENTED). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC12-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC12-S01 |
| 2 | Related AC ID | AC-12 |
| 3 | Covered Requirement IDs | COM-5 |
| 4 | Test Objective | Check that documentation tables are generated from the YAML and that a CI staleness check exists. |
| 5 | Verification Method | Inspection (non-gating SHOULD check). |
| 6 | Test Level / Environment | Non-gating. E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Generator available. |
| 8 | Required Equipment and Fixtures | Generator (NOT IMPLEMENTED). |
| 9 | Input Conditions | YAML. |
| 10 | Execution Procedure | Generate the documentation; modify the YAML; confirm the staleness check reports the difference. |
| 11 | Expected Results | Documentation generated; staleness detected. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check [DOC COM-5 SHOULD]: result recorded; it does not affect the AC-12 MUST result. |
| 13 | Evidence to Collect | Generator output; CI log. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | CI system not defined [OPEN]. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-13 — Verification Level MUST

Baseline Covers: TST-4, TST-5, TST-7. Gating tests: VT-AC13-01, VT-AC13-02. Non-gating tests: VT-AC13-S01.

#### VT-AC13-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC13-01 |
| 2 | Related AC ID | AC-13 |
| 3 | Covered Requirement IDs | TST-7 |
| 4 | Test Objective | Show that a clean checkout builds all three targets and runs the unit and SIL tests with the documented commands, with toolchain versions recorded. |
| 5 | Verification Method | Fresh-clone execution. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint); E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations; SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Fresh clone in a clean environment. |
| 8 | Required Equipment and Fixtures | Clean Linux environment (exists); ARM toolchain (UNCONFIRMED). |
| 9 | Input Conditions | Documented commands only. |
| 10 | Execution Procedure | 1. Clone. 2. Run the documented build command. 3. Run the documented test command. 4. Record toolchain versions. |
| 11 | Expected Results | Builds and tests run without undocumented steps; versions recorded. |
| 12 | Pass/Fail Criteria | Pass if no undocumented step is needed and toolchain versions are recorded [DOC TST-7]. Fail otherwise. |
| 13 | Evidence to Collect | Session transcript. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Test outcomes themselves are to be verified by the other ACs. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC13-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC13-02 |
| 2 | Related AC ID | AC-13 |
| 3 | Covered Requirement IDs | TST-4 |
| 4 | Test Objective | Review sample logs for state transitions, errors, controller-state changes and output-command changes with millisecond timestamps. |
| 5 | Verification Method | Log review. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | A logged SIL session including a fault. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Logged session. |
| 10 | Execution Procedure | Check each required event category and the timestamp resolution. |
| 11 | Expected Results | All categories present with millisecond timestamps. |
| 12 | Pass/Fail Criteria | Pass if every category is present with millisecond timestamps [DOC TST-4]. Fail otherwise. |
| 13 | Evidence to Collect | Annotated log excerpt. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | The non-blocking property is to be verified by VT-AC06-03. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC13-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC13-S01 |
| 2 | Related AC ID | AC-13 |
| 3 | Covered Requirement IDs | TST-5 |
| 4 | Test Objective | Check the basic CAN log decoder (SHOULD) and the YAML-generated decoder (STRETCH). |
| 5 | Verification Method | Tool demonstration (non-gating). |
| 6 | Test Level / Environment | Non-gating. E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Decoder tools available. |
| 8 | Required Equipment and Fixtures | Decoder (NOT IMPLEMENTED). |
| 9 | Input Conditions | A recorded capture. |
| 10 | Execution Procedure | Decode the capture. |
| 11 | Expected Results | Decoded message listing. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD/STRETCH check [DOC TST-5]: result recorded; it does not affect the AC-13 MUST result. |
| 13 | Evidence to Collect | Decoder output. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-14 — Verification Level MUST

Baseline Covers: ERR-4, TP-6, TST-3. Gating tests: VT-AC14-01, VT-AC14-02, VT-AC14-03, VT-AC14-04, VT-AC14-05, VT-AC14-06, VT-AC14-07, VT-AC14-08, VT-AC14-09. Non-gating tests: VT-AC14-S01, VT-AC14-S02.

#### VT-AC14-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-01 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | Drop BCL during charging. |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | C10; BCL withheld. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | TO-14 expires; CEM SPN3925 = 01; timeout handling entered; output command 0. |
| 12 | Pass/Fail Criteria | Pass if timeout handling with CEM SPN3925 is entered and the output reaches 0 [STD Figure A.1; DOC TST-3; DOC SAF-1]. The reconnection outcome (SHOULD, ERR-2) is recorded without gating. Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Boundary timing is to be verified by VT-AC05-01. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-02 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | Drop the first CCS (first-CCS wait, TO-13). |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | Vehicle in V10 after CRO 0xAA; first CCS withheld. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | TO-13 expires; the vehicle enters timeout handling and sends BEM with SPN3905 = 01. |
| 12 | Pass/Fail Criteria | Pass if the vehicle enters timeout handling with BEM SPN3905 = 01 [STD Figure A.1 p.51; OWNER AC14-02 treatment]. The expiry time is recorded; no tolerance is applied [OPEN]. Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | Covers only the first-CCS wait; charging-phase CCS loss is VT-AC14-S01 (TO-17 OPEN). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-S01 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | Observe charging-phase CCS loss. |
| 5 | Verification Method | SIL fault injection (non-gating). |
| 6 | Test Level / Environment | Non-gating. E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | Vehicle in V11; CCS withheld. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | Observed behaviour recorded; no expected timeout behaviour is defined while TO-17 is OPEN. |
| 12 | Pass/Fail Criteria | Observational, non-gating [OWNER AC14-02 treatment]: no pass/fail; TO-17 OPEN (ODS-01; OQ-S4). It does not affect the AC-14 MUST result. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | TO-17 OPEN. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-03 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | Invalid frame length. |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | C10; BCL frames with a wrong DLC. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | Frames rejected and counted; no output change; the session continues until TO-14 expires. |
| 12 | Pass/Fail Criteria | Pass if the frames are rejected, counted and do not affect output or timers [DOC ERR-3, SAF-3]. Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-04 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | Out-of-range BCL current. |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | C10; BCL current demand raw value above 4000. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | The charger exits charging with CST (SPN3522 bits 11–12 = 01). |
| 12 | Pass/Fail Criteria | Pass if exit with CST and the fault bits occurs [STD §5.4 with OWNER D-01; DOC ERR-6]. The SPN3521 value is OPEN (RQ-08). Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-05

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-05 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3, TP-6 |
| 4 | Test Objective | Missing TP_DT. |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | BCS transfer with one data frame suppressed; subsequent BCS transfers normal. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | The transfer fails (transport timer); no BEM/CEM field is set by the transport failure alone; the next BCS period delivers normally. |
| 12 | Pass/Fail Criteria | Pass if no BEM/CEM field is set by the transport failure alone [DOC TP-6] and the failure is counted as a transport failure. Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | Transport timers per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-06

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-06 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | Wrong transport sequence number. |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | BRM transfer with a sequence gap. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | Transfer aborted; message not delivered; counted as a transport failure. |
| 12 | Pass/Fail Criteria | Pass if aborted and counted [DOC TP-5]. Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | Abort codes per SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-07

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-07 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | Injected bus-off status. |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | C10; bus-off injected. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | As VT-AC09A-01: output 0 within 10 ms; no transmission; fault latched. |
| 12 | Pass/Fail Criteria | Pass if the VT-AC09A-01 expectations hold [DOC CAN-1, ERR-5]. Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | hal_test_inject_bus_off NOT IMPLEMENTED. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-08

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-08 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | RX overflow while charging. |
| 5 | Verification Method | SIL fault injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | C10; RX queue flooded beyond capacity. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | Overflow counted; output command 0 within 10 ms; local stop handling. |
| 12 | Pass/Fail Criteria | Pass if the overflow is counted and the output reaches 0 within 10 ms [DOC SAF-1]. Fail otherwise. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | "While charging" per RQ-04 (PHASE 02). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-09

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-09 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | ERR-4 |
| 4 | Test Objective | Verify that the software keeps the four ERR-4 failure classes — application timeouts, transport failures, controller failures and physical failures — separate in category-specific counters and category-specific diagnostic log entries, with no cross-class misclassification. |
| 5 | Verification Method | SIL fault injection: existing AC-14 scenarios provide the application-timeout, transport-failure and controller-failure stimuli; a synthetic physical-layer fault indication injected through an abstract HAL/driver fault-reporting boundary provides the physical-failure stimulus. Software classification and reporting only. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; an abstract HAL/driver fault-reporting boundary through which a synthetic physical-layer fault indication can be delivered (interface to be defined in PHASE 02). |
| 7 | Preconditions | Each of the five stimuli is applied in its own independently initialized SIL session, with the four class counters and the diagnostic-log position reset or snapshotted before that stimulus. Protocol state per stimulus: S1 BCL timeout — charger in C10; S2 missing TP_DT on BCS — charging phase, charger in C10; S3 wrong sequence number on BRM — identification phase, vehicle in V05 sending BRM and charger in C04 receiving it; S4 injected bus-off status — C10; S5 synthetic physical-fault indication — C10. Fault injector active; an abstract fault-reporting boundary able to deliver a physical-layer fault indication to the software (interface defined in PHASE 02). |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED); injection of a synthetic physical-fault indication at the abstract fault-reporting boundary (NOT IMPLEMENTED; interface defined in PHASE 02). |
| 9 | Input Conditions | Four failure classes, five independently applied stimuli: S1 application timeout — the VT-AC14-01 scenario (BCL withheld until TO-14 expires); S2 transport failure — the VT-AC14-05 scenario (BCS transfer with one data frame suppressed, missing TP_DT); S3 transport failure — the VT-AC14-06 scenario (BRM transfer with a sequence gap); S4 controller failure — the VT-AC14-07 scenario (injected bus-off status); S5 physical failure — a synthetic physical-layer fault indication delivered through the abstract fault-reporting boundary (a software test input; no real bus fault). |
| 10 | Execution Procedure | For each stimulus S1 to S5 separately: 1. Start a new SIL session and bring it to the protocol state given for that stimulus in the preconditions (C10 for S1, S2, S4 and S5; identification phase, V05 / C04, for S3). 2. Reset or snapshot the four class counters and the diagnostic-log position. 3. Apply the stimulus. 4. Record all four class counters and the new diagnostic log entries with their categories. 5. Compare the deltas with the expected class of that stimulus. Each stimulus is evaluated on its own; results are not accumulated across stimuli. |
| 11 | Expected Results | S1 increments only the application-timeout counter and log category; S2 and S3 each increment only the transport-failure counter and log category; S4 increments only the controller-failure counter and log category; S5 increments only the physical-failure counter and log category. For every stimulus, the other three class counters and log categories are unchanged. |
| 12 | Pass/Fail Criteria | Pass if each of the five stimuli, evaluated independently, increments its own class counter and log category and no other class counter or log category, so that all four classes are reached (the transport-failure class by both S2 and S3) [DOC ERR-4]. Fail if any stimulus increments another class counter or log category, or does not increment its own class counter. Frame/signal validation events (VT-AC14-03, VT-AC14-04; ERR-3) are not one of the four ERR-4 classes and are not used as a class stimulus. The class of events not named by ERR-4 (e.g. RX overflow, VT-AC14-08) is not asserted. |
| 13 | Evidence to Collect | Per stimulus S1 to S5: session identifier and protocol state at injection, the four class counters before and after, the new diagnostic log entries with their categories and timestamps; for S5, the record of the injected indication. |
| 14 | External Dependencies | NONE for software classification. The abstract fault-reporting boundary and the indication format are defined in PHASE 02. |
| 15 | Known Limitations | The physical-failure stimulus (S5) verifies only that the software classifies and records a physical-fault indication received at the abstract boundary. It does not verify detection of real physical-layer faults: vcan cannot generate CRC, stuff or bit errors or electrical bus faults, and SIL classification is not evidence of physical fault detection. Actual physical-layer fault generation and hardware robustness testing remain SHOULD (PROJECT_SCOPE.md §5.2; AC-9b to AC-9d). Bus-off (stimulus S4) is treated as a controller failure, not as a physical failure. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC14-S02 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC14-S02 |
| 2 | Related AC ID | AC-14 |
| 3 | Covered Requirement IDs | TST-3 |
| 4 | Test Objective | SHOULD fault scenarios: delay, duplicate, reorder, undefined PGN, invalid CTS window, missing EOM ACK and abort. |
| 5 | Verification Method | SIL fault injection (non-gating). |
| 6 | Test Level / Environment | Non-gating. E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | SIL session in the state stated in the input conditions; fault injector active. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED); fault injector (NOT IMPLEMENTED). |
| 9 | Input Conditions | Each SHOULD fault in an appropriate state. |
| 10 | Execution Procedure | 1. Reach the stated state. 2. Inject the fault. 3. Record state, transmitted frames, error fields, output command and diagnostic counters. |
| 11 | Expected Results | Behaviour per the related requirements (e.g. an undefined PGN is not processed, COM-8). |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check [DOC TST-3]: results recorded; they do not affect the AC-14 MUST result. STRETCH "extensive scenarios" are not defined [OPEN]. |
| 13 | Evidence to Collect | Fault-report entry: injected fault, timestamps, frames, counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-15 — Verification Level MUST

Baseline Covers: SAF-4. Gating tests: VT-AC15-01, VT-AC15-02. Non-gating tests: none.

#### VT-AC15-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC15-01 |
| 2 | Related AC ID | AC-15 |
| 3 | Covered Requirement IDs | SAF-4 |
| 4 | Test Objective | Show that the SIL suite runs with the simulated backend and that the output command and feedback interfaces are in app/output_if.h outside protocol/. |
| 5 | Verification Method | SIL run plus static check. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan); E3 (Static analysis / generator / lint). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | Simulated backend selected. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | Full SIL suite. |
| 10 | Execution Procedure | 1. Run the SIL suite. 2. Locate the interface definitions. |
| 11 | Expected Results | The SIL suite runs; the interfaces are only in app/output_if.h. |
| 12 | Pass/Fail Criteria | Pass if the suite runs with the simulated backend and the interfaces are located as required [DOC SAF-4]. Fail otherwise. |
| 13 | Evidence to Collect | SIL run log; file listing. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC15-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC15-02 |
| 2 | Related AC ID | AC-15 |
| 3 | Covered Requirement IDs | SAF-4 |
| 4 | Test Objective | Show that selecting a different backend changes only the build-time backend selection. |
| 5 | Verification Method | Build with an alternate (stub) backend and source diff. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | A stub alternate backend. |
| 8 | Required Equipment and Fixtures | Host toolchain (exists). |
| 9 | Input Conditions | Build with the stub backend selected. |
| 10 | Execution Procedure | 1. Build. 2. Diff against the simulated-backend build configuration. |
| 11 | Expected Results | Only the backend selection differs. |
| 12 | Pass/Fail Criteria | Pass if no protocol or application source differs [DOC SAF-4]. Fail otherwise. |
| 13 | Evidence to Collect | Build log; diff. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | A real Project 2 backend is not available. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-16 — Verification Level MUST

Baseline Covers: COM-5, STD-2, TP-3. Gating tests: VT-AC16-01. Non-gating tests: none.

#### VT-AC16-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC16-01 |
| 2 | Related AC ID | AC-16 |
| 3 | Covered Requirement IDs | STD-2, COM-5, TP-3 |
| 4 | Test Objective | Verify conflict-record and source-reference integrity of the YAML under the five D-10 conditions. |
| 5 | Verification Method | Generator report review. |
| 6 | Test Level / Environment | Gating (MUST). E3 (Static analysis / generator / lint). Infrastructure — E3: Linux compiler toolchain EXISTS; protocol YAML, generator and lint tool NOT IMPLEMENTED; G474/F103 build targets NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: YAML, generator, lint tool, target build configurations. |
| 7 | Preconditions | YAML and generator available. |
| 8 | Required Equipment and Fixtures | Generator (NOT IMPLEMENTED). |
| 9 | Input Conditions | Complete YAML. |
| 10 | Execution Procedure | 1. Run the generator report. 2. Check: (1) each conflict has an evidence source; (2) each conflict has status OPEN or OWNER-APPROVED; (3) each affected value references its conflict record; (4) no unresolved interpretation is presented as an approved value, and PROVISIONAL values are flagged; (5) EXTERNAL values are flagged until verified. 3. Check that every standard value carries a 2023 clause, table or figure reference. |
| 11 | Expected Results | All five conditions and the reference check are satisfied. |
| 12 | Pass/Fail Criteria | Pass if all conditions hold [OWNER D-10, ODS-03; DOC STD-2]. Fail otherwise. A pass does not resolve any conflict, does not make an interpretation normative and does not satisfy STD-3. |
| 13 | Evidence to Collect | Generator report. |
| 14 | External Dependencies | EXTERNAL values are counted, not verified (SAE J1939, GB/T 18487.1). |
| 15 | Known Limitations | YAML NOT IMPLEMENTED. STD-3 is not covered (OPEN / UNVERIFIED). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-17 — Verification Level MUST

Baseline Covers: COM-6, COM-8, STD-4, STD-5. Gating tests: VT-AC17-01, VT-AC17-02, VT-AC17-03, VT-AC17-04. Non-gating tests: none. STD-5 forward compatibility (SHOULD) has no test; see §7.

#### VT-AC17-01

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC17-01 |
| 2 | Related AC ID | AC-17 |
| 3 | Covered Requirement IDs | STD-4, COM-6, COM-8 |
| 4 | Test Objective | Verify the encoding of every mandatory message: low byte first, resolution and offset, signed charging current, specified length, undefined bits and unsent optional parameters set to 1. |
| 5 | Verification Method | Host unit tests of the codec against 01.2_PROTOCOL_TABLES.md. |
| 6 | Test Level / Environment | Gating (MUST). E1 (Host unit tests / CTest); E7 (External-standard-dependent). Infrastructure — E1: CTest host infrastructure EXISTS (L04/L05). E7: referenced external document NOT AVAILABLE in the project source set. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: protocol/app modules under test; the referenced external document, verified and registered. |
| 7 | Preconditions | Codec built for host. |
| 8 | Required Equipment and Fixtures | Linux host with CTest (exists). |
| 9 | Input Conditions | Reference field values for every mandatory message, with optional parameters unset. |
| 10 | Execution Procedure | Encode; compare byte by byte with independently computed reference frames. |
| 11 | Expected Results | Byte-exact match. |
| 12 | Pass/Fail Criteria | Pass if every frame matches [STD §5.3, §5.6, §8.7, §8.8, Tables 8–29]. Fail otherwise. Bit-field positions depend on the bit numbering within bytes [EXTERNAL U-P2]; bit-field cases are INCONCLUSIVE until U-P2 is resolved. |
| 13 | Evidence to Collect | Unit-test report with reference frames. |
| 14 | External Dependencies | SAE J1939 bit numbering [EXTERNAL] (U-P2). |
| 15 | Known Limitations | Reference frames must be computed independently of the codec tables to avoid a shared misreading. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC17-02

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC17-02 |
| 2 | Related AC ID | AC-17 |
| 3 | Covered Requirement IDs | STD-4, STD-5 |
| 4 | Test Objective | Verify on captured hardware traffic that mandatory messages decode to the expected values and that CHM and BRM carry their version bytes. |
| 5 | Verification Method | Capture decode against reference values. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Hardware session as VT-AC01-01. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | One complete session. |
| 10 | Execution Procedure | 1. Decode each mandatory message. 2. Compare with the configured values. 3. Check CHM bytes 1–3 and BRM bytes 1–3. |
| 11 | Expected Results | Decoded values equal configured values; CHM and BRM version bytes as printed in Tables 8 and 11. |
| 12 | Pass/Fail Criteria | Pass if all values match and the version bytes equal the printed values (byte1 = 01H, byte2 = 01H, byte3 = 00H) [STD Tables 8, 11; OQ-4 noted]. Fail otherwise. |
| 13 | Evidence to Collect | Capture; decode comparison. |
| 14 | External Dependencies | U-P2 [EXTERNAL] for bit fields. |
| 15 | Known Limitations | OQ-4: identical bytes for V1.1 and V1.10; tested as printed. STD-5 forward compatibility is not tested (§7). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC17-03

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC17-03 |
| 2 | Related AC ID | AC-17 |
| 3 | Covered Requirement IDs | COM-8 |
| 4 | Test Objective | Verify that received not-credible values are ignored and received unspecified messages are not processed. |
| 5 | Verification Method | SIL injection. |
| 6 | Test Level / Environment | Gating (MUST). E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | C10. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | (a) BSM with a status field = 10 (not credible). (b) A frame with an identifier not defined by GB/T 27930-2023. |
| 10 | Execution Procedure | Inject each; record state, outputs and counters. |
| 11 | Expected Results | (a) the value is ignored and triggers no stop. (b) no state or output change; counted. |
| 12 | Pass/Fail Criteria | Pass if (a) and (b) cause no state or output change [STD §8.9; §5.5 with OWNER D-01]. Fail otherwise. |
| 13 | Evidence to Collect | SIL log; counters. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC17-04

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC17-04 |
| 2 | Related AC ID | AC-17 |
| 3 | Covered Requirement IDs | COM-8 |
| 4 | Test Objective | Verify that no message unspecified by GB/T 27930-2023 appears on the network. |
| 5 | Verification Method | Capture review. |
| 6 | Test Level / Environment | Gating (MUST). E4 (Physical CAN hardware); E5 (STM32 target boards). Infrastructure — E4: USB-CAN adapter: model and availability UNCONFIRMED; capture/decode tooling NOT IMPLEMENTED. E5: STM32G474RE and STM32F103ZE (named in Spec v2.0): board availability UNCONFIRMED; STM32 target ports NOT IMPLEMENTED (no ports/stm32). Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: confirmed USB-CAN adapter, capture and decode tooling, two physical nodes; available boards, G474 FDCAN and F103 bxCAN ports, flashing/debug set-up. |
| 7 | Preconditions | Hardware sessions including stop and error paths. |
| 8 | Required Equipment and Fixtures | STM32G474RE charger node and STM32F103ZE vehicle node (board availability UNCONFIRMED; target ports NOT IMPLEMENTED); USB-CAN adapter with capture and decode tooling (model and availability UNCONFIRMED). |
| 9 | Input Conditions | Complete sessions. |
| 10 | Execution Procedure | List every identifier observed; classify it against the 22 specified messages and the transport frames. |
| 11 | Expected Results | Only specified messages (and transport frames, see limitation). |
| 12 | Pass/Fail Criteria | Pass if no unspecified message is observed [STD §5.5 with OWNER D-01]. Transport-frame classification is OPEN (RQ-09): if transport frames are observed, the verdict for them is INCONCLUSIVE until RQ-09 is decided. Fail on any other unspecified message. |
| 13 | Evidence to Collect | Capture; identifier classification. |
| 14 | External Dependencies | SAE J1939-21 [EXTERNAL]. |
| 15 | Known Limitations | RQ-09 OPEN. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

### AC-18 — Verification Level SHOULD

Baseline Covers: ERR-2, PROT-10, PROT-12. Gating tests: none (SHOULD-only row). Non-gating tests: VT-AC18-S01, VT-AC18-S02, VT-AC18-S03. The CTS aspect of AC-18 has no test while OQ-6 is OPEN. The PROT-10 MUST part is tested in AC-8 (VT-AC08-07).

#### VT-AC18-S01 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC18-S01 |
| 2 | Related AC ID | AC-18 |
| 3 | Covered Requirement IDs | PROT-10 |
| 4 | Test Objective | Check the full pause/resume behaviour. |
| 5 | Verification Method | SIL scenario (non-gating SHOULD). |
| 6 | Test Level / Environment | Non-gating. E2 (Linux SIL / SocketCAN / vcan); E7 (External-standard-dependent). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E7: referenced external document NOT AVAILABLE in the project source set. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; the referenced external document, verified and registered. |
| 7 | Preconditions | C10 with output. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | BSM SPN3096 = 00 (SPN3090–3095 all 00), later SPN3096 = 01. |
| 10 | Execution Procedure | 1. Pause. 2. Record CCS SPN3929 and the output. 3. Resume. 4. Record. |
| 11 | Expected Results | Pause then resume; CCS SPN3929 = 00 during the pause. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check [STD §11.3.4; DOC Table 19 note]: result recorded. Pause/resume conditions per GB/T 18487.1-2023 B.4.5 [EXTERNAL]; behaviour beyond 10 min OPEN (OQ-S6). |
| 13 | Evidence to Collect | SIL log. |
| 14 | External Dependencies | GB/T 18487.1-2023 B.4.5 [EXTERNAL]. |
| 15 | Known Limitations | The PROT-10 MUST inhibit is to be verified by VT-AC08-07. CTS is not tested: OQ-6 OPEN. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC18-S02 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC18-S02 |
| 2 | Related AC ID | AC-18 |
| 3 | Covered Requirement IDs | PROT-12 |
| 4 | Test Objective | Check the vehicle-side older-version path: CRM 0x00 received without CHM leads directly to identification (BRM). |
| 5 | Verification Method | SIL scenario (non-gating SHOULD). |
| 6 | Test Level / Environment | Non-gating. E2 (Linux SIL / SocketCAN / vcan). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation. |
| 7 | Preconditions | Vehicle in V02. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | CRM 0x00 without any preceding CHM. |
| 10 | Execution Procedure | Inject; record the vehicle response. |
| 11 | Expected Results | The vehicle sends BRM. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check [STD Figure A.1; OWNER PROT-12 vehicle SHOULD]: result recorded. |
| 13 | Evidence to Collect | SIL log. |
| 14 | External Dependencies | NONE. |
| 15 | Known Limitations | NONE. |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

#### VT-AC18-S03 (non-gating)

| # | Field | Content |
|---|---|---|
| 1 | Test Case ID | VT-AC18-S03 |
| 2 | Related AC ID | AC-18 |
| 3 | Covered Requirement IDs | ERR-2 |
| 4 | Test Objective | Check reconnection after a BCL timeout, including observation of TO-22, TO-23 and TO-24. |
| 5 | Verification Method | SIL scenario (non-gating SHOULD). |
| 6 | Test Level / Environment | Non-gating. E2 (Linux SIL / SocketCAN / vcan); E7 (External-standard-dependent). Infrastructure — E2: vcan and SocketCAN HAL integration EXIST (L05); SIL harness (tests/sil empty), simulated plant/backend, hal_tick_ms, hal_log and fault-injection seams NOT IMPLEMENTED. E7: referenced external document NOT AVAILABLE in the project source set. Implementation under test: NOT IMPLEMENTED (no protocol/app code). Executable today: NO. Before execution: SIL harness, simulated plant/backend, HAL tick/log/status seams, protocol/app implementation; the referenced external document, verified and registered. |
| 7 | Preconditions | C10; reconnection enabled; vehicle configured as supporting reconnection. |
| 8 | Required Equipment and Fixtures | Linux host with vcan (exists); SIL harness, simulated plant/backend and fault-injection seams (NOT IMPLEMENTED). |
| 9 | Input Conditions | BCL withheld until TO-14 expires; handling completed; then normal traffic. |
| 10 | Execution Procedure | 1. Trigger TO-14. 2. Complete handling. 3. Record the CRM re-send, BRM and the reconnection count. 4. Separately withhold BRM after reconnection (TO-22) and CRM at the vehicle (TO-23, TO-24). |
| 11 | Expected Results | The charger re-sends CRM and the session resumes from identification; the timeouts are observed when the messages are withheld. |
| 12 | Pass/Fail Criteria | Non-gating SHOULD check [STD Figure A.2 note b; DOC ERR-2]: results recorded. Reconnection conditions per GB/T 18487.1-2023 B.4.7.3/B.4.7.4 [EXTERNAL]; BSM-timeout reconnection is not tested (OQ-S1 OPEN). |
| 13 | Evidence to Collect | SIL log. |
| 14 | External Dependencies | GB/T 18487.1-2023 B.4.7.3, B.4.7.4 [EXTERNAL]. |
| 15 | Known Limitations | Reconnection counter scope OPEN (RQ-06). |

Definition Status: DEFINED. Execution Status: NOT_EXECUTED.

## §6 Timeout Verification Matrix

### 6.1 Matrix (TO-01 to TO-25)

Durations are those of `docs/phase01/01.2_TIMEOUT_TABLE.md`; none is changed or invented. The safety-critical T±1 ms set is TO-14, TO-15 and TO-16 (D-05, ODS-01). That is a project-level verification classification, not one prescribed by GB/T 27930-2023, and exclusion from it is not a statement that a timeout has no safety relevance. TO-01 is a separate compatibility test without a ±1 ms acceptance requirement.

| Timeout | Node / state | Awaited message or event | Duration and source | Applicable AC | Test Case IDs | Boundary or observation strategy | Pass/fail basis | Environment | OPEN / external limitations |
|---|---|---|---|---|---|---|---|---|---|
| TO-01 | Charger / C02 | BHM | 10 s [STD Figure A.1 p.47; note c] | AC-5 | VT-AC05-05 | Continuation observed after 10 s from the first CHM; no boundary tolerance | No CEM, no timeout response, CRM 0x00 follows | E2 | Continuation-time tolerance OPEN; lower-version self-check EXTERNAL |
| TO-02 | Vehicle / V03 | CRM 0x00 | 30 s [STD Figure A.1 p.48] | AC-5, AC-1 | VT-AC05-08, VT-AC05-S01, VT-AC01-03 | Mapping inspection; wait identity and expiry behaviour (VT-AC01-03 case a1); expiry observation | Mapping matches (gating); TO-02 identity and BEM SPN3901 = 01 on expiry (gating, VT-AC01-03; expiry time recorded, no tolerance); expiry recorded (non-gating) | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-03 | Charger / C04 | BRM | 5 s [STD Figure A.1 p.48] | AC-5 | VT-AC05-08, VT-AC05-S01, VT-AC05-07 | Mapping inspection; peer silence; expiry observation | Mapping matches; detection on peer silence (gating); expiry recorded (non-gating) | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-04 | Vehicle / V05 | CRM 0xAA | 5 s [STD Figure A.1 p.48] | AC-5, AC-1 | VT-AC05-08, VT-AC05-S01, VT-AC05-07, VT-AC01-03 | Mapping; wait identity and expiry behaviour (VT-AC01-03 case a2); peer silence; expiry observation | Mapping matches; detection on peer silence (gating); TO-04 identity and BEM SPN3902 = 01 on expiry (gating, VT-AC01-03; expiry time recorded, no tolerance); expiry recorded (non-gating) | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-05 | Charger / C05 | BCP | 5 s [STD Figure A.1 p.48] | AC-5 | VT-AC05-08, VT-AC05-S01 | Mapping inspection; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-06 | Vehicle / V06 | CML | 5 s [STD Figure A.1 p.49] | AC-5 | VT-AC05-08, VT-AC05-S01 | Mapping inspection; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN; combined field SPN3903 (OQ-7) |
| TO-07 | Charger / C07 | BRO (any value) | 5 s [STD Figure A.1 p.49; §11.2.4] | AC-5, AC-1 | VT-AC05-08, VT-AC05-S01, VT-AC01-03 | Mapping; wait identity; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-08 | Charger / C07b | BRO 0xAA | 60 s [STD Figure A.1 p.49; §11.2.4] | AC-5, AC-1 | VT-AC05-08, VT-AC05-S01, VT-AC01-03 | Mapping; wait identity; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-09 | Charger / C08 | BRO (continued) | 5 s [STD Figure A.1 p.50] | AC-5 | VT-AC05-08, VT-AC05-S01 | Mapping inspection; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-10 | Vehicle / V09 | CRO (any value) | 5 s [STD Figure A.1 p.50; §11.2.5] | AC-5, AC-1 | VT-AC05-08, VT-AC05-S01, VT-AC01-03 | Mapping; wait identity; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-11 | Vehicle / V09 | CRO 0xAA | 60 s [STD Figure A.1 p.50; §11.2.5] | AC-5, AC-1 | VT-AC05-08, VT-AC05-S01, VT-AC01-03 | Mapping; wait identity; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-12 | Charger / C09 | First BCL | 1 s [STD Figure A.1 p.50] | AC-5 | VT-AC05-08, VT-AC05-S01 | Mapping inspection; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-13 | Vehicle / V10 | First CCS | 1 s [STD Figure A.1 p.51] | AC-5, AC-14 | VT-AC05-08, VT-AC05-04, VT-AC14-02, VT-AC05-S01 | Mapping; error response; gating fault test on the first-CCS wait (owner AC14-02 treatment); expiry observation | VT-AC14-02: timeout handling with BEM SPN3905 = 01 (gating, no timing tolerance) | E3, E2 | Outside the safety-critical set; covers the first-CCS wait only, not charging-phase CCS loss |
| TO-14 | Charger / C10 and C10P | BCL | 1 s [STD Figure A.1 p.51; §11.3.1] | AC-5, AC-11, AC-14 | VT-AC05-01, VT-AC11-02, VT-AC14-01, VT-AC05-04, VT-AC05-07 | Safety-critical T−1 ms / T+1 ms in C10 and C10P (§6.2) | No timeout at T−1 ms; timeout and CEM SPN3925 = 01 at T+1 ms | E1, E2 (E5 + E6 real-time confirmation: optional; not defined in B2) | INCONCLUSIVE if the time base cannot discriminate the two cases (§6.2) |
| TO-15 | Charger / C10 and C10P | BCS | 5 s [STD Figure A.1 p.51; §11.3.2] | AC-5, AC-11 | VT-AC05-02, VT-AC11-02 | Safety-critical T−1 ms / T+1 ms in C10 and C10P | No timeout at T−1 ms; timeout and CEM SPN3924 = 01 at T+1 ms | E1, E2 (E5 + E6 real-time confirmation: optional; not defined in B2) | As TO-14 |
| TO-16 | Charger / C10 and C10P | BSM (armed after the first BSM) | 5 s [STD Figure A.1 p.51; note h; §11.3.4] | AC-5, AC-8, AC-11 | VT-AC05-03, VT-AC11-02, VT-AC08-06 | Safety-critical T−1 ms / T+1 ms in C10 and C10P; not armed before the first BSM | No timeout at T−1 ms; timeout and CEM SPN3928 = 01 at T+1 ms; no expiry before the first BSM | E1, E2 (E5 + E6 real-time confirmation: optional; not defined in B2) | As TO-14; reconnection after expiry not asserted (OQ-S1) |
| TO-17 | Vehicle / V11 | CCS (charging phase) | 1 s [STD §11.3.3 text only] | AC-14 | VT-AC14-S01 | Observation only | No pass/fail (OPEN) | E2 | OPEN (ODS-01; OQ-S4; OQ-T3) |
| TO-18 | Charger / C11 | BST | 5 s [STD Figure A.1 p.52] | AC-5 | VT-AC05-08, VT-AC05-S01 | Mapping inspection; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-19 | Charger / C12 | BSD | 10 s [STD Figure A.1 p.52] | AC-5 | VT-AC05-08, VT-AC05-S01 | Mapping inspection; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-20 | Vehicle / V12 | CST | 5 s [STD Figure A.1 p.52] | AC-5 | VT-AC05-08, VT-AC05-S01, VT-AC05-07 | Mapping; peer silence (vehicle-initiated stop only); expiry observation | As TO-03 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-21 | Vehicle / V13 | CSD | 10 s [STD Figure A.1 p.52] | AC-5 | VT-AC05-08, VT-AC05-S01 | Mapping inspection; expiry observation | As TO-02 | E3, E2 | Outside the safety-critical set; expiry tolerance OPEN |
| TO-22 | Charger / C-RC (wait BRM) | BRM after reconnection | 5 s [STD Figure A.2 / A.3] | AC-5, AC-18 | VT-AC05-08, VT-AC18-S03 | Mapping inspection; observation in the reconnection scenario | Mapping matches (gating); observation recorded (non-gating) | E3, E2, E7 | Outside the safety-critical set; reconnection is SHOULD; conditions EXTERNAL (B.4.7.3 / B.4.7.4) |
| TO-23 | Vehicle / V-CE | CRM after CEM | 30 s [STD Figure A.2] | AC-5, AC-18 | VT-AC05-08, VT-AC18-S03 | As TO-22 | As TO-22 | E3, E2, E7 | As TO-22 |
| TO-24 | Vehicle / V-RC | CRM after BEM | 30 s [STD Figure A.3] | AC-5, AC-18 | VT-AC05-08, VT-AC18-S03 | As TO-22 | As TO-22 | E3, E2, E7 | As TO-22 |
| TO-25 | Charger and vehicle / C-TO, C-BE, V-TO, V-CE | Completion of communication-timeout handling | EXTERNAL — GB/T 18487.1-2023 B.3.7.3 (no value in GB/T 27930-2023) | AC-5 | VT-AC05-06, VT-AC05-08 | Entry/exit path with a configurable deadline parameter | Path behaviour only; no duration pass/fail | E2, E7 | Value and start condition OPEN (U-T1); the 1 s placeholder is a [REC] harness parameter, not a standard limit |

### 6.2 Timing evidence strategy for TO-14, TO-15 and TO-16

- **Clock source.** E1 and time-controlled E2: a test-controlled virtual millisecond tick delivered through the protocol tick entry point (DES-6). Real-time E2: the Linux port millisecond tick (hal_tick_ms, NOT IMPLEMENTED) and kernel receive timestamps on an independent vcan socket. E5 + E6 (optional real-time confirmation; not defined in B2): the target tick, plus logic-analyzer time shared by GPIO markers and the decoded CAN signal.
- **Timestamp resolution.** Virtual time: one tick = 1 ms [DOC DES-5]. Linux kernel socket timestamps: finer than 1 ms but subject to scheduling (below). Logic analyzer: instrument-dependent (UNCONFIRMED).
- **Timestamp origin.** t0 is the reception of the last awaited frame by the device under test (each timeout is defined from the last reception, 01.2_TIMEOUT_TABLE.md). In virtual time t0 is the tick at which the harness delivers the frame. On hardware t0 is the end of the frame on the bus, with the device reception event marked by a GPIO.
- **Frame time vs software event time.** The frame on the bus precedes the software reception event by the frame duration (about 0.52–0.64 ms for an 8-byte frame at 250 kbit/s, [DOC Spec v2.0 §9]) plus interrupt and queue-drain delay (the main loop drains the RX queue every 1 ms, [DOC Spec v2.0 §8]). These are reported as uncertainty components, not as tolerances.
- **Scheduler and processing latency.** Linux SIL is not real-time; scheduling jitter is uncontrolled and is recorded per run. Virtual-time tests are free of it by construction. Target timing excludes host scheduling but includes the 1 ms drain loop and tick quantization.
- **Measurement uncertainty.** Every boundary run records the combined uncertainty of its time base (tick quantization, drain interval, timestamp resolution and, where applicable, scheduling jitter). This document introduces no numerical accuracy requirement.
- **SIL vs hardware timing.** Virtual-time results verify the logic boundary exactly in tick units; they are not evidence of real-time behaviour. Real-time confirmation would require E5 with E6; it is optional and not defined in B2 — no real-time measurement procedure or equipment configuration is defined in this document.
- **Discrimination rule.** A boundary verdict (no timeout at T − 1 ms, timeout at T + 1 ms) is valid only if the documented combined uncertainty of the run allows the two cases to be distinguished. Otherwise the boundary result is NOT VERIFIABLE / INCONCLUSIVE pending a suitable measurement set-up. A software delay alone is not independent timing evidence.
- **Required evidence.** Harness log with t0 and the evaluation instants (virtual or measured); state and transmitted-frame logs; for an optional real-time confirmation (not defined in B2), logic-analyzer traces with the GPIO reception and timeout markers and the CAN signal on one time base; the uncertainty statement of the run.

## §7 Requirement-to-Test Traceability

Dispositions: TEST-DEFINED (at least one gating test); NON-GATING-TEST-DEFINED (non-gating tests only); OPEN / NO TEST; EXTERNAL DEPENDENCY (added where a gating verdict depends in part on an external value); NOT APPLICABLE (none used). A test definition is not test execution. AC columns use the baseline v0.3.0 mapping.

| Requirement | Level | AC (baseline) | Gating tests | Non-gating tests | Disposition | Notes |
|---|---|---|---|---|---|---|
| STD-1 | MUST | AC-1, AC-10 | VT-AC01-01, VT-AC10-03 | — | TEST-DEFINED | — |
| STD-2 | MUST | AC-16 | VT-AC16-01 | — | TEST-DEFINED | — |
| STD-3 | MUST | none (ODS-03) | — | — | OPEN / NO TEST | No test by decision ODS-03; OPEN / UNVERIFIED; AC-16 does not cover it. |
| STD-4 | MUST | AC-17 | VT-AC17-01, VT-AC17-02 | — | TEST-DEFINED; EXTERNAL DEPENDENCY | Bit numbering within bytes EXTERNAL (U-P2). |
| STD-5 | MUST/SHOULD | AC-17 | VT-AC17-02 | — | TEST-DEFINED | MUST part (version bytes): VT-AC17-02. SHOULD part (forward compatibility, §5.7 "宜"): NO TEST / OPEN — no source-supported criterion exists. |
| COM-1 | MUST | AC-2 | VT-AC02-01, VT-AC02-03 | — | TEST-DEFINED | — |
| COM-2 | MUST | AC-2 | VT-AC02-02 | — | TEST-DEFINED | — |
| COM-3 | MUST | AC-2 | VT-AC02-02 | — | TEST-DEFINED; EXTERNAL DEPENDENCY | Identifier bit assignment per SAE J1939-21 EXTERNAL (U-P1). |
| COM-4 | MUST | AC-3 | VT-AC03-01 | — | TEST-DEFINED | Interval metric OPEN (NF-03). |
| COM-5 | MUST/SHOULD | AC-12, AC-16 | VT-AC12-03, VT-AC16-01 | VT-AC12-S01 | TEST-DEFINED | — |
| COM-6 | MUST | AC-7, AC-17 | VT-AC07-10, VT-AC17-01 | — | TEST-DEFINED | — |
| COM-7 | MUST | AC-12 | VT-AC12-03 | — | TEST-DEFINED | — |
| COM-8 | MUST | AC-17 | VT-AC17-01, VT-AC17-03, VT-AC17-04 | — | TEST-DEFINED | Transport-frame classification OPEN (RQ-09). |
| PROT-1 | MUST | AC-1 | VT-AC01-01, VT-AC01-02 | — | TEST-DEFINED | — |
| PROT-2 | MUST | AC-1, AC-11 | VT-AC01-01, VT-AC11-06 | — | TEST-DEFINED | — |
| PROT-3 | MUST | AC-7, AC-8 | VT-AC07-09, VT-AC08-01 | — | TEST-DEFINED | — |
| PROT-4 | MUST | AC-7 | VT-AC07-07 | — | TEST-DEFINED | — |
| PROT-5 | MUST | AC-8 | VT-AC08-01, VT-AC08-02, VT-AC08-08 | — | TEST-DEFINED | — |
| PROT-6 | MUST | AC-1 | VT-AC01-03 | — | TEST-DEFINED | — |
| PROT-7 | MUST | AC-7 | VT-AC07-01, VT-AC07-02, VT-AC07-03 | — | TEST-DEFINED | Zero-current case excluded (OQ-12(a) OPEN). |
| PROT-8 | MUST/SHOULD | AC-8 | VT-AC08-03, VT-AC08-04 | VT-AC08-S01 | TEST-DEFINED | SPN2822 unit OPEN (OQ-3); SHOULD part non-gating. |
| PROT-9 | MUST | AC-8 | VT-AC08-05, VT-AC08-06 | — | TEST-DEFINED | Pause case not asserted (OQ-S11). |
| PROT-10 | MUST/SHOULD | AC-8, AC-18 | VT-AC08-07 | VT-AC18-S01 | TEST-DEFINED | MUST part: functional inhibit, latency recorded, no numeric limit (D-08). CTS OPEN (OQ-6). |
| PROT-11 | MUST/STRETCH | AC-1 | VT-AC01-04 | — | TEST-DEFINED | — |
| PROT-12 | MUST/SHOULD | AC-5, AC-18 | VT-AC05-05 | VT-AC18-S02 | TEST-DEFINED | Charger MUST via the TO-01 compatibility test; vehicle SHOULD via VT-AC18-S02. |
| TP-1 | MUST | AC-4 | VT-AC04-01, VT-AC04-02 | — | TEST-DEFINED; EXTERNAL DEPENDENCY | SAE J1939-21 values EXTERNAL. |
| TP-2 | MUST | AC-3, AC-4 | VT-AC03-02, VT-AC04-01 | — | TEST-DEFINED | DT-spacing tolerance OPEN. |
| TP-3 | MUST | AC-4, AC-16 | VT-AC04-02, VT-AC16-01 | — | TEST-DEFINED; EXTERNAL DEPENDENCY | SAE J1939-21 values EXTERNAL. |
| TP-4 | MUST | AC-4 | VT-AC04-04 | — | TEST-DEFINED | — |
| TP-5 | MUST/SHOULD | AC-4 | VT-AC04-03 | VT-AC04-S01 | TEST-DEFINED | — |
| TP-6 | MUST | AC-5, AC-14 | VT-AC14-05 | — | TEST-DEFINED | In the AC-5 Covers; tested in AC-14. |
| TP-7 | MUST | AC-4 | VT-AC04-05 | — | TEST-DEFINED | — |
| ERR-1 | MUST | AC-5 | VT-AC05-01, VT-AC05-02, VT-AC05-03, VT-AC05-08 | — | TEST-DEFINED | — |
| ERR-2 | MUST/SHOULD | AC-5, AC-18 | VT-AC05-04, VT-AC05-06 | VT-AC18-S03 | TEST-DEFINED; EXTERNAL DEPENDENCY | TO-25 duration EXTERNAL; reconnection SHOULD (VT-AC18-S03). |
| ERR-3 | MUST | AC-7 | VT-AC07-08 | — | TEST-DEFINED | — |
| ERR-4 | MUST | AC-5, AC-14 | VT-AC05-04, VT-AC14-09 | — | TEST-DEFINED | Physical-failure class covered only at the abstract software classification boundary (VT-AC14-09); actual physical-layer fault generation is SHOULD (PROJECT_SCOPE.md §5.2). Interval part of VT-AC05-04 INCONCLUSIVE until NF-03. |
| ERR-5 | MUST | AC-5, AC-9a | VT-AC05-06, VT-AC09A-01 | — | TEST-DEFINED; EXTERNAL DEPENDENCY | TO-25 value EXTERNAL. |
| ERR-6 | MUST | AC-7 | VT-AC07-06 | — | TEST-DEFINED | SPN3521/SPN3511 value not asserted (RQ-08). |
| CAN-1 | MUST | AC-6, AC-9a | VT-AC06-01, VT-AC06-02, VT-AC09A-01, VT-AC09A-03 | — | TEST-DEFINED | TEC/REC evidence sufficiency OPEN (VG-07). |
| CAN-2 | MUST | AC-9a | VT-AC09A-02 | — | TEST-DEFINED | — |
| CAN-3 | SHOULD | AC-9b | — | VT-AC09B-S01 | NON-GATING-TEST-DEFINED | SHOULD requirement; non-gating hardware test. |
| CAN-4 | MUST | AC-9a | VT-AC09A-03 | — | TEST-DEFINED | Not executable until the RQ-01 events are defined (PHASE 02). |
| CAN-5 | MUST | AC-5, AC-9c, AC-9d | VT-AC05-07 | VT-AC09C-S01, VT-AC09D-S01 | TEST-DEFINED | Steps without a wait OPEN (RQ-05); AC-9c/9d supplementary. |
| SAF-1 | MUST | AC-6 | VT-AC06-01, VT-AC06-02 | — | TEST-DEFINED | — |
| SAF-2 | MUST | AC-7 | VT-AC07-04 | — | TEST-DEFINED | SPN2817 case INCONCLUSIVE until OQ-1. |
| SAF-3 | MUST | AC-7 | VT-AC07-05 | — | TEST-DEFINED | Stale case INCONCLUSIVE (RQ-02). |
| SAF-4 | MUST | AC-15 | VT-AC15-01, VT-AC15-02 | — | TEST-DEFINED | — |
| SAF-5 | MUST | AC-7 | VT-AC07-07 | — | TEST-DEFINED | CST reason not asserted (RQ-03). |
| DES-1 | MUST | AC-10 | VT-AC10-02 | — | TEST-DEFINED | — |
| DES-2 | MUST | AC-10 | VT-AC10-01, VT-AC10-02 | — | TEST-DEFINED | — |
| DES-3 | MUST | AC-11, AC-12 | VT-AC11-05, VT-AC12-02 | — | TEST-DEFINED | — |
| DES-4 | MUST | AC-12 | VT-AC12-01 | — | TEST-DEFINED | — |
| DES-5 | MUST | AC-11 | VT-AC11-03 | — | TEST-DEFINED | — |
| DES-6 | MUST | AC-11 | VT-AC11-04 | — | TEST-DEFINED | — |
| TST-1 | MUST | AC-11 | VT-AC11-01, VT-AC11-02 | — | TEST-DEFINED | — |
| TST-2 | MUST/SHOULD | AC-5, AC-8 | VT-AC05-01, VT-AC05-02, VT-AC05-03, VT-AC08-01, VT-AC08-02, VT-AC08-03, VT-AC08-04, VT-AC08-05 | VT-AC05-S01 | TEST-DEFINED | — |
| TST-3 | MUST/SHOULD/STRETCH | AC-14 | VT-AC14-01, VT-AC14-02, VT-AC14-03, VT-AC14-04, VT-AC14-05, VT-AC14-06, VT-AC14-07, VT-AC14-08 | VT-AC14-S01, VT-AC14-S02 | TEST-DEFINED | — |
| TST-4 | MUST | AC-6, AC-13 | VT-AC06-03, VT-AC13-02 | — | TEST-DEFINED | — |
| TST-5 | SHOULD/STRETCH | AC-13 | — | VT-AC13-S01 | NON-GATING-TEST-DEFINED | SHOULD/STRETCH requirement. |
| TST-6 | SHOULD | AC-11 | — | VT-AC11-S01 | NON-GATING-TEST-DEFINED | SHOULD requirement. |
| TST-7 | MUST | AC-13 | VT-AC13-01 | — | TEST-DEFINED | — |

Coverage counts (computed from the table):

- TEST-DEFINED: 57
- NON-GATING-TEST-DEFINED: 3
- OPEN / NO TEST: 1
- of which TEST-DEFINED with an EXTERNAL DEPENDENCY: 6
- of which TEST-DEFINED with at least one limited sub-aspect recorded in the Notes column (OPEN, INCONCLUSIVE, NOT ASSERTED or EXTERNAL): 22 — STD-4, STD-5, COM-3, COM-4, COM-8, PROT-7, PROT-8, PROT-9, PROT-10, TP-1, TP-2, TP-3, ERR-2, ERR-4, ERR-5, ERR-6, CAN-1, CAN-4, CAN-5, SAF-2, SAF-3, SAF-5. The 6 EXTERNAL DEPENDENCY requirements (TP-1, TP-3, COM-3, STD-4, ERR-2, ERR-5) are included in these 22; they are not counted twice. (v0.1.3: ERR-4 moved into this list from the test-level list below.)
- In addition, 12 further TEST-DEFINED requirements carry limitations recorded only in individual test definitions (fields 12, 14 or 15) and not in the Notes column: COM-6 (U-P2), PROT-1 (INCONCLUSIVE without a shared time base), PROT-3 (OQ-12(a); NF-03), PROT-4 (RQ-03), PROT-5 (OQ-S10; RQ-04; NF-03), TP-4 (U-P6 derived size), TP-7 (NF-03), ERR-1 (U-T2; OQ-S1; boundary INCONCLUSIVE rule), CAN-2 (RQ-01), SAF-1 (RQ-04; INCONCLUSIVE rule), TST-2 (U-T2; OQ-S1; OQ-3; OQ-S8; OQ-S10; OQ-S11; NF-03), TST-3 (RQ-04; RQ-08).
- TEST-DEFINED therefore means that at least one gating test is defined; it does not mean that every aspect of the requirement is covered. Of the 57 TEST-DEFINED requirements, 22 have a limitation recorded in the Notes column and 12 more have test-level limitations.
- Total requirements: 61

STD-5 is counted as TEST-DEFINED for its MUST part only; its SHOULD part has no test. The counts describe defined tests, not executed verification.

## §8 External Dependencies and OPEN Issues

| Item | Effect on tests | Tests affected |
|---|---|---|
| GB/T 18487.1-2023 B.3.7.3 (TO-25, U-T1) | Handling-deadline value and start condition unknown; path tested, duration not assessed | VT-AC05-06 |
| GB/T 18487.1-2023 B.4.3 and the lower-version self-check method | Self-check simulated | VT-AC01-01, VT-AC05-05 |
| GB/T 18487.1-2023 B.4.5 | Pause/resume conditions | VT-AC18-S01 |
| GB/T 18487.1-2023 B.4.7.3 / B.4.7.4 | Reconnection conditions | VT-AC18-S03 |
| GB/T 18487.1-2023 parameter-matching criteria (Figure A.1 note d) | Additional matching criteria not tested | VT-AC08-04 |
| SAE J1939-21 (identifier bits U-P1, transport U-P3, T1–T3, abort codes, bit numbering U-P2) | Transport-timer results and bit-field cases INCONCLUSIVE until verified | VT-AC02-01, VT-AC02-02, VT-AC04-01 to VT-AC04-04, VT-AC14-05, VT-AC14-06, VT-AC17-01, VT-AC17-02 |
| SAE J1939-11 / J1939-15 | Physical-layer values | VT-AC02-03 |
| STD-3 (ODS-03) | No test; OPEN / UNVERIFIED | — |
| OQ-12(a) / zero-current (ODC-01) | Zero-current demand not tested | VT-AC07-01, VT-AC07-02, VT-AC07-09 |
| VG-07 / CAN-1 TEC/REC (ODC-01) | Sufficiency of injected values OPEN | VT-AC09A-01, VT-AC09B-S01 |
| RQ-01 recovery events (PHASE 02) | Attempt start and complete events, interval and operator restart undefined | VT-AC09A-02, VT-AC09A-03 |
| RQ-02 staleness threshold | Stale-request case INCONCLUSIVE | VT-AC07-05 |
| RQ-03 CST reason (SAF-5) | Reason not asserted | VT-AC07-07 |
| RQ-04 "while charging" / "active charging phase" | RX-overflow trigger scope; D-04 steps | VT-AC06-01, VT-AC08-08, VT-AC14-08 |
| RQ-05 steps without an applicable wait | Not tested | VT-AC05-07 |
| RQ-06 reconnection counter scope | Recorded only | VT-AC18-S03 |
| RQ-08 stop-reason value (ERR-6) | Not asserted | VT-AC07-06, VT-AC14-04 |
| RQ-09 transport frames under §5.5 | Transport-frame verdict INCONCLUSIVE | VT-AC17-04 |
| TO-17 (ODS-01; OQ-S4; OQ-T3) | Observation only | VT-AC14-S01 |
| OQ-S1 BSM reconnection | Not asserted after TO-16 | VT-AC05-03, VT-AC18-S03 |
| OQ-S8, OQ-S10 | Response sequences not asserted | VT-AC08-03, VT-AC08-04, VT-AC08-08 |
| OQ-S11 stop handling during pause | Not asserted | VT-AC08-05 |
| OQ-S6 pause limits | Behaviour beyond 10 min not tested | VT-AC18-S01 |
| OQ-6 / GAP-07 CTS | No CTS test | AC-18 (none) |
| OQ-1 SPN2817 unit | SPN2817 clamp INCONCLUSIVE | VT-AC07-04 |
| OQ-3 SPN2822 unit | PROVISIONAL voltage interpretation | VT-AC08-03 |
| OQ-4 version bytes | Tested as printed | VT-AC17-02 |
| NF-03 COM-4 interval metric | Interval verdicts INCONCLUSIVE until approved | VT-AC03-01, VT-AC03-02, VT-AC04-05, VT-AC05-04, VT-AC08-01 |
| NF-12 Spec §30 measurable detail | Not adopted; tests use the baseline wording | all |
| DT-spacing tolerance (TP-2) | Spacing verdict INCONCLUSIVE | VT-AC04-01 |
| Expiry tolerance outside the safety-critical set; TO-01 continuation tolerance | Observation only; no acceptance tolerance | VT-AC05-S01, VT-AC05-05 |
| Equipment, target ports, SIL harness, YAML/generator/lint | Tests not executable until available (§4) | all |

## §9 B2 Exit Criteria (document definition)

1. All 21 AC rows have a §5 subsection.
2. Every MUST AC row has at least one gating test.
3. Every SHOULD-only AC row (AC-9b, AC-9c, AC-9d, AC-18) has non-gating tests only.
4. Mixed rows keep gating and `-S` tests separate.
5. All 61 requirements have a §7 disposition.
6. STD-3 is OPEN / NO TEST.
7. STD-5 has a truthful disposition (MUST part tested; SHOULD part NO TEST).
8. All 25 TO-IDs appear exactly once in §6.1.
9. TO-14, TO-15 and TO-16 have T−1 ms / T+1 ms definitions in C10 and C10P.
10. TO-17 is OPEN.
11. TO-25 is EXTERNAL.
12. TO-01 carries no ±1 ms acceptance requirement.
13. AC14-02 follows the owner treatment (TO-13 gating; charging-phase CCS loss non-gating).
14. Every test has all 15 fields, none empty.
15. Every test states its environment and execution readiness.
16. The timing evidence strategy is documented (§6.2).
17. No test is marked executed; all are DEFINED / NOT_EXECUTED.
18. Test IDs are unique, every test belongs to one AC row and covers only requirements in that row's baseline Covers.
19. OPEN and EXTERNAL items are preserved (§8).
20. No baseline, protocol or requirement value is changed.

These are document-definition criteria, not test-execution criteria.

## §10 Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-09 | Initial B2 verification test definition: 90 tests (76 gating, 14 non-gating) across the 21 AC rows; TO-01–TO-25 matrix; timing evidence strategy; traceability of the 61 requirements. All tests DEFINED / NOT_EXECUTED. |
| 0.1.1 | 2026-11-09 | Documentation correction per the B2 Final Owner Audit: (I1) §6.1 TO-02 and TO-09 — removed the unsupported reference to VT-AC01-03 (its definition names only TO-04, TO-07, TO-08, TO-10, TO-11), with the dependent AC-1 and "wait identity" entries; (I2) E5/E6 real-time confirmation for TO-14 to TO-16 qualified as optional and not defined in B2 (VT-AC05-01 to VT-AC05-03, §6.1, §6.2); (I3) §7 coverage qualification added: 21 requirements with a limitation recorded in the Notes column (including the 6 EXTERNAL DEPENDENCY requirements) and 13 further requirements with test-level limitations; (I4) the three phrases identified by the audit (VT-AC18-S01, VT-AC13-02, VT-AC13-01), plus the same wording found in VT-AC14-01 during the correction, reworded to prospective "is to be verified by". No test ID, test logic, gating classification, threshold, timeout value or requirement disposition changed. Status: DRAFT — PENDING PROJECT OWNER ACCEPTANCE. |
| 0.1.1 (acceptance) | 2026-11-09 | Formal Project Owner acceptance of version 0.1.1 recorded (status OWNER-ACCEPTED). Metadata only; no test content, traceability mapping, expected result or acceptance criterion changed. |
| 0.1.2 | 2026-11-09 | PHASE 01.4-B B3-B controlled revision: (F-03) VT-AC05-07 — V12 case limited to a vehicle-initiated stop (TO-20 starts at the first BST of a vehicle-initiated stop, TV26); §6.1 TO-20 row annotated. (F-04) VT-AC01-03 — CRM pair split into case a1 (TO-02, V03, TV08, BEM SPN3901) and case a2 (TO-04, V05, TV11, BEM SPN3902) with distinct inputs, expected expiry behaviour and evidence; expiry times recorded without tolerance; §6.1 TO-02 and TO-04 rows updated. Same test IDs and AC ownership; no new test; no timeout value, tolerance or MUST/SHOULD classification changed. All tests NOT_EXECUTED. Status: DRAFT — PENDING PROJECT OWNER RE-ACCEPTANCE (v0.1.1 remains the last owner-accepted version). |
| 0.1.3 | 2026-11-09 | PHASE 01.4-B F-01 controlled correction (VG-06 / ERR-4): VT-AC14-09 corrected to verify all four ERR-4 failure classes — application timeout (VT-AC14-01 scenario), transport failure (VT-AC14-05/06 scenarios), controller failure (VT-AC14-07 bus-off scenario) and physical failure (synthetic physical-layer fault indication at an abstract HAL/driver fault-reporting boundary) — with category-specific counters and log entries and no cross-class misclassification; frame/signal validation is no longer used as a class. Physical-layer limitation stated (no real physical fault detection in SIL; hardware robustness remains SHOULD). §7: ERR-4 note added; Notes-column limitation count 21 → 22, test-level list 13 → 12. Same test ID; no test added or removed (90); no MUST/SHOULD classification, timeout value or tolerance changed. All tests NOT_EXECUTED. Status: DRAFT — PENDING PROJECT OWNER RE-ACCEPTANCE (v0.1.1 remains the last owner-accepted version). |
| 0.1.3 (amendment) | 2026-11-09 | F-01 final consistency correction within the pending v0.1.3: VT-AC14-09 fields 7 and 9–13 state four failure classes with five independently applied stimuli (S1 VT-AC14-01; S2 VT-AC14-05; S3 VT-AC14-06; S4 VT-AC14-07; S5 synthetic physical-fault indication), state-specific preconditions (C10 for S1, S2, S4, S5; identification phase V05 / C04 for S3) and per-stimulus initialization and evaluation; §7 "included in these 21" corrected to 22. Same test ID; no test added or removed; no classification, threshold or timeout value changed. Status unchanged: pending Project Owner re-acceptance. |
| 0.1.4 | 2026-11-09 | F-01 revision-control finalization: the content corrected in v0.1.3 and its amendment (VT-AC14-09 with four failure classes and five independently applied stimuli; state-specific preconditions; §7 count 22 + 12) is issued as v0.1.4. Status: DRAFT — PENDING PROJECT OWNER RE-ACCEPTANCE (v0.1.1 remains the last owner-accepted version, 2026-11-09). No test, test ID, classification, threshold or timeout value changed in this revision. |
| 0.1.5 | 2026-11-09 | Controlled correction: (F-01) VT-AC14-09 Field 15 stimulus references corrected to S5 (physical failure) and S4 (bus-off), consistent with Fields 7, 9 and 11; (F-02) §1 Requirements Baseline reference clarified: last formally owner-accepted baseline v0.3.0 (B1 accepted) and current pending baseline v0.3.3 (not accepted), as verified in `docs/REQUIREMENTS_BASELINE.md`. No test ID added or removed; no test logic, gating classification, threshold, timeout or acceptance criterion changed. All tests remain DEFINED / NOT_EXECUTED. Status: DRAFT — PENDING PROJECT OWNER RE-ACCEPTANCE (v0.1.1 remains the last owner-accepted version). |
| 0.1.6 | 2026-11-09 | Document-control cleanup: (F-03) §1 Requirements Baseline reference clarified — v0.3.0 is the last formally owner-accepted baseline version; v0.3.3 is the current pending revision, accepted for current PHASE 01 working use but not formally accepted as a new baseline version; final PHASE 01 freeze pending the PHASE 01 Exit Review. (F-04) Status wording at the end of the 0.1.2 and 0.1.3 history rows corrected from "OWNER-ACCEPTED — controlled revision pending Project Owner re-acceptance" to "DRAFT — PENDING PROJECT OWNER RE-ACCEPTANCE (v0.1.1 remains the last owner-accepted version)"; their change descriptions and dates are unchanged, and the v0.1.1 acceptance record is unchanged. No test, test ID, field, threshold, classification, timeout matrix or traceability mapping changed. All tests remain DEFINED / NOT_EXECUTED. Status: DRAFT — PENDING PROJECT OWNER RE-ACCEPTANCE (v0.1.1 remains the last owner-accepted version). |
| 0.1.6 (acceptance) | 2026-11-09 | Formal Project Owner acceptance of version 0.1.6 recorded (status OWNER-ACCEPTED). Metadata only; no test content, test IDs, traceability mapping, expected results or acceptance criteria changed. |
