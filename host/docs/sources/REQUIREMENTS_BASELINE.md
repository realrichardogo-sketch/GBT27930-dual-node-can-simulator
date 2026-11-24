# Requirements Baseline

## Document Control

| Field | Value |
|---|---|
| Project | GB/T 27930-2023 System A Charger-BMS Simulator |
| Project ID | P1 |
| Document ID | P1-DOC-003 |
| Version | 0.3.3 |
| Status | OWNER-ACCEPTED |
| Acceptance | v0.3.0 was formally accepted by the Project Owner for B1 on 2026-11-09. v0.3.3 was formally accepted by the Project Owner on 2026-11-09 as the PHASE 01 controlled requirements baseline; the intervening revisions v0.3.1 to v0.3.3 were reviewed and incorporated into the accepted v0.3.3 document. Final PHASE 01 exit approval and phase freeze remain pending the PHASE 01 Exit Review. This acceptance does not represent any OPEN or EXTERNAL item as verified or resolved. |
| Date | 2026-11-09 |
| Normative Authority | GB/T 27930-2023, System A |
| Project Specification | Project 1 Spec v2.0 — RECONSTRUCTED — DRAFT — NOT APPROVED (`docs/reference/source/PROJECT_1_SPEC_v2.0.txt`) |
| Standard Evidence | PHASE 01.2 documents `docs/phase01/01.2_*.md` (DRAFT) |
| Decision Inputs | `docs/phase01/01.4/OWNER_DECISION_RECORD.md` (D-05–D-10); `docs/phase01/OWNER_DECISION_SUPPLEMENT_01.md` (ODS-01–ODS-07); `docs/phase01/01.4/OWNER_DECISION_CLARIFICATION_01.md` (ODC-01); decision content approved 2026-11-09 |

## Revision History

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-04 | Initial draft register. |
| 0.2.0 | 2026-11-08 | PHASE 01.3 Step R2: reconciliation against Spec v2.0 (reconstructed) and PHASE 01.2 evidence per the approved R1 findings; owner decisions D-01–D-04 and the PROT-10 split incorporated (§6.4); source tags added; open questions, verification gaps and timeout identifier policy recorded (§6.3, §6.5, §6.6). No requirement ID added, removed or renumbered; no acceptance criterion changed. |
| 0.2.1 | 2026-11-08 | PHASE 01.3 R2 final documentation correction: (R2-01) PROT-10 CTS project transmission reclassified from SHOULD to OPEN pending owner confirmation (OQ-6 / GAP-07); the standard's optional-message definition and timing are preserved; full pause/resume remains SHOULD; AC-18 unchanged, its CTS aspect is conditional on OQ-6. (R2-02) PROT-10 reference to SAF-1 limited to the safe output state; the SAF-1 10 ms deadline is not inherited; applicability and response time remain OPEN (RQ-07). (R2-03) PROT-12 charger-side coverage records consolidated in §6.3 under VG-01 / GAP-06; the gap remains recorded. (R2-04) TST-6 wording corrected. (R2-05) Related cross-references aligned. No acceptance criterion, protocol parameter or timeout value modified; no requirement ID added, removed or renumbered; no other requirement priority changed. |
| 0.3.0 | 2026-11-09 | PHASE 01.4-B step B1 (controlled baseline revision): incorporated owner decisions D-05–D-10 and ODS-01–ODS-07 (§6.4). D-05/ODS-01: safety-critical T±1 ms timeout set TO-14, TO-15, TO-16 and TO-01/TO-17/TO-25 dispositions (§6.6). D-06: magnitude-based current comparison (COM-6, PROT-7); zero-current interpretation OPEN. D-07: recovery 1 s per attempt, at most 3 attempts, FAULT_LOCAL, operator restart (CAN-1); recovery events OPEN. D-08: PROT-10 functional output inhibit without a numeric limit, MUST coverage in AC-8; RQ-07 CLOSED. D-09/ODS-02: AC register keeps 18 IDs / 21 rows; Covers, Procedure and one Verification Level added to every row; owner-approved mixed MUST/SHOULD convention applied; GAP-05 CLOSED. D-10/ODS-03: AC-16 scope revised; STD-3 decoupled from AC-16 and kept OPEN/UNVERIFIED. ODS-05: B1 mapping changes and baseline-vs-Spec mapping differences recorded (§6.7). ODS-06: NF-01–NF-12 and sub-entries recorded (§6.5); VG dispositions added (§6.3). PROVISIONAL defined (§2.1). R2 note corrected. No requirement ID added, removed or renumbered; no requirement level changed; no AC ID or row added or renumbered; no protocol value, timing value or message definition changed. |
| 0.3.1 | 2026-11-09 | PHASE 01.4-B B3-B controlled corrections: (F-01) ODC-01 made traceable — Decision Inputs, §6.4 row, §6.5 zero-current and TEC/REC rows, ODS-06 limitation; (F-02) TO-01 wording in AC-5 procedure (c) and §6.6 aligned with B2 v0.1.1 (no ±1 ms acceptance tolerance currently imposed; continuation time recorded); (F-07) acceptance metadata: v0.3.0 accepted for B1, final PHASE 01 freeze pending the Exit Review. No requirement ID, level, AC, protocol value, timeout value or decision changed. Pending Project Owner review. |
| 0.3.2 | 2026-11-09 | PHASE 01.4-B F-01 controlled correction: §6.3 VG-06 finding and disposition updated for the corrected four-class ERR-4 verification definition (B2 v0.1.3, VT-AC14-09): software classification and reporting of application timeouts, transport failures, controller failures and physical failures, the last via a synthetic physical-layer fault indication at an abstract fault-reporting boundary; actual physical-layer fault generation and hardware robustness remain SHOULD (PROJECT_SCOPE.md §5.2). ERR-4 stays MUST; VG-06 stays tracked (not verified). No requirement ID, level, AC, protocol value or timeout value changed. Pending Project Owner review. |
| 0.3.2 (amendment) | 2026-11-09 | F-01 final consistency correction within the pending v0.3.2: §6.3 table heading "Disposition (v0.3.0)" corrected to "Current Disposition". No finding, disposition, requirement, AC or decision changed. Status unchanged: pending Project Owner review. |
| 0.3.3 | 2026-11-09 | F-01 revision-control finalization: the content corrected in v0.3.2 and its amendment (§6.3 VG-06 four-class disposition; §6.3 heading "Current Disposition") is issued as v0.3.3. Status: DRAFT — PENDING PROJECT OWNER REVIEW. No requirement, AC, finding, disposition, decision, protocol value or timeout value changed in this revision. |
| 0.3.3 (acceptance) | 2026-11-09 | Formal Project Owner acceptance of Requirements Baseline v0.3.3 following the PHASE 01.4-C joint B1/B2 consistency audit (CONDITIONAL PASS, no B1 acceptance blocker). C-1 and C-2 are carried as controlled, non-blocking Exit Review items. Metadata-only acceptance recording; no requirement, AC, priority, normative value, timeout, finding disposition or technical decision changed. Final PHASE 01 exit approval remains pending. |
| 0.3.3 (approval metadata alignment) | 2026-11-09 | Aligned §10 Approval with the previously recorded formal Owner Acceptance of v0.3.3. No new approval decision or technical change. Final PHASE 01 exit approval remains pending. |

---

# 1. Purpose

This document establishes the controlled requirements baseline
for Project 1.

It translates the requirements defined in Project 1 Spec v2.0
into a structured register for implementation, verification
and acceptance.

Every requirement retains its original Requirement ID.

No requirement may be silently removed, downgraded,
merged or reinterpreted.

The detailed implementation design will be established
during PHASE 02.

---

# 2. Baseline Management Rules

## 2.1 Requirement Types

| Type | Definition |
|---|---|
| Standard | Requirement originating from GB/T 27930-2023 |
| External | Requirement or parameter originating from a referenced external document |
| Project | Engineering decision or project-specific requirement |

Source tags used inside requirement text:

| Tag | Meaning | Type column |
|---|---|---|
| [STD] | Requirement verified against GB/T 27930-2023 (clause, table or figure cited) | Standard |
| [EXTERNAL] | Depends on another standard or document not yet verified (GB/T 18487.1-2023, SAE J1939-11/-15/-21) | External |
| [DOC] | Project documentation or engineering-defined value, including Spec v2.0 (reconstructed, not approved) | Project |
| [OWNER] | Explicit Project Owner decision recorded in §6.4 | Project |

A [DOC] or [OWNER] statement is never a claim about the standard.

Value status marker (consistent with the Spec v2.0 status vocabulary):

| Marker | Meaning |
|---|---|
| PROVISIONAL | An interim project interpretation or value, used where a conflict or dependency is unresolved. It must be explicitly marked, must not be presented as a verified standard requirement or an approved normative value, and must not support a compliance claim (D-10). |

## 2.2 Requirement Levels

| Level | Definition | Release Blocking |
|---|---|---|
| MUST | Mandatory project requirement | Yes |
| SHOULD | Planned enhancement | Normally no |
| STRETCH | Optional extension | No |

A SHOULD requirement must be reconsidered if it affects
protocol correctness or essential safety.

## 2.3 Source Authority

The requirement hierarchy is:

1. GB/T 27930-2023 System A.
2. Referenced external documents, where applicable.
3. Project Owner decisions recorded in §6.4 [OWNER].
4. Project 1 Spec v2.0 project-specific decisions [DOC]
   (reconstructed, DRAFT — not approved).
5. Approved project architecture and implementation decisions.

External values remain marked External until independently
verified against their referenced documents.

## 2.4 Requirement Status

The following status values will be used:

| Status | Meaning |
|---|---|
| NOT_STARTED | No implementation work completed |
| IN_PROGRESS | Implementation is ongoing |
| IMPLEMENTED | Implementation completed |
| VERIFIED | Required tests passed |
| ACCEPTED | Evidence reviewed and accepted |

All requirements start as NOT_STARTED.

Implementation alone does not constitute verification.

---

# 3. Requirement Register

## 3.1 Standard and Scope Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| STD-1 | Implement System A, chapters 5–11 and normative Annex A; use informative Annex B for start and stop conditions. Exclude System B, 50 kbit/s and adapter node. Where informative Annex B differs from normative Annex A, the Annex A path is the working implementation interpretation [OWNER, PHASE 01.2] (e.g. CRO stops on BCL per Figure A.1; Table B.1 states BCL and BCS); the discrepancy remains OPEN (OQ-S2). | Project / MUST | Spec STD-1; Figure A.1 p.50; Table B.1 p.56 | AC-1, AC-10 |
| STD-2 | Every standard-derived YAML value must carry a 2023 clause, table or figure reference. External values must retain External status until checked. | Project / MUST | Spec STD-2 | AC-16 |
| STD-3 | Record the three internal standard conflicts: BSM reconnection, BCP SPN2817 unit and CSD SPN3613 offset, including the selected interpretation. A selected interpretation requires Project Owner approval; until approved, each conflict remains OPEN (§6.2). Further source anomalies found in PHASE 01.2 (e.g. BCP SPN2822 unit, OQ-3) are tracked in §6.5. [OWNER ODS-03] STD-3 is a separate verification obligation from AC-16: passing AC-16 (conflict-record integrity) does not satisfy STD-3. STD-3 remains OPEN and UNVERIFIED until its own acceptance conditions can be demonstrated; at the PHASE 01 Exit Review the Project Owner determines whether it is carried into PHASE 02 as a controlled open item or blocks phase exit. No automatic closure is permitted. | Project / MUST | Spec STD-3; 01.2 OQ-1, OQ-2, OQ-S1 | None — OPEN / UNVERIFIED (ODS-03); not covered by AC-16 |
| STD-4 | Mandatory parameters carry real data. Optional parameters carry real data or all bits 1. Entirely optional messages may be omitted. | Standard / MUST | §8.7 | AC-17 |
| STD-5 | CHM uses V1.1 and BRM uses V1.10. Forward compatibility is maintained where practical. | Standard / MUST (version), SHOULD (compatibility) | §11.1.1, §11.1.4, §5.7 | AC-17 |

