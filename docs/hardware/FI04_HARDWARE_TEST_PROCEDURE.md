# FI04_HARDWARE_TEST_PROCEDURE — G474 shutdown response time on an F103 B reset

Status: **prepared, NOT executed.** The owner builds, flashes and runs it. Nothing was flashed by
the implementer.

Safety:
* **No battery, no power stage, no load.** The setup is the existing dual-node CAN bench only.
* The result is **software command acceptance by the ND-1 sink** on the G474. It is **not** a
  physical current-shutdown measurement and must not be reported as one.

## 0. Prerequisites

1. Both targets rebuilt from this working copy with ARMCC (`FI04_BUILD_REPORT.md` §2), 0 errors.
   The map files contain `g_gbt_fi04`.
2. Both boards flashed with the new images and *Verify OK* (HARDWARE_TEST_PROCEDURE.md §4).
   **Do not** flash the 2026-11-18 outputs.
3. Wiring, F103 A passive and start-order rules of HARDWARE_TEST_PROCEDURE.md §0–§2 apply unchanged.
4. Only the G474 is on USB (Run 1 configuration, HARDWARE_TEST_PROCEDURE.md §6). F103 B runs from
   its own supply and has an accessible RESET button.

## 1. What is being tested

The F103 B (vehicle) is reset while the G474 (charger) is charging (step 10).
* The G474 stops receiving BCL.
* After TO-14 (1000 ms) its BCL timer expires: `gbt_node_enter_timeout` → `gbt_safety_disable`.
  This is the FI-04 event.
* The G474 delivers the disabled command to the sink, then goes 10 → 40 (C-TO, sends CEM) → 42 (EXIT).

FI-04 measures from the start of the integration pass before the one that handled the timeout (T0)
to the sink's acceptance of the disabled command (T1).

**Timing constraint.** With the unchanged F103 configuration (SOC 500 → 520 ‰ at 10 ‰/s), step 10
lasts only **≈ 2 s** before the vehicle stops normally (BST).
* Press F103 B RESET as soon as `g_hw_diag_gbt_step` shows **10**.
* If the step path goes 10 → 12/13 → 41 instead of 10 → 40, the reset came too late. The run is
  then a normal BST stop and **not** an FI-04 run. Record it as "not FI-04", power-cycle per
  §3 step 9, and repeat.

## 2. Watch window (G474 project)

Add the existing list (HARDWARE_TEST_PROCEDURE.md §5), plus:

| Expression | Meaning | Expected after a valid FI-04 run |
|---|---|---|
| `g_gbt_fi04.shutdowns` | accepted + node-confirmed enabled→disabled transitions | **1** |
| `g_gbt_fi04.open` | 1 = disabled command delivered but not yet accepted + confirmed | **0** |
| `g_gbt_fi04.ref_flags` | 0 valid; 0x01 no previous pass, 0x02 RX limit, 0x04 RX error, 0x08 overflow, 0x10 status read failed | **0** |
| `g_gbt_fi04.verdict` | 1 PASS, 2 FAIL (≥ 10 ms), 3 INVALID reference, 0 none | **1** |
| `g_gbt_fi04.elapsed_ms` | T1 − T0 (ms) | **≤ 9** |
| `g_gbt_fi04.t0_ms`, `g_gbt_fi04.t1_ms` | T0 / T1 (`HAL_GetTick`) | t1 ≥ t0 |
| `g_gbt_fi04.first_seq`, `g_gbt_fi04.accept_seq`, `g_gbt_fi04.confirmed_seq` | output sequence that opened / was accepted / was confirmed by the node | accept = confirmed (= first unless a retry happened) |
| `g_gbt_fi04.apply_status` | last `gbt_hal_output_apply` result while open | **0 (GBT_HAL_OK)** |
| `g_gbt_fi04.refused`, `.mismatch`, `.aborted` | sink refusals / unconfirmed OK / superseded | **0 / 0 / 0** |
| `g_gbt_fi04.pass_count`, `.fail_count`, `.invalid_count`, `.max_valid_ms` | totals since reset | 1 / 0 / 0 / = elapsed |
| `g_hw_diag_gbt_step`, `g_hw_diag_gbt_timeouts`, `g_hw_diag_gbt_err_fields` | step, timeouts, CEM bytes | 42 (or 40), 1, BCL-timeout CEM bytes |
| `g_hw_diag_out_enabled`, `g_hw_diag_out_shutdown_count` (legacy) | sink state / legacy counter | 0 / 1 |
| `g_hw_diag_out_state`, `g_hw_diag_out_seq` | node output state (0 accepted, 1 pending, 2 failed) / last delivered seq | **0** (no pending command) / = `accept_seq` |
| `g_gbt_evlog` | event log | contains STEP a=40 b=10 |

