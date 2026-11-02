# Changelog

## 2.1.1 LTS UI repair - 2026-11-02

- Follow-up: move settings through one clipped content viewport and release opaque-window layering; share the Tools popup implementation with settings dropdowns and the main course selector, including continuous two-way arrow motion. See [the settings/dropdown follow-up](docs/V2.1.1_SETTINGS_POPUPS.md).
- Repair table/composer overlap, translated and scaled layouts, flat cards, shared button edges, Tab/Enter navigation and main/table wheel routing.
- Reduce settings scroll work, prevent wheel input from changing closed combos, and preserve font hierarchy, input alignment and dropdown metrics.
- Repair menu scrollbar input, popup frame invalidation, chart sizing/completion/close opacity, roll-call progress, and measured/scrollable dialogs.
- Preserve all settings across saving/reloading, accept Windows BOM/CRLF files, complete physical animation lifetimes and preserve process quit messages.
- Publish the two-round UI repair package and align the v2.1.1 source tag and release notes with the repaired implementation. Preserve the first withdrawal explanation and actual UI screenshots. See [the release notes](docs/RELEASE_NOTES_2.1.1.md) and [UI repair audit](docs/V2.1.1_UI_REPAIR.md).

## 2.1.1 LTS - 2026-11-01

- Remove Apply and keep Reset All Settings / Close in a fixed settings footer. Add continuous content scrolling and a fresh, reversible cached-window entrance on each opening.
- Unify cubic Bezier popup motion with clipped paper-style reveal, bounded item staggering and non-interactive native dropdown exit decoration.
- Standardize rounded surfaces, spacing, measured/wrapped action labels, touch targets and stable-width numeric drawing while keeping existing themes and preferences.
- Add truthful delayed loading and completion feedback for file and report operations; successful buttons draw a check and optional bounded particles.
- Write settings defaults atomically before applying reset, and provide inline persistence failure/retry feedback.
- Move document file work and report assembly to immutable background snapshots. Preserve newer edits when an older revision finishes saving.
- Keep worker parsing independent of the current active course, refresh advanced-filter results immediately, and validate task callbacks against their original window.
- Complete 14 maintenance messages across 20 languages. Retain V6 and existing settings keys.
- Detect copied CMake caches pointing at another source directory and regenerate the current project's configuration.

Current verification and limitations are recorded in [the 2.1.1 audit](docs/V2.1.1_AUDIT.md).

## 2.1.0 - 2026-10-21

- Follow-up UI fix: removed the separate Advanced personalization section; Settings is one responsive form with matching tab order and automatic focus scrolling.
- Fixed hidden settings being exposed during redraw initialization, repeated synchronous text/combo paints and whole-table reveal restarts on routine operations. Buffered buttons, toggles, statistics, headers and closed combos present complete control frames; native edits animate only their border.
- Completed 23 unified-settings translation keys across all 20 languages (22 previously absent from the shared feature catalog); missing legacy translations outside Simplified Chinese now fall back to English.
- Enabled Common Controls v6 so native list double buffering is effective. Corrected missing main-window controls under whole-window composition and Apply-time language refresh.
- Theme changes queue complete frames, reuse unchanged native themes and avoid rebuilding/repositioning settings. Autosave/grid/display changes update only the affected timer or controls.

- Added Leave/Pending, shared note validation, correct eligible-record statistics and six-state reports.
- Added transactional batch edits, reasoned make-up attendance, searchable lessons, lesson rename/copy and course templates.
- Added V6 saves, V1-V5 reading, overwrite-time migration safety and explicit V5 downgrade export.
- Fixed scoped bulk actions, calendar-week boundaries and current-course exact student profiles.
- Added ordinary-backup rotation, protected-recovery capacity checks and small isolated repeatable tests.
- Centralized version metadata and static runtime builds; fixed executable discovery for file associations.
- Removed requested old snapshots/baselines and corrected current English documentation.

See [the audit](docs/V2.1.0_AUDIT.md) for observed verification and limitations. No release upload is implied.

## 2.0.4 - historical

Modular compilation, broader personalization availability, improved animation scheduling, atomic writes and bounded report jobs. Original measurements are retained in the [historical audit](docs/V2.0.4_AUDIT.md) and are not 2.1.0 results.
