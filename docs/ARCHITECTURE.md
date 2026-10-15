# Architecture

AttendanceApp 2.0.4 (Ultra Optimization) uses independently compiled C++17 modules, linked into `AttendanceRuntime`. `src/main.cpp` delegates to the application entry point. No implementation file includes another implementation file.

| Directory | Responsibility |
| --- | --- |
| app | Entry point, message routing, commands, document operations |
| core | Platform declarations, shared types, text, encoding, constants |
| model | Canonical workbook, queries, history |
| storage | Bounded file reads, atomic writes, V1-V5 parsing and V5 encoding |
| settings | Preferences, validation, keyboard bindings, background font cache |
| ui | Windows, controls, menus, tables, themes, layout |
| animation | Scheduling, particles, and shared animation state |
| localization | Existing 20-language translations and display labels |
| reports | CSV, HTML/PDF, PowerPoint, database mirror, ZIP packaging |
| tests | In-application parser checks |
| ../tests | Standalone regression, localization equivalence and performance tests |

Document, UI, preferences, font-cache, and animation state each have one owner. Function-local state initialization avoids cross-translation-unit static initialization order dependencies. `Document().Records()` refers directly to the active sheet's records; there is no second mutable record vector to synchronize.

The UI thread owns document and window state. Background font enumeration publishes under its mutex; animation state uses its own mutex, atomic shutdown/FPS values, and coalesced posted messages. Workers do not directly draw controls. Existing theme and particle rendering behavior is retained.

Report jobs have one lazy worker and a maximum of two outstanding snapshots. CSV, HTML and database tasks own immutable copies of the data and localized labels. PowerPoint jobs own already-built XML entries before ZIP packaging. Workers never access live document, preference or window state; completion messages are delivered on the UI thread. Shutdown finishes accepted writes, joins the worker and suppresses orphan completion dialogs.

`localization/catalog.json` is the editable source for the five general translation tables. `node scripts/generate_catalog.mjs` regenerates the checked-in `catalog_data.h`; `--check` verifies it without rewriting. The header uses fixed string-pool offsets to reduce PE data and relocations. Node is only needed to regenerate translations, not for a normal C++ build. Hong Kong's independent dictionary remains in `include/zh_hk_strings.h`.

The deprecated `advanced_personalization` key is accepted for compatibility but does not gate settings. Writers emit `1` so older versions that understand this key can retain personalized choices. Numeric versions remain `2.0.4`; the display title includes `(Ultra Optimization)`.

## Building and Testing

Run `build.bat` for the Release executable and `AttendanceTests.exe`. The non-CMake fallback compiles each source to a uniquely named object, including modules with the same basename.

The application is in `build/Release`, test executables are in `build/tests/Release`, and the internal link library is in `build/lib/Release`. Only the application executable is needed for the application release; the static library is a build artifact.

Run `ctest --test-dir build -C Release --output-on-failure` for isolated regression tests. Run `scripts/verify_release.ps1 -SkipBuild`, `scripts/analyze_native.bat`, and `scripts/runtime_smoke_test.ps1` for release, static, and desktop runtime checks.

The optional `AttendanceBaseline` target is available only when the local pre-refactor snapshot exists. It is not required for users to build the app. It runs the same benchmark and localization inputs against the original implementation.

Run `scripts/check_architecture.ps1` to verify local includes and the header dependency graph. See [the 2.0.4 audit](V2.0.4_AUDIT.md) for measured results, confirmed fixes and verification limits.
