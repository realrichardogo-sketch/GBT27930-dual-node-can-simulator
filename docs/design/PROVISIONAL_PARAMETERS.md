# Provisional Parameter Register

Single register of every protocol value that is not directly verified against
GB/T 27930-2023 or SAE J1939-21:2006. Code references these IDs in comments.
Classifications (one per entry, never mixed):
VERIFIED_GBT, VERIFIED_SAE_2006, SECONDARY_CORROBORATION, PROJECT_DEFINED,
PROVISIONAL, UNKNOWN.

Source shorthand: PT = `docs/sources/01.2_PROTOCOL_TABLES.md`; BL =
`docs/sources/REQUIREMENTS_BASELINE.md`; AR422 = AUTOSAR R4.2.2 SWS
J1939 Transport Layer (normative ref. [16] = SAE J1939-21 2006-12); LNX =
Linux `net/can/j1939/transport.c` (no edition stated).

## 1. Verified reference values used by the code (for contrast)

| ID | Parameter | Value | Source | Class |
|---|---|---|---|---|
| V-01 | Byte order | low byte first | GB/T §5.3 (PT L56) | VERIFIED_GBT |
| V-02 | Valid charging current | −400 A ~ 0 A | GB/T §5.4 (PT L57) | VERIFIED_GBT |
| V-03 | TP data-frame interval | 10 ms | GB/T §7.5 (PT L66) | VERIFIED_GBT |
| V-04 | TP message size range | 9 – 1785 bytes | GB/T §3.1.13, §7.5, Table 1 note 7 | VERIFIED_GBT |
| V-05 | Message IDs (18 Lean) | PT L94–115 | GB/T Table 1, §7.4, Table 2; bit assignment delegated to J1939-21:2006 (§7.1) | VERIFIED_GBT (derived formula) |

## 2. Register

