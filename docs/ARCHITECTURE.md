# Architecture: AttendanceApp 2.1.0

The application is a C++17 / Win32 executable backed by independently compiled object modules in AttendanceRuntime. main.cpp delegates to the application entry point. Implementations do not include other .cpp files.

| Module | Responsibility |
| --- | --- |
| app | Window routing, documents, recovery and user workflows |
| core | Platform declarations, text/encoding/CSV and six-state vocabulary |
| model | Canonical workbook, filtering, statistics, transactional operations and undo |
| storage | Atomic bounded I/O, V1-V6 codec and backup retention policy |
| settings | Preferences, shortcuts, background font cache |
| ui | Native controls, searchable modal picker, roll call, layout and themes |
| animation | Scheduling, particles and shared animation state |
| localization | Editable catalog and independent Hong Kong vocabulary |
| reports | Immutable snapshots, bounded worker, CSV/HTML/PPTX/database exports |
| tests | In-application parser checks; standalone tests are in the root tests directory |

## Data and operations

The UI thread owns workbook/window/preferences state. The active course owns its record vector; no second mutable record collection is synchronized. Each record retains date/lesson label, student name, status key and note. Core status helpers centralize the six categories, required-note rule, eligible denominator and rate display. Unknown legacy status strings are preserved and count as Other.

The exact lesson filter is separate from advanced text search. Model operations validate a candidate workbook before committing, add one history snapshot and mark the document dirty. Lesson and course operations therefore share validation and undo with the UI. Whole-workbook history remains bounded by count and bytes. Names identify students only within a course.

Normal serialization is V6. Its structure matches V5, while its header identifies the new status semantics. V5 downgrade uses a copied workbook and does not mutate live state. Old-format overwrite protection happens during saving, not opening. Existing physical data limits are retained.

## Workers and reports

Font enumeration publishes a synchronized cache. Animation workers coalesce posted messages and never draw native controls directly. The lazy report worker accepts at most two outstanding snapshots and returns completion on the UI thread. CSV/HTML/database generation and report file writes run on that worker; PPTX XML assembly and snapshot capture remain synchronous. Shutdown finishes accepted writes before joining workers.

Report labels are captured with the data. CSV/HTML/PPTX use the statistics period; the database mirror intentionally includes the full workbook. PowerPoint chart arithmetic uses 64-bit products and is tested with synthetic counts without large files.

## Painting and settings

ui/redraw owns UI-thread redraw transactions and buffered control frames. RedrawScope preserves a hidden window's visibility and counts nested scopes per HWND, so an inner layout cannot resume an outer update. Changed text is scheduled once; unchanged text is ignored. It deliberately never sends WM_SETREDRAW(TRUE) to an initially hidden window, as documented by [Microsoft's WM_SETREDRAW contract](https://learn.microsoft.com/en-us/windows/win32/gdi/wm-setredraw).

Settings composes its child painting. The main window uses per-control buffers because whole-window WS_EX_COMPOSITED caused missing native controls on the validation host. First showing either window explicitly repaints its complete child tree after hidden initialization. The application and test executable share the Common Controls v6 manifest, which enables native list-view double buffering. Owner-drawn buttons, toggles, statistics, headers and closed combos complete background/text/effect painting in a temporary buffer before presenting it. Native edit hover animation draws the border without invalidating the text client. Routine table changes preserve content instead of restarting a whole-list reveal. Theme ticks queue painting and update DWM caption attributes without forcing frame/client reconstruction; native light/dark theme calls are cached per HWND.

Settings is a single row-major responsive form with labels above values, deferred layout positioning, one heading, shared actions, keyboard order and focus scrolling. Hidden prewarming finishes creation/theme/layout before the explicit ShowSettingsWindow call. There is no basic/advanced expansion state. Appearance preferences retain their existing persistence keys.

## Build and localization

VERSION is the numeric version source. CMake and the direct MSVC generator derive version.h, resource metadata and the application manifest in build/generated. Existing resource entry files are retained as compatibility entry points; the .in templates are authoritative. All targets use the static MSVC runtime. Normal builds exclude tests; opted-in tests reuse AttendanceRuntime objects and the same build directory. Visual Studio receives an object-target-only MSBuild import that disables the otherwise redundant Lib task; dependents link the explicit object files.

catalog.json is the editable translation source. Node regenerates catalog_data.h; --check performs no rewrite. Hong Kong vocabulary is also maintained in the independent zh_hk_strings.h dictionary. New feature phrases and the unified-settings appearance options are supplied for all 20 languages; missing legacy phrases use English outside Simplified Chinese; linguistic/visual review coverage is recorded separately from mechanical key coverage.

See [Testing and storage](TESTING_AND_STORAGE.md) for generated-file ownership, budgets and cleanup, and [the 2.1.0 audit](V2.1.0_AUDIT.md) for actual validation.
