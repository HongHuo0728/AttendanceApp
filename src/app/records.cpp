#include "app/records.h"
#include "model/workbook.h"
#include "ui/common.h"
#include "model/query.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "app/validation.h"
#include "ui/table.h"
#include "ui/popup_menu.h"
#include "model/history.h"
#include "model/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

std::vector<int> SelectedRows();
void EraseRows(std::vector<int> rows);
void DeleteSelectedRecords();
void DeleteRecordsByStatus(const std::wstring& status);
void ClearAllRecords();

void AddOrUpdateRecord(const std::wstring& status){
    std::wstring dateTime = GetText(Ui().dateEdit);
    std::wstring name = GetText(Ui().nameEdit);
    std::wstring other = status == L"Other" ? GetText(Ui().otherEdit) : L"";

    if (dateTime.empty()) {
        ShowMessage(Tr(L"Please enter a date and time.", L"\u8bf7\u8f93\u5165\u65e5\u671f\u548c\u65f6\u95f4\u3002"));
        return;
    }
    if (name.empty()) {
        ShowMessage(Tr(L"Please enter a name.", L"\u8bf7\u8f93\u5165\u59d3\u540d\u3002"));
        return;
    }
    if (status == L"Other" && other.empty()) {
        ShowMessage(Tr(L"Please fill the Other field.", L"\u8bf7\u586b\u5199\u5176\u4ed6\u5b57\u6bb5\u3002"));
        return;
    }
    if (!ValidateInputField(dateTime, MAX_RECORD_FIELD_BYTES)
        || !ValidateInputField(name, MAX_RECORD_FIELD_BYTES)
        || !ValidateInputField(other, MAX_RECORD_FIELD_BYTES)) return;

    int selected = VisibleToRecordIndex(ListView_GetNextItem(Ui().list, -1, LVNI_SELECTED));
    if (selected < 0 && !CanAppendRecords(1)) return;
    AttendanceRecord record{dateTime, name, status, other};
    PushUndo();
    MarkDirty();
    if (selected >= 0 && selected < (int)Document().Records().size()) {
        Document().Records()[selected] = record;
    } else {
        Document().Records().push_back(record);
    }
    RefreshList();
}

