# HARDWARE_TEST_PROCEDURE — STEP 07-C2.4 dual-node TP session

Status of this package: **ARMCC NOT YET VERIFIED, HARDWARE NOT YET TESTED.** Nothing here has run on
a board. You build, flash and test. Constraint respected throughout: only **one** board on the PC
USB at a time; the other board runs from an independent supply you have already verified.

## 0. Rules for every run

1. **Never halt, break or single-step either MCU between the G474 run start and ≥ 2 s after END.**
   Read values live (*View → Periodic Window Update*). TP timers: T1 750 ms, T3 1250 ms.
2. **One session per reset.** END and EXIT are terminal (no restart in either `main.c`).
3. **Start order: F103 B must be running in step 22 before the G474 runs, and F103 B must be reset
   *after* the G474 was last stopped.** The F103 B waits for CHM forever and is silent; the G474
   gives up after 15 s and sends CEM, which puts a waiting F103 B into EXIT (phase 7, step 42). A
   G474 that is reset or restarted by the debugger while the F103 B is mid-session or in END/EXIT
   hears nothing — this is the failure observed before this step (G474 `update_count` rising,
   `can_rx_count` = 0; F103 B phase 7, step 42, last RX `0x081FF456`).
4. A G474 connected to USB **runs its flashed program at power-up** before any debug session. Treat
   every USB connection of the G474 as a start of the charger.

## 1. Board identification

| Board | Marking | MCU | Keil project |
|---|---|---|---|
| G474 | NUCLEO-G474RE | STM32G474RET6 | `source\Project1_G474\MDK-ARM\Project1_G474.uvprojx` |
| F103 B | the board with its **own** PD1050S wired to PB8/PB9 | STM32F103ZET6 | `source\Project1_F103B_Receiver_CubeMX\MDK-ARM\Project1_F103.uvprojx` |
| F103 A | the board whose PD1050S TXD/RXD are wired to G474 PA12/PA11 | — | **none — do not flash** |

Label the boards physically before starting.

## 2. Wiring and power checks (everything unpowered first)

- [ ] G474 PA12 (CN10 pin 12 on NUCLEO-64 — confirm with the Nucleo user manual) → F103 A PD1050S **TXD**.
- [ ] G474 PA11 (CN10 pin 14 — confirm) ← F103 A PD1050S **RXD**.
- [ ] F103 B PB9 → its PD1050S TXD, PB8 ← RXD (onboard; confirm on the schematic).
- [ ] CANH–CANH, CANL–CANL between F103 A and F103 B transceivers.
- [ ] **≈ 60 Ω** between CANH and CANL (two 120 Ω terminations), all boards unpowered.
- [ ] Common GND between G474, F103 A and F103 B.
- [ ] PD1050S pin 8 (S/STB) on both transceivers tied to GND or otherwise in normal mode (HARDWARE_TOPOLOGY.md §2).
- [ ] Logic levels: G474 PA11 / F103 PB8 tolerate the PD1050S RXD high level (HARDWARE_TOPOLOGY.md §2).

### F103 A passive verification
- [ ] F103 A MCU held in reset for the whole test (NRST to GND) **or** confirmed to contain no program
      that configures its CAN pins.
- [ ] Power F103 A alone: measure TXD of its PD1050S — it must not be driven low by F103 A; CANH–CANL
      differential ≈ 0 V (recessive).
- [ ] Back-powering: with only F103 A powered, the G474 3V3 rail must stay at 0 V; with only the G474
      powered, the F103 A rails must stay at 0 V. If either rises, stop and fix the wiring.

## 3. Build (Windows, Keil µVision, ARMCC V5.06 update 7)

1. Copy the package (README.md) and keep the folder layout: both projects include
   `..\..\Project1_SharedCore`.
2. Open each `.uvprojx`, *Project → Rebuild all target files*. Expected: 0 errors. Record warnings
   (BUILD_REPORT.md lists what is known).
