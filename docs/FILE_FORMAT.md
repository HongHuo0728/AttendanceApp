# `.attd` File Format

AttendanceApp saves attendance workbooks using a custom `.attd` file format.

The outer file structure is:

```text
ATTD1: + Base64(XOR encrypted custom attendance data)
```

The current internal text format is `ATTENDANCE_V3`, which supports multiple courses/classes in one file and stores the last active course/class index.

Older `ATTENDANCE_V1` files are still supported on import and are converted into a single default course. `ATTENDANCE_V2` multi-course files are also supported, but they do not store the active course index, so they open on the first course.

## Security Note

The file uses XOR obfuscation plus Base64 encoding. This is useful for demonstrating a custom encoded project format, but it is not strong cryptographic protection.
