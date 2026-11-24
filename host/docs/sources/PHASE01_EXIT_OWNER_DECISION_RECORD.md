# PHASE 01 Exit — Owner Decision Record

## 1. Document Control

| Field | Value |
|---|---|
| Project | GB/T 27930-2023 System A Charger-BMS Simulator |
| Project ID | P1 |
| Document ID | P1-ODR-002 |
| Document | PHASE 01 Exit Owner Decision Record |
| Decision IDs | OD-1 to OD-11 |
| Version | 0.1.2 |
| Status | OWNER-ACCEPTED |
| Decision Content Approval Date | 2026-11-09 (OD-1 to OD-11, v0.1.0) |
| Current Revision Approval | OWNER-ACCEPTED — 2026-11-09 (v0.1.2) |
| Approval Scope Note | The OD-1 to OD-11 decision content was formally approved by the Project Owner in v0.1.0. v0.1.1 was superseded by v0.1.2 without separate formal Project Owner acceptance. v0.1.2 was formally accepted by the Project Owner on 2026-11-09 as the decision-record documentation. Acceptance of v0.1.2 does not constitute PHASE 01 Exit Approval. Technical Review remains Pending — not performed (OD-7). This acceptance does not authorize PHASE 02 commencement. |
| Date | 2026-11-09 |
| Phase | PHASE 01 — Exit |
| Owner | Project Owner |
| Requirements Baseline (B1) | `docs/REQUIREMENTS_BASELINE.md` v0.3.3, OWNER-ACCEPTED, SHA-256 `31dd01af1936296cd7a61ab6f7f92a21343c8beaec6898322b5f9b736ee709b6` |
| Verification Test Definition (B2) | `docs/phase01/01.4/VERIFICATION_TEST_DEFINITION.md` v0.1.6, OWNER-ACCEPTED, SHA-256 `b0a55dd06f924cb3bc369af8c2ff0b6e800564f86db554f0351a9713fb3bf8d8` |
| Related Decision Records | `docs/phase01/01.4/OWNER_DECISION_RECORD.md` (P1-ODR-001 (proposed); D-05 to D-10); `docs/phase01/OWNER_DECISION_SUPPLEMENT_01.md` (ODS-01 to ODS-07); `docs/phase01/01.4/OWNER_DECISION_CLARIFICATION_01.md` (ODC-01); B1 §6.4 (D-01 to D-10) |
| Exit Review Reference | PHASE 01 Formal Exit Review (read-only), 2026-11-09, recommendation CONDITIONAL PASS — RECOMMEND PHASE 01 EXIT SUBJECT TO EXPLICIT OWNER EXCEPTIONS. Delivered as a conversation record; not stored in the repository. |

Document ID note: `P1-ODR-002` continues the Owner Decision Record series
opened by `P1-ODR-001` (proposed). The original record still carries its
Document ID as "P1-ODR-001 (proposed)"; the series identifier is therefore
itself still proposed. No other repository document uses `P1-ODR-002`.

---

## 2. Authorization and Decision Authority

The Project Owner explicitly approved decisions OD-1 to OD-11 on 2026-11-09
after the read-only PHASE 01 Formal Exit Review. The Project Owner is the
decision authority. PROJECT_CHARTER.md §6.1 states:

> The project owner is responsible for:
>
> - Project scope and requirement approval.
> - Hardware and software implementation.
> - Technical decisions.
> - Verification and acceptance.
> - Project documentation.
> - Final release approval.

The decision content (OD-1 to OD-11) was approved by the Project Owner in
v0.1.0. Version 0.1.1 is a controlled documentation revision (R-1 to R-6)
that is pending Project Owner review; it does not change any decision.
This record is **not**:

- Formal PHASE 01 Exit Approval.
- Permission to begin PHASE 02 or any implementation.
- Architecture or state-machine freeze approval.
- Confirmation of GB/T 27930-2023 compliance.
- Closure of any unresolved MUST requirement or OPEN item.
- Independent Technical Review.

Terminology used in this record:

| Term | Meaning |
|---|---|
| OWNER-APPROVED decision | A decision OD-1 to OD-11 recorded in §4 |
| OPEN technical issue | An unresolved item in B1 §6 or B2 §8; its status is unchanged by this record |
| CONDITIONAL exception | An owner-approved permission to carry an OPEN item past PHASE 01 exit, subject to the stated restriction and milestone (§5) |
| Future verification obligation | A test definition, execution or evidence task required at a later phase |
| Future architecture-freeze decision | A decision reserved for the PHASE 02 architecture or state-machine freeze gate |