3. Check the map file (`MDK-ARM\Project1_G474\Project1_G474.map`, `MDK-ARM\Project1_F103\Project1_F103.map`)
   contains `g_gbt_tp_diag`, `g_gbt_can_diag`, `g_gbt_evlog`, `g_gbt_accept_fail` and the board's
   `g_gbt_brm_rx_snapshot` (G474) / `g_gbt_brm_tx_snapshot` (F103).
4. Open the `.htm` static call-graph file and record "Maximum Stack Usage" for both (GCC estimate:
   F103 main thread ≈ 464 B, RX ISR ≈ 140 B; G474 ≈ 488 / 216 B).
5. Record total RW+ZI from the map (GCC estimate: +≈ 2.1 KB per board vs STEP 07-C2).

## 4. Flash and verify (one board at a time)

1. Connect the board by USB. *Flash → Download* (F8).
2. *Flash → Verify* must report "Verify OK" (or enable *Options → Utilities → Settings → Verify*).
3. Disconnect. Repeat for the other board. Do not touch F103 A.

## 5. Watch configuration

Add (WATCH_VARIABLES.md §7): `g_hw_diag_gbt_step`, `g_hw_diag_update_count`, `g_gbt_tp_diag`,
`g_gbt_can_diag`, `g_gbt_accept_fail`, `g_gbt_accept_pass`, the four BRM snapshot names of the
board, `g_gbt_evlog`. Enable *View → Periodic Window Update*. µVision stores the Watch list in the
local `.uvoptx/.uvguix`; that is expected.

## 6. Run 1 — observe the G474 (receiver). G474 on USB, F103 B on its own supply, F103 A passive

1. F103 B **unpowered**. F103 A transceiver powered, F103 A MCU passive.
2. Connect the G474 by USB (it may run alone now: no peer, it ends in EXIT — harmless).
3. In µVision (G474 project) *Debug → Start/Stop Debug Session*. The G474 is reset and stops at `main`
   (Run to main) — FDCAN not yet initialised, the G474 is silent. Set up the Watch window.
4. Power **F103 B** now (or press its RESET if it was already powered). It boots, waits in step 22,
   sends nothing.
5. Press **Run (F5)** on the G474.
6. Observe live: `g_hw_diag_gbt_step` 2 → 3 → 4 → 5 → 7 → 8 → 9 → 10 → 13 → 41 within ≈ 4–6 s;
   `g_gbt_tp_diag.rx_completed` rising; `g_gbt_can_diag.rx_frames` rising within 0.3 s of F5.
7. ≥ 2 s after step 41: record all values of §5 (screenshot), the 49 snapshot bytes and `g_gbt_evlog`
   entries 0..`count`−1 (Watch or *Memory* window export). Fill the results table (§10).
8. Stop the debug session only now.

## 7. Run 2 — observe the F103 B (sender). F103 B on USB, G474 on its own supply

1. G474 **unpowered**.
2. Connect F103 B by USB. *Debug → Start/Stop Debug Session* (F103 project). It stops at `main` — this
   is at the stack-paint entry, before the paint: expected. Set up the Watch window.
3. Press **Run (F5)**. F103 B paints the stack, starts and waits (`g_hw_diag_gbt_step` = 22,
   `g_gbt_can_diag.tx_frames` = 0).
4. Power the **G474** (independent supply). It runs its flashed program and starts the session.
5. Observe: step 22 → 23 → 25 → 26 → 29 → 30 → 31 → 32 → 33 → 41; `g_gbt_tp_diag.tx_completed` rising;
   `g_gbt_can_diag.tx_confirmed` following `tx_frames`.
6. ≥ 2 s after step 41: record values, the 49 snapshot bytes, the event log.
7. Now halting is allowed: stack watermark (§8).

## 8. F103 B stack watermark (after Run 2, both nodes in END)

1. *Debug → Stop* (halt). Read `Stack_Mem` and `__initial_sp` from the **new** map (the `.bss` grew,
   so the addresses moved).
