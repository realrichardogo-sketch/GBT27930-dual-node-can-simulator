# Project 1 Lean Plan: two-week GB/T 27930-2023 simulator

2026年11月11日 · @Richard June

Project 1 ships in 14 days as a core subset of GB/T 27930-2023 System A: SIL carries the proof, and hardware shows one clean session and one timing measurement. This plan replaces the release scope of spec v2.0.

## Scope

Everything an interviewer can probe stays.

- Layered design: protocol/ in pure C, reached only through hal.h, built for the G474, the F103 and Linux.
- Core session: handshake, configuration, charging, both stop sequences, and BEM/CEM on timeout.
- Codec: byte order, scaling, the negative current offset and fill bits.
- BCL rules (PROT-7) and the output command going to 0 within 10 ms.
- Tests: unit tests, SIL on vcan, and one logic-analyzer measurement on hardware.

## Cuts

The machinery that only produces paperwork goes, and edge behaviour is deferred.

| v2.0 item | Two-week version | Why it is safe |
|---|---|---|
| YAML source, generator, lint, CI staleness check (COM-5, AC-12, AC-16) | One hand-written protocol_table.c with a clause comment per row | Biggest time sink; all protocol constants still live in one table |
| Reconnection, pause, time sync, older-version compatibility, optional BMV/BMT/BSP | Dropped; unknown messages are ignored | Edge behaviour, not the core flow |
| 11 waits tested at T ± 1 ms | 2 waits: BCL and CCS | They guard the charging phase; timeouts stay table-driven, so more rows are cheap later |
| Full transport policy (TP-5 to TP-7, T4, duplicates) | Happy path, sequence-gap abort and one timeout | BRM, BCP and BCS still need transport, so it cannot go entirely |
| Two TX queues with priority classes | One RX queue, one TX queue | The stop path sets the output to 0 before sending anything |
| Bus-off recovery (CAN-1 to CAN-4, AC-9a to 9d) | Stretch item | Not needed for a clean session or the timing proof |
| Parameter-mismatch and BSM-abnormal stops | Dropped | Both normal stop sequences already show the stop path |
| Both stops on hardware | Both in SIL; only the charger-initiated stop on hardware | SIL runs the same protocol sources |
| 8 fault scenarios | 3 in SIL only: drop BCL, out-of-range current, missing TP_DT | One per failure class; no fault testing on the boards |
| Configurable battery model | Linear SOC ramp with a fixed voltage and current demand | The protocol is the subject, not the battery |
| 21 acceptance criteria, change log, External-status tracking | The 6 checks below, a one-page README and a short note on ambiguities in the standard | The three conflicts found in the standard stay as an interview story |

## Schedule

Most of the project is finished in SIL by day 7, before the hardware ports start.

| Days | Work | Output |
|---|---|---|
| 1–2 | Repo, hal.h, SocketCAN port, protocol table, identifier builder, codec with unit tests. Also get each board to send and receive one frame at 250 kbit/s. | Codec tests pass; the hardware is known to work |
| 3–5 | Transport happy path and both state machines | A full normal session on vcan |
| 6–7 | Both stops, the 2 timeouts, the BCL rules, the zero-output path, with SIL tests | The 10 ms stop is proven in SIL |
| 8–10 | G474 and F103 ports, a full session on the bus, USB-CAN capture, GPIO timing on the logic analyzer | Hardware evidence |
| 11–12 | The 3 fault scenarios; README with diagram, decoded log and screenshot | A repo you can show |
| 13–14 | Buffer, then rehearse explaining it | Interview-ready |

The one-frame hardware check on days 1–2 matters most. Two different CAN controllers are the likeliest place to lose days to bit timing, filters or wiring, so find that out in week one.

## If you fall behind

Give things up in this order:

1. The third fault scenario (missing TP_DT).
2. The vehicle-initiated stop.
3. Running the F103 as the vehicle: use the Linux SIL node through the USB-CAN adapter instead, with the G474 as the only board.

Do not give up the unit tests, the SIL session or the logic-analyzer measurement. Those are what the job description asks about directly.

## Done when

The project is released when all six checks pass.

- A full session completes on both boards with no error messages.
- Captured identifiers and bytes match the tables by hand check, for example CHM = 0x1826F456.
- BRM, BCP and BCS transfer over the transport protocol on hardware.
- BCL and CCS timeouts each produce their error message and a safe state in SIL.
- The output command reaches 0 within 10 ms, in SIL and on the logic analyzer.
- One command builds all three targets and runs the tests.

## How it maps to the job

Each line of the job description has one piece of evidence in the repo.

| Job requirement | Evidence |
|---|---|
| C in a real-time embedded environment | Non-blocking state machines, no dynamic memory, wrap-safe timers |
| Unit tests, SIL, system level | The three test layers, plus why SIL came before hardware |
| Hardware bring-up, scopes, CAN tools | Two different CAN controllers, loopback first, the GPIO timing measurement |
| Specify requirements, break down tasks | The MUST/SHOULD split and what was cut to hit the deadline |
| GB DC charging protocol | The message flow, the current sign convention, the conflicts found in the standard |
| Clean design | One protocol code base on three targets through the HAL |

On the resume, call it a "core subset of GB/T 27930-2023 System A", since reconnection and pause are out. In the interview, name what was cut and why. If asked, also say that both nodes share one reading of the standard, so it has not been checked against real equipment.
