#pragma once

#include "core/types.h"

namespace attendance {

void SaveAttendance();
bool LoadAttendanceFile(const std::wstring& path, bool showSuccess,
                        AttendanceLoadIntent intent = AttendanceLoadIntent::OpenFile);
void ImportAttendance();

} // namespace attendance
