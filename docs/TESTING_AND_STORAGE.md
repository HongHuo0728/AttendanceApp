# Testing and storage

## Commands

~~~bat
build.bat
build.bat -Tests
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\run_tests.ps1 -SkipBuild
scripts\analyze_native.bat
build.bat -Direct
~~~

Normal CMake builds produce the Release application only. -Tests opts into the regression executable and reuses the runtime object files. run_tests.ps1 runs three repetitions by default; -Runs 1 permits one. CTest delegates to the same isolated runner with one repetition. benchmark.bat runs the small timing corpus once; there is no copied historical baseline.

The direct compiler path uses the same VERSION templates, static runtime and Release optimizations. CMake is required for the standalone regression target and native-analysis entry point. Native analysis reuses build rather than creating build-analysis. Do not run competing build/test/cleanup commands concurrently. The optional legacy runtime_smoke_test.ps1 GUI harness also uses a marked isolated root with final cleanup, but has not been executed for this delivery.

## Storage ownership and budgets

| Area | Policy |
| --- | --- |
| build | One reusable Release object tree without a duplicate static archive; generated artifacts monitored against 512 MiB |
| build/test-work | Fixed isolated test root with an ownership marker; removed on success/failure |
| Test payload | At most 2,000 records in normal tests; current benchmark maximum is 1,000 |
| Test output | At most 5 MiB per file and 20 MiB combined, including logs |
| Summaries | Fixed overwritten storage-summary.json and test-summary.json |
| Ordinary backup history | Maximum 10; oldest ordinary copies rotate first |
| Ordinary plus protected history | 128 MiB budget; protected files are never automatically pruned |
| Active autosave/latest backup | Fixed filenames outside history quota; each workbook retains the 64 MiB file limit |
| Exports/user workbooks | User-selected locations; excluded from automatic project cleanup |

The process monitor samples sizes every 500 ms and checks final test sizes. A compiler can temporarily exceed a sampled threshold between checks; this is a stop-and-clean guard, not an OS disk quota. At the generated-artifact budget, the monitored process tree is stopped and reproducible intermediates are cleaned. Do not call the raw compiler/CMake directly if you require this wrapper's enforcement.

Large-count overflow tests pass integer counts directly to geometry helpers. No GB-scale files or large on-disk fixtures are required. Existing formal application data limits are independent of test fixture limits.

Tests override ATTENDANCEAPP_DATA_DIR; real AppData is not read or changed. Startup cleanup removes only a previously marked test root; unowned directories and linked paths are refused. An interrupted test's owned residue is removed on the next run. Traversal, junction rejection and tiny injected total/per-file quota failures are exercised before each regression run. Failure summaries are retained, while payload/log scratch files are removed.

## Cleanup

~~~powershell
.\scripts\clean.ps1
.\scripts\clean.ps1 -Mode Intermediates
.\scripts\clean.ps1 -Mode All
.\scripts\clean.ps1 -Mode All -Execute
~~~

Every mode previews by default. Legacy targets only the explicitly listed old snapshots/caches. Intermediates removes reproducible compiler/library caches while retaining the application. All removes build and the listed legacy/cache directories. Execute is required for deletion. Paths must stay inside the project and every traversed component must be free of reparse points. Root source folders and real AppData are not targets.

Historical snapshots were removed by explicit request. The local .git directory was empty; this project does not currently provide Git recovery for them. Repeated source-directory backup copies are not part of the new workflow.

## Native UI regression coverage

The ordinary small regression executable also creates disposable off-screen Win32 fixtures and in-memory paint surfaces. It verifies the active native list control DLL is version 6 or later and checks Apply-time language refresh, hidden layout geometry, nested redraw ownership, hidden settings creation/language refresh, unchanged text, edit border invalidation, buffered pixel presentation, narrow/wide control bounds, bottom actions, tab order, focus scrolling and theme changes without combo rebuild/window moves. These checks run under the same isolated data root and create no screenshots, video or large output files. They verify messages/geometry and do not establish that every visible desktop frame is flicker-free. Actual Computer Use access and visual/DPI checks are recorded separately in the audit.

## Validation and packaging

verify_release.ps1 checks the current VERSION and executable metadata, built-in parser checks and local include architecture. check_localization.mjs checks 31 feature keys and 23 settings keys across all 20 languages, plus Hong Kong call-site keys; it requires Node only for development. package_release.ps1 creates a versioned ZIP containing the app, license, usage guide, small CSV example and optional association helper. It excludes object files, libraries, debug caches, tests and user data.

Actual pass/fail results, before/after sizes and unavailable environments belong in [the current audit](V2.1.0_AUDIT.md), not inferred from historical results.