void LoadRecordIntoEditor(int index){
    index = VisibleToRecordIndex(index);
    if (index < 0 || index >= (int)Document().Records().size()) {
        ShowMessage(Tr(L"Please select a record to edit.", L"\u8bf7\u9009\u62e9\u8981\u7f16\u8f91\u7684\u8bb0\u5f55\u3002"));
        return;
    }

    const auto& record = Document().Records()[index];
    SetText(Ui().dateEdit, record.dateTime);
    SetText(Ui().nameEdit, record.name);
    SetText(Ui().otherEdit, record.other);
    int visibleIndex = RecordToVisibleIndex(index);
    if (visibleIndex >= 0) {
        ListView_SetItemState(Ui().list, visibleIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_EnsureVisible(Ui().list, visibleIndex, FALSE);
    }
}

void UpdateSelectedRecord(){
    int selected = VisibleToRecordIndex(ListView_GetNextItem(Ui().list, -1, LVNI_SELECTED));
    if (selected < 0 || selected >= (int)Document().Records().size()) {
        ShowMessage(Tr(L"Please select a record to edit.", L"\u8bf7\u9009\u62e9\u8981\u7f16\u8f91\u7684\u8bb0\u5f55\u3002"));
        return;
    }

    std::wstring dateTime = GetText(Ui().dateEdit);
    std::wstring name = GetText(Ui().nameEdit);
    std::wstring other = GetText(Ui().otherEdit);
    if (dateTime.empty()) {
        ShowMessage(Tr(L"Please enter a date and time.", L"\u8bf7\u8f93\u5165\u65e5\u671f\u548c\u65f6\u95f4\u3002"));
        return;
    }
    if (name.empty()) {
        ShowMessage(Tr(L"Please enter a name.", L"\u8bf7\u8f93\u5165\u59d3\u540d\u3002"));
        return;
    }
    if (!ValidateInputField(dateTime, MAX_RECORD_FIELD_BYTES)
        || !ValidateInputField(name, MAX_RECORD_FIELD_BYTES)
        || !ValidateInputField(other, MAX_RECORD_FIELD_BYTES)) return;

    PushUndo();
    MarkDirty();
    Document().Records()[selected].dateTime = dateTime;
    Document().Records()[selected].name = name;
    Document().Records()[selected].other = Document().Records()[selected].status == L"Other" ? other : L"";
    RefreshList();
    int visibleIndex = RecordToVisibleIndex(selected);
    if (visibleIndex >= 0) {
        ListView_SetItemState(Ui().list, visibleIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

void MarkAllPresent(){
    if (Document().Records().empty()) {
        ShowMessage(Tr(L"There are no records to mark.", L"\u6ca1\u6709\u53ef\u6807\u8bb0\u7684\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstring allPresentMsg = Tr(L"Mark every record as Present?", L"\u5c06\u6240\u6709\u8bb0\u5f55\u6807\u8bb0\u4e3a\u51fa\u5e2d\uff1f");
    std::wstring allPresentTitle = Tr(L"All Present", L"\u5168\u5458\u51fa\u5e2d");
    if (ThemedMessageBox(Ui().hwnd, allPresentMsg, allPresentTitle, true) == IDYES) {
        PushUndo();
        MarkDirty();
        for (auto& record : Document().Records()) {
            record.status = L"Present";
            record.other.clear();
        }
        RefreshList();
    }
}

std::vector<int> SelectedRows(){
    std::vector<int> rows;
    int selected = -1;
    while ((selected = ListView_GetNextItem(Ui().list, selected, LVNI_SELECTED)) != -1) {
        int recordIndex = VisibleToRecordIndex(selected);
        if (recordIndex >= 0) rows.push_back(recordIndex);
    }
    return rows;
}

void EraseRows(std::vector<int> rows){
    auto& records = Document().Records();
    rows.erase(std::remove_if(rows.begin(), rows.end(), [&](int row) {
        return row < 0 || static_cast<size_t>(row) >= records.size();
    }), rows.end());
    std::sort(rows.begin(), rows.end());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    if (rows.empty()) return;
    PushUndo();
    MarkDirty();
    size_t next = 0, destination = 0;
    for (size_t source = 0; source < records.size(); ++source) {
        if (next < rows.size() && source == static_cast<size_t>(rows[next])) {
            ++next;
        } else {
            if (destination != source) records[destination] = std::move(records[source]);
            ++destination;
        }
    }
    records.resize(destination);
    ListView_SetItemState(Ui().list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    RefreshList();
}

void DeleteSelectedRecords(){
    auto rows = SelectedRows();
    if (rows.empty()) {
        ShowMessage(Tr(L"Please select one or more records to delete.", L"\u8bf7\u9009\u62e9\u8981\u5220\u9664\u7684\u4e00\u6761\u6216\u591a\u6761\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstringstream ss;
    ss << Tr(L"Delete", L"\u5220\u9664") << L" " << rows.size() << L" "
       << Tr(L"selected record(s)?", L"\u6761\u9009\u4e2d\u8bb0\u5f55\uff1f");
    std::wstring deleteTitle = Tr(L"Delete Selected", L"\u5220\u9664\u9009\u4e2d");
    if (ThemedMessageBox(Ui().hwnd, ss.str(), deleteTitle, true) == IDYES) {
        EraseRows(rows);
    }
}

void DeleteRecordsByStatus(const std::wstring& status){
    std::vector<int> rows;
    for (int i = 0; i < (int)Document().Records().size(); ++i) {
        if (Document().Records()[i].status == status) rows.push_back(i);
    }

    if (rows.empty()) {
        ShowMessage(Tr(L"No matching records found.", L"\u672a\u627e\u5230\u5339\u914d\u8bb0\u5f55\u3002"));
        return;
    }

    std::wstringstream ss;
    ss << Tr(L"Delete all", L"\u5220\u9664\u6240\u6709") << L" " << StatusDisplayText(status) << L" "
       << Tr(L"records?", L"\u8bb0\u5f55\uff1f") << L" (" << rows.size() << L")";
    std::wstring batchTitle = Tr(L"Batch Delete", L"\u6279\u91cf\u5220\u9664");
    if (ThemedMessageBox(Ui().hwnd, ss.str(), batchTitle, true) == IDYES) {
        EraseRows(rows);
    }
}

void ClearAllRecords(){
    if (Document().Records().empty()) {
        ShowMessage(Tr(L"There are no records to clear.", L"\u6ca1\u6709\u53ef\u6e05\u7a7a\u7684\u8bb0\u5f55\u3002"));
        return;
    }
    std::wstring clearMsg = Tr(L"Clear every attendance record in this sheet?", L"\u6e05\u7a7a\u5f53\u524d\u8868\u5185\u7684\u6240\u6709\u70b9\u540d\u8bb0\u5f55\uff1f");
    std::wstring clearTitle = Tr(L"Clear All", L"\u5168\u90e8\u6e05\u7a7a");
    if (ThemedMessageBox(Ui().hwnd, clearMsg, clearTitle, true) == IDYES) {
        PushUndo();
        MarkDirty();
        Document().Records().clear();
        RefreshList();
    }
}

void ShowDeleteMenu(HWND button){
    std::wstring selected = Tr(L"Delete selected records", L"\u5220\u9664\u9009\u4e2d\u8bb0\u5f55");
    std::wstring absent = Tr(L"Delete all Absent records", L"\u5220\u9664\u6240\u6709\u7f3a\u5e2d\u8bb0\u5f55");
    std::wstring late = Tr(L"Delete all Late records", L"\u5220\u9664\u6240\u6709\u8fdf\u5230\u8bb0\u5f55");
    std::wstring clear = Tr(L"Clear all records", L"\u6e05\u7a7a\u6240\u6709\u8bb0\u5f55");
    int command = ShowThemedPopupMenu(button, {
        {IDM_DELETE_SELECTED, selected, false},
        {IDM_DELETE_ABSENT, absent, false},
        {IDM_DELETE_LATE, late, false},
        {0, L"", true},
        {IDM_CLEAR_ALL, clear, false}
    });

    switch (command) {
    case IDM_DELETE_SELECTED: DeleteSelectedRecords(); break;
    case IDM_DELETE_ABSENT: DeleteRecordsByStatus(L"Absent"); break;
    case IDM_DELETE_LATE: DeleteRecordsByStatus(L"Late"); break;
    case IDM_CLEAR_ALL: ClearAllRecords(); break;
    }
}

} // namespace attendance