## 3.2 Communication Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| COM-1 | Use CAN 2.0B extended frames, 29-bit identifiers, 250 kbit/s and maximum 8-byte payloads. G474 FDCAN must operate in Classic CAN mode. [STD] Identifier bit assignment per SAE J1939-21:2006 [EXTERNAL] (§7.1); the charger–vehicle communication uses an independent CAN bus (§6). [DOC] No CAN FD frames are used. [DOC] Bench wiring: twisted pair with 120 Ω termination at each end (J1939-11 practice); physical-layer values per SAE J1939-11:2006 [EXTERNAL, unverified]. | Standard; External; Project / MUST | §5.1, §6, §7.1, Table 1; Spec COM-1 | AC-2 |
| COM-2 | Use fixed addresses: charger 0x56 and vehicle 0xF4. | Standard / MUST | §7.6, Table 2 | AC-2 |
| COM-3 | Use one identifier generator for priority, EDP, DP, PF, PS and SA fields. | Standard; Project / MUST | §7.3–7.4, Table 1 | AC-2 |
| COM-4 | Periodic messages follow Tables 3–7. Project timing tolerance is ±10%. | Standard; Project / MUST | Tables 3–7 | AC-3 |
| COM-5 | YAML is the single protocol source. Generate C tables as MUST. Generated documentation tables and CI staleness checks are SHOULD. Lint protocol constants outside generated files. | Project / MUST; SHOULD | Spec COM-5 | AC-12, AC-16 |
| COM-6 | Use low-byte-first encoding, correct resolution and offset, negative charging current on the wire, and non-negative current magnitude in the application in mA. [STD] On CAN, charging current is signed and negative in the charging direction, 0.1 A/bit with −400 A offset (§5.4, §5.6). [OWNER D-02] Application-internal current is a non-negative magnitude in mA; the Project 2 converter interface uses a non-negative current setpoint in mA. These are project-interface decisions, not standard requirements. [OWNER D-06] The application obtains a current magnitude by one conversion only: the standard decoding (physical value = resolution × raw + offset, §5.6, with 0.1 A/bit and −400 A offset) followed by expression of that physical value as a non-negative magnitude in mA (D-02). Values outside −400 A to 0 A are handled by ERR-6 before use. The conversion is not applied twice, and raw signed CAN current values are not compared directly in application logic. The CAN wire representation is unchanged. The meaning of a zero-current demand remains OPEN (OQ-12(a) sub-entry, §6.5). | Standard; Project / MUST | §5.3, §5.4, §5.6; D-02; D-06 | AC-7, AC-17 |
| COM-7 | Use GBT_MSG_CTS_TIME_SYNC for GB/T time synchronization and TP_CM_* identifiers for transport. A bare CTS identifier must fail lint. | Project / MUST | Spec COM-7 | AC-12 |
| COM-8 | Use specified message lengths, set undefined bits and unsent optional parameters to 1, ignore not-credible values and ignore undefined messages. [STD] Lengths and fill per §8.7–8.8; received not-credible values are ignored (§8.9). [STD, public-site provision applied per OWNER D-01] Messages not specified by GB/T 27930-2023 shall not appear on the charger–vehicle network, and a received unspecified message is not processed (§5.5). Whether SAE J1939-21 transport frames count as specified via §7.5 is a project interpretation (RQ-09). | Standard; Project / MUST | §5.5, §8.7–8.9; D-01 | AC-17 (transmit and receive; VG-11) |

## 3.3 Protocol Flow Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| PROT-1 | Follow the System A communication flow: connection, handshake, parameter configuration, charging and end. Simulate physical connection and auxiliary power. Send first CHM within 1 s of auxiliary power. | Standard / MUST | §9–10, Figure A.1, §11.1.1 | AC-1 |
| PROT-2 | Charger and vehicle must run independent state machines. | Project / MUST | Spec PROT-2 | AC-1, AC-11 (NF-10) |
| PROT-3 | Implement a configurable battery model. BCL demand reflects battery requirements and is not artificially limited by CML. | Standard; Project / MUST | §11.3.1 | AC-7, AC-8 |
| PROT-4 | CCS must report output feedback. Invalid or stale feedback must never be reported. | Project / MUST | SAF-5 | AC-7 |
| PROT-5 | Implement both stop procedures. BST and CST are sent every 10 ms and may start after CHM. Charger-first stop requires 5–10 BST frames before BSD. Vehicle-first stop terminates BST on CST and then sends BSD. CST contains exactly one stop reason. [STD] A node that receives the peer's stop message sends its own stop message (§11.3.8, §11.3.9; no phase restriction is stated). [OWNER D-04] When a valid peer stop message (BST at the charger, CST at the vehicle) is received outside the active charging phase, the receiving node must not initiate or continue non-zero charging output (charger: software output command; vehicle: charging current request) and remains in the safe no-output condition. The protocol transition and response sequence outside the charging phase remain OPEN (OQ-S10); "active charging phase" is defined in PHASE 02 (RQ-04). | Standard; Project / MUST | Figure A.1 notes k, l; Table B.1; §11.3.8–11.3.9; D-04 | AC-8 (including D-04; VG-14) |
| PROT-6 | CRM 0x00 and 0xAA waits, and BRO/CRO 0x00 and 0xAA waits, must be represented as separate wait rows. [STD] Figure A.1 draws the configuration waits as BRO (any value) and BRO 0xAA (charger), and CRO (any value) and CRO 0xAA (vehicle); no wait for a 0x00 value of BRO or CRO is drawn. Canonical rows: TO-02, TO-04 (CRM), TO-07, TO-08, TO-09 (BRO), TO-10, TO-11 (CRO). | Standard / MUST | Figure A.1 p.48–50; 01.2_TIMEOUT_TABLE.md | AC-1 |
| PROT-7 | Implement all CV and CC BCL rules, including voltage/current demand limits, CML minimum and maximum limits, and stop when voltage demand is below CML minimum. [STD] CV mode: output voltage meets the voltage demand and output current does not exceed the current demand. CC mode: output current meets the current demand and output voltage does not exceed the voltage demand. In CC mode a BCL voltage demand shall not exceed the CML maximum voltage; otherwise the charger uses its own maximum voltage as the voltage limit (behaviour in CV mode is not stated). Current demand greater than the CML maximum current: output at maximum capability; within range: output the requested current; less than the charger's minimum current: output at minimum capability. Voltage demand less than the charger's minimum voltage: send CST. [DOC, not approved] Spec v2.0 treats a zero current demand as below the minimum; this is not approved by D-02 and must not be assumed. [OWNER D-06] The current comparisons of these rules use non-negative magnitudes after the single conversion of COM-6; raw signed CAN values are not compared directly. LIMITATION: the meaning of a zero-current demand remains OPEN (OQ-12(a) sub-entry); PROT-7 verification excludes the zero-current case until it is resolved. The SPN2817 unit conflict (OQ-1) remains OPEN. | Standard; Project / MUST | §11.3.1 p.14; D-02; D-06 | AC-7 (zero-current case OPEN: VG-16) |
| PROT-8 | Implement charger-side and vehicle-side parameter matching. Charger checks BCP present voltage against CML range and rechecks the last BCP after BRO. Vehicle checks whether CML covers its rated battery voltage. Updated BCP sent at least twice is SHOULD. [STD] The charger judges a parameter mismatch when the battery present voltage sent by the vehicle is outside its output capability range, and then sends CML before CST (§10.2); it re-checks with the last BCP after receiving BRO (note g). [DOC] The present voltage is taken as BCP SPN2822 (unit conflict, OQ-3) and the capability range as the CML voltage range. [STD] The vehicle may judge from CML whether charging is possible (§10.2); [DOC] project rule: the CML range covers the rated battery voltage; on mismatch the vehicle sends BST (charging-parameter mismatch). [EXTERNAL] Further matching criteria per GB/T 18487.1-2023 or operator-defined (note d). Updated BCP: note f ("宜"). | Standard; External; Project / MUST; SHOULD | §10.2 p.8; Figure A.1 notes d, f, g | AC-8 |
| PROT-9 | Any abnormal SPN3090–3095 or BST causes charger stop through CST. BSM timeout starts only after the first BSM. [STD] §11.3.4 states no exception for a charging pause; PHASE 01.2 represents this during pause (TC54). The Figure A.1 drawing does not evaluate BST while paused; reconciliation remains OPEN (OQ-S11). | Standard / MUST | §11.3.4 p.15–16; Figure A.1 note h | AC-8 |
| PROT-10 | BSM pause and resume through SPN3096 and CCS SPN3929, maximum pause 10 min. CTS is sent and the vehicle decides whether to synchronize. **MUST [OWNER]:** when the charger receives a valid BSM with SPN3096 = 00 (charging prohibited), it inhibits charging output: the software output command enters the safe no-output condition. The reference to SAF-1 identifies only that state: output disabled and output command set to 0 A. [OWNER D-08] No numeric response-time limit applies to this trigger; the SAF-1 10 ms deadline is not inherited. Verification injects a valid BSM with SPN3096 = 00, observes the output-command transition to the safe no-output condition and records the observed latency as engineering evidence, not as a pass/fail threshold; any test-harness watchdog is a test parameter, not a product requirement (RQ-07 CLOSED). SAF-1 keeps its own 10 ms requirement for its own triggers. No hardware shutdown is required in Project 1; power-stage protection belongs to Project 2. **SHOULD (full pause/resume):** [STD] pause with SPN3090–3095 all 00 and SPN3096 = 00, resume with SPN3090–3095 all 00 and SPN3096 = 01; pause request to resume at most 10 min (§11.3.4). [DOC] CCS SPN3929 = 00 during pause (Table 19 note). [EXTERNAL] Pause and resume conditions per GB/T 18487.1-2023 B.4.5; consequence of exceeding 10 min OPEN (OQ-S6). **CTS — project transmission OPEN:** [STD] CTS is an optional message (SPN2823 optional; "可选" in Figure A.1 and Table B.1; §8.7); if sent, every 500 ms after BCP until BRO 0xAA; the vehicle decides whether to synchronize (§11.2.2, note e). Whether the charger actively sends CTS is not classified (no MUST, SHOULD or STRETCH) until the Project Owner confirms it (OQ-6 / GAP-07). The sentence "CTS is sent" above records the Spec v2.0 position [DOC] and is not an approved decision. | Standard; External; Project / MUST (SPN3096 = 00 output inhibit); SHOULD (full pause/resume); CTS project transmission: OPEN (OQ-6 / GAP-07) | §11.2.2, §11.3.3–11.3.4; Table 13; Table B.1; PROT-10 owner decision; D-08 | AC-8 (MUST: functional output inhibit, latency recorded; VG-13); AC-18 (SHOULD: pause/resume; CTS aspect conditional on OQ-6) |
| PROT-11 | Optional BMV, BMT and BSP absence is never an error and has no timeout. Sending these messages is STRETCH. BSP is limited to private sites. [STD] The charger applies no timeout to them and may refuse them through the transport protocol (§10.3). [STD, public-site profile per OWNER D-01] Without bilateral identity authentication, BSP is neither sent nor processed at public sites (Table 23 note); Project 1 therefore neither sends nor processes BSP, and the STRETCH sending item applies to BMV and BMT only. | Standard; Project / MUST (absence handling); STRETCH (sending) | §10.3, §11.3.7; D-01 | Unit tests; AC-1 |
| PROT-12 | Charger must support older-version vehicle handling when no BHM arrives within 10 s. Vehicle-side CRM 0x00 without CHM direct identification is SHOULD. [STD] Both are normative Figure A.1 paths; the MUST/SHOULD split is an [OWNER] scope decision. [STD] On TO-01 expiry the charger continues and performs its self-check per the compatible lower version; this is not an error and sets no CEM field (OQ-T4). [EXTERNAL] The lower-version self-check method is not defined in GB/T 27930-2023. | Standard; External / MUST (charger); SHOULD (vehicle) | Annex A Figure A.1 note c; Figure A.1 | AC-5 (charger: TO-01 compatibility test; VG-01); AC-18 (vehicle side) |

