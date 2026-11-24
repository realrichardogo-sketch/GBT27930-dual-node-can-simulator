# FI04_TEST_REPORT — STEP 07 FI-04 shutdown response-time instrumentation

Date: 2026-11-23. Host: WSL2 Linux, gcc (host, strict flags `-std=c99 -pedantic-errors -Wall -Wextra
-Werror -Wconversion`), arm-none-eabi-gcc (cross checks). Build directories were outside the
working copy (`$TMPDIR/fi04/build_base`, `build_post`). Logs: `evidence/fi04/`.

**Scope:** software command acceptance only. No hardware was run.

## 1. Regression: before and after

| Suite | Pre-change (working copy, unmodified) | Post-change |
|---|---|---|
| CMake ctest, 30 entries | 29 passed, 1 skipped (`linux_sil_vcan`: no `vcan0`, exit 77 = NOT RUN) | **29 passed, 1 skipped (same)**; host build: 0 warnings |
| `test_sil_session` | 718 checks, 0 failures | **1087 checks, 0 failures** |
| `test_port_g474_session` | 282 / 0 | **297 / 0** |
| `test_port_f103_session` (+ ASan/UBSan variant in ctest) | 429 / 0 | **439 / 0** |
| unit tests (ring, outbox, timer, id, codec, bits, codec_ext, tp, hal_fake, port_linux, spsc, spsc_signal, port_g474/_asan, port_f103/_asan) | pass | pass |
| `architecture_rules`, `protocol_no_writable_statics`, `source_manifest` | pass | pass |
| `cross_c99_cortex_m`, `cross_c99_spsc`, `cross_c99_g474`, `cross_c99_f103` | pass | pass |

**`linux_sil_vcan` (two-process SocketCAN SIL) was NOT RUN** before or after, because `vcan0` is
missing in this WSL instance. It is not reported as PASS. Its build target (`gbt_charger_node`,
`gbt_vehicle_node`, which link `gbt_integration.c`) compiles and links without warnings.

Logs: `pre_ctest.log`, `post_ctest.log`, `pre/post_host_build.log`, `pre/post_test_*.stdout`.

## 2. Protocol behaviour unchanged

* **Trace equivalence** (`scripts/trace_equiv.c`, five dual-node SIL scenarios; every CAN frame
  with time, sender, id, dlc and data, plus the final node state):
  * original sim-lean (`~/projects/gbt27930-sim-lean`), pre-change and post-change: **identical**;
  * 1186 lines, sha256 `fcc0b360f774163a0d0f4568f430e0ebf94000d22f65d0a5b78e74e45f1c7a94`;
  * files: `trace_orig.txt`, `trace_pre.txt`, `trace_post.txt`.
* **Test stdout.** Pre-existing output lines of the three session tests are identical pre/post. The
  only differences are the added FI-04 lines and the check counts.
* No file under `protocol/`, `hal/` or `ports/` changed (`FI04_CHANGED_FILES.txt`).

## 3. Required FI-04 tests

The unit level drives the bookkeeping functions directly: `scenario_fi04_unit`,
`test_sil_session.c:1030`. The integration level runs `gbt_integ_step()` with the fake HAL and
the sim plant.

| # | Requirement | Unit level | Integration level |
|---|---|---|---|
| 1 | enabled→disabled transition | yes: T0 1001, T1 1003, elapsed 2, PASS, seq 3 | 9 existing scenario runs plus G474 port session; all `shutdowns = 1`, PASS or the expected INVALID |
| 2 | repeated disabled command | same seq twice + another disabled seq → `shutdowns` stays 1 | `scenario_fi04_repeated_stop`: 3 stop requests → 1 sequence, `shutdowns = 1` |
| 3 | sink returns error | 2 refusals then OK: never counted while refused, `apply_status` E_IO→OK, T0 of the first delivery kept, elapsed 4; abort case | `scenario_fi04_sink_refusal`: 20 ms of E_IO → `refused 20`, first seq 3, accepted seq 23, elapsed 21 ms → **FAIL** (correctly) |
| 4 | sequence mismatch | OK but node confirmed an older seq → `mismatch 1`, not counted, stays open; closed by the next confirmed disable | not producible with a correct node (`apply()` delivers `node->out_cmd`); unit level only |
| 5 | invalid T0 reference | no previous pass (inside and outside a pass) → INVALID `0x01`, not PASS, `max_valid_ms` 0; outside a pass after passes → T0 = last pass start | — |
| 6 | RX frame limit | limit in previous pass → INVALID `0x02`; limit only in the current pass after the command → still valid | `scenario_fi04_rx_limit`: queue filled to exactly 16, no overflow → stop → INVALID `0x02` |
| 7 | unexpected RX HAL error | `RX_ERROR` in previous pass → INVALID `0x04` | not injectable through the fake HAL without breaking the sink (fake returns E_STATE/E_NOT_READY for receive *and* apply); covered by unit test and code review (`gbt_integration.c:384-402`) |
| 8 | RX overflow | previous pass, or current pass before the command → INVALID `0x08`; status read failure → `0x10` | T-5 software queue overflow and hardware FIFO overflow scenarios → INVALID `0x08`, never PASS |
| 9 | timestamp wraparound | T0 0xFFFFFFFF, T1 0x00000002 → elapsed 3, PASS | — (SIL clock starts at 100000) |
| 10 | 9 ms PASS boundary | elapsed 9 → PASS, `max_valid_ms` 9 | — |
| 11 | 10 ms FAIL boundary | elapsed 10 → FAIL, `fail_count` 1 | sink-refusal scenario: 21 ms → FAIL |

