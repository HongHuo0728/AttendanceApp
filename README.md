# AttendanceApp

AttendanceApp is a standalone C++/Win32 desktop application for roll call and attendance management. It supports custom `.attd` files, multiple courses/classes, course details, student rosters, lesson workflows, CSV import/export, printable reports, autosave, undo/redo, statistics charts, backup/restore, file icons, and persistent UI settings.

## Features

- Create and edit attendance records with date/time, name, status, and notes.
- Status buttons: `Present`, `Absent`, `Late`, `Other`.
- Save/import custom `.attd` attendance files.
- Multi-course/class management from the top course selector.
- Rename, add, and delete courses/classes.
- Store course/class details such as teacher/owner, location, and notes.
- Import student rosters from CSV files.
- Manage student roster entries and generate attendance records from the roster.
- Create today's lesson from the roster and switch between lesson/date filters.
- Review lesson completion against the roster, including missing and duplicate attendance records.
- Open per-student attendance profiles and risk-student summaries.
- Use advanced filter syntax for names, status, course, date, and date ranges.
- Search/filter records by date, name, status, or notes.
- Export attendance records to CSV.
- Print or export to PDF through a generated printable HTML report with selectable report templates.
- PowerPoint `.pptx` export with a pure-black report theme, summary cards, status charts, trend charts, and template/range-aware summaries.
- Autosave every 30 seconds to `%APPDATA%\AttendanceApp\autosave.attd`.
- Restore, inspect, ignore, or delete autosave files on startup when available.
- Backup and restore the latest attendance workbook from AppData, with timestamped backup history.
- Reopen the most recently saved or imported attendance file.
- Choose a default save folder for exported and saved files.
- Undo/redo with editable keyboard shortcuts.
- Graphical statistics chart window.
- Statistics range controls for all records, this week, or this month.
- Command palette for quick save/import/export/backup/statistics/settings actions and lesson review.
- Database mirror export to `%APPDATA%\AttendanceApp\attendance.attddb`.
- Persistent settings saved to `%APPDATA%\AttendanceApp\settings.ini`.
- Pure-black interface theme, interface font selection, animation/particle intensity controls, and multilingual UI.
- Bounded, scrollable Tools menu that keeps every action reachable on smaller screens.
- Supported languages: English, Simplified Chinese, Maltese, Japanese, French, German, Russian, Traditional Chinese Taiwan, Spanish, Italian, Mongolian, Esperanto, Classical Chinese, Thai, Filipino, Turkish, Lithuanian, Norwegian, Vietnamese, Traditional Chinese Hong Kong Cantonese.
- Default window size: `1600x900`; resizable with `F11` fullscreen.

## Project Structure

```text
Attendance Project/
+-- assets/                    # Application and .attd file icons
+-- docs/                      # Extra documentation
+-- examples/                  # Example CSV roster
+-- include/                   # Header files
+-- resources/                 # Windows resource script
+-- scripts/                   # Helper scripts
+-- src/                       # C++ source code
+-- CMakeLists.txt             # CMake build file
+-- build.bat                  # Windows one-command build helper
+-- register_attd_filetype.bat # Registers .attd file icon/association
+-- LICENSE
+-- README.md
```

## Requirements

- Windows 10 or later
- Visual Studio 2022/2026 or Visual Studio Build Tools
- C++ desktop workload / MSVC toolchain
- Optional: CMake 3.20 or later

## Build

### Option 1: Build with the helper script

Run from the project root:

```bat
build.bat
```

When CMake is available, the executable is generated at:

```text
build\Release\AttendanceApp.exe
```

If the helper script falls back to direct MSVC compilation, the executable is generated at:

```text
build\AttendanceApp.exe
```

### Option 2: Build with CMake

```bat
cmake -S . -B build -A x64
cmake --build build --config Release
```

The executable is generated at:

```text
build\Release\AttendanceApp.exe
```

## Usage

1. Launch `AttendanceApp.exe`.
2. Select or create a course/class from the top `Courses` controls.
3. Enter a date/time and student name.
4. Click one of the status buttons: `Present`, `Absent`, `Late`, or `Other`.
5. Use `Save .attd` to save the attendance workbook.
6. Use `Import .attd` to load a saved attendance workbook.
7. Use the search box above the table to filter the current course by date, name, status, or notes.
8. Use `Tools` for lesson workflows, roster management, course details, lesson completion checks, student profiles, advanced filters, report templates, print/PDF export, PowerPoint export, statistics, risk students, backup/restore, recent files, default save folder, autosave, command palette, shortcut editing, and database mirror export.

## Register `.attd` File Icon

After building, run:

```bat
register_attd_filetype.bat
```

This registers the `.attd` file icon and associates `.attd` files with AttendanceApp for the current Windows user.

## Example Roster CSV

See:

```text
examples\sample_roster.csv
```

The roster importer uses the first non-empty cell in each row as the student name.
Imported roster entries are added to the current course with `Absent` as the initial status, so they can be marked during roll call.

## File Format

`.attd` files use a custom attendance format wrapped in lightweight encoding:

```text
ATTD1: + Base64(XOR encrypted custom attendance data)
```

Current files use `ATTENDANCE_V5` internally for multi-course data, course details, student rosters, and the last active course/class index. Older `ATTENDANCE_V1`, `ATTENDANCE_V2`, `ATTENDANCE_V3`, and `ATTENDANCE_V4` files are still supported on import, with a `.pre-v5.bak` safety copy created before older files are migrated.

This is lightweight protection for a custom project file format, not high-security cryptography. Do not treat `.attd` encoding as privacy protection for sensitive student data.

## License

This project is released under the MIT License. See [LICENSE](LICENSE).
