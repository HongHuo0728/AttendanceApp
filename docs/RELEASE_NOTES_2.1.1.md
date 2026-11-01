# AttendanceApp v2.1.1 LTS

This maintenance release improves Settings, popup motion, operation feedback and background file processing while preserving the existing themes, attendance workflows and V6 compatibility.

## Settings

- Remove Apply because settings already take effect and save immediately.
- Keep Reset All Settings and Close visible in a fixed footer while the options scroll.
- Improve long translated labels, large text, keyboard focus scrolling and narrow-window layouts.
- Restart the cached window's entrance on reopening and reverse an in-progress close without rebuilding its controls.
- Add close-generation checks so an obsolete completion cannot hide a newly reopened window.
- Replace jumping settings scroll positions with continuous pixel updates and cached layout geometry.
- Show a persistent inline warning and Retry when preferences cannot be saved.
- Write reset defaults atomically before applying them; keep the previous settings if the write fails.

## Interface and Motion

- Add paper-style popup reveal using cubic Bezier easing, direction-aware clipping and bounded staggered item entrance.
- Preserve native dropdown selection and keyboard behavior. A non-interactive exit surface can finish the closing transition after a selection is already committed.
- Unify Courses, Tools and related menu surfaces, spacing, row measurement, hover and focus treatment.
- Improve rounded controls, action-label wrapping, balanced titles and stable-width attendance/progress numbers.
- Respect the existing animation, smooth-scrolling and particle preferences, including immediate final states when motion is disabled.

## Operation Feedback and Responsiveness

- Add working, success, warning and failure feedback for save, export, backup and reset.
- Show loading indicators only after 150 ms; do not display invented progress percentages.
- Animate successful action buttons with a drawn check and optional small particle burst. Cancelled and failed actions never show a success check.
- Use a separate non-modal notice for menu-driven tasks.
- Move workbook file work, CSV filtering, HTML statistics, PowerPoint assembly and backup writes to immutable background snapshots.
- Keep the course, language, template and statistics period captured when an export is requested.

## Bug Fixes

- Preserve newer unsaved edits when an older captured revision finishes saving.
- Commit imported workbooks only after complete parsing and recovery protection; failed imports retain current data.
- Restore input state after a full queue rejects an import.
- Return parsed course indexes without modifying the current document during worker or migration parsing.
- Deliver background results only to the original living owner window.
- Refresh existing search results immediately when advanced syntax is enabled or disabled.
- Refresh reset settings consistently across timers, fonts, language, grid and shortcuts.
- Regenerate the current build configuration when a copied CMake cache points at a former source folder.

## Verification

- CMake Release, direct MSVC Release and native code analysis passed. Numeric application versions are 2.1.1.
- Three consecutive regression runs passed, each generating approximately 127 KiB before cleanup. The largest test file was 72,035 bytes.
- Native settings checks cover 30 cached open/close cycles per run and 360 language/text-size/density/width configurations per run.
- Parser, report XML, localization, generated-catalog, static-runtime import, file-association discovery and local Markdown-link checks passed.
- Release ZIP: 515,181 bytes. SHA-256: `527e7cb26f31b0c8a43bc5a294a3550ecb2ba72c379102ab65c2e6e64bba5ff2`.

## Compatibility and Notes

- Continue the [2.1 LTS maintenance series](https://github.com/HongHuo0728/AttendanceApp/tree/2.1LTS). FileVersion and ProductVersion are 2.1.1; the release tag is v2.1.1.
- Continue writing ATTENDANCE_V6, reading V1-V5 and offering the existing separate V5 compatibility export.
- Preserve existing settings keys and production data limits. Fourteen maintenance messages cover all 20 interface languages.
- Current native regression checks do not establish that every rendered frame is flicker-free. Desktop animation recording, the full Windows scaling matrix and clean-machine GUI startup remain unverified.
- See the [2.1.1 audit](V2.1.1_AUDIT.md) for build, regression, storage and packaging results and their limits.