## 3.4 Transport Protocol Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| TP-1 | Support transport for parameter groups of 9–1,785 bytes, including BRM, BCP and BCS. Use required TP connection, transfer and close procedures. | Standard; External / MUST | §7.5, Table 1 note 7; SAE J1939-21:2006 §5.4.7, §5.10 | AC-4 |
| TP-2 | TP data frames are separated by 10 ms. The period applies to the complete packet. | Standard / MUST | §7.5 | AC-3, AC-4 |
| TP-3 | TP PGNs, control bytes, packet numbering, abort reasons and T1–T4 come from SAE J1939-21:2006 and remain marked External until verified. | External / MUST | SAE J1939-21:2006 | AC-4, AC-16 (NF-05) |
| TP-4 | Use a 64-byte receive buffer. Support one session per direction per peer. Refuse oversized optional messages as specified. | Standard; Project / MUST | §10.3; Spec TP-4 | AC-4 (refusal in the MUST gate; VG-02) |
| TP-5 | A sequence gap aborts the transfer. Duplicate handling, active-session RTS rejection and invalid CTS window handling follow the project policies and their assigned priorities. [DOC] Policies: a duplicate data frame is ignored; an RTS during an active session is rejected with an abort; an invalid TP_CM_CTS window aborts the transfer. Consistency of these policies with SAE J1939-21:2006 is [EXTERNAL] and unverified. | Project; External / MUST (gap/abort); SHOULD (others) | Spec TP-5 | AC-4 |
| TP-6 | Keep transport timers separate from application waits. Transport failure alone must never set BEM or CEM. | Standard; Project / MUST | §9; Spec TP-6 | AC-5, AC-14 (VG-03) |
| TP-7 | After a failed transfer, restart at the message's next period. No retry loop. | Project / MUST | Spec TP-7 | AC-4 (VG-04) |

## 3.5 Error and Timeout Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| ERR-1 | A timeout means no complete or correct message within the specified time. Map each wait to its timeout and error handling. [STD] Timeout definition (§9); BEM/CEM fields (Tables 28–29). [DOC] Only waits with a corresponding field in Tables 28–29 set that field; other waits produce local diagnostics. The wait-to-field correspondence is by message name, a project interpretation; the standard gives no mapping table (01.2_TIMEOUT_TABLE.md U-T2). Canonical wait identifiers: TO-01–TO-25 (§6.6). | Standard; Project / MUST | §9, Tables 28–29; Spec ERR-1 | AC-5 |
| ERR-2 | On timeout, stop current messages and send BEM or CEM every 250 ms. Reconnection is SHOULD and only follows listed eligible timeouts. [STD] Timeout response per Figures A.2–A.3; BEM/CEM period 250 ms (Table 7). Completion time of timeout handling per GB/T 18487.1-2023 B.3.7.3 [EXTERNAL] (TO-25; see ERR-5). Reconnection eligibility [STD]: after a charger timeout of BCP, BRO, BCL or BCS (Figure A.2 note b), and after a vehicle timeout of CML, CRO or CCS (Figure A.3 note b). Figure A.2 note b also lists BSM, which conflicts with §11.3.4 and Figure A.1 note h; OPEN (OQ-S1, U-STD-01). Spec v2.0 records "a BSM timeout never reconnects without re-plugging" as a [DOC] position, not approved. [STD] Reconnection restarts with the charger sending CRM periodically; the count limit is operator-defined (note a), project default 3 [DOC]; the vehicle exits if no CRM arrives within 30 s (TO-23, TO-24); a vehicle not supporting reconnection exits directly (note d). Reconnection conditions per GB/T 18487.1-2023 B.4.7.3 and B.4.7.4 [EXTERNAL]. | Standard; External; Project / MUST (timeout response); SHOULD (reconnection) | Figures A.2–A.3 p.54–55; Table 7; Spec ERR-2 | AC-5, AC-18; MUST part: gap VG-12 |
| ERR-3 | Validate frame identifiers and lengths, transport sequencing and size, and signal range and credibility. Invalid data is rejected, counted and must not refresh wait timers or affect output. | Standard; Project / MUST | §9; Spec ERR-3 | AC-7 (VG-05) |
| ERR-4 | Separate application timeouts, transport failures, controller failures and physical failures in implementation, counters and logs. | Project / MUST | Spec ERR-4 | AC-5, AC-14 (VG-06) |
| ERR-5 | BEM and CEM handling follows GB/T 18487.1-2023 B.3.7.3. The 1 s duration is a project placeholder until verified. Never send these messages while bus-off. | External; Project / MUST | GB/T 18487.1-2023 B.3.7.3 | AC-5, AC-9a |
| ERR-6 | Charging current outside -400 A to 0 A ends charging. Charger sends CST and vehicle sends BST with the specified fault reason. [STD, public-site provision applied per OWNER D-01] A vehicle or charger receiving a charging current value outside −400 A to 0 A exits the charging process (§5.4). [DOC] Exit mechanism and fault reason: the charger sends CST with SPN3522 bits 11–12 "other fault"; the vehicle sends BST with SPN3512 bits 15–16 "other reason". The SPN3521/SPN3511 stop-reason value to accompany them is not specified (RQ-08). | Standard; Project / MUST | §5.4 p.5; Tables 24–25; Spec ERR-6; D-01 | AC-7 |

## 3.6 CAN Controller Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| CAN-1 | Bus-off sets output or current request to zero within 10 ms, latches a fault, records TEC/REC and controller state, and starts software-controlled recovery. Recovery timeout is 1 s with at most 3 attempts. [DOC] Automatic hardware recovery is disabled. The 1 s and 3-attempt values are project decisions, not standard values. [OWNER D-07] Each recovery attempt has an independent 1 s timeout (not a total deadline); an attempt fails if no recovery-complete event occurs within its 1 s; at most 3 attempts; after the third failed attempt the node enters FAULT_LOCAL and remains there until operator restart (CAN-4). Recovery attempts do not delay or override the 10 ms output inhibit. The recovery-attempt start event, the observable recovery-complete event, the interval between attempts and the operator-restart mechanism are defined in PHASE 02 (RQ-01, PARTIALLY RESOLVED). After recovery the protocol restarts in IDLE; mapping of IDLE to the PHASE 01.2 steps is PHASE 02 work. | Project / MUST | Spec CAN-1; D-07 | AC-6 (10 ms inhibit), AC-9a (recovery); TEC/REC evidence OPEN (VG-07) |
| CAN-2 | After recovery, never resume the old session. A new session requires the full handshake from CHM. | Project / MUST | Spec CAN-2 | AC-9a |
| CAN-3 | Error-warning and error-passive states are logged with TEC and REC. Transmission continues while permitted. | Project / SHOULD | Spec CAN-3 | AC-9b |
| CAN-4 | After 3 failed recovery attempts, remain in FAULT_LOCAL until operator restart. | Project / MUST | Spec CAN-4 | AC-9a (VG-08) |
| CAN-5 | Detect peer loss without bus-off through application-level waits. [DOC] PHASE 01.2 shows steps with no applicable wait, e.g. before communication start, the vehicle waiting for CHM or CRM (V02, no timeout drawn) and the vehicle configuration steps V07–V08; peer loss in such steps is not detectable through waits and its handling is OPEN (RQ-05). | Project / MUST | Spec CAN-5; 01.2_STATE_MACHINES.md | AC-5 (steps with an applicable wait; VG-09; RQ-05 OPEN); AC-9c, AC-9d supplementary (SHOULD, non-gating) |

## 3.7 Safety Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| SAF-1 | On detecting a stop, fault, timeout, bus-off or RX overflow while charging, disable output and set 0 A within 10 ms. Execute before transmission and without waiting for remote response. [DOC] "While charging" is to be defined against the PHASE 01.2 steps in PHASE 02 (RQ-04). | Project / MUST | Spec SAF-1 | AC-6 |
| SAF-2 | Generate output from validated BCL and apply BCP and local current/voltage clamps. BHM SPN2601 is insulation-check voltage, not a charging limit. [STD] The output command derives from a validated BCL per PROT-7. [DOC] Project clamps: current not above BCP SPN2817 maximum allowable charging current (unit conflict, OQ-1) or the local limit; voltage not above BCP SPN2819 maximum allowable total charging voltage or the local limit. [STD, permissive] §11.2.1 states SPN2819 may serve as the charger's overvoltage-protection reference; no clause requires an SPN2817 current clamp. [STD] BHM SPN2601 is used only as the basis of the insulation-monitoring voltage of the charger's insulation self-check (Table 9). | Standard; Project / MUST | §11.2.1 p.11; Table 9; Spec SAF-2 | AC-7 |
| SAF-3 | Malformed, stale or not-credible requests produce 0 A while the session continues until its wait expires. Out-of-range current ends charging. [DOC] 0 A on a malformed, stale or not-credible request; the staleness threshold for a BCL request is not defined (RQ-02). [STD] Not-credible values are ignored (§8.9). [STD, public-site provision per OWNER D-01] Out-of-range charging current ends charging per ERR-6 (§5.4). | Project; Standard / MUST | §5.4, §8.9; Spec SAF-3; D-01 | AC-7 |
| SAF-4 | Output command and feedback interfaces must live in app/output_if.h outside protocol/. | Project / MUST | Spec SAF-4 | AC-15 |
| SAF-5 | Feedback is fresh only when valid and no more than 100 ms old. Stale or invalid feedback must not appear in CCS. Charger sets output to zero and enters C_STOP_LOCAL. [DOC] 100 ms equals twice the 50 ms CCS period. Sending CST ends CCS (Table B.1, informative). The CST stop reason for this case is not specified (RQ-03); C_STOP_LOCAL is a legacy Spec state name, mapped in PHASE 02. | Project / MUST | Table B.1 (informative); Table 5; Spec SAF-5 | AC-7 |

## 3.8 Software Design Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| DES-1 | protocol/ is pure C, contains no hardware headers and reaches hardware only through hal/hal.h. | Project / MUST | Spec DES-1 | AC-10 |
| DES-2 | The same protocol sources build for G474, F103 and Linux, with no platform-specific #ifdef in protocol/. | Project / MUST | Spec DES-2 | AC-10 |
| DES-3 | No dynamic memory allocation. Use bounded queues with specified capacities. | Project / MUST | Spec DES-3; Architecture Queues | AC-11, AC-12 |
| DES-4 | Compile with zero warnings using -Wall -Wextra -Werror. | Project / MUST | Spec DES-4 | AC-12 |
| DES-5 | Use a wrap-safe 32-bit millisecond tick. | Project / MUST | Spec DES-5 | AC-11 |
| DES-6 | Protocol processing has deterministic received-frame and tick entry points. | Project / MUST | Spec DES-6 | AC-11 |

## 3.9 Testing and Tooling Requirements

