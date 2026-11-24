# Source document provenance (K-9a / B1-03)

Imported unchanged on 2026-11-16 during STEP 07-B0 Batch 1. Every file was
verified against the approved SHA-256 value (STEP 07-A Revision 3.2 §0.2)
before and after copying. `SOURCES.sha256` is the machine-checkable manifest
(`sha256sum -c SOURCES.sha256`); CTest test `source_manifest` re-checks it.

| File | Origin (repository `~/projects/gbt27930-sim`, HEAD `3853ea6`) | Git state at origin | SHA-256 |
|---|---|---|---|
| 01.2_PROTOCOL_TABLES.md | docs/phase01/01.2_PROTOCOL_TABLES.md | tracked | 148c6d39d1108d6b36c8797cb9030a6cbb98c11af95827c1e05d8adf752b7433 |
| 01.2_STATE_MACHINES.md | docs/phase01/01.2_STATE_MACHINES.md | tracked | 1a16de29c5b3c1f549bf4e746c7d4c55a81d7feb54f88a12e19fdfaf2badedcb |
| 01.2_TIMEOUT_TABLE.md | docs/phase01/01.2_TIMEOUT_TABLE.md | tracked | 6a83191df462cdda9ad12071d1fad3aaa07c90fde95942e878ee6acb7ba0cc38 |
| REQUIREMENTS_BASELINE.md (v0.3.3) | docs/REQUIREMENTS_BASELINE.md | tracked | 31dd01af1936296cd7a61ab6f7f92a21343c8beaec6898322b5f9b736ee709b6 |
| VERIFICATION_TEST_DEFINITION.md (v0.1.6) | docs/phase01/01.4/VERIFICATION_TEST_DEFINITION.md | tracked | b0a55dd06f924cb3bc369af8c2ff0b6e800564f86db554f0351a9713fb3bf8d8 |
| PHASE01_EXIT_OWNER_DECISION_RECORD.md (v0.1.2) | docs/phase01/PHASE01_EXIT_OWNER_DECISION_RECORD.md | tracked | 97ec2505ecfeb669c0c639aca7fa0cf876e5f7defeb7c50688ae41f0ca2f7af5 |
| PHASE01_EXIT_APPROVAL_RECORD.md | docs/phase01/PHASE01_EXIT_APPROVAL_RECORD.md | **untracked** in origin repository | 03c09ca70f9053cb38419eb89a9f9703fbda9efeb2291136b9eaf3fd22f97fcd |
| STANDARD_SOURCE_REGISTER.md | docs/reference/STANDARD_SOURCE_REGISTER.md | tracked | 9a7cb7b5f18e4b8cbfcd7a5f37df83c4583970311ca71d918ea0fe385bea394d |

## Referenced but not imported

| Document | Location | SHA-256 | Status |
|---|---|---|---|
| GB/T 27930-2023 (PDF, 130 pages) | `~/projects/gbt27930-sim/reference/standards/GBT+27930-2023.pdf` (outside this repository) | f9eb420ee3bd8b2ba3640e1830bc1b2011433e024bbdda585d01cecc2e7992a6 | Referenced by path and hash only (K-9a) |
| SAE J1939-21:2006 | not available | — | EXTERNAL; required by K-4 (TP) and cited by GB/T §7.1, §7.3, §7.5, §7.7 |
| SAE J1939-11:2006 / J1939-15:2018 | not available | — | EXTERNAL (physical layer) |
| GB/T 18487.1-2023 | not available | — | EXTERNAL |

This repository is therefore **not** self-contained for normative evidence.

## Page-reference convention

The imported 01.2 documents cite the standard as printed page (p.) and PDF
page (pdf), with pdf = p. + 4.
