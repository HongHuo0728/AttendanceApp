#pragma once
#include "core/types.h"
namespace attendance {
inline constexpr std::array<const wchar_t *, 6> StatusKeys = {
    L"Present", L"Absent", L"Late", L"Other", L"Leave", L"Pending"};
int StatusIndex(const std::wstring &status);
bool StatusNeedsNote(const std::wstring &status);
bool ValidStatusNote(const std::wstring &status, const std::wstring &note);
void AccumulateStatus(PptStats &stats, const std::wstring &status);
std::wstring AttendanceRateText(const PptStats &stats);
} // namespace attendance
