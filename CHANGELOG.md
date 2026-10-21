# Changelog

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
