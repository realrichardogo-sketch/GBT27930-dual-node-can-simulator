# FI04_IMPLEMENTATION_REPORT — STEP 07 FI-04 shutdown response-time instrumentation

Date: 2026-11-23. Working copy (only modification target):
`/home/<user>/projects/step07_fi04_working/STEP07_C2.4_DUAL_NODE_TP_TEST_20261118_172749/`.
The D-drive package and the original WSL export were not touched.

Status: **implemented and host-verified. ARMCC build not run (Windows, owner). Hardware FI-04
ten-run test NOT performed.**

> **Scope of the measurement.** FI-04 measures **software command acceptance**: the moment the
> ND-1 output sink accepted the disabled output command (`gbt_hal_output_apply() == GBT_HAL_OK`).
> It is **not** a measurement of physical power-stage current decay. Project 1 has no power
> stage (`app/output_if.h`: "stores the command (no power stage)").

## 1. Baseline and backup (before any change)

| Item | Result |
|---|---|
| Original manifest `SHA256SUMS.txt` | sha256 `bd1ce980bb8a2fc27b95a3c2de53e49cb055c5bbc6e338dc25391d1aa5a0758b` (as authorised); **unchanged after implementation** |
| `sha256sum -c SHA256SUMS.txt` before change | 2469 OK, 2 FAILED = the two documented `.uvguix.<user>` exceptions |
| F103 `Project1_F103.uvguix.<user>` | `8b7ce94e…d1b719` before and after (untouched) |
| G474 `Project1_G474.uvguix.<user>` | `075cba76…f03146` before and after (untouched) |
| Files outside the manifest (explained, not mismatches) | 41 regression files: the `SYMLINKS.txt` links became regular files on the Windows copy; all 41 byte-identical to their `source/` targets. 260 Keil ARMCC outputs (`MDK-ARM/Project1_*/`, 2026-11-18). The manifest itself. A harness-created empty `.claude/.cc-writes/` directory (excluded everywhere). |
| Backup | `~/project_backups/STEP07_FI04_PRE_IMPL_20261123_185208/` (new; existing backups untouched). 2773 files; source and backup hash lists identical (list sha256 `a95cf13f…ac88e3`); `diff -r` clean. Details: `FI04_BACKUP_VERIFICATION.md`. |

Consequence of the link loss: the host regression compiles `regression/gbt27930-sim-lean/app/*`,
which are now **copies**. Both changed SharedCore files were therefore copied byte-identically into
the regression tree (checked with `cmp`; all 41 copies still identical to `source/`).

## 2. Defects addressed

| Audit ID | Defect | Resolution |
|---|---|---|
| D1 | T1 was stale: `sim_plant` timestamps with `s_gbt_now`, sampled before `gbt_integ_step()`, so trigger and acceptance shared one tick | T1 = `gbt_hal_now_ms()` read immediately after `gbt_hal_output_apply()` returns (`gbt_integration.c:129`). Proven by `scenario_fi04_t1_fresh` (a 3 ms slow sink is visible in T1) and by a stale-T1 mutant that this test kills. |
| D2 | `sim_plant.first_disable_ms` latched by the initial seq-1 command, never re-armed on target | The new record never counts the initial command (`acc_enabled` starts 0). `sim_plant` unchanged. |
| D4 | `output_shutdowns` counts even when the sink refuses | New counter `fi04.shutdowns` counts only accepted + confirmed shutdowns. The legacy `output_shutdowns` / `g_hw_diag_out_shutdown_count` is **unchanged** (preserved verified diagnostics); see §6. |
| D3 | No RX timestamps | Not changed (needs a HAL change). Handled by the conservative T0 (§3). |

## 3. Measurement definition (as implemented)

* **T0** = start time (`now` at `gbt_integration.c:369`) of the integration pass **preceding** the
  pass in which the disabled command was first delivered to the sink. Why it is never later than
  the event that caused the shutdown:
  * an RX frame handled in pass P was not in the queue when pass P‑1 drained it (`GBT_HAL_E_EMPTY`);
  * a protocol timer that expires in pass P had not expired at pass P‑1's `gbt_node_tick(now)`;
  * an API call made between passes (`gbt_node_request_stop`, `set_connected`) came after P‑1 began;
  * a disable created inside `gbt_node_confirm_output` (ND-7, enabled command refused in C10) is
    delivered in the same or the next pass, so it still happened after P‑1 began.
  A command first delivered outside a pass (`gbt_integ_detach`) uses the start of the most recent pass.
* **T1** = `gbt_hal_now_ms()` right after `gbt_hal_output_apply()` returned `GBT_HAL_OK`, and only if
  the node then confirmed exactly that sequence (`node->out_accepted_seq == cmd.seq`).
