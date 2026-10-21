#include "app/workflows.h"
#include "core/status.h"
#include "core/text.h"
#include "localization/strings.h"
#include "model/operations.h"
#include "model/query.h"
#include "model/state.h"
#include "model/workbook.h"
#include "ui/choice.h"
#include "ui/common.h"
#include "ui/dialogs.h"
#include "ui/state.h"
#include "ui/table.h"
namespace attendance {
namespace {
int PickStatus(bool preserve = false) {
  std::vector<std::wstring> names;
  for (auto key : StatusKeys)
    names.push_back(StatusDisplayText(key));
  if (preserve)
    names.push_back(Tr(L"Keep status", L"保留状态"));
  return ChooseItem(Tr(L"Status", L"状态"), names);
}
void Done(bool success) {
  if (success) {
    RefreshCourseCombo();
    RefreshList();
  } else
    ShowMessage(
        Tr(L"No changes: invalid input, duplicate target or data limit.",
           L"未修改：输入无效、目标重复或超过数据限制。"));
}
bool Confirm(size_t count) {
  return ThemedMessageBox(Ui().hwnd,
                          Document().sheets[Document().activeSheet].name +
                              L"\n" + Tr(L"Records", L"记录数") + L": " +
                              std::to_wstring(count),
                          Tr(L"Apply changes?", L"应用修改？"), true) == IDYES;
}
} // namespace
void BatchEditRecords() {
  std::vector<int> rows;
  int index = -1;
  while ((index = ListView_GetNextItem(Ui().list, index, LVNI_SELECTED)) !=
         -1) {
    auto real = VisibleToRecordIndex(index);
    if (real >= 0)
      rows.push_back(real);
  }
  if (rows.empty()) {
    ShowMessage(Tr(L"Select records first.", L"请先选择记录。"));
    return;
  }
  int status = PickStatus(true);
  if (status < 0)
    return;
  int mode =
      ChooseItem(Tr(L"Notes", L"备注"), {Tr(L"Keep notes", L"保留备注"),
                                         Tr(L"Replace notes", L"替换备注"),
                                         Tr(L"Append notes", L"追加备注")});
  if (mode < 0)
    return;
  std::wstring note;
  if (mode > 0 &&
      !PromptText(Tr(L"Notes", L"备注"), Tr(L"Notes:", L"备注："), note))
    return;
  if (!Confirm(rows.size()))
    return;
  Done(ChangeRecords(rows, status == 6 ? L"" : StatusKeys[status], note,
                     static_cast<NoteMode>(mode)));
}
void MakeUpAttendance() {
  auto lessons = CurrentLessonKeys();
  int lesson = ChooseItem(Tr(L"Make-up attendance", L"补签"), lessons);
  if (lesson < 0)
    return;
  std::set<std::wstring> unique;
  for (const auto &n : Document().sheets[Document().activeSheet].students)
    unique.insert(n);
  for (const auto &r : Document().Records())
    if (r.dateTime == lessons[lesson])
      unique.insert(r.name);
  std::vector<std::wstring> students(unique.begin(), unique.end());
  int student = ChooseItem(Tr(L"Name", L"姓名"), students);
  if (student < 0)
    return;
  int status = PickStatus();
  if (status < 0)
    return;
  std::wstring reason;
  if (!PromptText(Tr(L"Make-up attendance", L"补签"),
                  Tr(L"Reason (required):", L"原因（必填）："), reason) ||
      !Confirm(1))
    return;
  Done(MakeUpRecord(lessons[lesson], students[student], StatusKeys[status],
                    reason));
}
void ManageLesson(bool copy) {
  auto lessons = CurrentLessonKeys();
  int i = ChooseItem(Tr(L"Switch lesson", L"切换课次"), lessons);
  if (i < 0)
    return;
  std::wstring target = copy ? L"" : lessons[i];
  if (!PromptText(copy ? Tr(L"Copy lesson", L"复制课次")
                       : Tr(L"Rename lesson", L"重命名课次"),
                  Tr(L"Lesson name:", L"课次名称："), target))
    return;
  target = TrimWide(target);
  size_t count = 0;
  for (const auto &r : Document().Records())
    if (r.dateTime == lessons[i])
      ++count;
  if (!Confirm(count))
    return;
  bool success =
      copy ? CopyLesson(lessons[i], target) : RenameLesson(lessons[i], target);
  if (success) {
    Document().lessonFilter = target;
    SetText(Ui().dateEdit, target);
  }
  Done(success);
}
void CourseFromTemplate() {
  std::vector<std::wstring> names;
  for (const auto &s : Document().sheets)
    names.push_back(s.name);
  int i = ChooseItem(Tr(L"Course from template", L"从课程模板创建"), names);
  if (i < 0)
    return;
  std::wstring name;
  if (!PromptText(Tr(L"Course from template", L"从课程模板创建"),
                  Tr(L"Course or class name:", L"课程或班级名称："), name))
    return;
  Done(CreateCourseFromTemplate(static_cast<size_t>(i), TrimWide(name)));
}
} // namespace attendance
