# Usage Guide

## Basic Attendance

1. Open `AttendanceApp.exe`.
2. Select the course/class from the top course selector.
3. Enter date/time and name.
4. Click `Present`, `Absent`, `Late`, or `Other`.
5. Save with `Save .attd`.

## Courses / Classes

- Use the top course selector to switch courses.
- Use `Courses` -> `Add course/class` to create a course.
- Use `Courses` -> `Rename current course/class` to rename it.
- Use `Courses` -> `Delete current course/class` to delete it.

## Tools

- `Import student roster (CSV)`: imports student names into the current course.
- `Print / export PDF`: creates a printable HTML report and opens it in the browser.
- `Export PowerPoint (.pptx)`: creates a gradient presentation with summary cards, status charts, and attendance trend charts.
- `Statistics chart`: opens a graphical status chart.
- `Undo` / `Redo`: restores recent changes.
- `Keyboard shortcuts`: shows available shortcuts.
- `Export database mirror`: writes a tab-separated database mirror to AppData.
- `Open autosave`: opens the autosaved `.attd` file.

## Search / Filter

Use the search box above the table to filter the current course by date/time, name, status, or notes. `Clear Filter` restores the full table. Editing, double-clicking, and deleting still target the real record even when the table is filtered.

## Roster Import Behavior

Imported roster names are added to the current course with `Absent` as the initial status. This is intended for a roll-call workflow where each student starts unmarked/absent and is changed to `Present`, `Late`, or `Other` during attendance.

## Shortcuts

- `F11`: fullscreen
- `Ctrl+S`: save `.attd`
- `Ctrl+O`: import `.attd`
- `Ctrl+Z`: undo
- `Ctrl+Y`: redo
