#include "app/courses.h"
#include "model/workbook.h"
#include "ui/common.h"
#include "core/text.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "core/encoding.h"
#include "model/query.h"
#include "app/validation.h"
#include "ui/table.h"
#include "ui/popup_menu.h"
#include "ui/file_dialogs.h"
#include "core/csv.h"
#include "model/history.h"
#include "model/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

void AddCourse();
void RenameCurrentCourse();
void DeleteCurrentCourse();

void SwitchCourse(int index){
    EnsureSheets();
    if (index < 0 || index >= (int)Document().sheets.size() || index == Document().activeSheet) return;
    SyncActiveSheet();
    Document().activeSheet = index;
    Document().visibleRows.clear();
    Document().lessonFilter.clear();
    RefreshCourseCombo();
    RefreshList();
}

void AddCourse(){
    SyncActiveSheet();
    std::wstring name = L"Course " + std::to_wstring(Document().sheets.size() + 1);
    if (!PromptText(Tr(L"Add Course/Class", L"\u65b0\u589e\u8bfe\u7a0b/\u73ed\u7ea7"), Tr(L"Course or class name:", L"\u8bfe\u7a0b\u6216\u73ed\u7ea7\u540d\u79f0\uff1a"), name)) return;
    name = TrimWide(name);
    if (name.empty()) return;
    if (!ValidateCourseNameInput(name) || !ValidateInputField(name, MAX_METADATA_FIELD_BYTES) || !CanAppendSheets(1)) return;
    auto next = Document().sheets;
    next.push_back(MakeSheet(name));
    if (!ValidateWorkbookInput(next)) return;
    PushUndo();
    Document().sheets = std::move(next);
    MarkDirty();
    Document().activeSheet = (int)Document().sheets.size() - 1;
    Document().visibleRows.clear();
    Document().lessonFilter.clear();
    Document().Records().clear();
    RefreshCourseCombo();
    RefreshList();
}

void RenameCurrentCourse(){
    EnsureSheets();
    std::wstring name = Document().sheets[Document().activeSheet].name;
    if (!PromptText(Tr(L"Rename Course/Class", L"\u91cd\u547d\u540d\u8bfe\u7a0b/\u73ed\u7ea7"), Tr(L"New course or class name:", L"\u65b0\u8bfe\u7a0b\u6216\u73ed\u7ea7\u540d\u79f0\uff1a"), name)) return;
    name = TrimWide(name);
    if (name.empty()) return;
    if (name == Document().sheets[Document().activeSheet].name) return;
    if (!ValidateCourseNameInput(name, Document().activeSheet) || !ValidateInputField(name, MAX_METADATA_FIELD_BYTES)) return;
    auto next = Document().sheets;
    next[Document().activeSheet].name = name;
    if (!ValidateWorkbookInput(next)) return;
    PushUndo();
    Document().sheets = std::move(next);
    MarkDirty();
    RefreshCourseCombo();
}

void DeleteCurrentCourse(){
    EnsureSheets();
    if (Document().sheets.size() <= 1) {
        ShowMessage(Tr(L"At least one course must remain.", L"\u81f3\u5c11\u9700\u8981\u4fdd\u7559\u4e00\u4e2a\u8bfe\u7a0b\u3002"));
        return;
    }
    std::wstring deleteCourseMsg = Tr(L"Delete the current course and its records?", L"\u5220\u9664\u5f53\u524d\u8bfe\u7a0b\u53ca\u5176\u8bb0\u5f55\uff1f");
    std::wstring deleteCourseTitle = Tr(L"Delete Course", L"\u5220\u9664\u8bfe\u7a0b");
    if (ThemedMessageBox(Ui().hwnd, deleteCourseMsg, deleteCourseTitle, true) != IDYES) {
        return;
    }
    PushUndo();
    MarkDirty();
    Document().sheets.erase(Document().sheets.begin() + Document().activeSheet);
    Document().activeSheet = std::max(0, Document().activeSheet - 1);
    Document().visibleRows.clear();
    Document().lessonFilter.clear();
    RefreshCourseCombo();
    RefreshList();
}

