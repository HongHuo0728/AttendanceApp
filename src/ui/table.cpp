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

void StartListTransition();
void SetStaticTextClean(HWND hwnd, const std::wstring& text);

void StartListTransition(){
    if (!Ui().list) return;
    RestartAnimation(Ui().list, AnimChannel::ListReveal, 1.0, 260);
    RedrawWindow(Ui().list, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
}

LRESULT HandleListCustomDraw(LPARAM lParam){
    auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam);
    if (!draw) return CDRF_DODEFAULT;

    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
        return CDRF_NOTIFYITEMDRAW;
    }
    if (draw->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
        int item = (int)draw->nmcd.dwItemSpec;
        bool selected = (ListView_GetItemState(Ui().list, item, LVIS_SELECTED) & LVIS_SELECTED) != 0;
        double reveal = GetAnimationValue(Ui().list, AnimChannel::ListReveal, 1.0);
        int visibleOffset = std::max(0, item - ListView_GetTopIndex(Ui().list));
        double rowProgress = reveal >= 1.0 ? 1.0
            : std::clamp(reveal * 1.25 - visibleOffset * 0.035, 0.0, 1.0);

        COLORREF rowBase = selected
            ? Ui().COLOR_SELECTED
            : (item % 2 == 0 ? Ui().COLOR_PANEL : Ui().COLOR_ROW_ALT);
        draw->clrTextBk = BlendColor(Ui().COLOR_BG, rowBase, rowProgress);
        draw->clrText = BlendColor(Ui().COLOR_MUTED, Ui().COLOR_TEXT, rowProgress);
        return CDRF_NEWFONT;
    }
    return CDRF_DODEFAULT;
}

void SetStaticTextClean(HWND hwnd, const std::wstring& text){
    if (!hwnd) return;
    if (!Ui().languageUpdateActive) {
        HWND parent = GetParent(hwnd);
        RECT rc{};
        if (parent && GetWindowRect(hwnd, &rc)) {
            MapWindowPoints(nullptr, parent, reinterpret_cast<POINT*>(&rc), 2);
            RedrawWindow(parent, &rc, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
        }
    }
    SetWindowTextW(hwnd, text.c_str());
    InvalidateRect(hwnd, nullptr, FALSE);
    if (!Ui().languageUpdateActive) UpdateWindow(hwnd);
}

void UpdateStats(){
    int present = 0;
    int absent = 0;
    int late = 0;
    int other = 0;
    int totalInRange = 0;
    for (const auto& record : Document().Records()) {
        if (!RecordMatchesStatsRange(record)) continue;
        ++totalInRange;
        if (record.status == L"Present") ++present;
        else if (record.status == L"Absent") ++absent;
        else if (record.status == L"Late") ++late;
        else ++other;
    }

    double attendanceRate = totalInRange == 0 ? 0.0 : (present * 100.0 / totalInRange);
    double issueRate = totalInRange == 0 ? 0.0 : ((absent + late) * 100.0 / totalInRange);
    std::wstringstream ss;
    ss << StatsRangeName(Document().statsRange) << L"  |  "
       << Tr(L"Total", L"\u603b\u6570") << L" " << totalInRange
       << L"  |  " << Tr(L"Attendance", L"\u51fa\u52e4\u7387") << L" " << std::fixed << std::setprecision(1) << attendanceRate << L"%"
       << L"  |  " << Tr(L"Absent/Late", L"\u7f3a\u5e2d/\u8fdf\u5230") << L" " << std::fixed << std::setprecision(1) << issueRate << L"%"
       << L"  |  " << Tr(L"Present", L"\u51fa\u5e2d") << L" " << present
       << L"  " << Tr(L"Absent", L"\u7f3a\u5e2d") << L" " << absent
       << L"  " << Tr(L"Late", L"\u8fdf\u5230") << L" " << late
       << L"  " << Tr(L"Other", L"\u5176\u4ed6") << L" " << other;
    if (!Document().filterText.empty()) {
        ss << L"  |  " << Tr(L"Showing", L"\u663e\u793a") << L" " << Document().visibleRows.size() << L"/" << Document().Records().size();
    }
    SetStaticTextClean(GetDlgItem(Ui().hwnd, IDC_STATS), ss.str());

    std::wstringstream totalCard;
    totalCard << StatsRangeName(Document().statsRange) << L"\n" << totalInRange;
    SetStaticTextClean(GetDlgItem(Ui().hwnd, IDC_STAT_TOTAL), totalCard.str());

    std::wstringstream attendanceCard;
    attendanceCard << Tr(L"Attendance", L"\u51fa\u52e4\u7387") << L"\n" << std::fixed << std::setprecision(1) << attendanceRate << L"%";
    SetStaticTextClean(GetDlgItem(Ui().hwnd, IDC_STAT_ATTENDANCE), attendanceCard.str());

    std::wstringstream issuesCard;
    issuesCard << Tr(L"Absent/Late", L"\u7f3a\u5e2d/\u8fdf\u5230") << L"\n" << std::fixed << std::setprecision(1) << issueRate << L"%";
    SetStaticTextClean(GetDlgItem(Ui().hwnd, IDC_STAT_ISSUES), issuesCard.str());

    std::wstringstream visibleCard;
    visibleCard << Tr(L"Showing", L"\u663e\u793a") << L"\n" << Document().visibleRows.size() << L" / " << Document().Records().size();
    SetStaticTextClean(GetDlgItem(Ui().hwnd, IDC_STAT_VISIBLE), visibleCard.str());
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
    StartListTransition();
}

void RefreshCourseCombo(){
    if (!Ui().courseCombo) return;
    EnsureSheets();
    SendMessageW(Ui().courseCombo, CB_RESETCONTENT, 0, 0);
    for (const auto& sheet : Document().sheets) {
        const std::wstring displayName = sheet.name == L"Default Course"
            ? Tr(L"Default Course", L"默认课程")
            : sheet.name;
        SendMessageW(Ui().courseCombo, CB_ADDSTRING, 0, (LPARAM)displayName.c_str());
    }
    SendMessageW(Ui().courseCombo, CB_SETCURSEL, Document().activeSheet, 0);
    if (!Ui().languageUpdateActive) {
        RedrawWindow(Ui().courseCombo, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
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