| ID | Requirement Baseline | Type / Level | Source | Verification |
|---|---|---|---|---|
| TST-1 | Unit tests cover mandatory message codec, both state machines, stops, parameter mismatch, BSM abnormal, critical waits, BCL rules, transport, timer rollover, queues and invalid signals. [DOC] Detail: codec of every mandatory message (byte order, resolution, offset, current sign, fill bits, version bytes); every normal transition of both machines; both stop procedures, both parameter-mismatch stops and the BSM-abnormal stop; every safety-critical wait at T−1 ms and T+1 ms; PROT-7 rules; core transport (RTS, CTS window, DT, EOM ACK, abort, sequence gap, T1–T3 [EXTERNAL], 10 ms spacing); timer rollover; RX and TX overflow; invalid and out-of-range signals. [OWNER D-05, ODS-01] The safety-critical waits are the canonical set TO-14, TO-15 and TO-16 (§6.6), tested at T−1 ms and T+1 ms (the ±1 ms boundary is a project verification choice). Spec v2.0 names its safety-critical waits by legacy IDs W2, W6a, W6, W7, W7b, W8, W9, W10, W15, W16, W17; these remain UNMAPPED (D-03) and are not substitutes for TO-IDs. | Project / MUST | Spec TST-1 | AC-11 |
| TST-2 | SIL tests cover normal session, both stops, both parameter mismatches, BSM abnormal and safety-critical application timeout rows. Remaining rows are SHOULD. [DOC] SIL on vcan. Category 1 = application timeouts (legacy W rows); Category 2 = transport failures (legacy T rows). SHOULD includes all other Category 1 and Category 2 rows, including reconnection and pause. State-hold scenarios measure message periods (AC-3). [OWNER D-05, ODS-01] The safety-critical Category 1 rows are TO-14, TO-15 and TO-16 (§6.6); legacy W/T rows remain UNMAPPED (D-03). | Project / MUST; SHOULD | Spec TST-2 | AC-5, AC-8 |
| TST-3 | MUST fault injection: drop BCL, drop CCS, invalid length, out-of-range BCL current, missing TP_DT, wrong sequence, injected bus-off and RX overflow while charging. [DOC] SHOULD: delay, duplicate, reorder, undefined PGN, invalid CTS window, missing EOM ACK and abort. STRETCH: extensive scenarios. Physical faults are tested on hardware only; vcan cannot produce CRC, stuff or bit errors or a physical bus-off. Expected outcomes of "drop BCL" and "drop CCS" depend on ERR-2 reconnection (SHOULD) and, for CCS during charging, on TO-17 (§11.3.3 text only, OQ-S4); "out-of-range BCL current" follows ERR-6 under D-01. | Project / MUST; SHOULD; STRETCH | Spec TST-3 | AC-14 |
| TST-4 | Log state transitions, errors, controller changes and output changes with millisecond timestamps. Logging never blocks safety timing. | Project / MUST | Spec TST-4 | AC-13 (log content), AC-6 (non-blocking; VG-10) |
| TST-5 | Basic CAN log decoder is SHOULD. YAML-generated decoder is STRETCH. | Project / SHOULD; STRETCH | Spec TST-5 | AC-13 |
| TST-6 | Target at least 80% line coverage for `protocol/` and report branch coverage. Coverage does not block release. | Project / SHOULD | Spec TST-6 | AC-11 |
| TST-7 | Provide reproducible build and test commands for all three targets from a clean checkout, with toolchain versions recorded. | Project / MUST | Spec TST-7 | AC-13 |

---

# 4. Preliminary Implementation Allocation

The following module allocation is provisional.

Final ownership and interfaces will be confirmed during PHASE 02.

| Requirement Group | Planned Modules |
|---|---|
| STD | protocol/gbt27930_2023_a.yaml; tools/gen_protocol_table.py |
| COM | protocol/codec.c; generated protocol tables; CAN identifier generator |
| PROT | protocol/charger_sm.c; protocol/bms_sm.c; app/charger_app.c; app/bms_app.c |
| TP | protocol/tp.c |
| ERR | protocol/timers.c; protocol/diag.c; state machines |
| CAN | hal/hal.h; ports/stm32; ports/linux |
| SAF | app/charger_app.c; app/output_if.h; app/sim_plant.c |
| DES | protocol/; hal/; ports/; build system |
| TST | tests/unit/; tests/sil/; tests/hw/; tools/ |

---

# 5. Acceptance Criteria Register

All MUST acceptance criteria require passing evidence.

