# AttendanceApp

AttendanceApp is a standalone C++/Win32 desktop application for roll call and attendance management. It supports custom `.attd` files, multiple courses/classes, CSV import/export, printable reports, autosave, undo/redo, statistics charts, file icons, and persistent UI settings.

## Features

- Create and edit attendance records with date/time, name, status, and notes.
- Status buttons: `Present`, `Absent`, `Late`, `Other`.
- Save/import custom `.attd` attendance files.
- Multi-course/class management from the top course selector.
- Rename, add, and delete courses/classes.
- Import student rosters from CSV files.
- Search/filter records by date, name, status, or notes.
- Export attendance records to CSV.
- Print or export to PDF through a generated printable HTML report.
- Experimental PowerPoint `.pptx` export with a gradient theme, summary cards, status charts, and trend charts.
- Autosave every 30 seconds to `%APPDATA%\AttendanceApp\autosave.attd`.
- Restore autosave on startup when available.
- Undo/redo with `Ctrl+Z` and `Ctrl+Y`.
- Graphical statistics chart window.
- Database mirror export to `%APPDATA%\AttendanceApp\attendance.attddb`.
- Persistent settings saved to `%APPDATA%\AttendanceApp\settings.ini`.
- Light/dark theme, interface font selection, and multilingual UI.
- Supported languages: English, Simplified Chinese, Maltese, Japanese, French, German, Russian, Traditional Chinese Taiwan, Spanish, Italian, Mongolian, Esperanto, Classical Chinese, Thai, Filipino, Turkish, Lithuanian.
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

The executable is generated at:

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
8. Use `Tools` for roster import, print/PDF export, experimental PowerPoint export, statistics chart, undo/redo, shortcuts, autosave, and database mirror export.

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

Current files use `ATTENDANCE_V3` internally for multi-course data and the last active course/class index. Older `ATTENDANCE_V1` and `ATTENDANCE_V2` files are still supported on import.

This is lightweight protection for a custom project file format, not high-security cryptography. Do not treat `.attd` encoding as privacy protection for sensitive student data.

## License

This project is released under the MIT License. See [LICENSE](LICENSE).
