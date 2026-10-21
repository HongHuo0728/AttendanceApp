# AttendanceApp 2.1.0 usage

## Getting started

1. Launch AttendanceApp.exe and select a course, or use Courses to create one.
2. Import examples/sample_roster.csv through Tools, or add roster members manually.
3. Create a lesson through Tools. Choose a unique label, preferably starting with YYYY-MM-DD, such as 2026-10-20 09:00 Math.
4. Use Quick roll call or select/edit records in the main window.
5. Save the workbook with Save .attd or Ctrl+S.

Roster import and lesson creation produce Pending records. Existing historical Absent records are never inferred to be Pending. Course-local student identity is the exact name; this release does not introduce student IDs or merge people across courses. Avoid using the same name for different students in one roster.

## Status and notes

| Status | Meaning | Included in attendance denominator | Counts as an absence/late issue |
| --- | --- | --- | --- |
| Present | Attended | Yes; numerator | No |
| Absent | Confirmed absence | Yes | Yes |
| Late | Arrived late | Yes | Yes |
| Other | Other recorded outcome; note required | Yes | No |
| Leave | Approved leave; reason required | No | No |
| Pending | Not yet marked | No | No |

Notes can be stored for every status. Other and Leave cannot be submitted without a nonblank note. The rate is Present divided by eligible records; no eligible records displays No data. Alerts require at least three absence/late issues or an issue rate of 20% or more.

The six main status buttons add a record or replace the selected record using the editor fields. Update Selected edits the selected record's date/name/notes without changing its status. Double-click loads a row into the editor. Use Ctrl/Shift to select multiple rows.

## Quick roll call

Enter the lesson in Date/Time, then choose Tools > Quick roll call. Keys 1-6 choose Present, Absent, Late, Other, Leave and Pending. Left Arrow goes back; Right Arrow skips. Number keys remain normal input while editing notes. Review and Pending counts are checked at Finish; partial results require confirmation. Cancel, Escape or closing the window discards the session. Finish applies one undo step. Resolve duplicate student/lesson records before starting.

## Batch changes and make-up attendance

Tools > Batch edit selected lets you retain or change status, then keep, replace or append notes. Confirm the course and number of selected records. The entire operation validates before applying and is one undo step.

Tools > Make-up attendance selects an existing lesson and a student from the roster/lesson, chooses a status and requires a reason. An existing unique record is corrected; a missing roster member receives a record. Duplicate records block the correction. Reasons append to existing notes. This is a correction workflow, not an immutable audit log.

Scope: Present and bulk deletion by status affect the current course's visible filter/lesson scope. Their confirmation shows the course and affected count. Clear all records remains an explicitly named whole-course operation.

## Lessons and templates

- Switch lesson opens a searchable list without a 20-item limit. It matches the complete lesson label independently of the advanced-filter preference.
- Clear Filter clears both text and lesson filters. Switching course clears the lesson filter.
- Rename lesson updates every record with that exact label. Existing target labels are rejected.
- Copy lesson retains its unique members, resets all copied states to Pending and clears notes. It never merges into an existing target lesson.
- Course from template copies a chosen course's roster, teacher, location and course notes into a new course. Attendance history is excluded; duplicate course names are rejected. No template files are generated.
- Rename/copy/template operations support undo and redo.

Use date-leading labels for date-based reports. Free-text labels can still identify lessons, but cannot reliably participate in calendar period filtering or chronological sorting.

## Search, statistics and student profiles

Text search checks date, name, status and notes. With advanced syntax enabled, name:Alice, status:Absent, from:2026-10-01, to:2026-10-31, thisweek and thismonth are available. Multiple conditions narrow the result. The separate exact lesson filter applies in addition to the text filter.

This week means Monday through the following Sunday in local calendar dates; This month means the local calendar month. Summary cards and exports use the selected statistics period. Student profiles use an exact name within the current course and display up to 12 records in descending date-label order. Risk summaries use the current course and statistics period.

## Save, compatibility and reports

Normal save writes V6, preserving all six statuses. Opening V1-V5 does not rewrite the file. A successful first overwrite requires an adjacent .pre-v6.bak safety copy. Save uses a file dialog; the default save folder can be selected through Tools.

Tools > Export V5 compatible file creates a separate copy for older programs. Leave/Pending become Other with notes beginning [2.1.0 status: ...]. Old versions calculate these as Other; original semantics and rates are not preserved. Reimporting the downgraded file does not automatically reconstruct the new statuses.

CSV, printable HTML/PDF and PowerPoint capture the current course and statistics period. PowerPoint contains a six-status summary, bars and recent date groups; report templates retain their metadata/roster emphasis. Print/PDF opens HTML in the browser. The database mirror exports all courses to attendance.attddb as tab-separated text. Export workers use immutable snapshots; accepted writes finish during shutdown.

## Recovery and backup

Autosave intervals are Off, 30 seconds, 1 minute or 5 minutes. Startup can restore, inspect, ignore or delete an existing autosave. Ignored recovery data is protected before replacement. A failed final autosave warns before exit.

Backup now maintains backup-latest.attd and timestamped history. Ordinary history is limited to 10 copies; ordinary plus protected history has a 128 MiB budget. Oldest ordinary copies rotate first. Protected recovery copies are never removed automatically. If they prevent a new protected copy, the original recovery file remains in place and the operation reports failure. Export or intentionally manage old protected files before retrying.

## Settings and distribution

Settings opens one unified form; there is no separate Advanced personalization section. Labels appear above values, with two columns when space permits and one on narrow displays. Scroll to reach all options and the Reset / Apply / Close actions. Tab follows the visible form order and scrolls the focused control into view. Reopening an already visible Settings window raises it without resetting its scroll position. Settings apply immediately and Apply saves them. Language, font, black/white themes, text scale, spacing, animation/particles, autosave, statistics cards, grid lines, shortcuts and smooth scrolling are available without an experimental gate. The legacy advanced_personalization key remains readable for compatibility. Dropdown scheduling supports 60-360 FPS, default 60.

Default shortcuts: Ctrl+S Save, Ctrl+O Import, Ctrl+Z Undo, Ctrl+Y Redo, Ctrl+K command palette, F11 fullscreen. Tools > Shortcut center changes them.

To register .attd double-click opening for the current Windows user, run register_attd_filetype.bat after placing the program in its intended location. It discovers an adjacent release executable or the project build output and uses the embedded file icon. Registration changes that user's file association; moving the executable later requires registering again.

Each recipient uses their own AppData and workbook files. Concurrent shared editing and synchronization are outside this release.