| ID | Acceptance Requirement | Covers | Procedure | Verification Evidence | Verification Level |
|---|---|---|---|---|---|
| AC-1 | Complete hardware session from handshake to statistics in 10/10 runs without BEM/CEM. [B1] The captured session also shows the first CHM within 1 s of the simulated auxiliary-power event (§11.1.1; NF-01). | STD-1, PROT-1, PROT-2, PROT-6, PROT-11 (MUST: absence handling) | Run 10 complete sessions on the hardware nodes; capture with USB-CAN and decode. Check: no BEM/CEM; message order against the PHASE 01.2 state machines (DRAFT); first CHM within 1 s of auxiliary power; independent operation of both nodes; separate wait behaviour for CRM 0x00/0xAA, BRO/BRO 0xAA and CRO/CRO 0xAA (PROT-6); absent BMV/BMT/BSP raise no error or timeout. STD-1 exclusions are checked under AC-10. | USB-CAN log and decoder | MUST |
| AC-2 | Captured frames are Classic CAN 2.0B at 250 kbit/s with correct extended IDs and no FD frames. | COM-1, COM-2, COM-3 | Capture a session; check every frame is a Classic CAN 2.0B extended (29-bit) frame at 250 kbit/s with DLC ≤ 8 and no FD frame; compare identifiers with the identifier-generator output and the PHASE 01.2 derived identifiers (U-P1). Independent bus and bench wiring (COM-1) by inspection. | USB-CAN capture | MUST |
| AC-3 | SIL message periods remain within ±10% during state-hold tests. | COM-4, TP-2 (whole-packet period) | In SIL, hold each phase in a steady state and measure the transmission intervals of each periodic message against Tables 3–7 with the ±10% project tolerance (COM-4); for multi-packet messages measure the whole-packet period (TP-2). The interval metric and the sufficiency of SIL-only timing evidence for COM-4 are OPEN (NF-03). | SIL timing report | MUST |
| AC-4 | BRM, BCP and BCS transport works on hardware with required 10 ms DT spacing and transport unit tests. | TP-1, TP-2 (10 ms DT spacing), TP-3 (use of EXTERNAL values), TP-4, TP-5 (MUST: sequence-gap abort; SHOULD: duplicate, RTS during an active session, invalid CTS window), TP-7 | Transfer BRM, BCP and BCS on hardware and capture DT spacing; unit-test RTS, CTS window, DT sequencing, EOM ACK, abort, sequence gap and T1–T3 (EXTERNAL values, TP-3). MUST gate: transfers complete; a sequence gap aborts (TP-5); an oversized optional message is refused, the 64-byte buffer is never overrun and one session per direction per peer is enforced (TP-4, VG-02); after a failed transfer the next attempt starts only at the message's next period, without a retry loop (TP-7, VG-04). The DT-spacing tolerance is not established in this baseline. SHOULD checks, recorded separately and non-blocking: TP-5 duplicate, active-session RTS and invalid CTS window policies. | Capture and unit tests | MUST |
| AC-5 | Safety-critical timeout rows pass T-1 ms and T+1 ms tests with correct error fields and states. [B1] The safety-critical timeout rows are TO-14, TO-15 and TO-16 (§6.6, ODS-01). | ERR-1, ERR-2 (MUST: timeout response), ERR-4, ERR-5, TP-6, CAN-5 (steps with an applicable wait), PROT-12 (charger: TO-01 compatibility), TST-2 (MUST: safety-critical rows; SHOULD: remaining rows) | In SIL: (a) TO-14, TO-15, TO-16 in C10 and C10P at T−1 ms (no timeout) and T+1 ms (timeout), asserting the CEM field (SPN3925, SPN3924, SPN3928; by-name mapping, 01.2 U-T2) and resulting state; TO-16 is armed only after the first BSM (Figure A.1 note h); for TO-16 only detection and CEM SPN3928 are asserted, reconnection is not asserted (OQ-S1). (b) ERR-2: after a timeout the node stops its current messages and sends BEM/CEM every 250 ms (Table 7) within the COM-4 tolerance (VG-12). (c) TO-01 compatibility (PROT-12, VG-01, D-05): with no BHM the charger continues at 10 s after the first CHM on the compatibility path, generates no timeout error response and sets no CEM field, then sends CRM; the continuation time is recorded and no ±1 ms acceptance tolerance is currently imposed (any future measurement tolerance must be separately identified as a project verification choice under D-05); the lower-version self-check is simulated (EXTERNAL). (d) TO-25 handling (VG-12): entry into timeout handling at C-TO, C-BE, V-TO and V-CE and exit when the handling-deadline parameter expires; the deadline value and start condition are EXTERNAL (GB/T 18487.1-2023 B.3.7.3) and OPEN; the ERR-5 1 s placeholder may parameterise the harness but is not a conformance limit, and no pass/fail is assessed on the duration. (e) CAN-5: the peer stops transmitting in a step with an applicable wait; the wait expires and is handled (VG-09); steps without a wait are OPEN (RQ-05). (f) ERR-1 mapping of TO-01–TO-25 by inspection; ERR-4 separation of the timeout class (with AC-14). TP-6 is checked in AC-14; ERR-5 bus-off behaviour in AC-9a. SHOULD checks, non-blocking: remaining timeout rows (TST-2); TO-17 applicability is OPEN (ODS-01). | SIL report | MUST |
| AC-6 | Output command becomes disabled and 0 A within 10 ms in required safety scenarios. [B1] Includes runs with the log queue saturated (TST-4, VG-10). The PROT-10 SPN3096 = 00 inhibit is not an AC-6 scenario (D-08; covered by AC-8). | SAF-1, CAN-1 (10 ms output inhibit on bus-off), TST-4 (logging never blocks safety timing) | For each SAF-1 trigger (stop, fault, timeout, bus-off, RX overflow while charging; 'while charging' per RQ-04), measure the time from detection to the output command reaching disabled and 0 A, in SIL (sampled command) and on hardware (independent timing instrumentation). Pass: within 10 ms (SAF-1, CAN-1) and set before any transmission, without waiting for a remote response. Repeat with the log queue saturated: the same 10 ms result holds and log drops are counted (TST-4; no new threshold). Proves the software command only; physical current behaviour belongs to Project 2. Scenario enumeration is completed in B2. | SIL hook and logic analyzer | MUST |
| AC-7 | BCL, CC/CV, CML clamps, invalid requests and feedback freshness pass the test matrix. | COM-6, PROT-3, PROT-4, PROT-7, ERR-3, ERR-6, SAF-2, SAF-3, SAF-5 | Unit and SIL matrix: §11.3.1 CV and CC rules with current comparisons in non-negative magnitude after the single COM-6 conversion (D-06, VG-16); non-zero current demands above, within and below the CML limits; the zero-current case is excluded while OPEN (OQ-12(a) sub-entry); voltage demand below the charger minimum gives CST. SAF-2 clamps made binding in turn (SPN2817 unit OPEN, OQ-1). Malformed and not-credible requests give 0 A (staleness threshold OPEN, RQ-02). Out-of-range current exits charging with the ERR-6 fault reasons (stop-reason value OPEN, RQ-08). Feedback at most 100 ms old is reported; older or invalid feedback never appears in CCS and the output goes to 0 (SAF-5, PROT-4; CST reason OPEN, RQ-03). Invalid data never refreshes a wait timer (ERR-3, VG-05). BCL demand comes from the configurable battery model and is not capped by CML (PROT-3, NF-09). Signed CAN current encoded and decoded once (COM-6). | Unit tests and SIL | MUST |
| AC-8 | Both stop procedures, both parameter mismatches and BSM abnormal handling follow the specified sequence. [B1] Also covers the PROT-10 functional output inhibit on SPN3096 = 00 (D-08), the D-04 no-output condition on a peer stop message outside the charging phase, and the absence of a BSM timeout before the first BSM. | PROT-3, PROT-5 (including D-04), PROT-8 (MUST; SHOULD: updated BCP sent at least twice), PROT-9, PROT-10 (MUST part only), TST-2 (MUST scenarios) | In SIL and on hardware. Vehicle-first stop: BST ends on CST, then BSD. Charger-first stop: 5–10 BST frames, then BSD. BST and CST every 10 ms; CST carries exactly one stop reason. Charger mismatch: CML, then CST with the mismatch reason (§10.2; SPN2822 unit OPEN, OQ-3). Vehicle mismatch: BST with the mismatch reason. Abnormal SPN3090–3095 or BST gives CST; no BSM timeout before the first BSM (NF-02). PROT-10 (D-08, VG-13): inject a valid BSM with SPN3096 = 00 while charging; the output command enters the safe no-output condition (disabled, 0 A); record the observed latency as evidence; no numeric pass/fail limit; any harness watchdog is a test parameter; the SAF-1 10 ms limit is not applied; software command only, not physical shutdown. D-04 (VG-14): inject the peer stop message in steps clearly before the charging phase (RQ-04); the output command or current request never becomes non-zero; the protocol response sequence is not asserted (OQ-S10). Stop handling during pause is not asserted (OQ-S11). SHOULD check, non-blocking: updated BCP sent at least twice (PROT-8). | SIL and hardware logs | MUST |
| AC-9a | Software bus-off injection on all three ports passes safety and recovery requirements. | ERR-5 (no BEM/CEM while bus-off), CAN-1 (inhibit, latch, logging, recovery), CAN-2, CAN-4 | Software bus-off injection on each port: output or current request at 0 within 10 ms (CAN-1); nothing transmitted while bus-off, including BEM/CEM (ERR-5); fault latched and controller state logged. Recovery per D-07: an attempt fails if no recovery-complete event occurs within 1 s of that attempt; at most 3 attempts; after the third failure the node is in FAULT_LOCAL until operator restart (CAN-4, VG-08). After a successful recovery the old session is not resumed and a new session starts from CHM (CAN-2). The attempt-start event, recovery-complete event, inter-attempt interval and operator-restart mechanism are OPEN (RQ-01, PHASE 02) and must be defined before execution. Whether injected TEC/REC values are sufficient evidence is OPEN (VG-07 sub-entry). Proves application logic only. | SIL and board logs | MUST |
| AC-9b | Hardware bus-off test, if reproducible, is documented. | CAN-3 | Hardware bus-off, if reproducible: record TEC, REC and controller-state transitions; if not reproducible, record the limitation. Objective pass/fail conditions to be defined in B2. | Hardware logs | SHOULD |
| AC-9c | CAN_H disconnection robustness test. | CAN-5 (supplementary physical evidence; non-gating, the MUST gate is AC-5) | On hardware, disconnect CAN_H during charging and observe the resulting state against SAF-1 and CAN-2. The expected result is to be defined in B2. | Board logs | SHOULD |
| AC-9d | CAN_H and CAN_L disconnection test with safe shutdown. | CAN-5 (supplementary physical evidence; non-gating, the MUST gate is AC-5) | On hardware, disconnect CAN_H and CAN_L during charging; observe the expiring waits (identified by TO-ID; the Spec §30 W6/W15 references are UNMAPPED) and the output command reaching disabled and 0 A. The 'safe shutdown' result is to be defined in B2. | Board logs | SHOULD |
| AC-10 | Same protocol sources build on G474, F103 and Linux without hardware headers in protocol/. | STD-1 (exclusions, by inspection), DES-1, DES-2 | Build the same protocol sources for G474, F103 and Linux. Scripted check: no hardware headers in protocol/, hardware reached only through hal/hal.h, and no platform-specific conditional compilation in protocol/ (DES-2, NF-04). Inspection: System B, 50 kbit/s and the adapter node are absent (STD-1). | Build script and header check | MUST |
| AC-11 | All mandatory unit tests pass; coverage is reported. | DES-3 (bounded queues), DES-5, DES-6, TST-1, PROT-2 (NF-10), TST-6 (SHOULD) | Run the TST-1 unit-test inventory and check that every TST-1 item has a passing test, including TO-14, TO-15 and TO-16 at T±1 ms (§6.6); timer wrap (DES-5); deterministic replay of frame and tick scripts (DES-6); queue overflow at the configured capacities (DES-3); each state machine driven alone by scripted peer frames (PROT-2, NF-10). SHOULD check, non-blocking: line and branch coverage reported (TST-6). | Test report | MUST |
| AC-12 | Zero warnings, no malloc, protocol constant lint passes and no bare CTS identifier. | COM-5 (MUST: single source, generated C tables, lint; SHOULD: generated documentation tables and CI staleness check), COM-7, DES-3 (no dynamic memory), DES-4 | Build all targets with -Wall -Wextra -Werror: zero warnings. Static check: no dynamic memory allocation. Lint: no protocol constant outside generated files and no bare CTS identifier. SHOULD checks, non-blocking: documentation tables generated and CI staleness check present (COM-5). | Build log and lint | MUST |
| AC-13 | Clean checkout can build all targets and run tests using documented commands. | TST-4 (log content and timestamps), TST-7, TST-5 (SHOULD; STRETCH) | From a clean checkout, run the documented commands to build all three targets and run the unit and SIL tests; check that toolchain versions are recorded (TST-7). Review sample logs for state transitions, errors, controller-state and output-command changes with millisecond timestamps (TST-4); the non-blocking property of logging is verified in AC-6. SHOULD and STRETCH checks, non-blocking: log decoder (TST-5). | Fresh clone and review | MUST |
| AC-14 | All eight MUST fault-injection scenarios pass. [B1] Expected outcomes include: no BEM/CEM field set by a transport failure alone (TP-6) and each injected fault recorded only under its own failure class (ERR-4). | TST-3 (MUST: the 8 scenarios; SHOULD and STRETCH scenarios), TP-6 (VG-03), ERR-4 (VG-06) | In SIL, run the 8 MUST fault scenarios of TST-3 against expected outcomes defined per scenario (B2). Missing TP_DT: the transport failure alone sets no BEM/CEM field (TP-6). Each injected fault increments only its own failure-class counters and log category (ERR-4). Outcomes that depend on ERR-2 reconnection (SHOULD) or on TO-17 (OPEN, ODS-01; OQ-S4) are recorded without gating. SHOULD and STRETCH scenarios are recorded separately and are non-blocking. | SIL fault report | MUST |
| AC-15 | Project 1 passes SIL independently with simulated backend; Project 2 changes only backend selection. | SAF-4 | Run the SIL suite with the simulated backend; confirm the output command and feedback interfaces are in app/output_if.h outside protocol/; build with an alternate backend selection and confirm the source difference is limited to backend selection. | Build and source diff | MUST |
| AC-16 | YAML values have correct source references, External status and conflict records. [B1, D-10, ODS-03] Passes when: (1) every applicable conflict has a documented evidence source; (2) every conflict has an explicit status, OPEN or OWNER-APPROVED; (3) every affected YAML value references its conflict record; (4) no unresolved interpretation is presented as an approved standard value, and PROVISIONAL values are marked PROVISIONAL; (5) EXTERNAL values keep EXTERNAL status until independently verified; unresolved issues are traceable to §6.5. Passing AC-16 does not resolve a conflict, does not make an interpretation normative, does not approve a value for a compliance claim and does not satisfy STD-3. | STD-2, COM-5 (single source), TP-3 (EXTERNAL status; NF-05). Not STD-3 (ODS-03). | Generator report over the YAML: standard-derived values carry a GB/T 27930-2023 clause, table or figure reference; EXTERNAL values are flagged with their source document; conflict records (§6.2) show evidence and status; affected values reference their conflict record; PROVISIONAL values are flagged. Executable only once the YAML exists. | Generator report | MUST |
| AC-17 | Mandatory messages conform in encoding, length, fill bits and version bytes. | STD-4, STD-5 (MUST: version bytes; SHOULD: forward compatibility), COM-6, COM-8 (transmit and receive; VG-11, NF-06) | Captures and unit tests for every mandatory message: low byte first, resolution and offset, signed charging current on CAN, specified length, undefined bits and unsent optional parameters set to 1, CHM and BRM version bytes as printed (OQ-4). Receive side: not-credible values ignored and unspecified messages not processed. Capture check: no message unspecified by GB/T 27930-2023 appears on the network (D-01; status of SAE J1939-21 transport frames OPEN, RQ-09). Bit numbering within bytes is EXTERNAL (U-P2). SHOULD part of STD-5 (forward compatibility): no acceptance-level procedure is defined; non-blocking. | Captures and unit tests | MUST |
| AC-18 | Pause/resume, CTS, vehicle-side CRM without CHM and BCL-timeout reconnection work in SIL. | PROT-10 (SHOULD: full pause/resume; CTS aspect conditional on OQ-6), PROT-12 (SHOULD: vehicle side), ERR-2 (SHOULD: reconnection) | In SIL: pause on SPN3096 = 00 and resume on SPN3096 = 01 per §11.3.4 (conditions per GB/T 18487.1-2023 B.4.5, EXTERNAL); CTS checked only after OQ-6 is resolved (the CTS aspect is not owner approval of CTS transmission); vehicle CRM-without-CHM path; reconnection after a BCL timeout (Figure A.2 note b; conditions B.4.7.3/B.4.7.4 EXTERNAL). Does not cover the PROT-10 MUST part (AC-8). | SIL report | SHOULD |

AC-9 grouping note: AC-9 is a controlled top-level identifier represented only by its four sub-criteria AC-9a, AC-9b, AC-9c and AC-9d, each individually identifiable with its own Verification Level, Covers, Procedure and evidence (D-09, ODS-02). No separate AC-9 row exists.

R2 note (no acceptance criterion changed in v0.2.0):

- Counting: 18 top-level acceptance IDs (AC-1 to AC-18), displayed
  as 21 rows because AC-9 is shown as AC-9a to AC-9d. (v0.3.0: the
  convention is now approved, D-09 / ODS-02; GAP-05 CLOSED.)
- AC-18 lists CTS. Its CTS aspect is conditional on resolution of
  OQ-6 / GAP-07 and does not itself constitute owner approval of
  CTS transmission.
- Correction (v0.3.0): this note previously stated that AC-5 and AC-9d
  reference legacy wait identifiers. The baseline AC-5 and AC-9d wording
  never did; the legacy W identifiers (W2…W17 in AC-5; W6, W15 in AC-9d)
  appear only in Spec v2.0 §30. The safety-critical set is now defined
  by TO-ID (§6.6); the W identifiers remain UNMAPPED.
- Coverage gaps and objectivity limitations are recorded in §6.3 for
  PHASE 01.4. AC revision and new AC identifiers belong to PHASE 01.4.
  (v0.3.0: no new AC identifier or row is permitted, ODS-02.)

B1 note (v0.3.0):

- Structure: 18 top-level AC identifiers and 21 displayed rows are
  retained (D-09, ODS-02). Every row has Covers, Procedure and exactly
  one Verification Level. The original acceptance wording is kept as the
  leading text of each row; B1 additions are marked [B1].
