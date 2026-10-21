#pragma once

#include "core/types.h"

namespace attendance {

void SaveAttendance();
void ExportV5Compatible();
bool EnsureMigrationBackup(const std::filesystem::path& path);
bool LoadAttendanceFile(const std::wstring& path, bool showSuccess,
                        AttendanceLoadIntent intent = AttendanceLoadIntent::OpenFile);
void ImportAttendance();

} // namespace attendance