| ID | Parameter | Value | Unit | Source / edition / location | Class | Reason | Modules | Tests | Replace when |
|---|---|---|---|---|---|---|---|---|---|
| P-K3-01 | Data-field bit numbering (K-3) | Candidate A: bit 1 = LSB; field LSB at lowest bit; numbering continues low byte first | — | GB/T silent (U-P2); J1939-71 practice (not a GB/T reference) | PROVISIONAL | Needed to encode CCS, BSM, BST, CST, BEM, CEM, BCS SPN3077 | `protocol/gbt_bits.c`, `gbt_codec_ext.c` | `test_bits`, `test_codec_ext` | J1939-21:2006 check or Owner interpretation (K-3.ii); change `gbt_bits.c` + re-derive vectors |
| P-K5-01 | BCP SPN2817 unit | A, 0.1 A/bit, −400 A | — | PT L205, note 2 vs table cell "V" (OQ-1) | PROVISIONAL | Conflict inside GB/T | `gbt_codec_ext.c` | `test_codec_ext` | STD-3 / OQ-1 resolution |
| P-K5-02 | CSD SPN3613 offset | 0 | — | PT L386 prints 1; SPN2561 uses 0 (OQ-2) | PROVISIONAL | Printed offset contradicts printed range | `gbt_provisional.h`, `gbt_codec_ext.c` | `test_codec_ext` | OQ-2 resolution |
| P-K5-03 | BCP SPN2822 unit | V, 0.1 V/bit, 0 V | — | PT L210, note 7 vs cell "℃" (OQ-3) | PROVISIONAL | Conflict inside GB/T | `gbt_codec_ext.c` | `test_codec_ext` | OQ-3 resolution |
| P-UR2-01 | Current range endpoints | both valid (raw 0x0000 and 0x0FA0) | — | GB/T §5.4 silent on inclusion (B1-14) | PROVISIONAL | Range check needs a boundary | `gbt_provisional.h`, `gbt_codec.c` | `test_codec` | Owner decision |
| P-S5-01 | Timer expiry boundary | expired when elapsed ≥ T | ms | Not defined in GB/T (S-5) | PROVISIONAL | T−1 / T+1 tests unaffected | `gbt_timer.c`, `gbt_tp.c` | `test_timer`, `test_tp` | Owner decision |
| P-S9-01 | Node outbox depth | 16 | frames | B1-06 / S-9 | PROJECT_DEFINED | Bounded memory | `gbt_outbox.h` | `test_outbox` | Owner decision |
| P-TP4-01 | TP receive buffer | 64 | bytes | BL TP-4 | PROJECT_DEFINED | Largest Lean TP payload is BRM 49 B | `gbt_tp.h` | `test_tp` | — |
| P-K4-01 | TP.CM PDU format (PGN 0x00EC00) | 0xEC | — | AR422 abbreviations; LNX `0xec00` | SECONDARY_CORROBORATION | J1939-21:2006 not available (K-4) | `gbt_tp.c` (params) | `test_tp` | J1939-21:2006 text |
| P-K4-02 | TP.DT PDU format (PGN 0x00EB00) | 0xEB | — | AR422; LNX | SECONDARY_CORROBORATION | same | `gbt_tp.c` | `test_tp` | same |
| P-K4-03 | TP frame priority | 7 | — | none found | PROVISIONAL | Needed for the identifier | `gbt_tp.c` | `test_tp` | same |
| P-K4-04 | Control bytes RTS/CTS/EOM ACK/Abort | 16 / 17 / 19 / 255 | — | LNX constants; roles in AR422 p.22–23 | SECONDARY_CORROBORATION | Values not in AR422 | `gbt_tp.c` | `test_tp` | same |
| P-K4-05 | Timers T1 / T2 / T3 / T4 | 750 / 1250 / 1250 / 1050 | ms | Roles: AR422 error table p.39; values: model knowledge only | PROVISIONAL | Bounded waits required (K-4.b) | `gbt_tp.c` | `test_tp` (T1, T3, T4 boundaries) | same |
| P-K4-06 | Tr / Th | not used | ms | AR422 names only | UNKNOWN | Receiver never holds (no CTS(0) sent); sender pacing fixed by V-03 | — | — | same |
| P-K4-07 | CM/DT byte layouts; RTS byte 5 = 0xFF (no per-CTS limit); last-DT fill 0xFF | see `gbt_tp.c` header | — | Fill 0xFF: AR422 SWS_J1939Tp_00068 (SECONDARY); field positions: model knowledge | PROVISIONAL | Required to build frames | `gbt_tp.c` | `test_tp` | same |
| P-K4-08 | Data bytes per DT | 7 | bytes | AR422 p.23 | SECONDARY_CORROBORATION | — | `gbt_tp.h` | `test_tp` | same |
| P-K4-09 | First sequence number | 1 | — | model knowledge | PROVISIONAL | — | `gbt_tp.c` | `test_tp` | same |
| P-K4-10 | Abort reasons: busy / resources / timeout / sequence | 1 / 2 / 3 / 7 | — | 3: AR422 SWS_J1939Tp_00100 (SECONDARY); 1, 2: LNX; 7: LNX, later edition | PROVISIONAL (3 is SECONDARY_CORROBORATION) | 2006 reason list unknown | `gbt_tp.c` | `test_tp` | same |
| P-K4-11 | Duplicate DT handling | treated as gap (abort) | — | BL TP-5 SHOULD policy dropped by Lean | PROJECT_DEFINED | Lean scope | `gbt_tp.c` | `test_tp` | — |
| P-S1-01 | CST content for a charger fault stop (ERR-6, RX overflow, output refusal) | SPN3521 bits 5-6 = 01 (fault stop), SPN3522 bits 11-12 = 01 (other fault) | — | BL ERR-6 [DOC] for SPN3522; SPN3521 value unspecified (RQ-08) | PROVISIONAL | S-1 | `gbt_charger_sm.c` | `test_sil_session` (out-of-range current; T-5 RX overflow, CST `10 00 04 C0`) | Owner decision on RQ-08 |
| P-S11-01 | CST content for voltage demand below charger minimum (T-3c) | SPN3521 bits 5-6 = 01, SPN3523 bits 3-4 = 01 (voltage abnormal) | — | PT L426 requires CST; reason not specified | PROVISIONAL | S-11 | `gbt_charger_sm.c` | `test_sil_session` (T-3c, CST `10 00 00 C4`) | Owner decision |
| P-TM06-01 | BST frames before BSD in a charger-initiated stop | 5 | frames | GB/T note k / Table B.1: 5–10 (TT TM-06) | PROJECT_DEFINED | Value inside the normative range | `gbt_vehicle_sm.c` | `test_sil_session` (charger stop) | — |
| P-K2-01 | Vehicle CCS supervision | one timer: armed at CRO 0xAA (TO-13), re-armed on each valid CCS (TO-17 start = last valid CCS); TV41 before TV24 | 1000 ms | Durations VERIFIED_GBT (TT L70, L74); TO-17 start not stated (U-T4) | PROVISIONAL | K-2 | `gbt_vehicle_sm.c` | `test_sil_session` (CCS timeout) | Owner decision K-2 |
| P-TO15-01 | TO-15 (BCS 5 s) reference before the first BCS | armed at C10 entry | — | TT L72 "last BCS received" | PROVISIONAL | Needs a start event | `gbt_charger_sm.c` | — | Owner decision |
| P-K6A-01 | Timeout handling duration T_HANDLE; peer error message | 1000 ms (config) -> 4 CEM/BEM frames, then EXIT; BEM/CEM received -> immediate EXIT | ms | GB/T 18487.1 B.3.7.3 (EXTERNAL, TO-25); BL ERR-5 placeholder | PROVISIONAL | K-6A | `gbt_node.c` (`cfg.t_handle_ms`) | `test_sil_session` (BCL / CCS timeout) | GB/T 18487.1 or Owner decision |
| P-SAF5-01 | CCS feedback freshness | feedback reported only if valid and <= 100 ms old; otherwise CCS reports 0 V / 0 A | ms | BL SAF-5 [DOC] (100 ms); reporting 0 when stale is a project choice; SAF-5 output stop not implemented (RQ-03 OPEN) | PROVISIONAL | SAF-5 | `gbt_charger_sm.c` | — | RQ-03 decision |
| P-ND7-01 | Sink refuses a non-safety (enabled) command | charger fault stop (DISABLE_OUTPUT + CST, P-S1-01) | — | Revision 3.2 ND-7 | PROVISIONAL | ND-7 | `gbt_node.c` | — | Owner decision |
| P-OQ12-01 | Zero-current demand | commanded as 0 A (not raised to CML minimum) | — | OQ-12(a) zero-current OPEN | PROVISIONAL | Meaning of 0 A not decided | `gbt_safety.c` | — | OQ-12(a) decision |
| P-S8-01 | Charger self-check and readiness; vehicle readiness | self-check after `cfg.selfcheck_delay_ms`; charger ready after >= 1 CRO 0x00; vehicle ready after >= 1 BRO 0x00 | — | GB/T 18487.1 B.4.3 / B.4.4 (EXTERNAL) | PROJECT_DEFINED | Simulation | `gbt_charger_sm.c`, `gbt_vehicle_sm.c` | `test_sil_session` | Project 2 / hardware integration |
| T-ND6 | ND-6 policy used by the SIL tests | OPTION_A, set explicitly in the test configuration | — | ND-6 OPEN | PROVISIONAL (test setting only, not a project decision) | Node refuses an UNSET policy | `tests/sil/test_sil_session.c` | `test_sil_session` | Owner decision ND-6 |

## 3. Open items (no value chosen)

| ID | Item | Status |
|---|---|---|
| K-2 | CCS timeout interpretation (TO-13/TO-17) | Proposal in STEP 07-A package; Owner decision pending |
| K-6A | BEM/CEM duration T_HANDLE and exit | Proposal 1000 ms PROVISIONAL; Owner decision pending |
| ND-6 | TX after refused safety output | OPEN; no default (`apply_fail_tx_policy` UNSET = config error) |
| ND-3 | Handling of E_BUS_OFF / E_IO on send | PROVISIONAL: discard and count |
| U-R1 | Special raw codes in current SPNs | UNRESOLVED; not implemented |
