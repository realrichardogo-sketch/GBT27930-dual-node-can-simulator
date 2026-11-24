# FI04_BUILD_REPORT — STEP 07 FI-04

## 1. Builds run in WSL (host and GCC; NOT ARMCC)

| Build | Command | Result |
|---|---|---|
| Host regression configure | `cmake -S regression/gbt27930-sim-lean -B $TMPDIR/fi04/build_post -DCMAKE_BUILD_TYPE=Debug` | OK |
| Host regression build | `cmake --build $TMPDIR/fi04/build_post -j8` | OK, **0 warnings** (`-Werror -Wconversion -pedantic-errors`) |
| Host tests | `cd $TMPDIR/fi04/build_post && ctest --output-on-failure` | 29 passed, 1 skipped (`linux_sil_vcan`, no `vcan0`) |
| Board sources, GCC compile-only | `sh scripts/gcc_board_compile.sh source $TMPDIR/fi04/board_post` | PASS: G474 43/43 (-O0), F103 36/36 (-O3) |
| Cross C99 (G474/F103 HAL headers) | ctest `cross_c99_*` | pass |

Pre-change reference builds used the same commands with `build_base` / `board_base`. Logs are in
`evidence/fi04/`.

**GCC success is not ARMCC success.** ARMCC V5.06 update 7 is not available in WSL.

## 2. ARMCC build — to be run by the owner (Windows, Keil MDK 5.38, ARMCC V5.06u7 build 960)

The previous owner build (2026-11-18, outputs present in `MDK-ARM/Project1_*`) used this toolchain:
* `Armcc.exe V5.06 update 7 (build 960)`, at `D:\Flushbonding\Keil_v5\ARM\ARMCC\Bin`;
* G474: 0 errors, 0 warnings, Code 30604 / RO 1076 / RW 372 / ZI 5940, max stack 392 B;
* F103: 0 errors, 0 warnings, Code 21482 / RO 814 / RW 336 / ZI 5728, max stack 432 B.

**Those outputs predate FI-04. Do not flash them.**

No `.uvprojx` change is needed: no file was added. Both projects already compile
`..\..\Project1_SharedCore\app\gbt_integration.c`.

### Option A — µVision GUI
1. Copy the working copy to Windows keeping the folder layout (a separate timestamped export,
   §4). Do not overwrite the original D-drive package.
2. Open `source\Project1_G474\MDK-ARM\Project1_G474.uvprojx` → *Project → Rebuild all target files*.
3. Open `source\Project1_F103B_Receiver_CubeMX\MDK-ARM\Project1_F103.uvprojx` → *Rebuild all*.

### Option B — command line (existing helper; builds only, never flashes)
```
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\keil_rebuild.ps1 -UV4 "D:\Flushbonding\Keil_v5\UV4\UV4.exe"
```
It runs the following for each target, writes logs to `evidence\armcc\`, and makes `.bin` files and
SHA-256 values:
```
UV4.exe -r "<proj>" -t Project1_G474 -j0 -o "<log>"
UV4.exe -r "<proj>" -t Project1_F103 -j0 -o "<log>"
```
UV4 exit codes: 0 = no errors and no warnings, 1 = warnings, > 1 = errors.

### Record for each target
| Item | Expected |
|---|---|
| `*.axf - N Error(s), M Warning(s)` | 0 errors; any warning recorded. None expected; the new code is C99 with explicit casts. |
| Program Size (Code/RO/RW/ZI) | G474 ZI about +176 B, F103 ZI about +88 B vs 2026-11-18. Code about +0.8–0.9 KB (GCC estimate). |
| Map file contains `g_gbt_fi04` (G474) | yes |
| `.htm` Maximum Stack Usage | G474 about 392 B + tens of bytes; **F103 ≤ 768 B** (budget). Expected about 432 B + tens of bytes. |
| ARMCC-specific checks | volatile struct assignment `g_gbt_fi04 = s_gbt_integ.fi04;` follows the existing pattern (`g_gbt_can_status = cs;`); `gbt_hal_status_t` enum member in `gbt_fi04_t`: no packing assumption is made |

Then flash per HARDWARE_TEST_PROCEDURE.md §4 (one board at a time, *Verify OK*), and run
`FI04_HARDWARE_TEST_PROCEDURE.md`.

## 3. Resource estimates (GCC arm-none-eabi)

See `FI04_IMPLEMENTATION_REPORT.md` §7. Summary:
* `gbt_integ_t` 172 → 260 B; `gbt_fi04_t` 88 B;
* `gbt_integration.o` text +834 B (G474 -O0) / +908 B (F103 -O3);
* G474 `main.o` bss +88 B (`g_gbt_fi04`);
* stack frames +8 B in `apply`; `gbt_fi04_on_apply` 24 B (G474 -O0) / 8 B (F103 -O3).

## 4. Export to D drive

The working copy is ready for a **separate, timestamped export**. For example, from Windows:
```
robocopy \\wsl$\<distro>\home\<user>\projects\step07_fi04_working\STEP07_C2.4_DUAL_NODE_TP_TEST_20261118_172749 ^
         D:\Download\STEP07_FI04_<yyyymmdd_hhmmss>\STEP07_C2.4_DUAL_NODE_TP_TEST_20261118_172749 /E /COPY:DAT
```
* Never copy onto `D:\Download\STEP07_C2.4_DUAL_NODE_TP_TEST_20261118_172749`.
* After copying, verify the export with `FI04_POST_CHANGE_SHA256SUMS.txt`.
* Exclude the harness directory `.claude\`.
