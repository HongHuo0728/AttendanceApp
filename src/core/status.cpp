#include "core/status.h"
#include "core/text.h"
#include "localization/strings.h"
namespace attendance {
int StatusIndex(const std::wstring &status) {
  for (size_t i = 0; i < StatusKeys.size(); ++i)
    if (status == StatusKeys[i])
      return static_cast<int>(i);
  return 3; // Unknown legacy values remain intact and count as Other.
}
bool StatusNeedsNote(const std::wstring &status) {
  return status == L"Other" || status == L"Leave";
}
bool ValidStatusNote(const std::wstring &status, const std::wstring &note) {
  return !StatusNeedsNote(status) || !TrimWide(note).empty();
}
void AccumulateStatus(PptStats &s, const std::wstring &status) {
  ++s.total;
  switch (StatusIndex(status)) {
  case 0:
    ++s.present;
    break;
  case 1:
    ++s.absent;
    break;
  case 2:
    ++s.late;
    break;
  case 4:
    ++s.leave;
    break;
  case 5:
    ++s.pending;
    break;
  default:
    ++s.other;
  }
}
std::wstring AttendanceRateText(const PptStats &s) {
  if (s.Eligible() <= 0)
    return Tr(L"No data", L"暂无数据");
  std::wostringstream out;
  out << std::fixed << std::setprecision(1) << s.present * 100.0 / s.Eligible()
      << L"%";
  return out.str();
}
} // namespace attendance
