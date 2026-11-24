# docs/hardware: bench documents (verbatim from the hardware-tested package)

These files are unchanged copies from the STEP 07-C2.4 / FI-04 package root. They are the documents the
bench work was done with, so they keep their original wording and file references.

| File | Use |
|---|---|
| `HARDWARE_TOPOLOGY.md` | boards, wiring, bit timing, filters, transceiver notes, open wiring checks |
| `HARDWARE_TEST_PROCEDURE.md` | normal dual-node session: start order, flashing, Runs 1/2, F103 stack watermark (§8), troubleshooting |
| `FI04_HARDWARE_TEST_PROCEDURE.md` | FI-04 communication-loss test and its Watch variables |
| `ACCEPTANCE_CRITERIA.md` | firmware self-evaluation mask bits 0–17 and operator criteria A1–A13 |
| `WATCH_VARIABLES.md` | meaning and pass values of every Keil Watch variable |
| `TP_SESSION_FLOW.md` | expected TP frame sequence and payload bytes |

**References that point elsewhere in this repository:**

| Reference in these files | Now |
|---|---|
| `README.md`, `BUILD_REPORT.md` (C2.4 package root) | not published (historical package documents); current build steps in [`../VERIFICATION.md`](../VERIFICATION.md) §6 |
| `FI04_BUILD_REPORT.md` | [`../evidence/FI04_BUILD_REPORT.md`](../evidence/FI04_BUILD_REPORT.md) (path-redacted copy) |
| `regression/gbt27930-sim-lean/...` | `host/...` |
| `source/...` | unchanged (`source/...`) |

**Status notes:**
- Statements such as "ARMCC NOT YET VERIFIED" or "hardware not tested" describe the package state on
  2026-11-18.
- The current status is in [`../VERIFICATION.md`](../VERIFICATION.md). ARMCC builds succeeded on 2026-11-18 and
  2026-11-23. FI-04 was run 10/10 (owner-reported). The normal-session criteria A1–A13 have no recorded results.
