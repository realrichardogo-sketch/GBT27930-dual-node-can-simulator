# ACCEPTANCE_CRITERIA — STEP 07-C2.4 hardware TP session

Evaluate **≥ 2 s after both nodes show `g_hw_diag_gbt_step` = 41** (END), without halting before
that. One reset of both boards = one session. Repeat at least 3 sessions.

## 1. Firmware self-evaluation (`g_gbt_accept_fail`, both boards)

The firmware evaluates the criteria below every 100 ms from the same sources as the Watch structs
(`gbt_watch_update`, `Core/Src/main.c`). Host replicas of this mask are asserted 0 in the port-level
SIL sessions (`tests/sil/test_port_g474_session.c`, `test_port_f103_session.c`).

| Bit | Criterion (bit set = NOT met) | Source |
|---|---|---|
| 0 | node step = 41 (END) | node status |
| 1 | BRM: ≥ 1 completed TP session (G474 received / F103 sent) | integration per-message tally |
| 2 | BCP: ≥ 1 completed TP session | " |
| 3 | BCS: ≥ 1 completed TP session | " |
| 4 | no TP Abort sent or received | TP stats `abort_tx`, `abort_rx` |
| 5 | no TP timeout (T1/T2/T3/T4) | TP stats `timeout_tx`, `timeout_rx` |
| 6 | no transport failure | node `transport_failures` |
| 7 | both TP state machines IDLE | `tp.tx.state`, `tp.rx.state` |
| 8 | no RX overflow (software queue, hardware FIFO, node-reported) | CAN status, node diag |
| 9 | no invalid frame rejected by the RX ISR | CAN status `rx_invalid` |
| 10 | no unexpected discard: node `rx_invalid`, `outbox_overflow`, `tx_discarded`, TP `outbox_full` all 0 | node / TP |
| 11 | no TX failure (`g_gbt_can_diag.tx_failures` = 0) | port + integration |
| 12 | CAN error active, no error-passive and no bus-off event | port |
| 13 | BRM snapshot frozen with 49 bytes | TP snapshot |
| 14 | no application timeout (BEM/CEM path), no output-sink failure, no API error ("unexpected safety error") | node diag |
| 15 | no HAL error (error callbacks, RX read errors) | port |
| 16 | TX accounting consistent: `tx_confirmed + tx_pending = tx_frames` | port |
| 17 | no completed TP session with an unexpected PGN | integration tally |

`g_gbt_accept_pass = 1` ⇔ mask 0.

## 2. Criteria checked by the operator (one USB at a time, see HARDWARE_TEST_PROCEDURE.md)

| # | Check | Pass |
|---|---|---|
| A1 | G474 `g_gbt_accept_pass` | 1 (`g_gbt_accept_fail` = 0) |
| A2 | F103 B `g_gbt_accept_pass` | 1 |
| A3 | G474 step path seen live / in `g_gbt_evlog` STEP entries | 2 → 3 → 4 → 5 → 7 → 8 → 9 → 10 → 13 → 41 |
| A4 | F103 B step path in `g_gbt_evlog` | 22 → 23 → 25 → 26 → 29 → 30 → 31 → 32 → 33 → 41 |
| A5 | G474 `g_gbt_tp_diag`: `rx_started = rx_completed = brm+bcp+bcs completed`, `tx_started = 0` | equal, ≥ 3 |
| A6 | F103 B `g_gbt_tp_diag`: `tx_started = tx_completed = brm+bcp+bcs completed`, `rx_started = 0` | equal, ≥ 3 |
| A7 | BRM TP sequence in G474 `g_gbt_evlog` (first RTS with pgn 512) | RX CM 16 (b 49) → TX CM 17 (b 7, c 1) → RX DT 1..7 → TP_END (a 1, b 7, c 7) → TX CM 19 |
| A8 | same in F103 B log | TX CM 16 → RX CM 17 → TX DT 1..7 → RX CM 19 → TP_END (a 4, b 7, c 7) |
| A9 | `g_gbt_brm_rx_snapshot` (G474) = `g_gbt_brm_tx_snapshot` (F103 B) = expected bytes (TP_SESSION_FLOW.md §3) | 49/49 equal, both `_state` = 2 |
| A10 | `g_gbt_can_diag` both: `state` 1, `tec` 0, `rec` 0, `tx_failures` 0, all event/overflow counters 0 | yes |
| A11 | `max_rx_dt_interval_ms` (G474), `max_tx_dt_interval_ms` (F103) | ≤ 50 ms (expected ≈ 10–12) |
| A12 | `g_gbt_evlog.frozen` | 0 (if 1: log full, information only — counters remain authoritative) |
| A13 | F103 B stack watermark (painted stack, after END) | ≤ 768 B used of 1024 B, paint bottom intact |

Cross-board consistency (different sessions, so counts may differ by ±1 BCS): G474 `rx_completed`
and F103 B `tx_completed` both ≥ 3 and with BRM = BCP = 1.

## 3. Not acceptance criteria

* Exact BCS count (timing dependent; SIL 9).
* `g_gbt_can_diag.tx_backpressure` (normal retry mechanism).
* `rx_ignored` of the node.
* Frame counters after END (the charger keeps sending CSD).
