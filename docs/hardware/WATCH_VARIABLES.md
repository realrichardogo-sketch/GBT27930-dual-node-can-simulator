# WATCH_VARIABLES — STEP 07-C2.4

All variables are **public globals** of `Core/Src/main.c` (type the name in a Keil Watch window;
no file qualifier, no `s_gbt_node` expressions). They are written only in the main loop
(`gbt_app_diag` every 100 ms, the event log by the integration loop), never by an ISR, and never
read by the firmware. Enable *View → Periodic Window Update*; never halt during a session.

Names are identical on both boards unless marked G474 / F103.

## 1. `g_gbt_tp_diag` (struct, all fields uint32_t)

| Field | Meaning | Normal final value (≥ 2 s after END) |
|---|---|---|
| `now_ms` | HAL_GetTick() at the refresh | increasing |
| `tp_role` | 1 TP sender (F103 B), 2 TP receiver (G474) | F103 1 / G474 2 |
| `tx_started` / `tx_completed` | send sessions opened (RTS queued) / completed (EOM_ACK accepted) | F103: equal, ≥ 3. G474: 0 / 0 |
| `rx_started` / `rx_completed` | receive sessions opened (RTS accepted) / completed (last DT accepted, EOM_ACK queued) | G474: equal, ≥ 3. F103: 0 / 0 |
| `brm_completed` `bcp_completed` `bcs_completed` | completed sessions of this node's direction per message | 1 / 1 / ≥ 1 (SIL 9) |
| `other_completed` | completions with any other PGN | 0 |
| `last_tx_pgn` `last_tx_len` | PGN / length of the current or last send session | F103: 4352 / 9 (BCS) |
| `last_rx_pgn` `last_rx_len` | same, receive side | G474: 4352 / 9 |
| `tx_expected_packets` / `tx_actual_packets` | DT packets of the current/last send session / DTs queued in it | F103: 2 / 2 |
| `rx_expected_packets` / `rx_actual_packets` | packets announced by the last RTS / DTs accepted in that session | G474: 2 / 2 |
| `tx_next_seq` / `rx_next_seq` | next DT sequence to send / expected | session-internal (after BCS: 3 / 2) |
| `tx_state` | 0 IDLE, 2 wait CTS/EOM_ACK, 3 sending DT, 4 hold | **0** |
| `rx_state` | 0 IDLE, 1 receiving DT | **0** |
| `aborts_sent` / `aborts_received` | Connection Abort frames queued / received aborts that ended a session | **0 / 0** |
| `transport_failures` | node: TP sessions ended by abort or timeout | **0** |
| `timeouts_tx` / `timeouts_rx` | sessions ended by T3/T4 / T1/T2 | **0 / 0** |
| `last_end_ev` | 1 RX done, 2 RX aborted, 4 TX done, 8 TX aborted | G474 1, F103 4 |
| `sessions_ended` | session ends, both directions | = completed count |
| `last_tx_duration_ms` / `max_tx_duration_ms` | last / longest completed send session (RTS queued → EOM_ACK accepted) | F103: BCS ≈ 10–15 / BRM ≈ 60–75 |
| `last_rx_duration_ms` / `max_rx_duration_ms` | receive side (RTS accepted → last DT accepted) | G474: ≈ 10–15 / ≈ 60–75 |
| `max_tx_dt_interval_ms` | largest gap between two consecutive DTs queued in one send session | ≈ 10–11 (> 20 means the F103 main loop stalled) |
| `max_rx_dt_interval_ms` | largest gap between two accepted DTs of one receive session | ≈ 10–12; must stay well below T1 = 750 |
| `outbox_full` | TP frames that could not be queued | 0 |
| `step_changes` | protocol step changes seen by the integration loop | 10 (both nodes, normal path) |

Times are main-loop processing times (ms tick), not CAN bus timestamps.

## 2. `g_gbt_can_diag` (struct, all fields uint32_t)

| Field | G474 source | F103 B source | Normal |
|---|---|---|---|
| `state` | PSR | ESR | **1** (error active); 2 warning, 3 passive, 4 bus-off |
| `tec` / `rec` | ECR | ESR | 0 / 0 |
| `rx_frames` | frames accepted by the filter and queued by the RX ISR | same | > 0 |
| `tx_frames` | requests accepted into the **TX FIFO** — not bus proof | frames loaded into a **TX mailbox** | > 0 |
| `tx_confirmed` | requests whose **TXBTO** bit was observed: frame completed on the bus = acknowledged by another node (the F103 B controller) | mailboxes completed with **TXOK** = 1 (acknowledged by the G474 controller) | ≈ `tx_frames` |
| `tx_pending` | requests not yet confirmed (0..3) | 1 while a mailbox is in flight | 0 (G474 may show 1 for an instant: CSD continues after END) |
| `tx_backpressure` | TX FIFO full → retried (not a loss) | mailbox busy → retried (normal, one mailbox in flight) | any |
| `rx_overflow_sw` / `rx_overflow_hw` | software queue full / RX FIFO 0 message lost | queue full / FIFO 0 overrun | **0 / 0** |
| `rx_invalid` | FD / remote / bad DLC / 11-bit / HAL read errors | bad DLC / RTR / 11-bit / HAL read errors | **0** |
| `hal_errors` | HAL error callbacks + RX read errors | same | **0** |
| `tx_failures` | bus-off refusals + HAL TX errors + buffer reuse without confirmation + integration drops | TXOK = 0 completions + bus-off refusals + HAL TX errors + integration drops | **0** |
| `warning_events` / `error_passive_events` / `bus_off_events` | FDCAN status-change **interrupts** (count entry and exit) | **entries** detected by polling ESR every main-loop pass | **0 / 0 / 0** |

