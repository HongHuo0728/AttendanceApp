#pragma once

#include "core/types.h"

namespace attendance {

std::wstring TodayDateKey();
bool DateWithinCalendarWeek(const std::wstring& date, const std::wstring& today);
bool RecordMatchesStatsRange(const AttendanceRecord& record);
bool RecordMatchesFilter(const AttendanceRecord& record);
int VisibleToRecordIndex(int visibleIndex);
int RecordToVisibleIndex(int recordIndex);
std::wstring CurrentDateTimeText();
PptStats CountRecords(const std::vector<AttendanceRecord>& records);
std::vector<std::wstring> CurrentLessonKeys();

} // namespace attendance