void ShowCourseMenu(HWND button){
    std::wstring addCourseText = Tr(L"Add course/class", L"\u65b0\u589e\u8bfe\u7a0b/\u73ed\u7ea7");
    std::wstring renameCourseText = Tr(L"Rename current course/class", L"\u91cd\u547d\u540d\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7");
    std::wstring deleteCourseText = Tr(L"Delete current course/class", L"\u5220\u9664\u5f53\u524d\u8bfe\u7a0b/\u73ed\u7ea7");
    int command = ShowThemedPopupMenu(button, {
        {IDM_COURSE_ADD, addCourseText, false},
        {IDM_COURSE_RENAME, renameCourseText, false},
        {IDM_COURSE_DELETE, deleteCourseText, false}
    });
    if (command == IDM_COURSE_ADD) AddCourse();
    else if (command == IDM_COURSE_RENAME) RenameCurrentCourse();
    else if (command == IDM_COURSE_DELETE) DeleteCurrentCourse();
}

void ImportRosterCsv(){
    std::wstring path = OpenCsvFileDialog();
    if (path.empty()) return;
    std::string csvData;
    if (!ReadBinaryFileLimited(std::filesystem::path(path), csvData, MAX_ROSTER_FILE_BYTES)) {
        ShowMessage(Tr(L"Could not open the roster file.", L"\u65e0\u6cd5\u6253\u5f00\u540d\u5355\u6587\u4ef6\u3002"));
        return;
    }
    std::vector<std::wstring> names;
    if (!ParseRosterCsv(csvData, names)) {
        ShowMessage(Tr(L"The roster file is too large or contains invalid data.", L"名單檔案過大或包含無效資料。"));
        return;
    }
    const std::wstring importDateTime = CurrentDateTimeText();
    std::vector<AttendanceRecord> imported;
    for (const auto& name : names) imported.push_back({importDateTime, name, L"Pending", L""});
    if (!imported.empty()) {
        SyncActiveSheet();
        auto& students = Document().sheets[Document().activeSheet].students;
        std::unordered_set<std::wstring> existingStudents(students.begin(), students.end());
        std::set<std::pair<std::wstring, std::wstring>> existingRecords;
        for (const auto& record : Document().Records()) {
            existingRecords.insert({record.dateTime, record.name});
        }
        std::vector<std::wstring> newStudents;
        std::vector<AttendanceRecord> newRecords;
        for (const auto& record : imported) {
            if (existingStudents.insert(record.name).second) newStudents.push_back(record.name);
            if (existingRecords.insert({record.dateTime, record.name}).second) newRecords.push_back(record);
        }
        if ((!newStudents.empty() && !CanAppendStudents(newStudents.size()))
            || (!newRecords.empty() && !CanAppendRecords(newRecords.size()))) return;
        if (!newStudents.empty() || !newRecords.empty()) {
            auto next = Document().sheets;
            auto& target = next[Document().activeSheet];
            target.students.insert(target.students.end(), newStudents.begin(), newStudents.end());
            target.records.insert(target.records.end(), newRecords.begin(), newRecords.end());
            if (!ValidateWorkbookInput(next)) return;
            PushUndo();
            Document().sheets = std::move(next);
            MarkDirty();
            RefreshList();
        }
    }
    std::wstringstream ss;
    ss << Tr(L"Imported", L"\u5df2\u5bfc\u5165") << L" " << imported.size() << L" " << Tr(L"students into the current course.", L"\u540d\u5b66\u751f\u5230\u5f53\u524d\u8bfe\u7a0b\u3002");
    if (!imported.empty()) {
        ss << L"\n" << Tr(L"Imported roster entries start as Pending until marked.",L"导入名单的记录在点名前为未点名。");
    }
    ShowMessage(ss.str());
}

void AddStudentToRoster(){
    SyncActiveSheet();
    std::wstring name;
    if (!PromptText(Tr(L"Add student to roster", L"\u6dfb\u52a0\u5b66\u751f\u5230\u540d\u5355"), Tr(L"Student name:", L"\u5b66\u751f\u59d3\u540d\uff1a"), name)) return;
    name = TrimWide(name);
    if (name.empty()) return;
    if (!ValidateInputField(name, MAX_RECORD_FIELD_BYTES)) return;
    auto& students = Document().sheets[Document().activeSheet].students;
    if (std::find(students.begin(), students.end(), name) == students.end()) {
        if (!CanAppendStudents(1)) return;
        auto next = Document().sheets;
        next[Document().activeSheet].students.push_back(name);
        if (!ValidateWorkbookInput(next)) return;
        PushUndo();
        Document().sheets = std::move(next);
        MarkDirty();
        ShowMessage(Tr(L"Student added.", L"\u5b66\u751f\u5df2\u6dfb\u52a0\u3002"));
    }
}