Confirmation semantics: classic CAN completes a transmission only when at least one other node
sends the ACK bit. With exactly two active controllers (F103 A passive), `tx_confirmed` on one board
means the other board's controller received the frame correctly. Protocol-level proof of peer
reception is the peer's answer (CTS after RTS, next DT after CTS, EOM_ACK after DT n).

## 3. BRM snapshots

| G474 | F103 B | Meaning |
|---|---|---|
| `g_gbt_brm_rx_snapshot[49]` | `g_gbt_brm_tx_snapshot[49]` | the BRM payload received and reassembled / sent |
| `g_gbt_brm_rx_snapshot_state` | `g_gbt_brm_tx_snapshot_state` | 0 none, 1 captured (send side: session not completed yet), **2 frozen** |
| `g_gbt_brm_rx_snapshot_len` | `g_gbt_brm_tx_snapshot_len` | **49** |
| `g_gbt_brm_rx_snapshot_captures` | `g_gbt_brm_tx_snapshot_captures` | copies made; 1 normally (sender: +1 per BRM resend before the first completion) |

Frozen rule (`protocol/gbt_tp.c`): the receive copy is taken when the first BRM session completes;
the send copy is taken at every BRM session start until one BRM send session completes. After
`state = 2` nothing writes the TP copy again — later BCP/BCS sessions use other PGNs and BRM resends
are ignored. Expected content: TP_SESSION_FLOW.md §3. View as an array in Watch, or *Memory 1* at
`&g_gbt_brm_rx_snapshot` (49 bytes).

## 4. `g_gbt_evlog` — 128-entry event log

`count` (0..128), `dropped`, `frozen`, `e[0..127]`. Each entry: `t_ms`, `pgn`, `type`, `a`, `b`, `c`.

| type | Event | a | b | c | pgn |
|---|---|---|---|---|---|
| 1 RX_TP_CM | TP.CM received (before the node processes it) | control: 16 RTS, 17 CTS, 19 EOM_ACK, 255 Abort | data[1] (size low / packets granted / abort reason) | data[2] (size high / next seq) | data[5..7] |
| 2 RX_TP_DT | TP.DT received | sequence | first payload byte | 0 | open receive session PGN (0 if none) |
| 3 TX_TP_CM | TP.CM accepted by the CAN driver | as type 1 | | | |
| 4 TX_TP_DT | TP.DT accepted by the CAN driver | sequence | first payload byte | 0 | send session PGN |
| 5 STEP | protocol step change | new step | previous step | phase | 0 |
| 6 TP_END | TP session ended | 1 RX done, 2 RX aborted, 4 TX done, 8 TX aborted | DTs of that session | packets expected | session PGN |

Single writer (main context), no ISR access, no locking, no blocking. When `count` reaches 128 the
log **freezes**: nothing is overwritten and further events only increment `dropped`. A normal session
uses about 81 entries per node (SIL). An entry being written can be seen half-updated in a live
Watch refresh; read the log after END. Non-TP frames are not logged.

## 5. Acceptance summary

`g_gbt_accept_fail` (bit set = criterion currently not met) and `g_gbt_accept_pass` (1 only when the
mask is 0). Re-evaluated every 100 ms. Bit list: ACCEPTANCE_CRITERIA.md. Mid-session the mask is
non-zero (bit 0 = END not reached yet) by design.

## 6. Kept from STEP 07-C2 (unchanged)

The 89 (G474) / 91 (F103) flat `g_hw_diag_*` scalars and `g_gbt_can_status`, `g_gbt_port_diag`,
`g_gbt_phase`, ... remain. Most useful during a run: `g_hw_diag_gbt_step`, `g_hw_diag_gbt_phase`,
`g_hw_diag_update_count` (liveness, +10/s), `g_hw_diag_can_rx_count`. The new G474 port fields
`g_gbt_port_diag.tx_confirmed / tx_pending / tx_unconfirmed_reuse` and the new F103 port fields
`g_gbt_port_diag.warning_events / error_passive_events / bus_off_events / last_polled_state` are also
visible there.

## 7. Minimum Watch set

| Run | Variables |
|---|---|
| live | `g_hw_diag_gbt_step`, `g_hw_diag_update_count`, `g_gbt_can_diag`, `g_gbt_tp_diag` |
| final | `g_gbt_accept_fail`, `g_gbt_accept_pass`, `g_gbt_tp_diag`, `g_gbt_can_diag`, BRM snapshot (4 names), `g_gbt_evlog` |
