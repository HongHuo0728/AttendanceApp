#include "model/operations.h"
#include "core/constants.h"
#include "core/status.h"
#include "core/text.h"
#include "model/history.h"
#include "model/query.h"
#include "model/state.h"
#include "model/workbook.h"
#include "storage/workbook_codec.h"
namespace attendance {
namespace {
bool Known(const std::wstring &s) {
  return std::find(StatusKeys.begin(), StatusKeys.end(), s) != StatusKeys.end();
}
bool Field(const std::wstring &s) {
  return WorkbookFieldIsValid(s, MAX_RECORD_FIELD_BYTES);
}
bool Commit(std::vector<AttendanceSheet> next, int active = -1) {
  if (ValidateWorkbookModel(next) != WorkbookValidationIssue::None)
    return false;
  PushUndo();
  Document().sheets = std::move(next);
  if (active >= 0) {
    Document().activeSheet = active;
    Document().visibleRows.clear();
    Document().lessonFilter.clear();
  }
  MarkDirty();
  return true;
}
} // namespace
std::vector<int> CurrentScopeRows() {
  std::vector<int> rows;
  const auto &records = Document().Records();
  for (size_t i = 0; i < records.size(); ++i)
    if (RecordMatchesFilter(records[i]))
      rows.push_back(static_cast<int>(i));
  return rows;
}
bool ChangeRecords(std::vector<int> rows, const std::wstring &status,
                   const std::wstring &note, NoteMode mode) {
  if ((!status.empty() && !Known(status)) || !Field(note))
    return false;
  std::sort(rows.begin(), rows.end());
  rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
  if (rows.empty())
    return false;
  auto next = Document().sheets;
  auto &records = next.at(Document().activeSheet).records;
  for (int row : rows) {
    if (row < 0 || static_cast<size_t>(row) >= records.size())
      return false;
    auto &r = records[row];
    if (!status.empty())
      r.status = status;
    if (mode == NoteMode::Replace)
      r.other = note;
    if (mode == NoteMode::Append && !note.empty())
      r.other += (r.other.empty() ? L"" : L"; ") + note;
    if (!ValidStatusNote(r.status, r.other) || !Field(r.other))
      return false;
  }
  return Commit(std::move(next));
}
bool MakeUpRecord(const std::wstring &lesson, const std::wstring &name,
                  const std::wstring &status, const std::wstring &reason) {
  if (TrimWide(lesson).empty() || TrimWide(name).empty() ||
      TrimWide(reason).empty() || !Known(status) || !Field(lesson) ||
      !Field(name) || !Field(reason))
    return false;
  auto next = Document().sheets;
  auto &sheet = next.at(Document().activeSheet);
  bool lessonExists = false;
  int found = -1;
  for (size_t i = 0; i < sheet.records.size(); ++i) {
    if (sheet.records[i].dateTime != lesson)
      continue;
    lessonExists = true;
    if (sheet.records[i].name == name) {
      if (found >= 0)
        return false;
      found = static_cast<int>(i);
    }
  }
  if (!lessonExists)
    return false;
  if (found < 0) {
    if (std::find(sheet.students.begin(), sheet.students.end(), name) ==
        sheet.students.end())
      return false;
    sheet.records.push_back({lesson, name, status, reason});
  } else {
    auto &r = sheet.records[found];
    r.status = status;
    r.other += (r.other.empty() ? L"" : L"; ") + reason;
  }
  return Commit(std::move(next));
}
bool RenameLesson(const std::wstring &source, const std::wstring &target) {
  if (source == target || TrimWide(target).empty() || !Field(target))
    return false;
  auto next = Document().sheets;
  auto &records = next.at(Document().activeSheet).records;
  bool found = false;
  for (const auto &r : records)
    if (r.dateTime == target)
      return false;
  for (auto &r : records)
    if (r.dateTime == source) {
      r.dateTime = target;
      found = true;
    }
  if (!found || !Commit(std::move(next)))
    return false;
  if (Document().lessonFilter == source)
    Document().lessonFilter = target;
  return true;
}
bool CopyLesson(const std::wstring &source, const std::wstring &target) {
  if (TrimWide(target).empty() || !Field(target))
    return false;
  auto next = Document().sheets;
  auto &records = next.at(Document().activeSheet).records;
  std::set<std::wstring> names;
  for (const auto &r : records) {
    if (r.dateTime == target)
      return false;
    if (r.dateTime == source)
      names.insert(r.name);
  }
  if (names.empty())
    return false;
  for (const auto &name : names)
    records.push_back({target, name, L"Pending", L""});
  return Commit(std::move(next));
}
bool CreateCourseFromTemplate(size_t source, const std::wstring &name) {
  if (source >= Document().sheets.size() || TrimWide(name).empty() ||
      !WorkbookFieldIsValid(name, MAX_METADATA_FIELD_BYTES))
    return false;
  auto next = Document().sheets;
  for (const auto &s : next)
    if (s.name == name)
      return false;
  auto sheet = next[source];
  sheet.name = name;
  sheet.records.clear();
  next.push_back(std::move(sheet));
  return Commit(std::move(next), static_cast<int>(Document().sheets.size()));
}
} // namespace attendance