Phase references follow PROJECT_CHARTER.md §7: PHASE 02 System
Architecture, PHASE 04 Protocol Database, PHASE 05 Protocol Core, PHASE 06
Application & Safety, PHASE 07 SIL Testing, PHASE 08 Hardware Bring-up,
PHASE 09 Hardware Integration, PHASE 10 Verification & Release.

### 2.1 Governance and Approval Arrangements

This section records approval arrangements. They are not technical
exceptions and are not entries of the Controlled Exception Register (§5).

**OD-7 — Owner-only exit approval path (governance arrangement).**

| Aspect | Arrangement |
|---|---|
| Exit approval | The Project Owner may approve PHASE 01 exit without an independent Technical Review sign-off being completed first. |
| Prerequisite | Technical Review completion is not a prerequisite for PHASE 01 exit approval under OD-7. |
| Technical Review status | Pending — not performed (OD-7). B1 §10 and PROJECT_CHARTER.md §11 continue to show Technical Review as Pending. |
| Fabrication | No independent Technical Review approval is recorded or implied. |
| Characterization | Owner approval is not independent technical verification. |
| Visibility | The outstanding Technical Review status remains visible in project records. |
| Technical closure milestone | None. OD-7 is a governance arrangement; no technical closure milestone is assigned. |

### 2.2 Role Authorization (R-6, Option A)

Formally authorized roles (PROJECT_CHARTER.md §6):

| Role | Basis | Note |
|---|---|---|
| Project Owner | PROJECT_CHARTER.md §6.1 | Responsibilities as quoted in §2 |
| Technical Review Support | PROJECT_CHARTER.md §6.2 | Assigning evidence or review tasks to this role does not mean an independent Technical Review has been performed. Where OD-2 refers to "Technical Review — supporting technical evidence", the Technical Review Support role is meant. |

All other roles named in this record are labelled "Proposed — Pending
Owner Confirmation". This includes the Architecture Role (PHASE 02
architecture work), the Verification Role (verification definition and
execution), and any Implementation Role or Test / Validation Role. No such
role has been formally delegated; until confirmed, responsibility rests
with the Project Owner.

---

## 3. Background and Exit Review Reference

The PHASE 01 Formal Exit Review assessed eight gates:

| Gate | Subject | Exit Review result |
|---|---|---|
| A | Source authority and evidence integrity | CONDITIONAL PASS |
| B | Protocol-definition completeness | CONDITIONAL PASS |
| C | Requirements completeness and Owner Acceptance | PASS |
| D | Test-definition completeness and Owner Acceptance | PASS |
| E | Decision and OPEN-item traceability | CONDITIONAL PASS |
| F | Safety-critical unresolved issues | CONDITIONAL PASS — requires Owner exceptions |
| G | Architecture readiness | CONDITIONAL PASS |
| H | Approval and configuration-management readiness | FAIL (current state; correctable without content change) |

Findings carried from the PHASE 01.4-C joint B1/B2 consistency audit
(conversation record; not stored in the repository):

| Finding | Subject | Disposition in this record |
|---|---|---|
| C-1 | B2 §7 test-level limitation summary not exhaustive | OD-5 |
| C-2 | PROT-6 / TO-09 wait-identity coverage | OD-6 |
| C-3 | Working-use acceptance wording / B2 historical snapshot | §3.1 item 1 |

Identifier check: the identifier `ca-03` referred to in the Owner
instruction does not appear in any repository document or in the Formal
Exit Review. It is not recognised as a controlled identifier, and no
exception is created or accepted for it. The nearest existing identifier is
C-3 above.

### 3.1 Historical and governance clarifications

Recorded without modifying the source documents:

1. C-3 is a historical snapshot issue. B2 v0.1.6 §1 accurately reflected
   the B1 status at its acceptance point; B1 v0.3.3 was formally accepted
   afterwards (B1 Revision History, "0.3.3 (acceptance)"). No contradiction
   in accepted technical content is established.
2. B2 historical metadata and other stale decision-document metadata
   (Owner Decision Supplement 01 status text; Owner Decision Record
   "(proposed)" Document ID and B3 follow-up note) remain eligible for a
   later controlled metadata cleanup.
3. PROJECT_CHARTER.md (v0.1.0) and PROJECT_SCOPE.md (v0.1.1) remain DRAFT.
   This record does not formally accept them.
4. STANDARD_SOURCE_REGISTER.md remains DRAFT and does not establish
   verification of any external standard.
5. Project 1 Spec v2.0 remains RECONSTRUCTED — DRAFT — NOT APPROVED and is
   not an authoritative replacement for the accepted B1 baseline.

---

## 4. Owner Decisions OD-1 to OD-11

### OD-1 — PHASE 01 Exit Criteria

**Decision: APPROVED.**

Adopted:

- B1 §9 as the acceptance criteria for the Requirements Baseline document.
- The PHASE 01 Formal Exit Review Gates A–H as the project-level phase exit
  assessment framework.