`g_gbt_fi04` is refreshed every 100 ms. Read it ≥ 2 s after the step change. If `g_gbt_evlog.frozen`
is 1, the log was full (128 entries) before the event. The step-40 entry is then missing: record
that and rely on the Watch values.

## 3. Per-run steps (repeat until 10 valid FI-04 runs are recorded)

1. F103 B unpowered. G474 on USB. *Debug → Start/Stop Debug Session* (G474 stops at `main`).
   Set up the Watch window and enable *View → Periodic Window Update*.
2. Power F103 B (or press its RESET). It waits in step 22.
3. Press **Run (F5)** on the G474. **Never halt until step 8.**
4. Watch `g_hw_diag_gbt_step`: 2 → 3 → 4 → 5 → 7 → 8 → 9 → **10**.
5. As soon as it shows **10**, press **F103 B RESET** once. Note the G474 `g_gbt_tp_diag.now_ms`
   (or the wall-clock time) as the reset mark.
6. Wait ≥ 3 s. Expected: step → 40 about 1 s after the reset, then 42.
7. Record every item of §4 (screenshot of the Watch window, plus a Memory-window export of
   `g_gbt_evlog` entries 0..`count`−1).
8. Stop the debug session.
9. Before the next run, power-cycle or reset **F103 B after** the G474 was stopped (start-order
   rule 3). The G474 restarts from `main` with all counters zeroed (one session per reset).

## 4. Evidence per run

| # | Item | Source |
|---|---|---|
| 1 | Run number, date/time, operator | — |
| 2 | F103 reset mark (G474 `g_gbt_tp_diag.now_ms` when RESET was pressed) | Watch |
| 3 | Step path | `g_gbt_evlog` STEP entries: must contain 10 → 40 |
| 4 | `t_ms` of the evlog STEP entry a=40 b=10 | event log |
| 5 | `g_gbt_fi04.t0_ms`, `t1_ms`, `elapsed_ms` | Watch |
| 6 | `shutdowns`, `open`, `ref_flags`, `verdict` | Watch |
| 7 | `first_seq`, `accept_seq`, `confirmed_seq`, `apply_status` | Watch |
| 8 | `refused`, `mismatch`, `aborted` | Watch |
| 9 | `g_hw_diag_gbt_timeouts`, `g_hw_diag_gbt_err_fields`, `g_hw_diag_out_enabled` | Watch |
| 10 | CAN overflow counters (`g_gbt_can_diag`) | Watch |

Consistency checks per run:
* `t0_ms ≤ evlog(step 40).t_ms ≤ t1_ms`. The step entry is time-stamped with the pass start.
* `t1_ms − evlog(step 40).t_ms` ≤ elapsed.
* `evlog(step 40).t_ms − reset mark` ≈ 1000 ms + boot/arbitration. This is informative only:
  the reset mark is manual.

## 5. Acceptance (all must hold)

* **10 valid FI-04 runs**, i.e. runs whose step path contains 10 → 40. Runs that ended via BST are
  not counted.
* Every run: `shutdowns = 1`, `open = 0`, `ref_flags = 0`, `verdict = 1`, `apply_status = 0`,
  `accept_seq = confirmed_seq = g_hw_diag_out_seq`, `refused = mismatch = aborted = 0`,
  `g_hw_diag_out_state = 0` (no pending output command).
* **max(`elapsed_ms`) over the 10 runs ≤ 9 ms.**
* Any run with `verdict = 3` (INVALID) or `ref_flags ≠ 0` is **not** a PASS. Record it, investigate
  it (overflow, RX limit) and repeat; do not discard it silently.
* Any run with `elapsed_ms ≥ 10` (verdict 2) is a **FAIL** of FI-04.

## 6. Results table

| Run | Reset mark (ms) | evlog step-40 t_ms | t0_ms | t1_ms | elapsed_ms | shutdowns | open | ref_flags | verdict | first/accept/confirmed seq | apply_status | refused/mismatch/aborted | Valid FI-04 run? |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | | | | | | | | | | | | | |
| 2 | | | | | | | | | | | | | |
| 3 | | | | | | | | | | | | | |
| 4 | | | | | | | | | | | | | |
| 5 | | | | | | | | | | | | | |
| 6 | | | | | | | | | | | | | |
| 7 | | | | | | | | | | | | | |
| 8 | | | | | | | | | | | | | |
| 9 | | | | | | | | | | | | | |
| 10 | | | | | | | | | | | | | |

Summary: valid runs ___ / 10; max elapsed ___ ms; invalid ___; fail ___.

Optional (not part of FI-04 acceptance): the normal vehicle stop (BST, no reset) gives the same
record for the BST-triggered shutdown.
