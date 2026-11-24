# PHASE 01 Exit Approval Record

## A. Document Control

| Field | Value |
|---|---|
| Project | GB/T 27930-2023 System A Charger-BMS Simulator |
| Project ID | P1 |
| Document | PHASE 01 Exit Approval Record |
| Document ID | Not assigned (no existing repository numbering convention covers approval records) |
| Version | Not assigned |
| Approval Date | 2026-11-09 |
| Decision | CONDITIONAL PASS |
| Decision Authority | Project Owner. Controlling reference: P1-ODR-002 §2.1 and OD-7 (owner-only exit approval path). Supplementary, non-controlling reference: PROJECT_CHARTER.md §6.1 (the Charter is DRAFT and has not been accepted as a controlling document). |
| Record Date | 2026-11-09 |
| Related Controlled Documents | `docs/REQUIREMENTS_BASELINE.md` (B1, P1-DOC-003) v0.3.3, OWNER-ACCEPTED; `docs/phase01/01.4/VERIFICATION_TEST_DEFINITION.md` (B2, P1-B2-001) v0.1.6, OWNER-ACCEPTED; `docs/phase01/PHASE01_EXIT_OWNER_DECISION_RECORD.md` (P1-ODR-002) v0.1.2, OWNER-ACCEPTED |
| Record Purpose | Standalone, traceable record of the Project Owner's PHASE 01 exit approval decision |

### A.1 Source of the recorded decision

The approval was granted by the Project Owner in the project
conversation. It is recorded here on the basis of the Project Owner's
statement in the PHASE 02.1 Documentation Close-out authorization
(2026-11-09). The approval conversation is not stored in the
repository; this record transcribes the Owner's stated decision and does
not independently verify it. No signature, approval token or commit
reference is associated with the approval itself.

Before this record was created, the repository recorded the PHASE 01
Exit Approval as pending: P1-ODR-002 §10 Approval Record, row "PHASE 01
Exit Approval | Project Owner | Pending — separate future decision". That
row is not modified by this record (see Section G).

---

## B. Approval Statement

The Project Owner grants PHASE 01 a CONDITIONAL PASS exit approval based
on the approved exit criteria and the controlling Owner Decisions.

A CONDITIONAL PASS is not unconditional completion. PHASE 01 exits with
owner-approved exceptions. Each exception keeps its restriction,
accountable role and closure milestone as recorded in P1-ODR-002 §5 and
§6.

---

## C. Approval Basis