- This record as the approved disposition of the exceptions and
  conditions.

Not adopted: Spec v2.0 §34 is not an authoritative normative exit
standard, because Spec v2.0 remains DRAFT — NOT APPROVED.

A CONDITIONAL PASS is acceptable only when:

1. All non-excepted exit gates pass.
2. Every exception is explicitly owner-approved.
3. Each exception has a documented restriction, responsible role and
   closure milestone.
4. No unapproved FAIL or NOT ESTABLISHED condition remains.

Gate H configuration-management requirements must be satisfied before
exit approval takes effect.

### OD-2 — STD-3: Controlled Exception

**Decision: APPROVED — OPTION A (carry into PHASE 02 as a controlled
exception).**

STD-3 remains **MUST / OPEN / UNVERIFIED**.

Restrictions:

1. SPN2817 unit scaling (OQ-1) and SPN3613 offset (OQ-2) remain
   configurable and PROVISIONAL.
2. BSM reconnection (OQ-S1) remains configurable and disabled by default.
3. No final codec scaling or BSM reconnection behaviour may be frozen
   before the corresponding interpretations are resolved.
4. A dedicated STD-3 acceptance criterion and verification test must be
   added before PHASE 07.
5. Project release remains blocked while STD-3 is OPEN.

Accountability: Project Owner — interpretation decisions; Technical Review
— supporting technical evidence.

Role clarification: For the supporting-evidence responsibility in this
decision, the formally authorized role is Technical Review Support (see
§2.2). This does not mean that an independent Technical Review has been
performed or approved. The project-level Technical Review status remains
Pending — not performed (OD-7).

STD-3 must never be downgraded or marked VERIFIED merely because PHASE 01
exits. ODS-03 (AC-16 does not satisfy STD-3) remains in force.

### OD-3 — CTS / OQ-6 / GAP-07

**Decision: APPROVED — CARRY AS OPEN.**

- OQ-6 remains OPEN. GAP-07 remains OPEN.
- No MUST/SHOULD classification is assigned by this decision.
- No final CTS transmission policy is established.
- No assumption that CTS must be transmitted or must be omitted is
  authorized.
- PHASE 02 may retain an optional/configurable design extension point but
  must not freeze CTS behaviour without further evidence and
  authorization.

Milestone: resolve before the relevant PHASE 02 architecture freeze, or
record a further explicit Owner Decision.

### OD-4 — Reverse Coverage

**Decision: APPROVED — ACCEPT AS A CONTROLLED EXCEPTION.**

A reverse-coverage matrix is not required before PHASE 01 exit.

Milestone: before the PHASE 02 State Machine Freeze.

Scope: applicable GB/T 27930-2023 System A clauses 5–11 and normative
Annex A. Each applicable clause shall map to requirement ID(s), an explicit
scope exclusion with rationale, or an identified coverage gap.

Reverse coverage shall not be claimed complete before the matrix has been
produced and audited.

### OD-5 — C-1: B2 Limitation Summary

**Decision: APPROVED — ACCEPT AS A NON-BLOCKING DOCUMENTATION
LIMITATION.**

- B2 v0.1.6 remains formally accepted and is not reopened or modified.
- Individual test definition fields remain authoritative.
- The B2 §7 summary is known to omit at least two relevant test-level
  limitations (PROT-6, PROT-12). No exact corrected total is asserted.

Milestone: before PHASE 07, an authorized B2 revision with a complete
recount of the limitation entries.

### OD-6 — C-2: PROT-6 / TO-09

**Decision: APPROVED — ACCEPT WITH MILESTONE REQUIREMENTS.**

1. PHASE 02: model TO-09 as a distinct canonical timeout ID.
2. Before PHASE 07: update B2 under change control with an explicit TO-09
   wait-identity verification case.
3. PHASE 07: execute the test and collect evidence.

The known coverage limitation is not closed by this decision.

### OD-7 — Technical Review

**Decision: APPROVED — OWNER-ONLY EXIT APPROVAL PATH.**

The Project Owner may approve PHASE 01 exit without an independent
Technical Review sign-off being completed first. However:

- B1 §10 Technical Review remains Pending.
- PROJECT_CHARTER.md Technical Review remains Pending.
- No Technical Review approval may be fabricated.
- Owner approval must not be described as independent technical
  verification.
- The outstanding review status must remain visible in project records.

Classification: governance and approval arrangement (§2.1); not a
technical exception.

### OD-8 — PHASE 01.2 Deliverables

**Decision: APPROVED — ACCEPT AS CONTROLLED PHASE 02 WORKING INPUTS.**

Accepted for use as controlled PHASE 02 inputs:

| Deliverable | Path | SHA-256 at decision |
|---|---|---|
| Protocol Tables | `docs/phase01/01.2_PROTOCOL_TABLES.md` | `148c6d39d1108d6b36c8797cb9030a6cbb98c11af95827c1e05d8adf752b7433` |
| State Machines | `docs/phase01/01.2_STATE_MACHINES.md` | `1a16de29c5b3c1f549bf4e746c7d4c55a81d7feb54f88a12e19fdfaf2badedcb` |
| Timeout Table | `docs/phase01/01.2_TIMEOUT_TABLE.md` | `6a83191df462cdda9ad12071d1fad3aaa07c90fde95942e878ee6acb7ba0cc38` |

Full hashes recalculated on 2026-11-09 (v0.1.1); each matches the 16-digit
prefix recorded in v0.1.0.

Status: DRAFT / WORKING-USE ACCEPTED.

Superseding status clarification: the wording "Final PHASE 01 freeze
remains pending the PHASE 01 Exit Review" in the Document Control of these
three files is superseded, for control purposes, by this decision. Any
metadata-only wording correction in those files is a separate controlled
change; the files are not modified by this record.

This decision does not formally approve every provisional protocol
interpretation, does not claim final protocol completeness, does not freeze
state-machine transitions and does not authorize implementation of
unresolved behaviours. The PHASE 02 architecture gate remains responsible
for final design decisions and freeze approval.

### OD-9 — External Standards Acquisition

**Decision: APPROVED.**

Authorized: acquisition of legitimate, lawfully accessible copies of
SAE J1939-21, SAE J1939-11, SAE J1939-15 and GB/T 18487.1-2023.

Requirements:

1. Record edition and publication identity.
2. Record source provenance.
3. Record SHA-256 where a local copy is maintained.
4. Record relevant clause references.
5. Keep unreviewed dependencies EXTERNAL / UNVERIFIED.

Milestones:

| Dependency | Milestone |
|---|---|
| U-P2 bit numbering (SAE J1939) | Before PHASE 02 codec freeze |
| SAE J1939-21 transport parameters and abort codes (U-P3, T1–T4) | Before relevant TP implementation and VT-AC04 assessment |
| GB/T 18487.1-2023 B.4.3, B.4.4 and Figure A.1 note d | Before affected PHASE 06 implementation |
| GB/T 18487.1-2023 B.3.7.3 / TO-25 | Before affected PHASE 06/07 acceptance |

Other external dependencies listed in B1 §6.1 (including GB/T 18487.1-2023
B.4.5 and B.4.7.3 / B.4.7.4, SAE J1939-11 / J1939-15 physical-layer values
and the lower-version self-check method) keep their existing B1 status; no
additional milestone is assigned by this decision.

Acquisition does not constitute verification.

### OD-10 — Git Configuration Management

**Decision: APPROVED — CONTROLLED PHASE 01 ARCHIVAL (in principle).**

See §8.

### OD-11 — Safety-Relevant OPEN Items

**Decision: APPROVED — CARRY INDIVIDUALLY WITH EXPLICIT RESTRICTIONS.**

The milestones in §6 are adopted. Other OPEN items preserve their existing
B1 §6.5 statuses and milestones. No RQ, OQ, GAP, VG, NF or TO entry is
blanket-closed.

None of these exceptions authorizes unsafe physical operation or a
production compliance claim.

---

## 5. Controlled Exception Register

Every entry is a CONDITIONAL technical exception. The underlying item keeps
its B1 or B2 status. Entries are keyed by existing decision and issue
identifiers; no new identifier is created. OD-1 condition 3 (documented
restriction, responsible role and closure milestone) applies to every entry
of this register. OD-7 is a governance arrangement (§2.1) and is not an
entry of this register.

Role labels: "Proposed" = Proposed — Pending Owner Confirmation (§2.2).

