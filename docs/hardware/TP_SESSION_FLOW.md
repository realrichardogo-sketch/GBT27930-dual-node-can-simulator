# TP_SESSION_FLOW — STEP 07-C2.4

All values below come from the implementation (`source/Project1_SharedCore`) and from the
deterministic dual-node SIL run of it (`evidence/static_analysis/final_trace_inst.txt`, scenario
"normal order 0", same node parameters as both `main.c` files). They are the implementation
baseline; the TP byte layouts and timers marked PROVISIONAL in `protocol/gbt_tp.c` are not an
independent proof of SAE J1939-21 / GB/T 27930 conformance.

## 1. Roles

| | F103 B (vehicle, SA 0xF4) | G474 (charger, SA 0x56) |
|---|---|---|
| TP role | **sole TP data sender** (`gbt_tp_tx_start`, called by the node scheduler for BRM/BCP/BCS, `protocol/gbt_node.c` `scheduler`) | **TP receiver** (`rx_on_cm` / `rx_on_dt`, payload accepted by `gbt_csm_accept_tp`: BRM 49, BCP 13, BCS 9 bytes) |
| Sends | RTS, DT, (Abort) | CTS, EOM_ACK, (Abort) |
| Never sends | CTS, EOM_ACK | RTS, DT (the charger has no message longer than 8 bytes) |

## 2. Messages (`protocol/gbt_id.c` table)

| Message | PGN (dec / hex) | Length | DT packets | Period while enabled |
|---|---|---|---|---|
| BRM | 512 / 0x000200 | 49 | 7 | 250 ms (V05, until CRM 0xAA) |
| BCP | 1536 / 0x000600 | 13 | 2 | 500 ms (V06, until CML) |
| BCS | 4352 / 0x001100 | 9 | 2 | 250 ms (V10/V11, while charging) |

A normal session therefore runs **1 BRM + 1 BCP + N BCS** TP sessions. In the SIL with the board
parameters (SOC 50.0 → 52.0 %, 1 %/s) N = 9: 11 sessions, 27 DT in total. On hardware N varies
with timing (expect 8–10). "All three TP transfers" = at least one completed session of each.

## 3. Frame sequence of one transfer (BRM shown; SIL times in ms)

| # | t | From → to | CAN ID | Data | Code |
|---|---|---|---|---|---|
| 1 | 100002 | F103 B → G474 | `1CEC56F4` | `10 31 00 07 FF 00 02 00` RTS: ctrl 16, size 49, 7 packets, no per-CTS limit, PGN 0x000200 | `gbt_tp_tx_start` |
| 2/3 | 100003 | G474 → F103 B | `1CECF456` | `11 07 01 FF FF 00 02 00` CTS: 7 packets from seq 1 | `rx_on_cm` |
| 4/5 | 100003 … 100063 | F103 B → G474 | `1CEB56F4` | `01 01 01 00 03 DC 05 AC`, `02 0D FF FF FF FF FF FF`, `03`…`07 FF…` DT 1..7, 10 ms apart | `tx_on_cm` (DT 1 immediately), `gbt_tp_tick` → `tx_send_dt` |
| 6/7 | 100063 | (G474) | — | DT 1..7 stored in order; after DT 7 the payload is complete and validated (length = RTS size, packets = ceil(49/7), PGN/length accepted) | `rx_on_dt` |
| 8 | 100064 | G474 → F103 B | `1CECF456` | `13 31 00 07 FF 00 02 00` EOM_ACK: ctrl 19, size 49, 7 packets | `rx_on_dt` |
| 9 | 100064 | (F103 B) | — | EOM_ACK accepted → send session complete | `tx_on_cm` |
| 10 | | both | — | receive and send sessions IDLE (`GBT_TP_IDLE`) | |

BCP: RTS `10 0D 00 02 FF 00 06 00`, CTS `11 02 01 FF FF 00 06 00`, DT `01 6D 01 D0 07 58 02 68`,
`02 10 69 F4 01 AC 0D FF`, EOM_ACK `13 0D 00 02 FF 00 06 00`.
BCS: RTS `10 09 00 02 FF 00 11 00`, CTS `11 02 01 FF FF 00 11 00`, DT `01 A0 0F B8 0B 6D 01 32`,
`02 00 00 FF FF FF FF FF`, EOM_ACK `13 09 00 02 FF 00 11 00`.
Byte order: little-endian sizes and PGNs; DT byte 0 = sequence (first = 1); unused bytes of the last
DT = 0xFF.