The controlling exit basis is defined by P1-ODR-002 OD-1 ("PHASE 01 Exit
Criteria", Decision: APPROVED):

| Basis element | Source | Role in the exit decision |
|---|---|---|
| B1 §9 | `docs/REQUIREMENTS_BASELINE.md` §9 "Acceptance of This Document" (8 criteria) | Acceptance criteria for the Requirements Baseline document. B1 v0.3.3 is OWNER-ACCEPTED (B1 Document Control; Revision History rows "0.3.3 (acceptance)" and "0.3.3 (approval metadata alignment)"). |
| Gates A–H | PHASE 01 Formal Exit Review gates, as listed in P1-ODR-002 §3 | Project-level phase exit assessment framework. |
| P1-ODR-002 | `docs/phase01/PHASE01_EXIT_OWNER_DECISION_RECORD.md` v0.1.2 | Approved disposition of the exceptions and conditions (OD-1 to OD-11). |
| OD-1 | P1-ODR-002 §4 | Adopts the three elements above; states that Spec v2.0 §34 is not adopted as an authoritative normative exit standard. |

OD-1 conditions for a CONDITIONAL PASS (P1-ODR-002 §4, OD-1):

1. All non-excepted exit gates pass.
2. Every exception is explicitly owner-approved.
3. Each exception has a documented restriction, responsible role and
   closure milestone.
4. No unapproved FAIL or NOT ESTABLISHED condition remains.

OD-1 also states: "Gate H configuration-management requirements must be
satisfied before exit approval takes effect."

Gate results as recorded in P1-ODR-002 §3 (Exit Review results; not
re-rated by this record):

| Gate | Subject | Exit Review result (P1-ODR-002 §3) |
|---|---|---|
| A | Source authority and evidence integrity | CONDITIONAL PASS |
| B | Protocol-definition completeness | CONDITIONAL PASS |
| C | Requirements completeness and Owner Acceptance | PASS |
| D | Test-definition completeness and Owner Acceptance | PASS |
| E | Decision and OPEN-item traceability | CONDITIONAL PASS |
| F | Safety-critical unresolved issues | CONDITIONAL PASS — requires Owner exceptions |
| G | Architecture readiness | CONDITIONAL PASS |
| H | Approval and configuration-management readiness | FAIL (current state; correctable without content change) |

### C.1 Gate H chronology and relationship to OD-1

The Gate H result above is the original Exit Review assessment as
recorded in P1-ODR-002 §3. It is preserved unchanged; this record does not
re-rate Gate H. The events relevant to Gate H occurred in this order:

| # | Event | Source | Evidence class |
|---|---|---|---|
| 1 | Original Gate H assessment: "FAIL (current state; correctable without content change)". At that time no PHASE 01 deliverable was committed to Git. | P1-ODR-002 §3 | Repository (recorded result) |
| 2 | Configuration-management actions required: OD-1 states "Gate H configuration-management requirements must be satisfied before exit approval takes effect"; P1-ODR-002 §5 states "Gate H (configuration management) is not an exception"; OD-10 and P1-ODR-002 §8 authorize archival in principle under eight rules. | P1-ODR-002 §4 (OD-1, OD-10), §5, §8 | Repository |
| 3 | Completion of the six archival commits on `main`, 2026-11-09 15:43:28 to 15:44:12 (+0800), HEAD `3853ea65c475a5e6d785f9587986ef2ea3cf5a53` (commits listed below). | Git history | Repository |
| 4 | Project Owner grants the PHASE 01 CONDITIONAL PASS exit approval, 2026-11-09, after the archival. | Project Owner statement (Section A.1) | Owner-reported; exact time not recorded |

Status of the P1-ODR-002 §8 archival rules against the repository:

| §8 rule | Status | Evidence |
|---|---|---|
| 1. Preserve accepted B1 and B2 content and verified hashes | Satisfied | Committed B1 and B2 are byte-identical to the accepted files (SHA-256 `31dd01af1936296cd7a61ab6f7f92a21343c8beaec6898322b5f9b736ee709b6` and `b0a55dd06f924cb3bc369af8c2ff0b6e800564f86db554f0351a9713fb3bf8d8`) |
| 2. Preserve the PHASE 01.1 to 01.4 deliverables | Satisfied | Commits `a3329e9c…`, `4aa262cb…`, `aa2c5014…`, `1fe4c426…`; committed content equals the working-tree files |
| 3. Preserve the Owner Decision records and P1-ODR-002 | Satisfied | Commits `aa2c5014…` and `3853ea65…` |
| 4. Keep L05 work separate | Satisfied | `docs/L05_DEVELOPMENT.md` and `docs/PROJECT_HISTORY.md` are not in the six commits and remain uncommitted |
| 5. Exclude sandbox and personal configuration files | Satisfied | None of the 12 sandbox entries is tracked |
| 6. Exclude the standard PDF | Satisfied | `git ls-files reference/` is empty; the PDF is ignored by `.gitignore:32` |
| 7. Stage only by explicit path allowlist | NOT VERIFIED from repository evidence | The staging method is not recorded in Git. The archival execution report (conversation record, not stored in the repository) states that only explicit-path staging was used. Each commit contains exactly its authorized paths. |
| 8. Verify every commit and the final tree against the approved path list | NOT VERIFIED from repository evidence | The per-commit verification is recorded only in the archival execution report (conversation record). Re-inspection of the commit file lists for this record found exactly the 13 authorized paths. |

Relationship to OD-1: the configuration-management content requirements
of P1-ODR-002 §8 (rules 1–6) are satisfied by the six archival commits,
which were completed before the Owner's exit approval. Rules 7 and 8 are
procedural and are supported only by the conversation record. Gate H
also has an approval component; that component is addressed by OD-7
(Technical Review not a prerequisite) and is not a configuration-management
requirement. Whether Gate H as a whole would now be rated differently is
not assessed here.

Archival commits:

| Commit | Message |
|---|---|
| `f56b9fa867901d6f0b80d00e015dce7d35965151` | chore(config): archive PHASE 01 ignore rules |
| `a3329e9c9f75e49a3a34e93bbeb2d0075ec6f117` | docs(phase01.1): archive standard source documents |
| `4aa262cb923edb819e49d5e3b9b15fb932942ce6` | docs(phase01.2): archive protocol definition documents |
| `aa2c5014f85c9d7c48c5201517fa598f21e273ea` | docs(phase01): archive accepted requirements and decisions |
| `1fe4c42671673ba7db263a25ec6be0dbb42150fc` | test(phase01.4): archive accepted verification definition v0.1.6 |
| `3853ea65c475a5e6d785f9587986ef2ea3cf5a53` | docs(phase01): archive accepted exit owner decision record v0.1.2 |

Re-rating Gate H is not part of this record.

---

## D. Owner Decision Restrictions (P1-ODR-002 OD-1 to OD-11)

The following decisions remain in force exactly as recorded in
P1-ODR-002 v0.1.2 §4. The summaries below quote or closely follow that
text; P1-ODR-002 governs in case of any difference.

| Decision | Recorded decision (P1-ODR-002 §4) | Key conditions preserved |
|---|---|---|
| OD-1 | APPROVED — PHASE 01 Exit Criteria | Exit basis B1 §9, Gates A–H and P1-ODR-002; Spec v2.0 §34 not adopted; the four CONDITIONAL PASS conditions; Gate H configuration-management requirements satisfied before exit approval takes effect. |
| OD-2 | APPROVED — OPTION A (carry into PHASE 02 as a controlled exception) | STD-3 remains MUST / OPEN / UNVERIFIED. (1) SPN2817 unit scaling (OQ-1) and SPN3613 offset (OQ-2) remain configurable and PROVISIONAL. (2) BSM reconnection (OQ-S1) remains configurable and disabled by default. (3) No final codec scaling or BSM reconnection behaviour may be frozen before the corresponding interpretations are resolved. (4) A dedicated STD-3 acceptance criterion and verification test must be added before PHASE 07. (5) Project release remains blocked while STD-3 is OPEN. STD-3 must never be downgraded or marked VERIFIED merely because PHASE 01 exits. |
| OD-3 | APPROVED — CARRY AS OPEN | OQ-6 and GAP-07 remain OPEN; no MUST/SHOULD classification; no final CTS transmission policy; no assumption that CTS must be transmitted or omitted; PHASE 02 may retain an optional/configurable design extension point but must not freeze CTS behaviour without further evidence and authorization. Milestone: before the relevant PHASE 02 architecture freeze, or a further explicit Owner Decision. |
| OD-4 | APPROVED — ACCEPT AS A CONTROLLED EXCEPTION | Reverse-coverage matrix not required before PHASE 01 exit; required before the PHASE 02 State Machine Freeze; scope GB/T 27930-2023 System A clauses 5–11 and normative Annex A; each applicable clause maps to requirement ID(s), an explicit scope exclusion with rationale, or an identified coverage gap; no claim of completeness before the matrix has been produced and audited. |
| OD-5 | APPROVED — ACCEPT AS A NON-BLOCKING DOCUMENTATION LIMITATION | C-1: B2 v0.1.6 not reopened; individual test definition fields remain authoritative; the B2 §7 summary omits at least two test-level limitations (PROT-6, PROT-12); no exact corrected total asserted; authorized B2 revision with a complete recount before PHASE 07. |
| OD-6 | APPROVED — ACCEPT WITH MILESTONE REQUIREMENTS | C-2: (1) PHASE 02: model TO-09 as a distinct canonical timeout ID; (2) before PHASE 07: update B2 under change control with an explicit TO-09 wait-identity verification case; (3) PHASE 07: execute the test and collect evidence. The coverage limitation is not closed. |
| OD-7 | APPROVED — OWNER-ONLY EXIT APPROVAL PATH (governance arrangement, P1-ODR-002 §2.1) | The Project Owner may approve PHASE 01 exit without an independent Technical Review sign-off; Technical Review completion is not a prerequisite; Technical Review status remains "Pending — not performed (OD-7)"; no Technical Review approval may be fabricated; Owner approval is not independent technical verification. |
| OD-8 | APPROVED — ACCEPT AS CONTROLLED PHASE 02 WORKING INPUTS | The three PHASE 01.2 documents are DRAFT / WORKING-USE ACCEPTED; no formal approval of every provisional interpretation; no claim of final protocol completeness; no state-machine freeze; no authorization to implement unresolved behaviours; the PHASE 02 architecture gate remains responsible for final design decisions and freeze approval. |
| OD-9 | APPROVED | Acquisition of legitimate, lawfully accessible copies of SAE J1939-21, SAE J1939-11, SAE J1939-15 and GB/T 18487.1-2023 authorized, with edition, provenance, SHA-256 (where a local copy is kept) and clause references recorded; unreviewed dependencies remain EXTERNAL / UNVERIFIED. Milestones: U-P2 bit numbering before PHASE 02 codec freeze; SAE J1939-21 transport parameters and abort codes before relevant TP implementation and VT-AC04 assessment; GB/T 18487.1-2023 B.4.3, B.4.4 and Figure A.1 note d before affected PHASE 06 implementation; GB/T 18487.1-2023 B.3.7.3 / TO-25 before affected PHASE 06/07 acceptance. Acquisition does not constitute verification. |
| OD-10 | APPROVED — CONTROLLED PHASE 01 ARCHIVAL (in principle) | Archival by explicit path allowlist; no `git add -A` / `git add .`; L05 work kept separate; sandbox files and the standard PDF excluded (P1-ODR-002 §8). Executed in the six commits listed in Section C. |
| OD-11 | APPROVED — CARRY INDIVIDUALLY WITH EXPLICIT RESTRICTIONS | The 14 milestones of P1-ODR-002 §6 are adopted; other OPEN items keep their B1 §6.5 statuses and milestones; no RQ, OQ, GAP, VG, NF or TO entry is blanket-closed; none of the exceptions authorizes unsafe physical operation or a production compliance claim. |

---

## E. Conditional Pass Limitations

- CONDITIONAL PASS does not mean that every OPEN issue has been resolved.
  All OPEN, EXTERNAL and NOT VERIFIED items keep their recorded
  dispositions (B1 §6; P1-ODR-002 §5 and §6).
- CONDITIONAL PASS does not establish full GB/T 27930-2023 compliance.
- Test definitions (B2) do not establish test execution. All B2 tests
  remain DEFINED / NOT_EXECUTED.
- Technical Review has not been performed. Its status remains
  "Pending — not performed (OD-7)".
- The three PHASE 01.2 documents are not formally frozen (OD-8).
- Architecture Freeze has not been approved.
- Project Release remains subject to its existing blocking conditions,
  including STD-3 (OD-2 restriction 5).
- PROJECT_CHARTER.md and PROJECT_SCOPE.md remain DRAFT and are not
  accepted by this record (P1-ODR-002 §3.1 item 3).

---

## F. PHASE 02 Boundary

Three separate approvals are distinguished:

| Approval | Status |
|---|---|
| PHASE 01 Exit Approval | CONDITIONAL PASS granted by the Project Owner, 2026-11-09 (this record). |
| PHASE 02 start authorization | A separate authorization. Stated by the Project Owner to have been previously granted; not recorded in a repository document before this record. |
| PHASE 02 Architecture Freeze approval | Not granted. Remains a separate future approval gate. |

This record does not approve, freeze or accept any PHASE 02
architecture. The PHASE 02 start authorization does not imply
Architecture Freeze authorization. The PHASE 02 handover restrictions of
P1-ODR-002 §7 remain in force.

---

## G. Record Integrity

This is an additive traceability record. It does not amend, replace or
supersede B1, B2, P1-ODR-002 or any previously accepted Owner Decision
(D-01 to D-10, ODS-01 to ODS-07, ODC-01, OD-1 to OD-11).

P1-ODR-002 §10 still shows "PHASE 01 Exit Approval | Project Owner |
Pending — separate future decision". That row is not modified by this
record. Any alignment of P1-ODR-002 with this record requires a separate
authorized controlled change.

**End of PHASE 01 Exit Approval Record**
