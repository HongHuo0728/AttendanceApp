#include "model/workbook.h"
#include "model/state.h"
#include "core/constants.h"

namespace attendance {



void MarkDirty(){
    Document().dirty = true;
    ++Document().revision;
}

AttendanceSheet MakeSheet(const std::wstring& name, std::vector<AttendanceRecord> records){
    AttendanceSheet sheet;
    sheet.name = name.empty() ? L"Course" : name;
    sheet.records = std::move(records);
    return sheet;
}

void EnsureSheets(){
    if (Document().sheets.empty()) {
        Document().sheets.push_back(MakeSheet(L"Default Course"));
        Document().activeSheet = 0;
    }
}

void SyncActiveSheet(){
    EnsureSheets();
    if (Document().activeSheet < 0) Document().activeSheet = 0;
    if (Document().activeSheet >= (int)Document().sheets.size()) Document().activeSheet = (int)Document().sheets.size() - 1;
}

} // namespace attendance
