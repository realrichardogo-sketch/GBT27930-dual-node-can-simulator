# Provenance: hardware-tested source → this repository

## 1. Origin

| Stage | Identity |
|---|---|
| Hardware-tested release source (FI-04, 2026-11-23) | STEP 07 working copy with manifest `FI04_POST_CHANGE_SHA256SUMS.txt`, sha256 `2ba663a394b7536a268eb131ae56a372be046cf2a1c4985679dae81f4d2daf8c`. Built with ARMCC on Windows and tested on hardware (owner-reported). |
| Closeout archive (2026-11-24) | immutable snapshot, `ARCHIVE_SHA256SUMS.txt` sha256 `5f0e3b23bc1bef99534e785cff59f0a4d39a7c1fb6fa17df96e572b910cb02e6` |
| Organised STEP 07 edition (2026-11-24) | reorganised working edition; firmware byte-identical to the release source |
| **This repository (2026-11-25)** | curated subset of the organised edition, prepared for publication |

The working copy, the archive and the organised edition are not published. They are kept by the owner.

## 2. Source equivalence (verified 2026-11-25)

| Check | Result |
|---|---|
| Project-authored firmware files (board `Core/`, `MDK-ARM/` project files, `.ioc`, `.mxproject`, `Project1_SharedCore/`), excluding build outputs, listings, `.uvguix` and `DebugConfig` | **84 of 84 present.** All byte-identical, except `MIRROR_SHA256SUMS.txt` (below). |
| Combined digest of their 71 `.c/.h/.s` files (sorted `sha256sum` list, hashed) | `e91c1ccc5bea71c9ad02cbde9851c40e9cfba32ddb02a66acf443045bd0bd7a8`, **identical** for the release source and this repository |
| All 482 files under `source/` checked against the FI-04 manifest | 481 OK, 1 differs (`MIRROR_SHA256SUMS.txt`), 0 not listed |
| `host/` against the release harness (`regression/gbt27930-sim-lean/`) | identical, except `CMakeLists.txt` and `README.md`; the 41 shared-core files are symlinks to byte-identical files in `source/` |

**No firmware source file (`.c`, `.h`, `.s`, `.ioc`, `.uvprojx`, `.uvoptx`, `.sct`) was changed.** No CAN
timing, transport protocol, state machine, safety or shutdown logic was touched.

## 3. Files that differ from the release source

| File | Release source (sha256 prefix) | This repository | Change | Executable effect |
|---|---|---|---|---|
| `source/Project1_SharedCore/MIRROR_SHA256SUMS.txt` | `a95445b9adf86211` | `1efa409c63105885` | regenerated: 8 entries were stale (organised edition, CR-03) | none (checksum list) |
| `host/CMakeLists.txt` | `23de75c8ec5fc2c4` | `255eaf6f37da65f8` | defaults of `GBT_G474_REF` / `GBT_F103_REF` point to `../source/<board>` instead of directories outside the repository | host build only: the `cross_c99_g474/f103` checks use the HAL headers of the board projects in this repository |
| `host/README.md` | `6fd0d565277f2e27` | `c601add0c3d556b9` | rewritten for this layout | none |
| `scripts/link_harness_to_source.sh` | `917f111ccfa68ae3` | `320240def42451a3` | harness path `host/` (one level up instead of two) and comments | host tooling only |
| `scripts/keil_rebuild.ps1` | `c84e49091ee1ebd7` | `91b03a0a3e797f9b` | default `UV4.exe` path changed from a machine-specific location to the Keil default `C:\Keil_v5\UV4\UV4.exe`; usage comment | Windows build helper only (never run, as before) |
| `scripts/run_host_checks.sh` | — (new) | `fb1335eef36da4c0` | chains the existing host checks and counts *not run* tests explicitly | host tooling only |

Documentation under `docs/` (except the verbatim `docs/hardware/*` bench documents and `docs/design/*`)
and the root `README.md` were written or adapted for this repository. `docs/hardware/README.md` was
adapted to this layout. The evidence copies are listed in [`evidence/README.md`](evidence/README.md).

## 4. Excluded on purpose

| Excluded | Reason |
|---|---|
| Keil output directories (`MDK-ARM/Project1_G474/`, `MDK-ARM/Project1_F103/`), `*.lst`, `*.uvguix.*`, `MDK-ARM/DebugConfig/` | build outputs from before FI-04 (risk of flashing a stale image), per-user GUI and debug-session state |
| Vendor CMSIS `DSP/`, `NN/`, `RTOS/`, `RTOS2/`, `Lib/`, `docs/`, `Core/`, `Core_A/` and device `Source/Templates/` | not referenced by either Keil project. The projects use `CMSIS/Include` and the device `Include/`. 1652 vendor files were left out, and 398 were kept. |
| STEP 07 `history/` (package reports, older evidence, superseded integration packages, bring-up firmware) | historical; superseded by the current documents. Selected FI-04 records are in `docs/evidence/`. |
| Trace-equivalence script and `trace_equiv.c` | need the unpublished historical development tree as reference |
| STEP 07 review documents (change review, compatibility with a separate reconstruction, edition manifest) | internal to the organisation work |
| Host build directory `build/` | generated (`.gitignore`) |
| GB/T 27930-2023 PDF | copyrighted standard; never part of the source |

## 5. Firmware traceability (unchanged from the closeout)

| Link | Status |
|---|---|
| Source in this repository = hardware-tested source | verified (section 2) |
| ARMCC builds of the tested source: G474 and F103, 0 errors / 0 warnings | owner-reported. Logs, MAP and AXF are on Windows only. AXF SHA-256: G474 `5A097A1F43E644FA70E18C70F0BBDC76C294A6ECDCF3DD6E756C9BFDF31763A4`, F103 `3E8CC5120BF7EAA423A3776826DFE315029A25554C3EA11BF7F93807024D3A05` |
| Tested F103 `.uvprojx` = this repository | **differs** (ARMCC selection metadata, `Ir2Chk` 1 → 0); see OI-02 in [OPEN_ISSUES.md](OPEN_ISSUES.md) |
| Flashed image = AXF | not verified (no Flash readback) |

A rebuild from this repository is **not** the tested binary until it is rebuilt with ARMCC and the AXF
hash is compared, and the F103 project setting of OI-02 is applied.