void RemoveStudentFromRoster(){
    SyncActiveSheet();
    if (Document().sheets[Document().activeSheet].students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"\u540d\u5355\u4e2d\u6ca1\u6709\u5b66\u751f\u3002"));
        return;
    }
    std::wstring name;
    if (!PromptText(Tr(L"Remove student from roster", L"\u4ece\u540d\u5355\u79fb\u9664\u5b66\u751f"), Tr(L"Student name:", L"\u5b66\u751f\u59d3\u540d\uff1a"), name)) return;
    auto& students = Document().sheets[Document().activeSheet].students;
    auto it = std::find(students.begin(), students.end(), name);
    if (it != students.end()) {
        PushUndo();
        MarkDirty();
        students.erase(it);
        ShowMessage(Tr(L"Student removed.", L"\u5b66\u751f\u5df2\u79fb\u9664\u3002"));
    } else {
        ShowMessage(Tr(L"No matching records found.", L"\u672a\u627e\u5230\u5339\u914d\u8bb0\u5f55\u3002"));
    }
}

void CreateRecordsFromRoster(){
    SyncActiveSheet();
    const auto& students = Document().sheets[Document().activeSheet].students;
    if (students.empty()) {
        ShowMessage(Tr(L"No students in the roster.", L"\u540d\u5355\u4e2d\u6ca1\u6709\u5b66\u751f\u3002"));
        return;
    }
    std::wstring dateTime = GetText(Ui().dateEdit);
    if (dateTime.empty()) dateTime = CurrentDateTimeText();
    std::vector<AttendanceRecord> additions;
    std::unordered_set<std::wstring> recordedStudents;
    for (const auto& record : Document().Records()) {
        if (record.dateTime == dateTime) recordedStudents.insert(record.name);
    }
    for (const auto& student : students) {
        if (recordedStudents.insert(student).second) {
            additions.push_back({dateTime, student, L"Pending", L""});
        }
    }
    if (!additions.empty()) {
        if (!CanAppendRecords(additions.size())) return;
        auto next = Document().sheets;
        auto& records = next[Document().activeSheet].records;
        records.insert(records.end(), additions.begin(), additions.end());
        if (!ValidateWorkbookInput(next)) return;
        PushUndo();
        Document().sheets = std::move(next);
        MarkDirty();
        RefreshList();
    }
    std::wstringstream ss;
    ss << Tr(L"Roster records created.", L"\u5df2\u4ece\u540d\u5355\u521b\u5efa\u70b9\u540d\u8bb0\u5f55\u3002")
       << L" (" << additions.size() << L")";
    ShowMessage(ss.str());
}

void EditCourseDetails(){
    SyncActiveSheet();
    auto& sheet = Document().sheets[Document().activeSheet];
    std::wstring teacher = sheet.teacher;
    std::wstring location = sheet.location;
    std::wstring notes = sheet.notes;
    if (!PromptText(Tr(L"Course details", L"\u8bfe\u7a0b\u8d44\u6599"), Tr(L"Teacher/owner:", L"\u6559\u5e08/\u8d1f\u8d23\u4eba\uff1a"), teacher)) return;
    if (!PromptText(Tr(L"Course details", L"\u8bfe\u7a0b\u8d44\u6599"), Tr(L"Location:", L"\u5730\u70b9\uff1a"), location)) return;
    if (!PromptText(Tr(L"Course details", L"\u8bfe\u7a0b\u8d44\u6599"), Tr(L"Course notes:", L"\u8bfe\u7a0b\u5907\u6ce8\uff1a"), notes)) return;
    if (!ValidateInputField(teacher, MAX_METADATA_FIELD_BYTES)
        || !ValidateInputField(location, MAX_METADATA_FIELD_BYTES)
        || !ValidateInputField(notes, MAX_METADATA_FIELD_BYTES)) return;
    auto next = Document().sheets;
    auto& target = next[Document().activeSheet];
    target.teacher = teacher;
    target.location = location;
    target.notes = notes;
    if (!ValidateWorkbookInput(next)) return;
    PushUndo();
    Document().sheets = std::move(next);
    MarkDirty();
}

} // namespace attendance