* **elapsed** = `T1 − T0`, unsigned 32-bit (wrap-safe). With 1 ms ticks the true latency is
  < elapsed + 1 ms, so **PASS ⇔ elapsed ≤ 9 ms** (`GBT_FI04_LIMIT_MS`); **elapsed ≥ 10 ms ⇒ FAIL**.
* **Reference validity** — `ref_flags`, 0 = valid. These HAL facts were checked in the code before
  the logic was written:
  * `gbt_hal_can_receive` returns `GBT_HAL_OK` or `GBT_HAL_E_EMPTY` normally.
  * It returns `E_PARAM`, `E_STATE` or `E_NOT_READY` on misuse: `port_g474.c:398-408`,
    `port_f103.c:354-364`, `fake_hal.c:232-246`.
  * `gbt_hal_can_get_status` returns OK, `E_PARAM`, `E_STATE` or `E_NOT_READY`.

  | Flag | Set when |
  |---|---|
  | `0x01 NO_PREV` | no previous pass exists |
  | `0x02 RX_LIMIT` | previous pass popped `GBT_INTEG_RX_PER_STEP` (16) frames, so the queue may not have been drained (conservative even if it happened to be empty) |
  | `0x04 RX_ERROR` | previous pass ended its RX loop with a status other than `GBT_HAL_E_EMPTY` |
  | `0x08 OVERFLOW` | an RX overflow was reported in the previous pass, or in the current pass before the command was delivered |
  | `0x10 STATUS_ERR` | `gbt_hal_can_get_status()` failed in those passes, so overflow could not be checked |

  An invalid reference gives verdict **INVALID**. It is never PASS and is excluded from `max_valid_ms`.
* **Counting.** A measurement opens when a new sequence with `enabled == 0` is delivered while the
  last command accepted by the sink was enabled.
  * It closes only on `GBT_HAL_OK` plus node confirmation of that sequence.
  * A refusal (`refused++`) keeps it open, so the retry time is inside the latency.
  * An accepted re-enable before that closes it as `aborted` and does not count it.
  * OK without confirmation (`mismatch++`) keeps it open.
  * Repeated disables cannot double count: an unchanged command gets no new sequence
    (`gbt_safety.c:25-28`), and `acc_enabled` is 0 after the first accepted disable.
* **ZERO_CURRENT** (`enabled = 1`, 0 mA, SAF-3) is not a shutdown and is not measured.

## 4. Changes (exact)

| File | Lines (post) | Change | Why necessary |
|---|---|---|---|
| `source/Project1_SharedCore/app/gbt_integration.h` | 57-138, 179 | `gbt_fi04_t` record, `GBT_FI04_*` constants, 4 bookkeeping functions; field `fi04` appended **at the end** of `gbt_integ_t` (existing offsets unchanged; `evlog` still at 169) | storage and test interface for the measurement |
| `source/Project1_SharedCore/app/gbt_integration.c` | 7-116 | pure bookkeeping (`gbt_fi04_pass_begin/_note/_end`, `gbt_fi04_on_apply`, `fi04_open/_close`) | measurement logic, unit-testable without HAL |
| same | 123, 129, 139 (`apply`) | fresh T1 after the sink; record after `gbt_node_confirm_output` (that call is unchanged) | T1 and acceptance confirmation |
| same | 362, 370, 375, 380-382, 384-385, 398-402, 413 (`gbt_integ_step`) | pass start, overflow / status / RX-limit / RX-error notes; RX status kept in `rs` (same loop semantics: break on any non-OK) | T0 and its validity |
| `regression/gbt27930-sim-lean/app/gbt_integration.[ch]` | — | byte-identical copies of the two files above | regression copies are no longer links |
| `source/Project1_G474/Core/Src/main.c` | 244-247 (USER CODE PV), 438 (USER CODE 4, `gbt_app_diag`) | `volatile gbt_fi04_t g_gbt_fi04;` copied every 100 ms | Keil Watch: all Watch variables are public globals (WATCH_VARIABLES.md); `s_gbt_integ` is file-static. `scripts/check_user_code.sh`: PASS (5 lines, all inside USER CODE, markers identical, CRLF 711/0) |
| `regression/…/tests/sil/test_sil_session.c` | 263-302, 994-1356, `main` (1358-) | FI-04 checks in 9 existing scenarios; unit tests; 4 new SIL scenarios | required tests |
| `regression/…/tests/sil/test_port_g474_session.c` | 268-288 | FI-04 checks on the real G474 port (HAL_GetTick fake) | port-level evidence |
| `regression/…/tests/sil/test_port_f103_session.c` | 222-228 | vehicle: no shutdown measured | F103 behaviour unchanged |