| Decision / Item | Source | Status (unchanged) | Restriction | Accountable role | Closure milestone | Objective closure evidence |
|---|---|---|---|---|---|---|
| OD-2 / STD-3 (with OQ-1, OQ-2, OQ-S1) | B1 STD-3, §6.2, §6.5; ODS-03 | MUST; OPEN / UNVERIFIED | OD-2 restrictions 1–5 | Project Owner (interpretations); Technical Review Support (evidence) | STD-3 AC and test before PHASE 07; interpretations before relevant codec and reconnection freeze; release blocked while OPEN | Owner-approved interpretations recorded in B1 §6.2; STD-3 AC and test added to B1/B2 under change control; executed evidence |
| OD-3 / OQ-6, GAP-07 (CTS) | B1 PROT-10, §6.5 | OPEN; no level | No CTS level assumed; optional/configurable extension point only; no freeze | Project Owner | Before the relevant PHASE 02 architecture freeze, or further Owner Decision | B1 change assigning a classification, or recorded Owner Decision |
| OD-4 / reverse coverage | Exit Review Gate B | Not produced | No claim of completeness | Project Owner; Technical Review Support | Before PHASE 02 State Machine Freeze | Audited matrix covering clauses 5–11 and Annex A |
| OD-5 / C-1 | B2 §7; 01.4-C audit | Documentation limitation | B2 not reopened; test fields authoritative; no exact total asserted | Project Owner; Verification Role (Proposed — Pending Owner Confirmation) | Before PHASE 07 | Authorized B2 revision with complete recount |
| OD-6 / C-2 (PROT-6, TO-09) | B2 VT-AC01-03 field 15, §6.1 | Coverage limitation OPEN | TO-09 not treated as covered | Project Owner; Architecture Role (Proposed — Pending Owner Confirmation); Verification Role (Proposed — Pending Owner Confirmation) | PHASE 02 (distinct TO-ID); before PHASE 07 (B2 case); PHASE 07 (execution) | Architecture shows TO-09 as distinct TO-ID; B2 revision; executed evidence |
| OD-8 / PHASE 01.2 deliverables | `docs/phase01/01.2_*.md` | DRAFT / WORKING-USE ACCEPTED | No freeze; no implementation of unresolved behaviours | Project Owner; Architecture Role (Proposed — Pending Owner Confirmation) | PHASE 02 architecture / state-machine freeze gate | Owner-approved PHASE 02 freeze record |
| OD-9 / external standards | B1 §6.1, §6.5; B2 §8 | EXTERNAL / UNVERIFIED | Acquisition is not verification | Project Owner (acquisition); Technical Review Support (clause review) | Per OD-9 milestone table | Source Register entries (edition, provenance, SHA-256, clauses); B1 updates under change control |
| OD-11 items | §6 | As recorded in B1 §6.3 / §6.5 | Per §7 | Per §6 | Per §6 | Per §6 (Decision and Verification Closure Evidence columns) |

Gate H (configuration management) is not an exception. Under OD-1 it must
be satisfied before exit approval takes effect.

---

## 6. Closure Milestones and Accountable Roles

Formally authorized roles: Project Owner and Technical Review Support
(§2.2). Roles marked "(Proposed — Pending Owner Confirmation)" have not
been formally delegated.

Objective closure evidence is split into:

- **Decision Closure Evidence**: the controlled record that resolves the
  open decision or definition.
- **Verification Closure Evidence**: the executed test or inspection
  evidence that demonstrates the resolved behaviour.

A decision record alone is not proof of implementation or verification.
No closure evidence listed below exists yet; every entry describes evidence
still to be produced. Test IDs are those named in B2 §8 for the item.
Where an exact closure artifact is not yet defined, the entry reads
"TBD — requires controlled definition before the relevant milestone."

