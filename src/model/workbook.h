#pragma once

#include "core/types.h"

namespace attendance {

void MarkDirty();
AttendanceSheet MakeSheet(const std::wstring& name, std::vector<AttendanceRecord> records = {});
void EnsureSheets();
void SyncActiveSheet();

} // namespace attendance
