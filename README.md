# AttendanceApp 2.1.1 LTS

AttendanceApp is an offline C++17 / Win32 attendance manager for Windows 10 and later. Each user runs an independent local copy. It manages courses, rosters, lessons, attendance, recovery files and reports without a server.

## What is new in 2.1.1 LTS

- Settings save immediately without an Apply button. Reset All Settings and Close remain visible in a fixed footer while only the options scroll.
- Cached settings reopen with a fresh animation; scrolling, dropdowns and command menus share continuous motion and cubic Bezier easing.
- Paper-style popup reveal, rounded surfaces, consistent spacing, wrapped action labels and stable-width statistics retain the existing themes.
- Save, export, backup and reset operations report real working/success/failure states. File import and save, report generation and backup writes use background snapshots.
- Atomic settings reset, immediate advanced-filter refresh, pure workbook parsing and per-window task callbacks address maintenance defects.
- New maintenance messages are available in all 20 interface languages. Existing V6 workbooks and settings remain compatible.

The 2.1 LTS series is maintained on [2.1LTS](https://github.com/HongHuo0728/AttendanceApp/tree/2.1LTS). Subsequent 2.1.x releases provide accumulated maintenance fixes; no fixed end-of-support date is specified.

Attendance rate is Present / (Total - Leave - Pending). A zero denominator displays No data. Late remains separate from Present. Leave and Pending do not trigger absence/late alerts.

## Downloaded application

Extract the complete release ZIP and launch AttendanceApp.exe. No development tools or separate MSVC runtime installation are required by the statically linked C++ runtime. Windows system components are still required. See the included QUICK_START.md, or [Usage](docs/USAGE.md), for workflows.

The application uses the existing 20 interface languages, dark/light themes, configurable fonts and layout, optional particles, 60-360 FPS dropdown scheduling, keyboard shortcuts, autosave and undo/redo. Actual animation presentation depends on hardware and Windows composition.

## Build

Install Visual Studio / Build Tools with the C++ desktop workload. CMake 3.20+ is recommended. From the project directory:

~~~bat
build.bat
build.bat -Tests
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\run_tests.ps1 -SkipBuild
~~~

The normal CMake executable is build/Release/AttendanceApp.exe. Tests are opt-in and run three small isolated repetitions by default. Direct MSVC fallback is available with build.bat -Direct and produces build/AttendanceApp.exe. Both use the static C++ runtime. Node is needed only to regenerate translations, not to build checked-in C++ sources.

Optional checks:

~~~powershell
.\scripts\verify_release.ps1 -SkipBuild
.\scripts\check_architecture.ps1
node scripts/generate_catalog.mjs --check
.\scripts\clean.ps1
.\scripts\package_release.ps1
~~~

Cleanup previews by default. See [Testing and storage](docs/TESTING_AND_STORAGE.md) for exact limits, safe cleanup and analysis commands.

## Workbooks and reports

Normal saves use V6. V1-V5 remain readable and are upgraded only when saved. Before overwriting an older valid file, the program creates a .pre-v6.bak copy; failure blocks the overwrite. V5 compatibility export maps Leave/Pending to Other with explanatory notes, warns about different old-version statistics and cannot overwrite the active workbook or an existing V6 file. This export is a downgrade, not a lossless interchange format.

CSV, printable HTML and PowerPoint use the current statistics period. PDF export uses the browser's print-to-PDF feature. The database mirror is a tab-separated full-workbook export, not a database server. Report writes run through the bounded background worker and atomic file replacement.

The ATTD1 wrapper uses XOR obfuscation and Base64, not strong encryption. See [File format](docs/FILE_FORMAT.md).

## Data locations

- Settings, active autosave, latest backup and database mirror: %APPDATA%\AttendanceApp.
- Timestamped backups and protected recovery copies: its backups subdirectory.
- Saved workbooks and exported reports: the location selected by the user.
- ATTENDANCEAPP_DATA_DIR overrides the application data root; tests always use an isolated project-local directory.

User data is separate from build cleanup. Existing protected recovery files are never automatically pruned to meet the backup budget.

## Documentation

- [Usage](docs/USAGE.md)
- [Architecture](docs/ARCHITECTURE.md)
- [File format](docs/FILE_FORMAT.md)
- [Testing and storage](docs/TESTING_AND_STORAGE.md)
- [Changelog](CHANGELOG.md)
- [2.1.1 implementation and validation audit](docs/V2.1.1_AUDIT.md)
- [2.1.0 implementation and validation audit](docs/V2.1.0_AUDIT.md)
- [Historical 2.0.4 audit](docs/V2.0.4_AUDIT.md)

MIT License. See [LICENSE](LICENSE).
