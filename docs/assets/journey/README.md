# docs/assets/journey: screenshots for the project journey

**No images are stored here yet.** Every item below is *recommended evidence to add*. Add only
original captures from the owner's bench, debugger or Windows build. Do not add re-creations or mock-ups.

| # | Suggested file | Content | Status |
|---|---|---|---|
| 1 | `architecture_diagram.png` | Layer diagram: board application → integration layer → protocol core / HAL contract → ports (G474 FDCAN, F103 bxCAN, Linux SocketCAN, fake HAL). It may be drawn from [`../../ARCHITECTURE.md`](../../ARCHITECTURE.md) §1. | recommended evidence to add |
| 2 | `hardware_topology.jpg` | Photo or drawing of the bench: NUCLEO-G474RE, F103 A (transceiver carrier), F103 B, CANH/CANL wiring, two 120 Ω terminations ([`../../hardware/HARDWARE_TOPOLOGY.md`](../../hardware/HARDWARE_TOPOLOGY.md)) | recommended evidence to add |
| 3 | `g474_armcc_build_pass.png` | µVision build output of `Project1_G474`: "0 Error(s), 0 Warning(s)" with the Code/RO/RW/ZI line (FI-04 build: 31336 / 1076 / 372 / 6116) | recommended evidence to add |
| 4 | `f103_armcc_build_pass.png` | µVision build output of `Project1_F103`: "0 Error(s), 0 Warning(s)" (FI-04 build: 21914 / 814 / 336 / 5816) | recommended evidence to add |
| 5 | `dual_debug_session.png` | Both boards in debug sessions at the same time: G474 at charging step 10 and F103 B in its charging step | recommended evidence to add |
| 6 | `fi04_watch_results.png` | G474 Watch window after an FI-04 run: `g_hw_diag_gbt_step = 0x2A`, `timeouts = 1`, `out_enabled = 0`, `inhibit_latched = 1`, `g_gbt_fi04.pass_count = 1`, `fail_count = 0`, `invalid_count = 0`, `max_valid_ms = 1` | recommended evidence to add |
| 7 | `event_log_evidence.png` | G474 event log entries of the same run: step 10 → 40 and 40 → 42, 1000 ms apart | recommended evidence to add |
| 8 | `fi04_10_run_summary.png` (or `.md` table) | Per-run table for the 10 FI-04 runs: run number, date, final step, `pass/fail/invalid`, `max_valid_ms`. Only from values actually recorded. | recommended evidence to add |

When an image is added:
- record its capture date and the source revision (commit SHA) it shows;
- reference it from [`../../PROJECT_JOURNEY.md`](../../PROJECT_JOURNEY.md);
- change its status here to "added".

Hardware results in the current documents are labelled **owner-reported**. Adding these captures
supports those statements but does not change the recorded verdicts.
