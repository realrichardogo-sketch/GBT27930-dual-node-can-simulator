# GB/T 27930-2023 Dual-Node CAN Charging Simulator (STM32G474 + STM32F103)

A two-board CAN simulator for a **core subset of GB/T 27930-2023 System A**, the communication protocol
between an off-board conductive DC charger and an electric vehicle. One STM32 board acts as the charger
and the other as the vehicle. Both run the same portable C99 protocol core, which is also built and
tested on Linux with unit tests, software-in-the-loop (SIL) sessions and fault-injection tests.

> **Project status: STEP 07 closed as CONDITIONAL PASS (2026-11-24).** The software tests pass and both
> ARMCC builds are clean. The hardware results below were reported by the owner, and several of the
> original hardware acceptance checks are incomplete (see [Verification](#verification-results)).
> This is a portfolio simulator. It is **not** a certified, standard-conformant or production
> charging system, and it controls no power stage.

## Engineering highlights

- **Portable protocol core in pure C99.** No dynamic memory, no blocking calls, no writable static
  state, no platform headers. These rules are enforced by CTest checks and cross-compiled for Cortex-M.
- **One hardware abstraction contract** ([`hal.h`](source/Project1_SharedCore/hal/hal.h)) with four
  implementations: STM32G4 FDCAN, STM32F1 bxCAN, Linux SocketCAN, and a fake HAL for SIL.
- **J1939-style transport protocol** (RTS/CTS/DT/EOM, abort, T1–T4 waits) for the multi-frame
  messages BRM, BCP and BCS.
- **Charger and vehicle state machines** with the protocol timeouts TO-01 … TO-25 and a single safety
  module that owns the output command.
- **ISR → main-loop single-producer/single-consumer queue**, stress-tested with signals on Linux and
  checked with ASan/UBSan.
- **Fault-injection instrumentation (FI-04)** that times the safe-state shutdown command on the target
  from the last pass start to confirmed acceptance.
- **Evidence discipline:** every result is labelled as independently verified, owner-reported,
  historical or not verified.

## Hardware architecture

```
  NUCLEO-G474RE  (charger, SA 0x56)              STM32F103ZET6 board "F103 B" (vehicle, SA 0xF4)
  FDCAN1 classic, 250 kbit/s                     bxCAN1, 250 kbit/s
  PA12/PA11 ──> PD1050S on board "F103 A"        PB9/PB8 ──> on-board PD1050S
                 (transceiver carrier only)
                        │                                     │
                        └──────── CANH / CANL, 2 × 120 Ω ─────┘
```

- CAN 2.0B, 29-bit identifiers, 250 kbit/s. Sample points are 88.2 % on the G474 and 87.5 % on the F103.
- The third board ("F103 A") only carries the G474's CAN transceiver. It is not a protocol node.
- The output stage is simulated in software (`sim_plant`). There is no power electronics and no
  current measurement.

Details: [`docs/hardware/HARDWARE_TOPOLOGY.md`](docs/hardware/HARDWARE_TOPOLOGY.md).

## Software architecture

```
 Board application (CubeMX USER CODE: init, main loop, debugger Watch variables)
 └─ Integration layer  app/gbt_integration.c   ← the only caller of the HAL
      loop pass: RX overflow check → up to 16 RX frames → protocol tick → apply output → send
    ├─ Protocol core (protocol/, common/)       pure C99, no HAL includes
    │    codec · 29-bit IDs · transport protocol · charger/vehicle state machines · safety · timers
    └─ HAL contract  hal/hal.h
         ports/stm32/g474 (FDCAN) · ports/stm32/f103 (bxCAN) · host/ports/linux (SocketCAN) · fake HAL
```

- **TX gate.** While an output command is pending, no further protocol frame is sent. This means a
  peer never sees a message that contradicts the output state.
- **Safety shutdown.** A protocol timeout, a received peer error message (BEM/CEM), an RX overflow,
  a stop request or a sink refusal while charging calls `gbt_safety_disable`. It latches an output
  inhibit and commands zero output. The resulting CEM/CST/… frames are sent only after the output
  command has been accepted: output first, then the message.
- **Interrupts.** The CAN RX interrupt only validates frames and copies them into the SPSC queue. All
  protocol work runs in the main loop.

Details: [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md), including the step codes, timers, interrupt
priorities and the stack estimate.

## CAN / TP communication and state machines

| Phase | Charger (G474) | Vehicle (F103 B) |
|---|---|---|
| Handshake | CHM | BHM |
| Identification | CRM | BRM (TP, 49 B) |
| Parameters | CTS, CML | BCP (TP) |
| Ready | CRO | BRO |
| Charging | CCS | BCL, BCS (TP) |
| Stop / error | CST, CSD, CEM | BST, BSD, BEM |

The project implements a reduced ("lean") message set, not the full standard. Values that the
standard text available to the project could not confirm are listed as provisional in
[`docs/design/PROVISIONAL_PARAMETERS.md`](docs/design/PROVISIONAL_PARAMETERS.md).

The frame-level TP sequence is in [`docs/hardware/TP_SESSION_FLOW.md`](docs/hardware/TP_SESSION_FLOW.md).

## Repository structure

| Path | Content |
|---|---|
| [`source/Project1_SharedCore/`](source/Project1_SharedCore) | protocol core, HAL contract, integration layer, STM32 ports, SPSC queue: **the single copy** used by both boards and the host |
| [`source/Project1_G474/`](source/Project1_G474) | charger firmware: STM32CubeMX project (`.ioc`) and Keil MDK project (`MDK-ARM/`) |
| [`source/Project1_F103B_Receiver_CubeMX/`](source/Project1_F103B_Receiver_CubeMX) | vehicle firmware: STM32CubeMX and Keil MDK project |
| [`host/`](host) | Linux CMake harness: unit tests, SIL, port tests against FDCAN/bxCAN fakes, SocketCAN nodes. The shared-core files here are **symlinks** into `source/`. |
| [`scripts/`](scripts) | one-command host checks, GCC board compile, Keil reference check, ARMCC rebuild helper, stack call-graph tool |
| [`docs/`](docs) | architecture, verification, open issues, provenance, hardware procedures, design records, selected evidence |

The vendor `Drivers/` folders contain only the STM32 HAL, BSP and CMSIS parts that the projects use.
Their license files are kept (see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)).

## Build and test

### Linux / WSL host tests

Requirements: `gcc`, `cmake` ≥ 3.16, and optionally `arm-none-eabi-gcc` (for the cross-compile
checks). Use a Linux, macOS or WSL checkout so that the `host/` symlinks work.

```sh
sh scripts/run_host_checks.sh     # symlinks, host build, ctest, GCC board compile, Keil file references
```

Manual equivalent:

```sh
cmake -S host -B build/host && cmake --build build/host
(cd build/host && ctest --output-on-failure)
```

The two-process SocketCAN test `linux_sil_vcan` needs a `vcan0` interface. Without it, the test
reports **Not Run**, never Pass:

```sh
sudo ip link add vcan0 type vcan && sudo ip link set vcan0 up
```

### STM32 firmware (Windows, Keil MDK)

Open `source/Project1_G474/MDK-ARM/Project1_G474.uvprojx` and
`source/Project1_F103B_Receiver_CubeMX/MDK-ARM/Project1_F103.uvprojx` in µVision and run *Rebuild all*.
The toolchain used was ARMCC V5.06 update 7 (build 960). [`scripts/keil_rebuild.ps1`](scripts/keil_rebuild.ps1)
is a build-only command-line helper; it has not been run itself.

Bench procedures (start order, flashing, debugger Watch variables) are in [`docs/hardware/`](docs/hardware).

## Verification results

| Item | Result | Evidence |
|---|---|---|
| Linux CTest (this repository, 2026-11-25) | **29 passed, 0 failed, 1 not run** (`linux_sil_vcan`: no `vcan0`). SIL session 1087 checks, G474 port session 297, F103 port session 439; 0 failures, 0 build warnings | re-run locally |
| GCC compile-only of every board source file | PASS (G474 43 files, F103 36 files). This is **not** the ARMCC build. | re-run locally |
| ARMCC build, G474 | 0 errors, 0 warnings | owner-reported (Windows) |
| ARMCC build, F103 | 0 errors, 0 warnings | owner-reported (Windows) |
| FI-04 communication loss during charging | **10/10 independent hardware trials passed**: vehicle held in reset, charger timed out (TO-14), output disabled, inhibit latched, exit step reached | owner-reported |
| FI-04 shutdown-command confirmation time | **1 ms** maximum observed (limit 9 ms), measured with the 1 ms software tick, not a logic analyzer | owner-reported |

**Original lean acceptance checks** (wording unchanged, from [`docs/design/PROJECT_PLAN.md`](docs/design/PROJECT_PLAN.md)):

| # | Check | Status |
|---|---|---|
| 1 | Full session on both boards with no error messages | **not verified** (no recorded session to END) |
| 2 | Captured identifiers and bytes match the tables by hand check | **not verified on hardware** (verified in SIL) |
| 3 | BRM, BCP and BCS over TP on hardware | **partial** (BRM and BCP implied by the charger reaching charging; BCS not recorded) |
| 4 | BCL and CCS timeouts produce their error message and a safe state in SIL | **met** |
| 5 | Output command reaches 0 within 10 ms, in SIL and on a logic analyzer | **met in SIL**; on hardware 1 ms by software timing; **logic analyzer not used** (accepted substitution) |
| 6 | One command builds all three targets and runs the tests | **not met as written** (the ARMCC build is a separate Windows step) |

Full detail and evidence labels: [`docs/VERIFICATION.md`](docs/VERIFICATION.md). Selected FI-04
records: [`docs/evidence/`](docs/evidence).

## Known limitations

- No recorded full hardware session to END, no hardware identifier capture, and BCS over TP not
  recorded on hardware (lean checks 1–3).
- FI-04 hardware results are owner-reported. The individual trial records were not archived, and the
  flashed images were never read back from Flash.
- **CAN bus-off** is not handled within 10 ms. The output is disabled only by the protocol timeout,
  about 1 s later. Bus-off handling was moved to a stretch goal in the lean plan.
- The hardware-tested F103 build used a Windows copy of `Project1_F103.uvprojx` that differs slightly
  from the one here (ARMCC selection metadata and the IRAM2 memory-region setting). See OI-02 in
  [`docs/OPEN_ISSUES.md`](docs/OPEN_ISSUES.md).
- The stack usage of both boards is estimated statically (F103 ≈ 652 B, G474 ≈ 800 B of 1024 B). No
  runtime measurement has been made.
- Several protocol parameters are provisional. The project used the GB/T 27930-2023 text; SAE J1939-21
  was not available.
- There is no power stage, and no physical current or timing measurement.

The full list is in [`docs/OPEN_ISSUES.md`](docs/OPEN_ISSUES.md).

## Development journey

[`docs/PROJECT_JOURNEY.md`](docs/PROJECT_JOURNEY.md) traces the project from STEP 00 to STEP 07:
- a roadmap table;
- for each step, its objective, implementation, key decisions, evidence and verdict;
- a final summary of the ARMCC builds, the 10 FI-04 hardware runs and the CONDITIONAL PASS closeout.

Recommended screenshots are listed in [`docs/assets/journey/`](docs/assets/journey/README.md).

## Project status

STEP 07 (dual-node STM32 integration, transport protocol on hardware, FI-04 fault injection) is
**closed as CONDITIONAL PASS**.

The firmware source in this repository is **byte-identical to the source that was built with ARMCC and
tested on hardware**. Packaging changed only documentation, scripts, the host CMake paths and one
regenerated checksum list, and removed unused vendor files. No firmware behaviour was changed. See
[`docs/PROVENANCE.md`](docs/PROVENANCE.md).

## License

No open-source license has been chosen for the project's own code. All rights reserved by the
author unless a license file is added later. Third-party components keep their own licenses: see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
