# `.attd` File Format

AttendanceApp saves attendance workbooks using a custom `.attd` file format.

The outer file structure is:

```text
ATTD1: + Base64(XOR encrypted custom attendance data)
```

The current internal text format is `ATTENDANCE_V4`, which supports multiple courses/classes in one file, stores the last active course/class index, and includes course details plus per-course student rosters.

Older `ATTENDANCE_V1` files are still supported on import and are converted into a single default course. `ATTENDANCE_V2` multi-course files are also supported, but they do not store the active course index, so they open on the first course. `ATTENDANCE_V3` files are also supported and are upgraded in memory when opened.

`ATTENDANCE_V4` stores each course/class as:

```text
course name
teacher / owner
location
notes
student count
student names...
record count
attendance records...
```

## Security Note

The file uses XOR obfuscation plus Base64 encoding. This is useful for demonstrating a custom encoded project format, but it is not strong cryptographic protection.

Do not use `.attd` encoding as a privacy or security mechanism for sensitive student data. If the attendance data must be protected, store and transfer the files using proper OS, disk, or file encryption outside AttendanceApp.