| Item | Source status (unchanged) | Classification | Required milestone | Accountable role | Decision Closure Evidence | Verification Closure Evidence |
|---|---|---|---|---|---|---|
| STD-3 / OQ-1 / OQ-2 / OQ-S1 | OPEN / UNVERIFIED | CONDITIONAL exception; future architecture-freeze decision | Before relevant codec and reconnection freeze | Project Owner; Technical Review Support | Owner-approved interpretations of OQ-1, OQ-2 and OQ-S1 recorded in B1 §6.2 under change control; dedicated STD-3 acceptance criterion added to B1 (OD-2 restriction 4) | Dedicated STD-3 verification test defined in B2 before PHASE 07 and executed with recorded evidence. Test artifact: TBD — requires controlled definition before the relevant milestone. |
| RQ-01 CAN-1 recovery events | PARTIALLY RESOLVED (D-07); events OPEN | Future architecture-freeze decision | PHASE 02 architecture freeze | Project Owner; Architecture Role (Proposed — Pending Owner Confirmation) | Owner-approved PHASE 02 architecture-freeze record defining the attempt-start event, recovery-complete event, inter-attempt interval and operator-restart mechanism; B1 §6.5 RQ-01 updated under change control | Executed evidence for VT-AC09A-02 and VT-AC09A-03, after any B2 update required by the decision |
| RQ-04 SAF-1 scope ("while charging") | OPEN | Future architecture-freeze decision | PHASE 02 architecture freeze | Project Owner; Architecture Role (Proposed — Pending Owner Confirmation) | Owner-approved PHASE 02 architecture-freeze record defining "while charging" / "active charging phase" against the PHASE 01.2 steps; B1 §6.5 RQ-04 updated under change control | Executed evidence for VT-AC06-01, VT-AC08-08 and VT-AC14-08, after any B2 update required by the decision |
| RQ-05 peer-loss behaviour (CAN-5, VG-09) | OPEN | Future architecture-freeze decision | PHASE 02 architecture freeze | Project Owner; Architecture Role (Proposed — Pending Owner Confirmation) | Owner-approved PHASE 02 architecture-freeze record defining peer-loss handling in steps without an applicable wait; B1 §6.5 RQ-05 updated under change control | TBD — requires controlled definition before the relevant milestone. (B2 §8: steps without an applicable wait are not tested in VT-AC05-07.) |
| RQ-02 SAF-3 staleness | OPEN | OPEN technical issue | Before PHASE 06 implementation | Project Owner | Owner decision on the staleness threshold recorded; B1 §6.5 RQ-02 updated under change control | Executed evidence for VT-AC07-05 with the stale-request case assessed against the decided threshold, after the required B2 update |
| RQ-03 SAF-5 CST reason | OPEN | OPEN technical issue | Before PHASE 06 implementation | Project Owner | Owner decision on the CST stop reason recorded; B1 §6.5 RQ-03 updated under change control | Executed evidence for VT-AC07-07 with the reason asserted, after the required B2 update |
| RQ-08 ERR-6 stop reason | OPEN | OPEN technical issue | Before PHASE 06 implementation | Project Owner | Owner decision on the SPN3521/SPN3511 stop-reason value recorded; B1 §6.5 RQ-08 updated under change control | Executed evidence for VT-AC07-06 and VT-AC14-04 with the value asserted, after the required B2 update |
| OQ-12(a) zero-current (VG-16) | OPEN (ODC-01) | OPEN technical issue | Before PHASE 06 implementation | Project Owner; Technical Review Support | Owner decision on the meaning of a zero-current demand recorded; B1 §6.5 OQ-12(a) / zero-current updated under change control | TBD — requires controlled definition before the relevant milestone. (B2 §8: zero-current demand not tested in VT-AC07-01, VT-AC07-02, VT-AC07-09.) |
| TO-17 classification | OPEN (ODS-01) | OPEN technical issue | Before PHASE 07 verification | Project Owner | Owner amendment or confirmation of the ODS-01 TO-17 disposition recorded; B1 §6.5 and §6.6 updated under change control | Currently observation only (VT-AC14-S01). Gating verification if TO-17 is classified safety-relevant: TBD — requires controlled definition before the relevant milestone. |
| TO-25 / U-T1 | EXTERNAL; OPEN | OPEN technical issue (EXTERNAL) | After external source verification, before affected acceptance | Project Owner; Technical Review Support | GB/T 18487.1-2023 registered per OD-9 (edition, provenance, SHA-256, clause B.3.7.3); clause review recorded; duration and start condition recorded in B1 under change control | Executed evidence for VT-AC05-06 with the duration assessed. Duration pass/fail criterion: TBD — requires controlled definition before the relevant milestone. |
| VG-06 / ERR-4 | Test defined; not verified | Future verification obligation | Interface in PHASE 02; execution evidence in PHASE 07 | Project Owner; Architecture Role (Proposed — Pending Owner Confirmation); Verification Role (Proposed — Pending Owner Confirmation) | Owner-approved PHASE 02 architecture record defining the abstract fault-reporting boundary and the per-class counters and logs | Executed evidence for VT-AC14-09 (stimuli S1–S5) in PHASE 07. VT-AC14-09 S5 applies a synthetic physical-layer fault indication at the abstract software fault-reporting boundary. It verifies software classification and diagnostic reporting only; it does not verify detection of real physical-layer faults. Actual physical-layer fault generation and hardware robustness remain SHOULD scope (B1 §6.3, VG-06; PROJECT_SCOPE.md §5.2). |
| VG-07 / CAN-1 TEC/REC | PARTIALLY DEFINED; OPEN (ODC-01) | Future verification obligation | Evidence basis before relevant hardware verification | Project Owner; Verification Role (Proposed — Pending Owner Confirmation) | Owner decision on whether injected TEC/REC values are sufficient or hardware verification is required; B1 §6.5 VG-07 / CAN-1 TEC/REC updated under change control | Executed evidence for VT-AC09A-01 and VT-AC09B-S01. Hardware evidence, if required by the decision: TBD — requires controlled definition before the relevant milestone. |
| NF-03 COM-4 interval metric | OPEN — owner decision pending | Future verification obligation | Owner-approved metric before PHASE 07 | Project Owner; Verification Role (Proposed — Pending Owner Confirmation) | Owner-approved interval metric recorded; B1 §6.5 NF-03 and B2 updated under change control | Executed evidence for VT-AC03-01, VT-AC03-02, VT-AC04-05, VT-AC05-04 and VT-AC08-01 with interval verdicts under the approved metric |
| NF-12 Spec §30 measurable detail | OPEN — owner decision pending | Future verification obligation | Owner-approved acceptance decision before PHASE 07 | Project Owner | Owner decision on adoption recorded; B1 §5 updated under change control if adopted | TBD — requires controlled definition before the relevant milestone. (Depends on the adoption decision; B2 §8 lists all tests as affected.) |
| OQ-6 / GAP-07 | OPEN | CONDITIONAL exception | OD-3 | Project Owner | Per §5 (OD-3) | Per §5 (OD-3) |
| Reverse coverage | Not produced | CONDITIONAL exception | OD-4 | Project Owner; Technical Review Support | Per §5 (OD-4) | Per §5 (OD-4) |
| C-1 | Documentation limitation | CONDITIONAL exception | OD-5 | Project Owner; Verification Role (Proposed — Pending Owner Confirmation) | Per §5 (OD-5) | Per §5 (OD-5) |
| C-2 / TO-09 | Coverage limitation | CONDITIONAL exception; future verification obligation | OD-6 | Project Owner; Architecture Role (Proposed — Pending Owner Confirmation); Verification Role (Proposed — Pending Owner Confirmation) | Per §5 (OD-6) | Per §5 (OD-6) |
| All other B1 §6.3 and §6.5 entries | As recorded | OPEN technical issues | Existing B1 §6.3 / §6.5 statuses and milestones | As recorded | As recorded | As recorded |