2. Memory window at `Stack_Mem`: the lowest word must read `0xA5A5A5A5` (paint present).
3. Find the lowest address above `Stack_Mem` whose word is not `0xA5A5A5A5`;
   used = `__initial_sp` − that address. Optional µVision debug function (operator tool, not firmware):
   ```
   FUNC void StackHigh(void) {
     unsigned long a, lo, top;
     lo = 0x20000000; top = 0x20000400;   /* replace with Stack_Mem and __initial_sp of the NEW map */
     a = lo;
     while (a < top && _RDWORD(a) == 0xA5A5A5A5) a += 4;
     printf("stack used = %u of %u bytes\n", top - a, top - lo);
   }
   ```
4. Pass: ≤ 768 B. Repeat over ≥ 3 sessions; report the maximum.

## 9. BRM byte comparison

Compare `g_gbt_brm_rx_snapshot` (Run 1, G474), `g_gbt_brm_tx_snapshot` (Run 2, F103 B) and the
expected bytes (TP_SESSION_FLOW.md §3: `01 01 00 03 DC 05 AC 0D` + 41 × `FF`). All three must be
identical, both `_state` = 2, `_len` = 49. Because the two runs are separate sessions, equality to the
expected bytes (deterministic configuration) is the end-to-end proof; additionally Run 1 alone proves
the G474 received exactly what the F103 B firmware builds.

## 10. Results table (copy per session)

| Item | G474 (Run 1) | F103 B (Run 2) |
|---|---|---|
| `g_gbt_accept_pass` / `g_gbt_accept_fail` | | |
| step path (from `g_gbt_evlog`) | | |
| rx/tx started / completed | | |
| BRM / BCP / BCS completed | | |
| aborts sent / received, timeouts, transport failures | | |
| max DT interval / max session duration (ms) | | |
| CAN state / TEC / REC | | |
| rx_frames / tx_frames / tx_confirmed / tx_pending | | |
| overflow sw / hw, rx_invalid, hal_errors, tx_failures | | |
| warning / error-passive / bus-off events | | |
| BRM snapshot state / len / bytes equal expected | | |
| event log count / frozen / dropped | | |
| F103 B stack used (B) | — | |

## 11. Troubleshooting

| Symptom | Most likely cause | Check |
|---|---|---|
| G474 `update_count` rises, `rx_frames` 0, step 2 → 4 (10 s) → 40 → 42 | F103 B not in step 22 when the G474 ran (start order) or not transmitting | Repeat with §6 order exactly. Then Run 2 and look at F103 B `tx_frames`, `tx_confirmed`, `state` |
| F103 B phase 7, step 42, last RX `0x081FF456` (CEM) | the G474 ran ≥ 15 s without the F103 B answering | start order (rule 3) |
| F103 B `tx_frames` > 0, `tx_confirmed` 0, `tx_pending` 1, TEC → 128, state 3 | its frames are not acknowledged: no running controller on the bus, wrong bit rate, CANH/CANL swapped, G474 side transceiver unpowered / in silent mode | wiring, PD1050S pin 8, F103 A power |
| F103 B state 4 (bus-off) quickly | bit errors on its own transmission: PB9 → TXD or RXD → PB8 path broken, transceiver in silent/standby | F103 B transceiver pins, pin 8 |
| G474 `tx_confirmed` < `tx_frames` with `tx_pending` 3 and TEC rising | G474 frames not acknowledged (F103 B not running or not on the bus) | F103 B power/reset, bus wiring |
| `rx_invalid` > 0 | 11-bit / remote / FD frames from another device | is anything else on the bus (F103 A MCU)? |
| `error_passive_events` > 0 at start only | one node transmitted while the other was not yet running | start order; harmless if TP criteria pass, but repeat |
| `aborts_*` / `timeouts_*` > 0 | DT gaps (main-loop stall) or lost frames | `max_*_dt_interval_ms`, overflow counters, `g_gbt_evlog` |
| `g_gbt_accept_fail` bit 7 only, read right after END | a BCS session straddling END | wait 2 s, re-read |
| nothing changes, `update_count` frozen | CPU halted / HardFault / `Error_Handler` (init failed) | call stack in the debugger |
