# docs/evidence: selected STEP 07 / FI-04 records

These files are copies of records written during STEP 07. They describe the state **at the time they
were written** and are not current instructions. The current summary is
[`../VERIFICATION.md`](../VERIFICATION.md).

Two files are **path-redacted**: the local user name in file-system paths was replaced by `<user>`.
No other byte was changed. All other files are verbatim. The "Original" column is the SHA-256
prefix of the archived original in the STEP 07 working edition (`history/` folder, not published).

| File | Original location (STEP 07 edition `history/`) | Original SHA-256 | Copy |
|---|---|---|---|
| [`FI04_IMPLEMENTATION_REPORT.md`](FI04_IMPLEMENTATION_REPORT.md) | `step07_package_docs/FI04_IMPLEMENTATION_REPORT.md` | `b95ba001ea796b23` | redacted → `085049a48cd1a06b` |
| [`FI04_TEST_REPORT.md`](FI04_TEST_REPORT.md) | `step07_package_docs/FI04_TEST_REPORT.md` | `eefed3d4fc62b4d1` | verbatim |
| [`FI04_BUILD_REPORT.md`](FI04_BUILD_REPORT.md) | `step07_package_docs/FI04_BUILD_REPORT.md` | `544e320952879977` | redacted → `0a8eafd96cc5305f` |
| [`STEP07_FINAL_REPORT.md`](STEP07_FINAL_REPORT.md) | `closeout_20261124/STEP07_FINAL_REPORT.md` | `8769a9e42bd7fd5d` | verbatim |
| [`fi04/post_ctest.log`](fi04/post_ctest.log) | `step07_package_evidence/fi04/post_ctest.log` | `13b2ba91b06214fd` | verbatim |
| [`fi04/post_test_sil_session.stdout`](fi04/post_test_sil_session.stdout) | `step07_package_evidence/fi04/post_test_sil_session.stdout` | `ba8dd52557f58263` | verbatim |
| [`fi04/post_test_port_g474_session.stdout`](fi04/post_test_port_g474_session.stdout) | `step07_package_evidence/fi04/post_test_port_g474_session.stdout` | `f21e09cd07d4b09b` | verbatim |
| [`fi04/post_test_port_f103_session.stdout`](fi04/post_test_port_f103_session.stdout) | `step07_package_evidence/fi04/post_test_port_f103_session.stdout` | `a9c9332ed4b79860` | verbatim |
| [`fi04/post_mutation_check.log`](fi04/post_mutation_check.log) | `step07_package_evidence/fi04/post_mutation_check.log` | `cb69bc13cd770ae2` | verbatim |
| [`fi04/post_user_code_check_g474.log`](fi04/post_user_code_check_g474.log) | `step07_package_evidence/fi04/post_user_code_check_g474.log` | `b60d5490a0750efb` | verbatim |
| [`fi04/FI04_SOURCE.diff`](fi04/FI04_SOURCE.diff) | `step07_package_evidence/fi04/FI04_SOURCE.diff` | `71162f35bc68d2cd` | verbatim |

## What these records show

- **FI04_IMPLEMENTATION_REPORT / FI04_SOURCE.diff:** the FI-04 instrumentation change (shutdown-command
  timing inside the integration layer, G474 Watch copy), made before the hardware trials.
- **FI04_TEST_REPORT and `fi04/` logs:** the host test results of that source on 2026-11-23, including the
  mutation check (the tests detect a 9 → 10 ms limit change and a stale T1).
- **FI04_BUILD_REPORT:** the Windows ARMCC build instructions. The ARMCC results themselves (0 errors,
  0 warnings on both boards) were owner-reported; logs and AXF files exist only on the owner's Windows
  machine.
- **STEP07_FINAL_REPORT:** the closeout report with the CONDITIONAL PASS verdict.

## Known inaccuracies in these records

- `STEP07_FINAL_REPORT.md` §9 says no hand-written assembly is on the runtime path. This is inaccurate:
  the F103 helper `stack_paint_f103.s` runs once before `main` (it uses no stack). See
  [`../VERIFICATION.md`](../VERIFICATION.md) §4.4.
- Reports written before the hardware trials naturally state that hardware results are pending.

## Not archived

The ten FI-04 hardware trials were **owner-reported**. Screenshots, debugger logs and per-trial values
were not archived; one trial was inspected in detail (see [`../VERIFICATION.md`](../VERIFICATION.md) §4.2).
No oscilloscope or logic-analyzer measurement exists.