- Verification-level convention (owner-approved, B1 authorization
  2026-11-09): a row covering both MUST and SHOULD obligations has
  Verification Level MUST; its MUST obligations determine pass/fail;
  its SHOULD obligations are named in Covers and recorded as separate,
  non-blocking checks. STRETCH items are likewise non-blocking (§2.2).
  A row with only SHOULD obligations keeps Verification Level SHOULD.
  No obligation was promoted or downgraded. Rows AC-4 and AC-11, whose
  level was previously "MUST / SHOULD", are MUST under this convention.
- Covers: initial source is the requirement-to-AC mapping of v0.2.1
  (ODS-05). B1 changes to that mapping are listed in §6.7.
- An AC row describes the acceptance-level procedure. Detailed test
  cases, fixtures and evidence formats are defined in PHASE 01.4-B2.
  No requirement is verified by this register; verification requires
  executed tests with retained evidence.

---

# 6. External Dependencies and Open Issues

## 6.1 External Documents

| Document | Required Verification |
|---|---|
| SAE J1939-21:2006 | TP PGNs, control bytes, packet numbering, abort reasons and T1–T4 |
| GB/T 18487.1-2023 | Timeout handling, self-check, pause and reconnection conditions |

All unverified external values remain External.

Clause-level external dependencies identified in PHASE 01.2 (all
UNVERIFIED; not resolved in this baseline):

| Dependency | Used by |
|---|---|
| GB/T 18487.1-2023 B.3.7.3 — completion time of communication-timeout handling (TO-25) | ERR-2, ERR-5 |
| GB/T 18487.1-2023 B.4.3 — charger self-check | PROT-1, PROT-12 |
| GB/T 18487.1-2023 B.4.4 — charging-ready process | PROT-8 |
| GB/T 18487.1-2023 B.4.5 — pause and resume | PROT-10 |
| GB/T 18487.1-2023 B.4.7.3 / B.4.7.4 — reconnection conditions | ERR-2 |
| GB/T 18487.1-2023 parameter-matching requirements (Figure A.1 note d) | PROT-8 |
| SAE J1939-21:2006 §5.4, §5.4.7, §5.10 — identifier bit assignment, transport protocol, message types | COM-1, COM-3, TP-1, TP-3, TP-5 |
| SAE J1939-11:2006 / J1939-15:2018 — physical layer | COM-1 |
| Lower-version self-check method (Figure A.1 note c) | PROT-12 |

## 6.2 Internal Standard Conflicts

| Issue | Reference | Required Action | Evidence (PHASE 01.2) | Status |
|---|---|---|---|---|
| BSM reconnection | §11.3.4, Annex A.1 note h, Annex A.2 note b | Record selected interpretation | §11.3.4 forbids re-handshake (重新握手) and note h forbids reconnection (重新连接) without re-plugging after a BSM timeout; Figure A.2 note b lists BSM as eligible for reconnection (p.15, p.53, p.54) | OPEN (OQ-S1, OQ-T1, U-STD-01) |
| BCP SPN2817 unit | Unresolved issues in Spec v2.0 | Verify and record selected unit | Table 12 cell "/V"; note 2 gives (A), 0.1 A/bit, −400 A offset (p.11–12) | OPEN (OQ-1, U-STD-02) |
| CSD SPN3613 offset | Unresolved issues in Spec v2.0 | Verify and record selected offset | Table 27 offset 1 with range 0–0xFFFFFFFF; CRM SPN2561 (same quantity) offset 0 (p.21, p.10) | OPEN (OQ-2, U-STD-03) |

No selected interpretation is approved for any of the three.

## 6.3 Verification Coverage Gap

The PHASE 01.3 R1 review identified the following acceptance-coverage
gaps, including the charger-side PROT-12 gap first recorded in v0.1.0
(VG-01, traced to GAP-06). They are recorded for PHASE 01.4; no acceptance criterion,
test or AC identifier is created or changed here, and no requirement
is verified merely because a test or AC references it.

| Gap | Requirement | Finding | Current Disposition |
|---|---|---|---|
| VG-01 | PROT-12 (charger, MUST) | The charger-side path lacks a dedicated, objectively identified acceptance procedure: the charger receives no BHM within 10 s, TO-01 expires, the charger continues using the compatible lower-version procedure without generating an error or CEM field, and proceeds with CRM. AC-18 names only the vehicle-side CRM-without-CHM behaviour; whether AC-1 or another existing procedure covers the charger-side path is unconfirmed. If none does, a dedicated SIL test is to be added to the controlled verification plan before implementation (PHASE 01.4). This is a verification-planning issue, not permission to change or downgrade PROT-12. Traced to GAP-06. | AC COVERAGE DEFINED — AC-5 (TO-01 compatibility test, D-05); GAP-06 linked; not verified. |
| VG-02 | TP-4 (MUST) | Refusal of oversized optional messages appears in AC-4 only as a SHOULD item. | AC COVERAGE DEFINED — AC-4 MUST gate; abort coding EXTERNAL (SAE J1939-21); not verified. |
| VG-03 | TP-6 (MUST) | No AC checks that a transport failure alone never sets a BEM/CEM field. | AC COVERAGE DEFINED — AC-14; not verified. |
| VG-04 | TP-7 (MUST) | AC-4 text does not check restart at the next message period without a retry loop. | AC COVERAGE DEFINED — AC-4; not verified. |
| VG-05 | ERR-3 (MUST) | AC-7 does not check that invalid data never refreshes a wait timer. | AC COVERAGE DEFINED — AC-7; not verified. |
| VG-06 | ERR-4 (MUST) | No AC measures the separation of timeout, transport, controller and physical failures in code, counters and logs. (v0.3.2: the earlier test definition used frame/signal validation in place of the physical-failure class; corrected in B2 v0.1.3.) | AC COVERAGE DEFINED — four-class software classification and reporting coverage is defined through AC-14 (VT-AC14-09: application timeout, transport failure, controller failure, and a synthetic physical-layer fault indication injected at an abstract software fault-reporting boundary); AC-5 continues to cover the application-timeout class. Actual physical-layer fault generation and hardware robustness are outside SIL and remain SHOULD scope (PROJECT_SCOPE.md §5.2). Not verified; VG-06 remains tracked until implementation and verification evidence exist. |
| VG-07 | CAN-1 (MUST) | TEC/REC logging is evidenced only by SHOULD AC-9b (software injection produces no TEC/REC); recovery semantics are undefined (RQ-01). | PARTIALLY DEFINED — recovery semantics (D-07) in AC-9a; sufficiency of injected TEC/REC evidence OPEN (§6.5 sub-entry); recovery events OPEN (RQ-01). |
| VG-08 | CAN-4 (MUST) | AC-9a does not exercise three failed recoveries, FAULT_LOCAL and operator restart. | AC COVERAGE DEFINED — AC-9a per D-07; execution requires the PHASE 02 event definitions (RQ-01); not verified. |
| VG-09 | CAN-5 (MUST) | Verified only indirectly by AC-5; the direct peer-loss tests AC-9c and AC-9d are SHOULD; steps without waits (RQ-05). | PARTIALLY DEFINED — AC-5 for steps with an applicable wait; steps without a wait OPEN (RQ-05). |
| VG-10 | TST-4 (MUST) | "Logging never blocks safety timing" is not measured (AC-6 excludes log writes; AC-13 is a review). | AC COVERAGE DEFINED — AC-6 under log-queue saturation; no new threshold; not verified. |
| VG-11 | COM-8 (MUST) | Receive-side behaviour (ignore not-credible values and unspecified messages) is covered only by the SHOULD fault "undefined PGN" in TST-3; AC-17 covers transmission only. | AC COVERAGE DEFINED — AC-17, transmit and receive (includes NF-06); J1939 transport-frame status OPEN (RQ-09); not verified. |
| VG-12 | ERR-2 (MUST part) | AC-5 checks error field and resulting state, not stopping of the current messages or the 250 ms BEM/CEM period. | AC COVERAGE DEFINED — AC-5, including the TO-25 handling path; TO-25 duration and start condition EXTERNAL and OPEN; not verified. |
| VG-13 | PROT-10 (MUST part) | No AC covers the SPN3096 = 00 output inhibit; AC-18 is SHOULD. | AC COVERAGE DEFINED — AC-8 per D-08 (functional, latency recorded, no numeric limit); not verified. |
| VG-14 | PROT-5 (D-04 part) | No AC covers the no-output condition on a peer stop message outside the charging phase. | AC COVERAGE DEFINED — AC-8 (no-output condition); protocol sequence OPEN (OQ-S10); not verified. |
| VG-15 | TST-1, TST-2; AC-5, AC-9d | Safety-critical wait set given only by legacy W identifiers, all UNMAPPED (§6.6). | SET DEFINED — TO-14, TO-15, TO-16 by D-05/ODS-01 (§6.6); AC-5, AC-11; legacy W identifiers remain UNMAPPED; not verified. |
| VG-16 | PROT-7 (MUST) | Not fully verifiable until OQ-12(a) is resolved. | PARTIALLY DEFINED — AC-7 magnitude-domain cases (D-06); zero-current case OPEN (OQ-12(a) sub-entry). |

Disposition terms: "AC COVERAGE DEFINED" means an existing AC row now
contains an acceptance-level procedure for the obligation; it does not
mean the requirement is verified. "PARTIALLY DEFINED" means part of the
obligation remains OPEN as stated. No gap is CLOSED.

Acceptance-criterion objectivity limitations (R1): AC-1 references the
state machines (now PHASE 01.2, DRAFT); AC-4 T1–T3 values are EXTERNAL;
AC-6 does not enumerate its scenario set; AC-13 README content is a
review judgement; AC-16 depends on the not-yet-created YAML; AC-3
references the legacy state name C_CONFIG. Partially evidenced MUST
items: PROT-2 (inspection), PROT-3 (CML-independence of BCL demand not
explicitly tested), COM-4 (±10% of a 10 ms period at 1 ms tick
resolution).

## 6.4 Project Owner Decisions

