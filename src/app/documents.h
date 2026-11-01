#pragma once

#include "core/types.h"

namespace attendance {

void SaveAttendance();
bool FinishSavedSnapshot(const std::wstring& path,uint64_t generation,uint64_t revision);
bool BeginLoadAttendanceFile(const std::wstring& path,AttendanceLoadIntent intent);
void ExportV5Compatible();
bool EnsureMigrationBackup(const std::filesystem::path& path);
bool LoadAttendanceFile(const std::wstring& path, bool showSuccess,
                        AttendanceLoadIntent intent = AttendanceLoadIntent::OpenFile);
void ImportAttendance();

} // namespace attendance
