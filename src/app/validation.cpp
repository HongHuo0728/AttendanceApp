#include "app/validation.h"
#include "localization/strings.h"
#include "storage/workbook_codec.h"
#include "model/workbook.h"
#include "ui/dialogs.h"
#include "model/state.h"
#include "core/constants.h"

namespace attendance {

size_t TotalStudentCount();
size_t TotalRecordCount();

void ShowWorkbookValidationError(WorkbookValidationIssue issue){
    const wchar_t* english = L"The attendance data exceeds the supported safety limits.";
    const wchar_t* chinese = L"考勤数据超过了软件支持的安全限制。";
    if (issue == WorkbookValidationIssue::FieldTooLarge) {
        english = L"One or more text fields are too large to save safely.";
        chinese = L"一个或多个文本字段过长，无法安全保存。";
    }
    ShowMessage(Tr(english, chinese), Tr(L"Data limit reached", L"已达到数据限制"));
}

bool ValidateInputField(const std::wstring& value, size_t maximumBytes){
    if (WorkbookFieldIsValid(value, maximumBytes)) return true;
    ShowWorkbookValidationError(WorkbookValidationIssue::FieldTooLarge);
    return false;
}

size_t TotalStudentCount(){
    size_t total = 0;
    for (const auto& sheet : Document().sheets) total += sheet.students.size();
    return total;
}

size_t TotalRecordCount(){
    size_t total = 0;
    for (const auto& sheet : Document().sheets) total += sheet.records.size();
    return total;
}

bool CanAppendSheets(size_t count){
    if (count <= MAX_SHEET_COUNT - std::min(Document().sheets.size(), MAX_SHEET_COUNT)) return true;
    ShowWorkbookValidationError(WorkbookValidationIssue::TooManySheets);
    return false;
}

bool CanAppendStudents(size_t count){
    SyncActiveSheet();
    const size_t activeCount = Document().sheets[Document().activeSheet].students.size();
    const size_t total = TotalStudentCount();
    if (count <= MAX_STUDENTS_PER_SHEET - std::min(activeCount, MAX_STUDENTS_PER_SHEET)
        && count <= MAX_TOTAL_STUDENTS - std::min(total, MAX_TOTAL_STUDENTS)) return true;
    ShowWorkbookValidationError(WorkbookValidationIssue::TooManyStudents);
    return false;
}

bool CanAppendRecords(size_t count){
    SyncActiveSheet();
    const size_t total = TotalRecordCount();
    if (count <= MAX_RECORDS_PER_SHEET - std::min(Document().Records().size(), MAX_RECORDS_PER_SHEET)
        && count <= MAX_TOTAL_RECORDS - std::min(total, MAX_TOTAL_RECORDS)) return true;
    ShowWorkbookValidationError(WorkbookValidationIssue::TooManyRecords);
    return false;
}

} // namespace attendance