Additional test: **T1 freshness** (`scenario_fi04_t1_fresh`). A sink that takes 3 ms of virtual time
gives plant acceptance at 101067, T1 101070 and elapsed 4. This proves T1 is read after the sink
returned (audit defect D1).

**Mutation check** (`post_mutation_check.log`, temporary copies only):

| Mutant | Result |
|---|---|
| `GBT_FI04_LIMIT_MS` 9 → 10 | 3 failures |
| T1 replaced by the pre-pass clock sample | 2 failures |

Both mutants are killed.

### Integration-level results (charger, node 0)

| Scenario | Trigger latency (existing metric) | FI-04 elapsed | ref_flags | Verdict |
|---|---|---|---|---|
| normal session + vehicle stop (order 0 / 1) | 1 / 0 ms | 1 / 1 ms | 0 | PASS |
| charger stop | 1 | 1 | 0 | PASS |
| BCL timeout (TO-14) | 1 | 1 | 0 | PASS |
| CCS timeout (BEM) | 1 | 1 | 0 | PASS |
| out-of-range current | 1 | 1 | 0 | PASS |
| voltage minimum (T-3c) | 1 | 1 | 0 | PASS |
| RX software queue overflow / hardware FIFO overflow | 1 / 1 | 1 / 1 | 0x08 | INVALID (by design) |
| repeated stop requests | 1 | 1 | 0 | PASS |
| RX frame limit | 1 | 1 | 0x02 | INVALID (by design) |
| sink refusal 20 ms | — | 21 | 0 | FAIL (by design) |
| G474 port session (real `port_g474.c`, HAL_GetTick fake) | — | 1 | 0 | PASS, `accept_seq` = node = port `accepted_seq` = plant `last.seq` |
| F103 port session (vehicle) | — | — | — | no shutdown measured (`shutdowns 0`), as expected |

In every valid case, FI-04 elapsed ≥ the existing trigger latency. The bound is never optimistic
compared with the scenario's own trigger timestamp.

SIL limits:
* The SIL clock advances 1 ms per tick and does not run between calls, so SIL proves logic and
  bookkeeping, not CPU time.
* The real G474 loop time is measured only by the hardware run.

## 4. Board-source and rule checks

| Check | Result |
|---|---|
| `scripts/gcc_board_compile.sh` (every `.c` of both `.uvprojx`, Keil optimisation levels, `-Werror`, SharedCore with `-Wconversion`) | pre: PASS; **post: PASS** (G474 43 files, F103 36 files, 0 failed). GCC only, not ARMCC. |
| `scripts/check_user_code.sh` G474 `main.c` vs backup | **PASS**: 5 lines added, all inside USER CODE; marker sequence identical; CRLF 711 / LF 0 |
| Regression copies vs `source/` (41 files of `SYMLINKS.txt`) | 41 identical, 0 differing |
| `g_gbt_fi04` in G474 `main.o` (GCC) | present, 0x58 = 88 B |

## 5. Not done (must not be read as passed)

* **ARMCC builds** (G474, F103): not run. Instructions are in `FI04_BUILD_REPORT.md`.
* **Hardware FI-04 ten-run test**: not run. Procedure: `FI04_HARDWARE_TEST_PROCEDURE.md`.
* `linux_sil_vcan`: not run (no `vcan0`).
* Physical power-stage shutdown: out of scope for Project 1; cannot be measured.

## 6. Verdict

**CONDITIONALLY APPROVED**

The implementation meets the software requirements on the host:
* all regressions pass;
* the protocol trace is unchanged;
* every required FI-04 case is tested;
* the threshold and T1 freshness are proven by mutation.

Conditions before FI-04 can be declared accepted:
1. ARMCC V5.06u7 rebuild of both targets: 0 errors, new warnings recorded, `g_gbt_fi04` in the G474
   map, and F103 maximum stack ≤ 768 B from the new `.htm`.
2. Ten valid hardware FI-04 runs per `FI04_HARDWARE_TEST_PROCEDURE.md` §5:
   * every run: one shutdown, no invalid reference, no pending command;
   * max elapsed ≤ 9 ms.
3. All reporting is limited to software command acceptance by the ND-1 sink, not physical
   current shutdown.

**Hardware acceptance is NOT claimed.**