**Not modified:**
* `protocol/`, `hal/hal.h`, `ports/`, `sim_plant.*`, `output_if.h`;
* F103 `main.c`, any CubeMX file, `.ioc`, `.uvprojx`, both `.uvguix.<user>` files;
* CMake (no new test executable), the original `SHA256SUMS.txt`.

No DWT/CYCCNT, no STM32 register, no new HAL function.

## 5. Behaviour preservation

* Protocol: no file under `protocol/` changed. The integration layer only **reads**
  `node->out_accepted_seq` (it already read `node->tp`, `node->step`).
* `gbt_node_confirm_output(…, gbt_hal_now_ms())` is kept literally, so the node receives the same
  value as before. The added `gbt_hal_now_ms()` at line 129 is a read-only clock call; the
  port implementations are `HAL_GetTick()` (`port_g474.c:411-417`, `port_f103.c:367-373`).
* Trace equivalence (`scripts/trace_equiv.c`, 5 dual-node scenarios, every CAN frame with time,
  sender, id, dlc, data, plus final node state):
  * original sim-lean, pre-change and post-change are **byte-identical**;
  * 1186 lines, sha256 `fcc0b360…1c7a94`.
* Pre-existing test output is identical except for the check-count lines (stdout diff of
  `test_sil_session`, `test_port_g474_session` and `test_port_f103_session`).
* TX gating, output sequencing, CAN IDs, timers, TP and state transitions: unchanged by construction,
  and confirmed by the identical traces.

## 6. Deviations and decisions to record

1. **Working copy location** changed by the owner from `D:` to the WSL working copy (authorised).
2. **Legacy `output_shutdowns` kept as is.** It still counts a refused enabled→disabled delivery
   (SIL sink-refusal scenario: legacy 1 while `fi04.shutdowns` was 0 during the refusal).
   Changing it would alter a verified STEP 07-C2 Watch value. **Use `g_gbt_fi04.shutdowns` for FI-04.**
3. **Bookkeeping functions are public** (`gbt_fi04_*`) so the RX-error and sequence-mismatch cases,
   which a correct HAL/node cannot produce, can be unit-tested. They are app-layer only.
4. **Tests live in existing files.** No new test executable, so no CMake change; this follows the
   "relevant existing test files" preference. The unit-level cases are in `test_sil_session.c`.
5. **RX_LIMIT is conservative.** A pass that pops exactly 16 frames is flagged even if the queue
   was then empty.
6. **G474 Watch copy is refreshed every 100 ms.** Read it ≥ 2 s after the event; the live record is
   `s_gbt_integ.fi04`.
7. **Stale build outputs.** The Keil outputs dated 2026-11-18 in `MDK-ARM/Project1_*` predate FI-04.
   Do not flash them; rebuild first (`FI04_BUILD_REPORT.md`).

## 7. Resource impact (GCC arm-none-eabi estimates; ARMCC `.map`/`.htm` are authoritative)

| Item | G474 (-O0) | F103 (-O3) |
|---|---|---|
| `sizeof(gbt_integ_t)` | 172 → 260 B (+88; `gbt_fi04_t` = 88 B) | same |
| RAM total | +88 (`s_gbt_integ`) +88 (`g_gbt_fi04`) = **+176 B** | **+88 B** |
| `gbt_integration.o` text | 2271 → 3105 B (+834) | 1815 → 2723 B (+908) |
| Stack frames (`-fstack-usage`) | `apply` 56→64, `gbt_integ_step` 88→96, new `gbt_fi04_on_apply` 24 + `fi04_close` 24 | `apply` 40→48, `gbt_integ_step` 96 (=), `gbt_fi04_on_apply` 8 |
| Previous ARMCC max stack (2026-11-18 `.htm`) | 392 B | 432 B |

Worst-case stack growth is a few tens of bytes, inside the 768 B F103 budget
(HARDWARE_TEST_PROCEDURE.md §8). It must be confirmed from the new ARMCC `.htm`.

## 8. Open items / risks

* ARMCC V5.06u7 build of both targets: not run (no ARMCC in WSL).
* Hardware ten-run FI-04 test: not run; it requires the owner (`FI04_HARDWARE_TEST_PROCEDURE.md`).
* T0 is "detectable event" (pass boundary), not "fault occurrence". The fault-to-detection time
  (TO-14 1000 ms BCL timeout after an F103 reset) is a protocol requirement outside the 10 ms
  criterion. There is no BCL arrival time on hardware (the event log records TP frames and step
  changes only).
* Margin: 1 ms tick quantisation; PASS needs ≤ 9 ms. The G474 main-loop period at -O0 is unmeasured.
  An elapsed value well above 1–2 ms on hardware would indicate a loop stall.
* RX ISR latency (frame in FIFO before the ISR copies it) is not covered: µs range, inferred.
* No physical-output evidence is possible in Project 1.

Verdict: see `FI04_TEST_REPORT.md` §6.