---

## 7. PHASE 02 Handover Restrictions

Controlled PHASE 02 inputs: B1 v0.3.3; B2 v0.1.6; the three PHASE 01.2
deliverables (OD-8); D-01 to D-10, ODS-01 to ODS-07, ODC-01 and OD-1 to
OD-11; B1 §4 allocation (provisional).

| Area | May be designed provisionally | Must not be frozen or implemented without further evidence or authorization |
|---|---|---|
| Codec / protocol database | Table-driven structure; scaling held as data | U-P2 bit numbering; SPN2817 and SPN3613 scaling (OQ-1, OQ-2); SPN2822 unit (OQ-3); CTS year byte order (OQ-5) |
| Transport protocol | Module boundary and session handling | SAE J1939-21 T1–T4, abort codes and identifier bit assignment |
| State machines | Structure; one TO-ID per wait, including a distinct TO-09; C10P | OQ-S3, OQ-S7, OQ-S8, OQ-S9, OQ-S10 sequence, OQ-S11; RQ-04; RQ-05; any transition freeze before reverse coverage (OD-4) |
| Timers and safety | TO-14, TO-15, TO-16 T±1 ms design (ODS-01); TO-17 and TO-25 as configurable timers | A safety classification for TO-17; any TO-25 value |
| Reconnection | Configurable, disabled by default (OD-2) | Any BSM reconnection behaviour (OQ-S1); reconnection counter scope (RQ-06) |
| CAN-1 recovery | D-07 semantics | Attempt-start and recovery-complete events, interval and operator-restart mechanism (RQ-01) until defined at the PHASE 02 freeze |
| Fault reporting | Abstract fault-reporting boundary and per-class counters and logs needed by VT-AC14-09 | Any claim that ERR-4 is verified; any claim of real physical-layer fault detection. VT-AC14-09 S5 is synthetic software fault injection, and passing S5 does not establish real physical-layer fault detection. Actual physical-layer fault generation and hardware robustness remain SHOULD scope (§6, VG-06). |
| Charger application | Magnitude comparison (D-06); PROT-10 output inhibit (D-08) | Zero-current semantics (OQ-12(a)); RQ-02, RQ-03 and RQ-08 values |
| CTS | Optional, configurable extension point | Any transmission level or policy (OD-3) |
| HAL | Existing Linux HAL (L04/L05) as an input for review | Treating the existing HAL as a frozen architecture; HAL interfaces not yet defined are not specified by this record |

Starting PHASE 02 requires a separate explicit Project Owner Exit Approval.

---

## 8. Configuration Management Authorization

OD-10 authorizes subsequent Git archival of PHASE 01 in principle. Staging
and commits are a separate controlled execution step after this record has
been reviewed.

The archive shall:

1. Preserve accepted B1 and B2 content and their verified hashes (§1).
2. Preserve the PHASE 01.1 to 01.4 deliverables.
3. Preserve the Owner Decision records and this record.
4. Keep L05 development work separate from PHASE 01.
5. Exclude sandbox files and unrelated personal configuration files.
6. Exclude the copyrighted standard PDF from Git.
7. Never use `git add -A` or `git add .`; stage by explicit path allowlist
   only.
8. Verify every commit and the final tree against the approved path list.

The commit sequence proposed in the Formal Exit Review may be used as a
starting point; exact paths shall be rechecked against the repository at
execution time.