### Expected BRM payload (49 bytes) with the board parameters

```
01 01 00 03 DC 05 AC 0D  FF FF FF FF FF FF FF FF  FF FF FF FF FF FF FF FF
FF FF FF FF FF FF FF FF  FF FF FF FF FF FF FF FF  FF FF FF FF FF FF FF FF  FF
```
Bytes 0–2 version 1.1 (`01 01 00`), byte 3 battery type 3, bytes 4–5 rated capacity 0x05DC =
150.0 Ah, bytes 6–7 rated voltage 0x0DAC = 350.0 V, bytes 8–48 not available (0xFF). This is what
`g_gbt_brm_tx_snapshot` (F103 B) and `g_gbt_brm_rx_snapshot` (G474) must contain.

## 4. Event log view (as recorded in `g_gbt_evlog`, see WATCH_VARIABLES.md §4)

G474 (receiver): `RX_TP_CM a=16` → `TX_TP_CM a=17` → `RX_TP_DT a=1..7` → `TP_END a=1 b=7 c=7` →
`TX_TP_CM a=19`. The receiver records TP_END when it accepts DT 7, i.e. before its EOM_ACK leaves in
the send phase of the same main-loop pass.

F103 B (sender): `TX_TP_CM a=16` → `RX_TP_CM a=17` → `TX_TP_DT a=1..7` → `RX_TP_CM a=19` →
`TP_END a=4 b=7 c=7`.

The SIL regression asserts both sequences entry by entry (`tests/sil/test_sil_session.c`
`check_brm_sequence`).

## 5. Protocol step paths (derived from the implementation, asserted by the SIL)

| Node | Path | Meaning |
|---|---|---|
| G474 charger | **2 → 3 → 4 → 5 → 7 → 8 → 9 → 10 → 13 → 41** | C02 CHM, C03 self-check, C04 CRM 00 / wait BRM (TP), C05 CRM AA / wait BCP (TP), C07 CML, C08 CRO 00, C09 CRO AA, C10 charging (BCS TP), C12 CST / wait BSD, END |
| F103 B vehicle | **22 → 23 → 25 → 26 → 29 → 30 → 31 → 32 → 33 → 41** | V02 wait CHM, V03 BHM, V05 BRM (TP), V06 BCP (TP), V09 BRO, V10 BCL+BCS (TP), V11 charging, V12 BST (SOC target), V13 BSD, END |

Codes: `protocol/gbt_node_internal.h:12-34`. Phases (`protocol/gbt_node.h`): 5 END, 6 TIMEOUT
handling, 7 EXIT.

## 6. Timers and failure handling (implementation values, PROVISIONAL unless stated)

| Timer | Value | Side | On expiry |
|---|---|---|---|
| DT spacing | 10 ms (GB/T §7.5, verified) | sender | next DT |
| T1 between DTs | 750 ms | receiver | Abort reason 3, receive session ended |
| T2 CTS → DT | 1250 ms | receiver | Abort reason 3 |
| T3 RTS/last DT → CTS/EOM_ACK | 1250 ms | sender | Abort reason 3 |
| T4 CTS(0) hold | 1050 ms | sender | Abort reason 3 |

A sequence gap or duplicate DT → receiver Abort reason 7. Any TP abort/timeout increments the
node's `transport_failures`; the protocol does not send BEM/CEM for a TP failure alone (TP-6) and
BRM/BCP/BCS are resent at their period.

## 7. Timing facts that matter on hardware

* **Start order.** The vehicle waits in V02 (step 22) without transmitting and without a timer
  (`gbt_vehicle_sm.c` `gbt_vsm_start`). The charger waits 10 s for BHM, then CRM 00 for 5 s, then
  sends CEM; a vehicle still in V02 that receives the CEM goes to EXIT (phase 7, step 42) — the
  exact state observed on hardware before this step (SIL `scenario_start_order` B reproduces it:
  vehicle phase 7 step 42, last RX `081FF456`, charger RX 0). EXIT and END are terminal in both
  `main.c` files: **only a reset starts a new session.** Therefore F103 B must be reset/running
  *after* the G474 is held (debugger halt or unpowered) and *before* the G474 runs.
* **END straddle.** A periodic BCS session opened just before the vehicle stop can still be running
  when both nodes reach END; the TP layer keeps running in END and completes it within ≈ 20 ms
  (port-level SIL). Read final values ≥ 2 s after both nodes show step 41.
* **After END** the charger keeps sending CSD (`181DF456`) every 250 ms; frame counters keep
  rising, TP counters do not.
