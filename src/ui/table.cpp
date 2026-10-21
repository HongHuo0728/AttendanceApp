#include "ui/redraw.h"
#include "core/status.h"
#include "ui/table.h"
#include "ui/common.h"
#include "animation/scheduler.h"
#include "core/text.h"
#include "model/query.h"
#include "localization/strings.h"
#include "model/workbook.h"
#include "model/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

LRESULT HandleListCustomDraw(LPARAM lParam){
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
    if (!draw) return CDRF_DODEFAULT;

    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        return CDRF_NOTIFYITEMDRAW;
    }
    if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        int item = (int)draw->nmcd.dwItemSpec;
        bool selected = (ListView_GetItemState(Ui().list, item, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        COLORREF rowBase = selected
            ? Ui().COLOR_SELECTED
            : (item % 2 == 0 ? Ui().COLOR_PANEL : Ui().COLOR_ROW_ALT);
        draw->clrTextBk = rowBase;
        draw->clrText = Ui().COLOR_TEXT;
        return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}

void SetStaticTextClean(HWND hwnd, const std::wstring& text){
    SetText(hwnd, text);
}

void UpdateStats(){
    if(!Ui().hwnd)return;
    auto stats=CountRecords(Document().Records());
    std::wostringstream details;details<<StatsRangeName(Document().statsRange)<<L" | ";
    const int counts[]={stats.present,stats.absent,stats.late,stats.other,stats.leave,stats.pending};
    for(size_t i=0;i<StatusKeys.size();++i)details<<StatusDisplayText(StatusKeys[i])<<L" "<<counts[i]<<L"  ";
    SetStaticTextClean(GetDlgItem(Ui().hwnd,IDC_STATS),details.str());
    SetStaticTextClean(GetDlgItem(Ui().hwnd,IDC_STAT_TOTAL),StatsRangeName(Document().statsRange)+L"\n"+std::to_wstring(stats.total));
    SetStaticTextClean(GetDlgItem(Ui().hwnd,IDC_STAT_ATTENDANCE),Tr(L"Attendance",L"出勤率")+L"\n"+AttendanceRateText(stats));
    std::wostringstream issues;issues<<Tr(L"Absent/Late",L"缺席/迟到")<<L"\n";
    if(stats.Eligible()>0)issues<<std::fixed<<std::setprecision(1)<<(stats.absent+stats.late)*100.0/stats.Eligible()<<L"%";else issues<<Tr(L"No data",L"暂无数据");
    SetStaticTextClean(GetDlgItem(Ui().hwnd,IDC_STAT_ISSUES),issues.str());
    SetStaticTextClean(GetDlgItem(Ui().hwnd,IDC_STAT_VISIBLE),Tr(L"Showing",L"显示")+L"\n"+std::to_wstring(Document().visibleRows.size())+L" / "+std::to_wstring(Document().Records().size()));
}

void RefreshList(){
    SyncActiveSheet();
    if (!Ui().list) return;
    std::unordered_set<int> selectedRecords;
    int focusedRecord = -1;
    int selectedVisible = -1;
    while ((selectedVisible = ListView_GetNextItem(Ui().list, selectedVisible, LVNI_SELECTED)) != -1) {
        int recordIndex = VisibleToRecordIndex(selectedVisible);
        if (recordIndex >= 0) selectedRecords.insert(recordIndex);
    }
    int focusedVisible = ListView_GetNextItem(Ui().list, -1, LVNI_FOCUSED);
    if (focusedVisible >= 0) focusedRecord = VisibleToRecordIndex(focusedVisible);
    int topRecord = VisibleToRecordIndex(ListView_GetTopIndex(Ui().list));
    RedrawScope redraw(Ui().list);
    Document().visibleRows.clear();
    for (int i = 0; i < (int)Document().Records().size(); ++i) {
        if (!RecordMatchesFilter(Document().Records()[i])) continue;
        Document().visibleRows.push_back(i);
    }
    ListView_SetItemCountEx(Ui().list, static_cast<int>(Document().visibleRows.size()),
        LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
    ListView_SetItemState(Ui().list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    for (int visibleIndex = 0; visibleIndex < (int)Document().visibleRows.size(); ++visibleIndex) {
        if (selectedRecords.find(Document().visibleRows[visibleIndex]) != selectedRecords.end()) {
            ListView_SetItemState(Ui().list, visibleIndex, LVIS_SELECTED, LVIS_SELECTED);
        }
        if (Document().visibleRows[visibleIndex] == focusedRecord) {
            ListView_SetItemState(Ui().list, visibleIndex, LVIS_FOCUSED, LVIS_FOCUSED);
        }
    }
    if (topRecord >= 0) {
        int newTop = RecordToVisibleIndex(topRecord);
        if (newTop >= 0) ListView_EnsureVisible(Ui().list, newTop, FALSE);
    }
    UpdateStats();
    InvalidateRect(Ui().list, nullptr, FALSE);
}

void RefreshCourseCombo(){
    if (!Ui().courseCombo) return;
    EnsureSheets();
    RedrawScope redraw(Ui().courseCombo);
    SendMessageW(Ui().courseCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& sheet : Document().sheets) {
        const std::wstring displayName = sheet.name == L"Default Course"
            ? Tr(L"Default Course", L"默认课程")
            : sheet.name;
        SendMessageW(Ui().courseCombo, CB_ADDSTRING, 0, (LPARAM)displayName.c_str());
    }
    SendMessageW(Ui().courseCombo, CB_SETCURSEL, Document().activeSheet, 0);
    if (!Ui().languageUpdateActive) {
        RedrawWindow(Ui().courseCombo, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
    }
}

void ApplyFilterText(const std::wstring& filter){
    if (Ui().filterEdit) SetText(Ui().filterEdit, filter);
    Document().filterText = LowerText(filter);
    RefreshList();
}

void InitListColumns(){
    const wchar_t* titles[] = {L"Date / Time", L"Name", L"Status", L"Other"};
    for (int i = 0; i < 4; ++i) {
        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.pszText = const_cast<wchar_t*>(titles[i]);
        col.cx = 180;
        col.iSubItem = i;
        ListView_InsertColumn(Ui().list, i, &col);
    }
}

} // namespace attendance