---

## 9. Explicit Non-Authorizations

This record does not authorize:

1. PHASE 01 exit or its formal approval.
2. Start of PHASE 02 or any implementation work.
3. Architecture freeze or state-machine freeze.
4. Any GB/T 27930-2023 compliance claim, or any production compliance
   claim.
5. Closure, downgrade or VERIFIED marking of STD-3 or any other OPEN,
   EXTERNAL or NOT VERIFIED item.
6. Any change to a requirement, priority, acceptance criterion, protocol
   value, timeout value or test definition.
7. Modification of B1, B2, PROJECT_SCOPE.md, PROJECT_CHARTER.md, the
   PHASE 01.2 documents, existing decision records, the Standard Source
   Register or Spec v2.0.
8. A Technical Review approval.
9. Unsafe physical operation.
10. Staging, committing, tagging or pushing.

---

## 10. Approval Record and Revision History

| Role | Name | Status |
|---|---|---|
| Project Owner — decisions OD-1 to OD-11 | Project Owner | Approved (2026-11-09) |
| Technical Review | Technical Review | Pending — not performed (OD-7) |
| PHASE 01 Exit Approval | Project Owner | Pending — separate future decision |
| Project Owner — v0.1.1 documentation revision | Project Owner | Superseded by v0.1.2 — not separately owner-accepted. |
| Project Owner — P1-ODR-002 v0.1.2 documentation revision | Project Owner | OWNER-ACCEPTED (2026-11-09) |

| Version | Date | Change |
|---|---|---|
| 0.1.0 | 2026-11-09 | Initial record of Owner Decisions OD-1 to OD-11 following the PHASE 01 Formal Exit Review. No requirement, AC, priority, protocol value, timeout value or OPEN-item status changed. |
| 0.1.1 | 2026-11-09 | PHASE 01.4-D controlled revision R-1 to R-6 following the P1-ODR-002 read-only review. (R-1) Objective closure evidence clarified: Decision Closure Evidence and Verification Closure Evidence columns added to §6 for all OD-11 milestone entries, with explicit TBD where no closure artifact is yet defined; no evidence is claimed to exist. (R-2) OD-7 reclassified as a governance and approval arrangement (§2.1) and removed from the Controlled Exception Register; OD-1 condition 3 applies to the remaining technical exceptions; no technical closure milestone assigned to OD-7. (R-3) Technical Review status worded "Pending — not performed (OD-7)". (R-4) Full SHA-256 references for the three PHASE 01.2 deliverables, each matching its v0.1.0 prefix. (R-5) P1-ODR-001 cited as "P1-ODR-001 (proposed)". (R-6, Option A) Role authorization clarified (§2.2): only Project Owner and Technical Review Support are formal roles; all other roles are Proposed — Pending Owner Confirmation; PROJECT_CHARTER.md §6.1 quoted verbatim. Status set to DRAFT — PENDING PROJECT OWNER REVIEW. No change to the substantive decisions OD-1 to OD-11, closure milestones, OPEN / EXTERNAL dispositions, requirement classifications, acceptance criteria, protocol values or timeout values. |
| 0.1.2 | 2026-11-09 | PHASE 01.4-F controlled revision following the PHASE 01.4-E final independent review. (F-1) VG-06 synthetic physical-fault verification boundary clarified in §6 (VG-06 Verification Closure Evidence) and §7 (Fault reporting): VT-AC14-09 S5 is synthetic software fault injection at the abstract fault-reporting boundary, verifies software classification and diagnostic reporting only and does not establish real physical-layer fault detection; actual physical-layer fault generation and hardware robustness remain SHOULD scope. VG-06 remains not verified. (F-2) Technical Review Support responsibility clarified by a separate role-clarification note after the OD-2 accountability statement; the approved OD-2 wording is unchanged. (F-3) Document Control: historical decision-content approval (OD-1 to OD-11, v0.1.0) and current revision approval (pending, v0.1.2) separated into distinct fields. Status remains DRAFT — PENDING PROJECT OWNER REVIEW. No substantive change to OD-1 to OD-11, the OD-11 milestones, OPEN / EXTERNAL dispositions, requirement classifications, acceptance criteria, protocol values or timeout values. |
| 0.1.2 (formal acceptance) | 2026-11-09 | Formal Project Owner acceptance of P1-ODR-002 v0.1.2 recorded. The §1 Document Control and §10 Approval Record have been reconciled, and v0.1.1 is explicitly identified as superseded without separate formal acceptance. Metadata-only acceptance recording. No substantive changes to OD-1 through OD-11, the 14 OD-11 milestones, technical requirements, OPEN/EXTERNAL dispositions or governance decisions. |

**End of PHASE 01 Exit Owner Decision Record**
