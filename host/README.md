# host/: Linux host harness for the STEP 07 shared core

This directory builds and tests the shared core of `../source/Project1_SharedCore` on Linux. It is
the former `regression/gbt27930-sim-lean/` harness of the STEP 07-C2.4/FI-04 package.

## Layout

| Path | Content |
|---|---|
| `CMakeLists.txt` | all host targets and CTest registrations (strict C99, `-Werror`) |
| `protocol/`, `common/`, `hal/`, `app/` (shared files), `ports/common/`, `ports/stm32/` | **symlinks** into `../source/Project1_SharedCore` (list: `MIRROR_FILES.txt` there). Do not edit here. |
| `app/linux_node.[ch]`, `app/linux_charger_main.c`, `app/linux_vehicle_main.c` | Linux SocketCAN node processes (host only) |
| `ports/linux/` | SocketCAN implementation of `hal/hal.h` (host only) |
| `tests/unit/`, `tests/sil/` | unit tests, dual-node SIL, port session tests |
| `tests/support/` | fake HAL, FDCAN and bxCAN register/HAL fakes |
| `tests/stm32/` | compile-time checks of STM32 HAL constants used by the ports |
| `tools/` | architecture and purity checks, cross-compile checks, vcan runner, CAN-log verifier |
| `docs/sources/` | hash-pinned requirement and standard extracts (checked by CTest `source_manifest`) |

## Use

From the repository root:

```sh
sh scripts/run_host_checks.sh        # or manually:
cmake -S host -B build/host && cmake --build build/host && (cd build/host && ctest --output-on-failure)
```

- If the symlinks became regular files (copy through Windows), restore them with
  `sh scripts/link_harness_to_source.sh`. It refuses to replace a copy that differs from `source/`.
- `linux_sil_vcan` needs a `vcan0` interface. Without it, the test reports NOT RUN (exit 77), never PASS.
- Results and coverage: [`../docs/VERIFICATION.md`](../docs/VERIFICATION.md).
- The symlinks need a Linux, macOS or WSL checkout (or Git for Windows with `core.symlinks=true`).
