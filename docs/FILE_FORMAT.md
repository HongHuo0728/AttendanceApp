# Attendance workbook format

## Wrapper and versions

Files contain ATTD1: followed by Base64 of XOR-obfuscated UTF-8 workbook text. This is obfuscation, not strong encryption. Use appropriate external disk/file protection for sensitive information.

2.1.0 writes ATTENDANCE_V6. V1-V5 remain readable. The V6 body keeps the V5 layout:

~~~text
ATTENDANCE_V6
active course index
course count
course name
teacher / owner
location
course notes
student count
student names, one per line
record count
records: date or lesson label<TAB>name<TAB>status<TAB>notes
... repeat course block
~~~

Fields use the existing backslash escaping for tabs, CR/LF and backslashes. Counts and indices are strictly parsed, input sizes/counts are bounded and trailing non-whitespace is rejected. Malformed imports preserve the current workbook. Date/lesson labels remain text; calendar statistics require a valid leading YYYY-MM-DD.

## State semantics

Canonical keys are Present, Absent, Late, Other, Leave and Pending. Notes are retained for every status. User actions require nonblank notes for Other and Leave. Existing historical status/note values remain readable; unknown status keys remain intact and count as Other rather than being silently rewritten.

Leave and Pending are excluded from the attendance denominator and absence/late risk calculations. Present is the numerator; Late is not Present. V6 uses a distinct header to prevent older readers from silently misinterpreting the new semantic contract.

## Old data and downgrade

- V1 becomes a default course. V2 has multiple courses but no saved active index. V3 includes that index. V4/V5 include course details and rosters.
- Opening an old file does not modify it. Before first overwriting a valid old file, save creates an adjacent .pre-v6.bak copy. A failed safety copy blocks overwrite.
- Export V5 compatible file writes a separate V5 workbook. Leave/Pending map to Other; notes include [2.1.0 status: Leave] or [2.1.0 status: Pending].
- The downgrade preserves readable information but changes the old program's statistics. No automatic reverse interpretation of those note markers is performed.
- The active file and existing V6 files cannot be overwritten by the downgrade action. Normal V6 saves use atomic sibling-file replacement.

Current limits: encoded workbook 64 MiB, decoded text 48 MiB, 512 courses, 100,000 roster members per course / 250,000 total, and 250,000 records per course / 500,000 total. Test fixtures are intentionally much smaller; these production limits were not reduced.