| ID | Decision | Applied in | Limitation |
|---|---|---|---|
| D-01 | Target charging-site profile: **public charging site**. The public-site provisions of GB/T 27930-2023 §5.4 and §5.5 (and the public-site BSP restriction, Table 23 note) apply. This is an owner-selected project scope decision, not a universal interpretation of the standard. | COM-8, ERR-6, SAF-3, PROT-11 | — |
| D-02 | Current representation: CAN — signed, charging direction negative (per GB/T 27930-2023); application internal — non-negative magnitude in mA; Project 2 converter interface — non-negative current setpoint in mA. The application and Project 2 representations are project-interface decisions, not standard requirements. | COM-6, PROT-7 | Comparison domain decided by D-06 (magnitude-based). Not assumed: that a 0 A demand means output at the CML minimum current (OPEN, OQ-12(a) sub-entry). |
| D-03 | Timeout identifier policy: TO-01 to TO-25 are canonical; legacy W/T identifiers are preserved; a legacy identifier is mapped only on explicit evidence; unsupported mappings are UNMAPPED; no mapping is inferred from numbering or similar names (§6.6). | ERR-1, TST-1, TST-2; AC-5, AC-9d | Individual legacy mappings remain UNMAPPED; the safety-critical set is defined directly by TO-ID (D-05). |
| D-04 | A node receiving a valid peer stop message outside the active charging phase must not initiate or continue non-zero charging output and remains in a safe no-output condition. Owner-defined safety requirement; standard-defined responses are preserved. | PROT-5 | Protocol transition and response sequence OPEN (OQ-S10); no transition or message sequence is introduced. |
| PROT-10 split | MUST [OWNER]: charger inhibits charging output (software output command to the safe no-output condition) on a valid BSM with SPN3096 = 00. SHOULD: full pause/resume. CTS project transmission not part of this decision: OPEN (OQ-6 / GAP-07). No hardware shutdown requirement in Project 1. | PROT-10 | SAF-1 referenced for the safe output state only; response time decided by D-08 (no numeric limit; RQ-07 CLOSED); pause limits OPEN (OQ-S6); GB/T 18487.1 B.4.5 EXTERNAL. |
| — | Working interpretation from PHASE 01.2: the normative Annex A path is used where Annex B differs (CRO stop on BCL). | STD-1 | Not a closure: OQ-S2 OPEN. |
| — | PROT-12 level split (charger MUST, vehicle SHOULD) preserved as instructed. | PROT-12 | — |
| D-05 | Safety-critical timeout set defined by canonical TO-IDs (Option A); no W-to-TO mapping inferred; T−1 ms / T+1 ms boundary verification applies to the identified set; TO-01 tested separately as a compatibility-path timeout (continues on the compatibility path, no erroneous timeout response, no erroneous CEM field). The 10 s TO-01 duration is standard-derived; the ±1 ms tolerance is a project verification choice. | TST-1, TST-2, ERR-1, PROT-12; AC-5, AC-11 | TO-25 external timing not inferred. Membership set by ODS-01. |
| D-06 | Current comparisons under §11.3.1 use non-negative magnitudes after the single approved decoding and unit conversion; no direct signed comparison of raw CAN current; conversion not applied twice; representation as D-02. | COM-6, PROT-7; AC-7 | Zero-current meaning OPEN (OQ-12(a) sub-entry). |
| D-07 | Recovery timeout 1 s per attempt (not a total deadline); at most 3 attempts; failure = no recovery-complete event within the attempt's 1 s; FAULT_LOCAL after the third failure; operator restart required. The 10 ms output inhibit and the disabled automatic hardware recovery of CAN-1 are unchanged. Project parameters, not GB/T 27930-2023 requirements. | CAN-1, CAN-4; AC-6, AC-9a | Attempt-start and recovery-complete events, inter-attempt interval and operator-restart mechanism: PHASE 02 (RQ-01). |
| D-08 | On a valid BSM with SPN3096 = 00 the charger inhibits charging output (software output command to the safe no-output condition) with no numeric response-time limit; SAF-1 10 ms not inherited; observed latency recorded as evidence only; harness watchdog is not a product requirement; no hardware shutdown requirement in Project 1. | PROT-10, SAF-1; AC-8, AC-18 | Full pause/resume remains SHOULD; CTS OPEN (OQ-6); B.4.5 EXTERNAL. |
| D-09 | 18 controlled top-level AC identifiers / 21 displayed verification rows; AC-9 represented by AC-9a–AC-9d; each row keeps its identifier, single level, requirement traceability, procedure and evidence; no AC added, removed or renumbered to reconcile counting. | §5 | Does not change the technical content of any AC by itself. |
| D-10 | AC-16 may pass its conflict-management and traceability checks while a standard conflict is OPEN, under five conditions (§5 AC-16); passing AC-16 does not resolve a conflict, make an interpretation normative, accept STD-3 or approve a value for a compliance claim; temporary values for unresolved conflicts are PROVISIONAL (§2.1). | AC-16, STD-2, STD-3, COM-5 | The three conflicts of §6.2 remain OPEN. |
| ODS-01 | Current safety-critical T±1 ms timeout test set: TO-14, TO-15, TO-16. TO-01 separate compatibility test; TO-02–TO-13 and TO-18–TO-24 not in the current set; TO-17 OPEN; TO-25 separate timeout-handling test. Project-level verification decision, not a classification by GB/T 27930-2023; exclusion is not a declaration of no safety relevance. | §6.6; TST-1, TST-2; AC-5, AC-11 | TO-17 OPEN; TO-25 value and start condition OPEN (EXTERNAL). |
| ODS-02 | AC structure preserved (18 IDs / 21 rows; no AC-19 or higher; no new rows; no renumbering); every row has Covers, Procedure and exactly one Verification Level; AC-9a–AC-9d remain sub-criteria with a grouping note. New MUST obligations absorbed into existing ACs only where the technical scope supports them; otherwise the gap stays OPEN. Mixed MUST/SHOULD convention approved with the B1 authorization (§5 B1 note). | §5 | — |
| ODS-03 | AC-16 and STD-3 are separate obligations; passing AC-16 does not satisfy STD-3; STD-3 remains OPEN and UNVERIFIED; the Project Owner decides at the PHASE 01 Exit Review whether STD-3 carries into PHASE 02 or blocks exit. | STD-3, AC-16 | No automatic closure. |
| ODS-04 | The AC register stays in this document, §5; no separate AC register file. | §5 | — |
| ODS-05 | The v0.2.1 requirement-to-AC mapping is the initial Covers source; differences from Spec v2.0 are recorded separately and not silently reconciled. | §5, §6.7 | Owner may revisit in a later controlled review. |
| ODS-06 | Findings and identifiers from PHASE 01.4-A/A1 are preserved in controlled documentation with original identifiers; missing details are not invented. | §6.3, §6.5 | Source reports are conversation records, not stored in the repository. The placement of CAN-1 TEC/REC is superseded by ODC-01 (below). |
| ODS-07 | The Owner Decision Record metadata records decision content APPROVED (2026-11-09) and controlled-document incorporation PENDING. | OWNER_DECISION_RECORD.md | — |
| ODC-01 | Owner Decision Clarification 01 (`docs/phase01/01.4/OWNER_DECISION_CLARIFICATION_01.md`, OWNER-APPROVED 2026-11-09): the zero-current interpretation is tracked under OQ-12(a) and the CAN-1 TEC/REC issue under VG-07; they are separate issues and are not grouped. ODC-01 supersedes only the original CAN-1 TEC/REC placement (under OQ-12(a), set by the B1 implementation instruction); the zero-current placement under OQ-12(a) is unchanged. No technical decision or status changes. | §6.5 | Both issues remain OPEN. |

## 6.5 Open-Question Register

IDs: OQ-n, OQ-Sn, OQ-Tn, U-xx from the PHASE 01.2 documents; U-STD-nn
and GAP-nn from Spec v2.0 Tab 5; RQ-nn introduced by this baseline.

| ID | Topic | Disposition | Affected |
|---|---|---|---|
| OQ-11 | Public or private site | CLOSED — OWNER-DECIDED (D-01: public site) | COM-8, ERR-6, SAF-3, PROT-11 |
| OQ-12(a) | Comparison domain of §11.3.1 current rules (signed or magnitude) | PARTIALLY RESOLVED — OWNER-DECIDED: magnitude-based comparison (D-06); zero-current sub-entry OPEN | PROT-7, COM-6 |
| OQ-12(a) / zero-current | Meaning of a zero-current demand (e.g. whether 0 A means CML-minimum output); not to be assumed. Placement under OQ-12(a) per ODC-01 | OPEN | PROT-7, AC-7 |
| OQ-12(b)(c) | Application and Project 2 current representation | CLOSED — OWNER-DECIDED (D-02) | COM-6 |
| U-T5 | Legacy W/T identifiers | PARTIALLY RESOLVED — policy approved (D-03); safety-critical set defined directly by TO-ID (D-05, ODS-01, §6.6); individual legacy mappings remain UNMAPPED | ERR-1, TST-1, TST-2, AC-5, AC-9d |
| OQ-S10 | Peer stop message outside the charging phase | PARTIALLY RESOLVED — safety requirement decided (D-04); protocol sequence OPEN | PROT-5 |
| OQ-S11 | Stop handling during pause (text vs Figure A.1 drawing) | OPEN | PROT-9 |
| OQ-S1 / OQ-T1 / U-STD-01 | BSM reconnection | OPEN | ERR-2, STD-3 |
| OQ-S2 | CRO stop: Annex A vs Annex B | OPEN (working interpretation Annex A) | STD-1 |
| OQ-S6 | Pause limits and charger-requested pause | OPEN | PROT-10 |
| OQ-1 / U-STD-02 | SPN2817 unit | OPEN | SAF-2, STD-3 |
| OQ-2 / U-STD-03 | SPN3613 offset | OPEN | STD-3 |
| OQ-3 | SPN2822 unit (℃ in table, V in note) | OPEN | PROT-8 |
| OQ-4 | Identical version bytes for V1.1 and V1.10 | OPEN — test bytes as printed | STD-5 |
| OQ-5 | CTS year byte order vs §5.3 | OPEN | PROT-10 |
| OQ-6 / GAP-07 | CTS classification | OPEN — standard evidence: optional message (SPN2823 optional, "可选", §8.7); whether the charger actively sends CTS is OPEN pending Project Owner confirmation and carries no MUST/SHOULD/STRETCH level until then; the CTS aspect of AC-18 is conditional on this item and does not constitute owner approval | PROT-10, AC-18 |
| OQ-7 | BEM SPN3903 combines time sync and CML | OPEN | ERR-1 |
| OQ-8, OQ-9, OQ-10 | Editorial anomalies in the standard | OPEN — no requirement impact | — |
| OQ-S3 | "Stop BEM" in Figure A.2 vehicle path | OPEN — PHASE 02 | ERR-2 |
| OQ-S4 / OQ-T3 | CCS charging-phase timeout only in §11.3.3 text | OPEN | TST-3 |
| OQ-S5 | Older-version paths | OPEN — levels preserved per owner instruction | PROT-12 |
| OQ-S7 | CHM end on BST during handshake | OPEN — PHASE 02 | PROT-5 |
| OQ-S8 | EXIT branches that send CST/BST | OPEN — PHASE 02 | PROT-8 |
| OQ-S9 | Precedence of BEM/CEM reception | OPEN — PHASE 02 | ERR-2 |
| OQ-T2 | Timer start-point wording, text vs figure | OPEN — test design | ERR-1 |
| OQ-T4 | TO-01 is not an error path | OPEN — recorded in PROT-12 | PROT-12 |
| OQ-T5 | Timeouts without an error field | OPEN | ERR-1 |
| U-P2 | Bit numbering within bytes (SAE J1939, EXTERNAL) | OPEN — must close before codec implementation | COM-6, STD-4 |
| U-P3 | Transport-protocol details (SAE J1939-21, EXTERNAL) | OPEN | TP-1, TP-3, TP-5 |
| GAP-05 | Acceptance counting: 18 IDs / 21 displayed rows | CLOSED — OWNER-DECIDED (D-09, ODS-02) | §5, §9 |
| GAP-06 | Charger-side PROT-12 acceptance coverage | OPEN — AC coverage defined in AC-5 (VG-01); not verified | PROT-12 |
| RQ-01 | Semantics of CAN-1 recovery 1 s / 3 attempts | PARTIALLY RESOLVED — OWNER-DECIDED semantics (D-07); attempt-start event, recovery-complete event, inter-attempt interval and operator-restart mechanism OPEN (PHASE 02) | CAN-1, CAN-4 |
| RQ-02 | Staleness threshold of a BCL request | OPEN | SAF-3 |
| RQ-03 | CST stop reason for stale or invalid feedback | OPEN | SAF-5 |
| RQ-04 | Definition of "while charging" / "active charging phase" against PHASE 01.2 steps | OPEN — PHASE 02 | SAF-1, PROT-5 |
| RQ-05 | Peer-loss handling in steps without an applicable wait | OPEN — PHASE 02 | CAN-5 |
| RQ-06 | Reconnection configuration: vehicle reconnection support (note d), counter scope | OPEN | ERR-2 |
| RQ-07 | PROT-10 MUST output inhibit: response-time bound, and whether a BSM with SPN3096 = 00 falls within the SAF-1 trigger conditions (SAF-1 is referenced for the safe output state only; its 10 ms deadline is not inherited) | CLOSED — OWNER-DECIDED (D-08): no numeric limit; SAF-1 10 ms not inherited; latency recorded as evidence | PROT-10, SAF-1 |
| RQ-08 | SPN3521/SPN3511 stop-reason value accompanying the ERR-6 fault reason | OPEN | ERR-6 |
| RQ-09 | Whether SAE J1939-21 transport frames are "specified" messages under §5.5 | OPEN | COM-8 |
| VG-07 / CAN-1 TEC/REC | Whether injected controller status (TEC/REC) values are sufficient evidence for CAN-1 or hardware verification is required. Placement under VG-07 per ODC-01 | OPEN | CAN-1, AC-9a, AC-9b |
| TO-17 (ODS-01) | Vehicle-side safety applicability of the CCS charging-phase timeout (§11.3.3 text only) | OPEN | TST-1, TST-2, TST-3 |
| U-T1 / TO-25 | TO-25 duration and start condition (GB/T 18487.1-2023 B.3.7.3, EXTERNAL) | OPEN | ERR-2, ERR-5, AC-5 |
| STD-3 (ODS-03) | STD-3 acceptance; independent of AC-16 | OPEN / UNVERIFIED — Project Owner decides at the PHASE 01 Exit Review | STD-3 |

