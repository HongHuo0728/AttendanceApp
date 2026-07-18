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
- Use `Tools` -> `Course details` to set teacher/owner, location, and notes.

## Tools

- `Import student roster (CSV)`: imports student names into the current course.
- `Add student to roster`: adds one student name to the current course roster.
- `Remove student from roster`: removes one student name from the current course roster.
- `Create records from roster`: creates Absent attendance records for roster students at the current date/time.
- `Create today's lesson`: creates a dated lesson for every roster student.
- `Switch lesson`: filters the table to an existing lesson/date.
- `Quick roll call`: opens a focused roster-by-roster attendance window for the lesson currently entered in Date/Time.
- `Lesson completion check`: compares the current date/lesson with the roster and reports missing or duplicate records before attendance is finalized.
- `Student profile`: summarizes one student's total, present, absent, late, and recent records.
- `Advanced filter`: opens syntax help and edits the table filter.
- `Report template`: chooses the Simple, Teacher, Parent, or Complete report layout.
- `Print / export PDF`: creates a printable HTML report and opens it in the browser.
- `Export PowerPoint (.pptx)`: creates a pure-black presentation with summary cards, status charts, and attendance trend charts.
- `Statistics chart`: opens a graphical status chart.
- `Statistics summary`: shows course and student attendance summary information.
- `Statistics range`: switches statistics and presentation export summaries between all records, this week, and this month.
- `Risk students`: lists roster students with repeated absences/lateness or high issue rates.
- `Undo` / `Redo`: restores recent changes.
- `Keyboard shortcuts`: shows available shortcuts.
- `Shortcut center`: edits key bindings such as `save=Ctrl+S` and `command=Ctrl+K`.
- `Command palette`: runs common commands from a typed prompt, including `rollcall` or `点名` for Quick roll call and `review` for the lesson completion check.
- `Backup now`: writes the latest backup to AppData.
- `Backup manager`: restores from timestamped backup history.
- `Restore latest backup`: restores the latest AppData backup.
- `Open recent file`: opens the last saved or imported `.attd` file.
- `Set default save folder`: chooses the initial folder for save/export dialogs.
- `Export database mirror`: writes a tab-separated database mirror to AppData.
- `Open autosave`: opens the autosaved `.attd` file.

## Quick Roll Call

1. Select the course and enter the target lesson/date in the main Date/Time field.
2. Open `Tools` -> `Quick roll call`, or press `Ctrl+K` and enter `rollcall` or `点名`.
3. Mark the current student with `Present`, `Absent`, `Late`, or `Other`. `Other` requires a note.
4. Use `Previous` to revisit the preceding student or `Skip` to leave the current record unchanged and continue.
5. Select `Finish` to apply the session. If students remain unchecked, confirm whether to keep the partial results.

Number keys `1` through `4` select Present, Absent, Late, and Other. Left Arrow moves to the previous student; Right Arrow skips forward. The window shows the existing unique record when one is available. Duplicate records for the same student and lesson must be resolved before the session can start.

All applied changes from one Quick roll call session form a single undo step. `Ctrl+Z` therefore restores the entire session at once. `Cancel`, `Escape`, or closing the window discards the working copy and leaves the workbook unchanged.

## Settings

All settings apply immediately when you change a selection or switch. `Apply` saves the current choices, while `Reset All Settings` restores the program defaults.

- `Language`: switches the entire interface to the selected language.
- `Interface Font`: lists every font installed on this PC. Selecting a font previews it across the interface immediately.
- `Animation level`: chooses the amount of interface motion. Use `Off` on a low-performance PC, `Standard` for the balanced default, or `Advanced` for the fullest transition effects.
- `Particle density`: controls how many particles appear after key actions. `Low` is the lightest option; `High` is the most decorative.
- `Report template`: chooses the information density used by exported reports. `Simple` is concise, `Teacher` emphasizes course details, `Parent` is easier to share with families, and `Complete` includes the full summary.
- `Enable particle effects`: turns the click particle burst on or off. Disable it when you prefer a quieter interface or need the smallest rendering load.
- `Enable risk student reminders`: allows the Tools menu to surface students with repeated absence or lateness.
- `Prompt for autosave recovery`: checks for a recoverable autosave when the program starts, so unsaved work can be restored.
- `Enable Ctrl+K command palette`: enables the quick command entry box. Use it for actions such as `save`, `backup`, `stats`, `lesson`, `rollcall`, `点名`, or `review`.
- `Enable advanced filter syntax`: enables terms such as `name:`, `status:`, `from:`, `to:`, `thisweek`, and `thismonth` in the search field.

### Advanced Personalization (Experimental)

Settings always opens in the compact basic view. Turn on `Advanced personalization (Experimental)` to open the personalization panel. The program first displays a warning that the experimental feature may make the software unstable; choose `Yes` to continue or `No` to leave the basic view unchanged.

- `Theme`: switches between the default pure-black interface and the pure-white light interface.
- `Accent tone`: selects Neutral, Muted sage, or Muted cyan accents while keeping text contrast restrained.
- `Text size`: selects Small, Standard, or Large interface text.
- `Interface density`: changes control spacing between Compact, Comfortable, and Spacious.
- `Autosave interval`: turns timed autosave off or runs it every 30 seconds, 1 minute, or 5 minutes.
- `Show statistics cards`: shows or hides the four summary cards above the record editor.
- `Show bottom usage hint`: shows or hides the usage hint at the lower-left window edge.
- `Show table grid lines`: shows or hides separators in the attendance list.

Turning the experimental switch off immediately discards every advanced personalization choice, collapses the panel, and restores the original pure-black theme and default layout. These discarded choices are not retained for the next time the feature is enabled.

## Search / Filter

Use the search box above the table to filter the current course by date/time, name, status, or notes. `Clear Filter` restores the full table. Editing, double-clicking, and deleting still target the real record even when the table is filtered.

When advanced filters are enabled in Settings, the filter box also accepts tokens such as:

```text
name:Alice status:Absent
date:2026-07-09
from:2026-07-01 to:2026-07-31
course:Math thisweek
```

## Roster Import Behavior

Imported roster names are added to the current course with `Absent` as the initial status. This is intended for a roll-call workflow where each student starts unmarked/absent and is changed to `Present`, `Late`, or `Other` during attendance.

## Shortcuts

- `F11`: fullscreen
- `Ctrl+S`: save `.attd`
- `Ctrl+O`: import `.attd`
- `Ctrl+Z`: undo
- `Ctrl+Y`: redo
- `Ctrl+K`: command palette

Open `Tools` -> `Shortcut center` to edit these bindings. Supported shortcut values are `Ctrl+letter`, `Ctrl+number`, or `F1` through `F24`.
