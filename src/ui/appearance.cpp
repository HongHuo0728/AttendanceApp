#include "ui/redraw.h"
#include "version.h"
#include "ui/appearance.h"
#include "ui/common.h"
#include "ui/title_bar.h"
#include "localization/strings.h"
#include "ui/table.h"
#include "ui/theme.h"
#include "ui/layout.h"
#include "settings/state.h"
#include "settings/font_state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

void SetListColumnText(int index, const std::wstring& text);

void SetListColumnText(int index, const std::wstring& text){
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT;
    col.pszText = const_cast<wchar_t*>(text.c_str());
    ListView_SetColumn(Ui().list, index, &col);
}

void ApplyMainLanguage(){
    if (!Ui().hwnd) return;
    std::wstring mainTitle = ATTENDANCE_TITLE;
    SetWindowTextW(Ui().hwnd, mainTitle.c_str());
    SetText(GetDlgItem(Ui().hwnd, IDC_TITLE), Tr(L"Attendance Manager", L"\u70b9\u540d\u7ba1\u7406\u5668"));
    SetText(GetDlgItem(Ui().hwnd, IDC_SUBTITLE),
        Tr(L"Create, edit, export, save, import, and batch clean .attd roll calls.",
           L"\u521b\u5efa\u3001\u7f16\u8f91\u3001\u5bfc\u51fa\u3001\u4fdd\u5b58\u3001\u5bfc\u5165\u5e76\u6279\u91cf\u6574\u7406 .attd \u70b9\u540d\u8bb0\u5f55\u3002"));
    SetText(GetDlgItem(Ui().hwnd, IDC_HINT),
        Tr(L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.",
           L"\u63d0\u793a\uff1a\u53cc\u51fb\u8bb0\u5f55\u53ef\u7f16\u8f91\uff0cCtrl/Shift \u53ef\u591a\u9009\u3002"));
    SetText(GetDlgItem(Ui().hwnd, 2001), Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4"));
    SetText(GetDlgItem(Ui().hwnd, 2002), Tr(L"Name", L"\u59d3\u540d"));
    SetText(GetDlgItem(Ui().hwnd, 2003), Tr(L"Other", L"\u5176\u4ed6"));
    SetText(GetDlgItem(Ui().hwnd, IDC_FILTER_LABEL), Tr(L"Search", L"\u641c\u7d22"));
    SetText(GetDlgItem(Ui().hwnd, IDC_CLEAR_FILTER), Tr(L"Clear Filter", L"\u6e05\u9664\u7b5b\u9009"));
    SetText(GetDlgItem(Ui().hwnd, IDC_LEAVE), Tr(L"Leave",L"请假"));
    SetText(GetDlgItem(Ui().hwnd, IDC_PENDING), Tr(L"Pending",L"未点名"));
    SetText(GetDlgItem(Ui().hwnd, IDC_PRESENT), Tr(L"Present", L"\u51fa\u5e2d"));
    SetText(GetDlgItem(Ui().hwnd, IDC_ABSENT), Tr(L"Absent", L"\u7f3a\u5e2d"));
    SetText(GetDlgItem(Ui().hwnd, IDC_LATE), Tr(L"Late", L"\u8fdf\u5230"));
    SetText(GetDlgItem(Ui().hwnd, IDC_OTHER_STATUS), Tr(L"Other", L"\u5176\u4ed6"));
    SetText(GetDlgItem(Ui().hwnd, IDC_ADD_UPDATE), Tr(L"Update Selected", L"\u66f4\u65b0\u9009\u4e2d"));
    SetText(GetDlgItem(Ui().hwnd, IDC_EDIT_SELECTED), Tr(L"Edit Selected", L"\u7f16\u8f91\u9009\u4e2d"));
    SetText(GetDlgItem(Ui().hwnd, IDC_ALL_PRESENT), Tr(L"Scope: Present", L"当前范围出席"));
    SetText(GetDlgItem(Ui().hwnd, IDC_NEW), Tr(L"Create New", L"\u65b0\u5efa\u70b9\u540d"));
    SetText(GetDlgItem(Ui().hwnd, IDC_DELETE), Tr(L"Delete Options", L"\u5220\u9664\u9009\u9879"));
    SetText(GetDlgItem(Ui().hwnd, IDC_SAVE), Tr(L"Save .attd", L"\u4fdd\u5b58 .attd"));
    SetText(GetDlgItem(Ui().hwnd, IDC_IMPORT), Tr(L"Import .attd", L"\u5bfc\u5165 .attd"));
    SetText(GetDlgItem(Ui().hwnd, IDC_EXPORT_CSV), Tr(L"Export CSV", L"\u5bfc\u51fa CSV"));
    SetText(GetDlgItem(Ui().hwnd, IDC_SETTINGS), Tr(L"Settings", L"\u8bbe\u7f6e"));
    SetText(GetDlgItem(Ui().hwnd, IDC_COURSE_OPTIONS), Tr(L"Courses", L"\u8bfe\u7a0b"));
    SetText(GetDlgItem(Ui().hwnd, IDC_TOOLS), Tr(L"Tools", L"\u5de5\u5177"));

    SetListColumnText(0, Tr(L"Date / Time", L"\u65e5\u671f / \u65f6\u95f4"));
    SetListColumnText(1, Tr(L"Name", L"\u59d3\u540d"));
    SetListColumnText(2, Tr(L"Status", L"\u72b6\u6001"));
    SetListColumnText(3, Tr(L"Other", L"\u5176\u4ed6"));
    SetAccessibleName(Ui().dateEdit, Tr(L"Date/Time", L"\u65e5\u671f\u65f6\u95f4"));
    SetAccessibleName(Ui().nameEdit, Tr(L"Name", L"\u59d3\u540d"));
    SetAccessibleName(Ui().otherEdit, Tr(L"Other", L"\u5176\u4ed6"));
    SetAccessibleName(Ui().filterEdit, Tr(L"Search", L"\u641c\u7d22"));
    SetAccessibleName(Ui().list, Tr(L"Attendance Sheet", L"\u70b9\u540d\u8868"));
    SetAccessibleName(Ui().courseCombo, Tr(L"Courses", L"\u8bfe\u7a0b"));
    UpdateStats();
}

void ApplyVisualSettings(bool preserveThemeTransition){
    RedrawScope mainRedraw(Ui().hwnd);
    RedrawScope settingsRedraw(Ui().settingsWindow);
    RedrawScope chartRedraw(Ui().chartWindow);
    RedrawScope rollCallRedraw(Ui().quickRollCallWindow);
    if (!preserveThemeTransition) {
        Ui().themeTransition.active = false;
        ApplyThemePalette();
    }
    RecreateFonts();
    ApplyMainLanguage();
    if (!preserveThemeTransition && Ui().list) {
        ApplyDarkMode(Ui().hwnd);
    }
    if (!preserveThemeTransition) {
        BOOL dark = IsRenderedPaletteDark() ? TRUE : FALSE;
        if (Ui().hwnd) DwmSetWindowAttribute(Ui().hwnd, 20, &dark, sizeof(dark));
        if (Ui().settingsWindow) DwmSetWindowAttribute(Ui().settingsWindow, 20, &dark, sizeof(dark));
        if (Ui().chartWindow) DwmSetWindowAttribute(Ui().chartWindow, 20, &dark, sizeof(dark));
        ApplyGlassTitleBar(Ui().hwnd);
        ApplyGlassTitleBar(Ui().settingsWindow);
        ApplyGlassTitleBar(Ui().chartWindow);
        ApplyGlassTitleBar(Ui().quickRollCallWindow);
        ApplyThemedControls(Ui().hwnd);
        ApplyThemedControls(Ui().settingsWindow);
        ApplyThemedControls(Ui().chartWindow);
        ApplyThemedControls(Ui().quickRollCallWindow);
    }
    if (Ui().settingsWindow) RedrawWindow(Ui().settingsWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
    if (Ui().chartWindow) RedrawWindow(Ui().chartWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
    if (Ui().quickRollCallWindow) RedrawWindow(Ui().quickRollCallWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
    if (Ui().hwnd) {
        ShowWindow(GetDlgItem(Ui().hwnd, IDC_STAT_TOTAL), Settings().showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(Ui().hwnd, IDC_STAT_ATTENDANCE), Settings().showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(Ui().hwnd, IDC_STAT_ISSUES), Settings().showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(Ui().hwnd, IDC_STAT_VISIBLE), Settings().showStatsCards ? SW_SHOW : SW_HIDE);
        ShowWindow(GetDlgItem(Ui().hwnd, IDC_HINT), Settings().showFooterHint ? SW_SHOW : SW_HIDE);
        KillTimer(Ui().hwnd, 1);
        if (Settings().autosaveIntervalSeconds > 0) {
            SetTimer(Ui().hwnd, 1, (UINT)Settings().autosaveIntervalSeconds * 1000, nullptr);
        }
        InvalidateRect(Ui().hwnd, nullptr, FALSE);
        ResizeLayout(Ui().hwnd);
    }
}

} // namespace attendance