PHASE 01.4-A / A1 findings (NF). Source: PHASE 01.4-A audit and A1
reconciliation reports delivered as conversation records (not stored in
the repository). Descriptions below are limited to what those reports
stated; no details are added.

| ID | Finding | Status (v0.3.0) | Affected |
|---|---|---|---|
| NF-01 | No measurable AC for "first CHM within 1 s of auxiliary power" | AC COVERAGE DEFINED — AC-1; not verified | PROT-1 |
| NF-02 | The PROT-9 first-BSM rule (no BSM timeout before the first BSM) is not covered; PROT-6 relied on the undefined AC-5 set (A1: PROT-6 is testable under AC-1) | AC COVERAGE DEFINED — AC-8 (PROT-9), AC-1 (PROT-6); not verified | PROT-6, PROT-9 |
| NF-03 | COM-4 timing is evidenced in SIL only; the interval metric is undefined | OPEN — owner decision pending | COM-4, AC-3 |
| NF-04 | DES-2 prohibition of platform-specific conditional compilation not checked by AC-10 | AC COVERAGE DEFINED — AC-10; not verified | DES-2 |
| NF-05 | TP-3 mapped to AC-4, but it is an EXTERNAL-status record obligation | AC COVERAGE DEFINED — AC-16 added (AC-4 retained); not verified | TP-3 |
| NF-06 | COM-8 transmit side (no unspecified message on the network, D-01) has no criterion | MERGED into VG-11 — AC-17 | COM-8 |
| NF-07 | ERR-6 expected field values incomplete | ALIAS of RQ-08 (OPEN) | ERR-6 |
| NF-08 | SAF-5 CST reason and target state undefined | ALIAS of RQ-03 (OPEN) | SAF-5 |
| NF-09 | PROT-3 configurable battery model and CML-independent BCL demand not explicitly tested | AC COVERAGE DEFINED — AC-7; not verified | PROT-3 |
| NF-10 | PROT-2 independence of the two state machines has no defined verification method | AC COVERAGE DEFINED — AC-11 (with AC-1); not verified | PROT-2 |
| NF-11 | SAF-3 staleness threshold undefined | ALIAS of RQ-02 (OPEN) | SAF-3 |
| NF-12 | Baseline AC wording lacks the measurable detail present only in Spec v2.0 §30 | OPEN — owner decision pending on adoption | §5 |

## 6.6 Timeout Identifier Policy (D-03) and Safety-Critical Timeout Set (D-05, ODS-01)

Canonical timeout identifiers are TO-01 to TO-25 of
`docs/phase01/01.2_TIMEOUT_TABLE.md` (DRAFT). Legacy identifiers from
Spec v2.0 are preserved below. Spec v2.0 §13.6 records that the original
W/T table was not recovered and that no duration or BEM/CEM mapping may
be inferred from a wait ID. No explicit evidence supports any mapping.

| Legacy identifier | Spec v2.0 usage | Mapping |
|---|---|---|
| W2, W6a, W6, W7, W7b, W8, W9, W10, W15, W16, W17 | Safety-critical Category 1 waits in Spec v2.0 TST-1 and Spec §30 AC-5; W6 and W15 also in Spec §30 AC-9d (not in the baseline AC wording) | UNMAPPED (each) |
| Other W rows | Remaining Category 1 rows (TST-2, AC-5) | UNMAPPED; not enumerated in Spec v2.0 |
| T rows | Category 2 transport failures (TST-2, AC-5) | UNMAPPED; not enumerated in Spec v2.0; transport timers are EXTERNAL (SAE J1939-21) |

Establishing mappings is a PHASE 01.4 dependency. The safety-critical
set below is defined directly by TO-ID and does not depend on any legacy
mapping.

Safety-critical timeout test set (D-05, ODS-01). This is a project-level
verification classification; GB/T 27930-2023 does not classify timeouts
as safety-critical. Exclusion from the set is not a declaration that a
timeout has no safety relevance. Durations and sources are those of
`01.2_TIMEOUT_TABLE.md` (unchanged).

| TO-ID | Disposition | Verification | Notes |
|---|---|---|---|
| TO-01 | Separate compatibility test | AC-5 (c) | 10 s standard-derived; continuation time recorded; no ±1 ms acceptance tolerance currently imposed (any future measurement tolerance is a project verification choice under D-05); no error response, no CEM field (D-05) |
| TO-02 to TO-13 | Not in the current safety-critical set | ERR-1 mapping; remaining rows SHOULD (TST-2) | — |
| TO-14 | In the set: T−1 ms / T+1 ms | AC-5 (a), AC-11 | BCL, C10 and C10P |
| TO-15 | In the set: T−1 ms / T+1 ms | AC-5 (a), AC-11 | BCS, C10 and C10P |
| TO-16 | In the set: T−1 ms / T+1 ms | AC-5 (a), AC-11 | BSM, C10 and C10P; armed after the first BSM (note h); only detection and CEM SPN3928 asserted; reconnection not asserted (OQ-S1) |
| TO-17 | OPEN | — | Vehicle-side applicability under review (§6.5) |
| TO-18 to TO-24 | Not in the current safety-critical set | ERR-1 mapping; remaining rows SHOULD (TST-2) | — |
| TO-25 | Separate timeout-handling test | AC-5 (d) | Value and start condition EXTERNAL (GB/T 18487.1-2023 B.3.7.3) and OPEN; no value inferred |

## 6.7 Mapping Records (ODS-05)

B1 changes to the v0.2.1 requirement-to-AC mapping (the initial Covers
source):

| Requirement | v0.2.1 mapping | v0.3.0 mapping | Rationale |
|---|---|---|---|
| STD-3 | AC-16 | None — OPEN / UNVERIFIED | ODS-03: AC-16 does not satisfy STD-3 |
| PROT-2 | AC-1 | AC-1, AC-11 | NF-10 |
| PROT-10 | AC-18 (MUST part: gap VG-13) | AC-8 (MUST part), AC-18 (SHOULD parts) | D-08, VG-13 |
| PROT-12 | State-machine test; AC-18 (vehicle); charger gap VG-01 | AC-5 (charger), AC-18 (vehicle) | D-05, VG-01 |
| TP-3 | AC-4 | AC-4, AC-16 | NF-05 |
| TP-6 | AC-5 (gap VG-03) | AC-5, AC-14 | VG-03 |
| ERR-4 | AC-5 (gap VG-06) | AC-5, AC-14 | VG-06 |
| TST-4 | AC-13 (gap VG-10) | AC-13, AC-6 | VG-10 |

Other gap absorptions (VG-02, VG-04, VG-05, VG-08, VG-09, VG-11, VG-12,
VG-14, NF-01, NF-02, NF-04, NF-09) add procedure content within AC rows
that already covered the requirement; the mapping itself is unchanged.

Differences between this baseline and Spec v2.0 §30 "Covers" lists
(recorded, not reconciled; the baseline mapping governs):

| Item | Baseline v0.3.0 | Spec v2.0 §30 |
|---|---|---|
| PROT-6 | AC-1 | AC-1, AC-5 |
| PROT-4 | AC-7 | AC-7, AC-15 |
| TST-2 | AC-5, AC-8 | AC-14 |
| TP-6 | AC-5, AC-14 | AC-4 |
| CAN-1 | AC-6, AC-9a | AC-6, AC-9a, AC-9b |
| CAN-3 | AC-9b | AC-9b, AC-9c |
| CAN-5 | AC-5 (gate); AC-9c, AC-9d (supplementary) | AC-9c, AC-9d |
| SAF-1 | AC-6 | AC-6, AC-9c, AC-9d |
| TST-5 | AC-13 | not covered |
| COM-5 | AC-12, AC-16 | AC-12 |
| DES-3 | AC-11, AC-12 | AC-11 |
| STD-1 | AC-1, AC-10 | not covered |
| STD-3 | None (ODS-03) | AC-16 |
| PROT-3 | AC-7, AC-8 | AC-8 |
| PROT-2 | AC-1, AC-11 (B1) | AC-1 |
| PROT-10 | AC-8 (MUST part, B1), AC-18 | AC-18 |
| PROT-12 | AC-5 (charger, B1), AC-18 | AC-18 |
| TP-3 | AC-4, AC-16 (B1) | AC-4 |
| ERR-4 | AC-5, AC-14 (B1) | AC-5 |
| TST-4 | AC-13, AC-6 (B1) | AC-13 |
| AC-16 meaning | Record integrity (D-10) | "recorded conflicts show the chosen value" |

---

# 7. Requirements Traceability Rules

Every requirement must be traceable through:

Requirement ID
    -> Source Reference
    -> Implementation Module
    -> Test Case
    -> Verification Evidence
    -> Acceptance Decision

The following rules apply:

1. Requirement IDs must remain stable.
2. Standard-derived values require exact source references.
3. External values must retain their verification status.
4. Every MUST requirement must have a verification method.
5. SHOULD and STRETCH items must remain visible.
6. Failed tests must be recorded, not silently removed.
7. Requirement changes require documented review.
8. Implementation status and verification status are separate.
9. No requirement is ACCEPTED solely because code exists.

---

# 8. Change Control

Changes to this baseline require:

1. Requirement ID.
2. Original requirement.
3. Proposed change.
4. Technical justification.
5. Standard or project source.
6. Impact on architecture.
7. Impact on testing and safety.
8. Project owner approval.

All approved changes must be recorded in the project change log.

---

# 9. Acceptance of This Document

This document is accepted when:

1. All 61 requirement IDs from Spec v2.0 are represented.
2. Original requirement priorities are preserved.
3. Source references are traceable.
4. Implementation allocation is identified as provisional.
5. All 18 acceptance criteria are represented (18 top-level IDs,
   displayed as 21 rows; convention approved by D-09 / ODS-02).
6. External dependencies and standard conflicts are recorded.
7. Verification gaps are explicitly identified.
8. The project owner approves the baseline.

---

# 10. Approval

Requirements Baseline v0.3.3 was formally accepted by the Project Owner on 2026-11-09 following the PHASE 01.4-C joint B1/B2 consistency audit. The historical acceptance of v0.3.0 on 2026-11-09 remains valid.

This document acceptance does not constitute final PHASE 01 exit approval, phase freeze, or verified protocol compliance. All OPEN, EXTERNAL and NOT VERIFIED items remain subject to their recorded dispositions.

| Role | Name | Status |
|---|---|---|
| Project Owner | Project Owner | v0.3.0 and v0.3.3 formally accepted for B1 (2026-11-09); PHASE 01 Exit Review and final phase freeze pending. |
| Technical Review | Technical Review | Pending |

