#include "app/main_window.h"
#include "model/workbook.h"
#include "ui/common.h"
#include "animation/scheduler.h"
#include "ui/title_bar.h"
#include "ui/table.h"
#include "core/text.h"
#include "settings/shortcuts.h"
#include "model/query.h"
#include "ui/controls.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "settings/fonts.h"
#include "ui/combobox.h"
#include "app/validation.h"
#include "app/courses.h"
#include "ui/theme.h"
#include "ui/appearance.h"
#include "app/records.h"
#include "reports/csv.h"
#include "reports/jobs.h"
#include "model/history.h"
#include "app/commands.h"
#include "app/recovery.h"
#include "app/documents.h"
#include "ui/settings_window.h"
#include "ui/layout.h"
#include "ui/drawing.h"
#include "model/state.h"
#include "settings/state.h"
#include "settings/font_state.h"
#include "animation/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {



LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch (msg) {
    case WM_APP_REPORT_READY:
        DispatchReportCompletions();
        return 0;
    case WM_CREATE: {
        Ui().hwnd = hwnd;
        ApplyThemePalette();
        ApplyGlassTitleBar(hwnd);
        RecreateFonts();

        HWND title = MakeControl(L"STATIC", L"Attendance Manager", 0, IDC_TITLE);
        HWND subtitle = MakeControl(L"STATIC", L"Create, edit, export, save, import, and batch clean .attd roll calls.", 0, IDC_SUBTITLE);
        HWND stats = MakeControl(L"STATIC", L"Total 0    Present 0    Absent 0    Late 0    Other 0", SS_LEFTNOWORDWRAP | SS_ENDELLIPSIS, IDC_STATS);
        HWND hint = MakeControl(L"STATIC", L"Tip: double-click a row to edit. Ctrl/Shift supports multi-select.", 0, IDC_HINT);
        HWND statTotal = MakeControl(L"STATIC", L"Total\n0", SS_LEFT | SS_NOPREFIX, IDC_STAT_TOTAL);
        HWND statAttendance = MakeControl(L"STATIC", L"Attendance\n0%", SS_LEFT | SS_NOPREFIX, IDC_STAT_ATTENDANCE);
        HWND statIssues = MakeControl(L"STATIC", L"Absent/Late\n0%", SS_LEFT | SS_NOPREFIX, IDC_STAT_ISSUES);
        HWND statVisible = MakeControl(L"STATIC", L"Showing\n0 / 0", SS_LEFT | SS_NOPREFIX, IDC_STAT_VISIBLE);
        SendMessageW(title, WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        SendMessageW(subtitle, WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(stats, WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(hint, WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(statTotal, WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(statAttendance, WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(statIssues, WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(statVisible, WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        EnableStatsPaint(stats);
        EnableStatsPaint(statTotal);
        EnableStatsPaint(statAttendance);
        EnableStatsPaint(statIssues);
        EnableStatsPaint(statVisible);
        Ui().courseCombo = MakeControl(L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_COURSE_COMBO);
        MakeButton(L"Courses", IDC_COURSE_OPTIONS);
        MakeControl(L"STATIC", L"Date/Time", 0, 2001);
        Ui().dateEdit = MakeControl(L"EDIT", CurrentDateTimeText().c_str(), WS_TABSTOP | ES_AUTOHSCROLL, IDC_DATE);
        MakeControl(L"STATIC", L"Name", 0, 2002);
        Ui().nameEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_NAME);
        MakeControl(L"STATIC", L"Other", 0, 2003);
        Ui().otherEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_OTHER);
        MakeControl(L"STATIC", L"Search", 0, IDC_FILTER_LABEL);
        Ui().filterEdit = MakeControl(L"EDIT", L"", WS_TABSTOP | ES_AUTOHSCROLL, IDC_FILTER);

        MakeButton(L"Leave", IDC_LEAVE);
        MakeButton(L"Pending", IDC_PENDING);
        MakeButton(L"Present", IDC_PRESENT);
        MakeButton(L"Absent", IDC_ABSENT);
        MakeButton(L"Late", IDC_LATE);
        MakeButton(L"Other", IDC_OTHER_STATUS);
        MakeButton(L"Update Selected", IDC_ADD_UPDATE);
        MakeButton(L"Edit Selected", IDC_EDIT_SELECTED);
        MakeButton(L"Mark All Present", IDC_ALL_PRESENT);
        MakeButton(L"Create New Attendance", IDC_NEW);
        MakeButton(L"Delete Options", IDC_DELETE);
        MakeButton(L"Save .attd", IDC_SAVE);
        MakeButton(L"Import .attd", IDC_IMPORT);
        MakeButton(L"Export CSV", IDC_EXPORT_CSV);
        MakeButton(L"Tools", IDC_TOOLS);
        MakeButton(L"Settings", IDC_SETTINGS);
        MakeButton(L"Clear Filter", IDC_CLEAR_FILTER);

        Ui().list = MakeControl(WC_LISTVIEWW, L"",
            LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | WS_TABSTOP | WS_BORDER, IDC_LIST);
        EnableMouseWheelForward(Ui().list);
        InitListColumns();
        EnableHeaderPaint(ListView_GetHeader(Ui().list));
        EnsureSheets();
        RefreshCourseCombo();
        ApplyMainLanguage();
        ResizeLayout(hwnd);
        ApplyDarkMode(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        if (Settings().autosaveIntervalSeconds > 0) {
            SetTimer(hwnd, 1, (UINT)Settings().autosaveIntervalSeconds * 1000, nullptr);
        }
        return 0;
    }
    case WM_SETTINGCHANGE:
    case WM_THEMECHANGED:
        RefreshOpenAppTitleBars(true);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    case WM_SIZE:
        ResizeLayout(hwnd);
        return 0;
    case WM_ENTERSIZEMOVE:
        Ui().liveResizing = true;
        return 0;
    case WM_EXITSIZEMOVE:
        Ui().liveResizing = false;
        ResizeLayout(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        return 0;
    case WM_VSCROLL:
        ScrollMainWindow(hwnd, SB_VERT, LOWORD(wParam), 0);
        return 0;
    case WM_HSCROLL:
        ScrollMainWindow(hwnd, SB_HORZ, LOWORD(wParam), 0);
        return 0;
    case WM_MOUSEWHEEL:
        if (Ui().openThemedPopupMenus > 0) return 0;
        ScrollMainWindow(hwnd, SB_VERT, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_MOUSEHWHEEL:
        if (Ui().openThemedPopupMenus > 0) return 0;
        ScrollMainWindow(hwnd, SB_HORZ, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    case WM_COMMAND: {
        if (HIWORD(wParam) == CBN_DROPDOWN) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 1.0, 340);
            ApplyComboDropDownTheme((HWND)lParam);
            return 0;
        }
        if (HIWORD(wParam) == CBN_CLOSEUP) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 0.0, 220);
            return 0;
        }
        if (LOWORD(wParam) == IDC_COURSE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            int index = (int)SendMessageW(Ui().courseCombo, CB_GETCURSEL, 0, 0);
            SwitchCourse(index);
            return 0;
        }
        if (LOWORD(wParam) == IDC_FILTER && HIWORD(wParam) == EN_CHANGE) {
            Document().filterText = LowerText(GetText(Ui().filterEdit));
            SetTimer(hwnd, 2, 120, nullptr);
            return 0;
        }
        switch (LOWORD(wParam)) {
        case IDC_LEAVE: AddOrUpdateRecord(L"Leave"); return 0;
        case IDC_PENDING: AddOrUpdateRecord(L"Pending"); return 0;
        case IDC_PRESENT: AddOrUpdateRecord(L"Present"); return 0;
        case IDC_ABSENT: AddOrUpdateRecord(L"Absent"); return 0;
        case IDC_LATE: AddOrUpdateRecord(L"Late"); return 0;
        case IDC_OTHER_STATUS: AddOrUpdateRecord(L"Other"); return 0;
        case IDC_ADD_UPDATE: UpdateSelectedRecord(); return 0;
        case IDC_EDIT_SELECTED: LoadRecordIntoEditor(ListView_GetNextItem(Ui().list, -1, LVNI_SELECTED)); return 0;
        case IDC_ALL_PRESENT: MarkAllPresent(); return 0;
        case IDC_EXPORT_CSV: ExportCsv(); return 0;
        case IDC_SETTINGS: ShowSettingsWindow(); return 0;
        case IDC_TOOLS: ShowToolsMenu(GetDlgItem(hwnd, IDC_TOOLS)); return 0;
        case IDC_COURSE_OPTIONS: ShowCourseMenu(GetDlgItem(hwnd, IDC_COURSE_OPTIONS)); return 0;
        case IDC_SAVE: SaveAttendance(); return 0;
        case IDC_IMPORT: ImportAttendance(); return 0;
        case IDC_CLEAR_FILTER:
            KillTimer(hwnd, 2);
            SetText(Ui().filterEdit, L"");
            KillTimer(hwnd, 2);
            Document().filterText.clear();
            Document().lessonFilter.clear();
            RefreshList();
            return 0;
        case IDC_NEW:
            {
            SyncActiveSheet();
            std::wstring sheetName = Tr(L"Attendance Sheet", L"\u70b9\u540d\u8868") + L" " + std::to_wstring(Document().sheets.size() + 1);
            std::wstring newTitle = Tr(L"New Attendance", L"\u65b0\u5efa\u70b9\u540d");
            if (!PromptText(newTitle, Tr(L"New attendance sheet name:", L"\u65b0\u70b9\u540d\u8868\u540d\u79f0\uff1a"), sheetName)) return 0;
            if (sheetName.empty()) return 0;
            if (!ValidateInputField(sheetName, MAX_METADATA_FIELD_BYTES) || !CanAppendSheets(1)) return 0;
            PushUndo();
            MarkDirty();
            Document().sheets.push_back(MakeSheet(sheetName));
            Document().activeSheet = (int)Document().sheets.size() - 1;
            Document().Records().clear();
            Document().visibleRows.clear();
            Document().lessonFilter.clear();
            RefreshCourseCombo();
            RefreshList();
            SetText(Ui().dateEdit, CurrentDateTimeText());
            SetText(Ui().nameEdit, L"");
            SetText(Ui().otherEdit, L"");
            }
            return 0;
        case IDC_DELETE: {
            ShowDeleteMenu(GetDlgItem(hwnd, IDC_DELETE));
            return 0;
        }
        }
        break;
    }
    case WM_KEYDOWN:
        if (ExecuteMainShortcut(wParam)) return 0;
        if (ShortcutMatches(Settings().shortcutCommand, wParam)) {
            RunCommandPalette();
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == 1) AutoSaveNow();
        else if (wParam == 2) {
            KillTimer(hwnd, 2);
            RefreshList();
        }
        return 0;
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measure->CtlType == ODT_COMBOBOX) {
            measure->itemHeight = ComboItemHeight();
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (DrawComboItem(draw)) return TRUE;
        if (DrawButtonItem(draw)) return TRUE;
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintAppBackground(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_NOTIFY: {
        auto* hdr = reinterpret_cast<NMHDR*>(lParam);
        if (hdr->idFrom == IDC_LIST && hdr->code == LVN_GETDISPINFOW) {
            auto* info = reinterpret_cast<NMLVDISPINFOW*>(lParam);
            int recordIndex = VisibleToRecordIndex(info->item.iItem);
            if (recordIndex >= 0 && (info->item.mask & LVIF_TEXT)
                && info->item.pszText && info->item.cchTextMax > 0) {
                const auto& record = Document().Records()[recordIndex];
                std::wstring value;
                if (info->item.iSubItem == 0) value = record.dateTime;
                else if (info->item.iSubItem == 1) value = record.name;
                else if (info->item.iSubItem == 2) value = StatusDisplayText(record.status);
                else if (info->item.iSubItem == 3) value = record.other;
                wcsncpy_s(info->item.pszText, static_cast<size_t>(info->item.cchTextMax),
                    value.c_str(), _TRUNCATE);
            }
            return 0;
        }
        if (hdr->idFrom == IDC_LIST && hdr->code == NM_CUSTOMDRAW) {
            return HandleListCustomDraw(lParam);
        }
        if (hdr->idFrom == IDC_LIST && hdr->code == LVN_ITEMCHANGED) {
            auto* item = reinterpret_cast<NMLISTVIEW*>(lParam);
            int recordIndex = VisibleToRecordIndex(item->iItem);
            if ((item->uNewState & LVIS_SELECTED) && recordIndex >= 0) {
                const auto& record = Document().Records()[recordIndex];
                SetText(Ui().dateEdit, record.dateTime);
                SetText(Ui().nameEdit, record.name);
                SetText(Ui().otherEdit, record.other);
            }
        } else if (hdr->idFrom == IDC_LIST && hdr->code == NM_DBLCLK) {
            auto* item = reinterpret_cast<NMITEMACTIVATE*>(lParam);
            if (item->iItem >= 0) LoadRecordIntoEditor(item->iItem);
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND control = (HWND)lParam;
        if (GetDlgCtrlID(control) == IDC_TITLE) SetTextColor(hdc, Ui().COLOR_TEXT);
        else if (GetDlgCtrlID(control) == IDC_STATS) {
            SetTextColor(hdc, Ui().COLOR_ACCENT);
        }
        else SetTextColor(hdc, Ui().COLOR_MUTED);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, Ui().COLOR_BG);
        return (LRESULT)Ui().bgBrush;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, Ui().COLOR_TEXT);
        SetBkColor(hdc, Ui().COLOR_INPUT);
        return (LRESULT)Ui().inputBrush;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_APP_ANIMATION_TICK:
        AcknowledgeAnimationTick(hwnd);
        UpdateThemeTransitionFrame();
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_APP_PREWARM_SETTINGS:
        RequestFontEnumeration();
        PrewarmSettingsWindow();
        return 0;
    case WM_APP_FONT_CACHE_READY:
        if (Ui().settingsWindow && IsWindow(Ui().settingsWindow)) {
            ApplyFontListToSettings(Ui().settingsWindow);
        }
        return 0;
    case WM_FONTCHANGE:
        RequestFontEnumeration();
        return 0;
    case WM_APP_TEST_PARTICLE_METRICS: {
        wchar_t testMode[8]{};
        if (GetEnvironmentVariableW(L"ATTENDANCEAPP_TEST_MODE", testMode, 8) == 0) return 0;
        if (wParam == 0) {
            Animations().lastParticleSetupUs.store(0, std::memory_order_relaxed);
            Animations().maxParticleFrameUs.store(0, std::memory_order_relaxed);
            return 1;
        }
        if (wParam == 1) return static_cast<LRESULT>(Animations().lastParticleSetupUs.load(std::memory_order_relaxed));
        if (wParam == 2) return static_cast<LRESULT>(Animations().maxParticleFrameUs.load(std::memory_order_relaxed));
        return 0;
    }
    case WM_APP_TEST_THEME_TRANSITION: {
        wchar_t testMode[8]{};
        if (GetEnvironmentVariableW(L"ATTENDANCEAPP_TEST_MODE", testMode, 8) > 0) {
            if (wParam == 3) {
                ShowSettingsWindow();
                return reinterpret_cast<LRESULT>(Ui().settingsWindow);
            } else if (wParam == 2) {
                ResetAppearanceValues();
                BeginThemeTransition(UiTheme::Dark);
                ApplyVisualSettings(true);
            } else {
                BeginThemeTransition(wParam != 0 ? UiTheme::Light : UiTheme::Dark);
            }
        }
        return 0;
    }
    case WM_APP_TEST_LANGUAGE_SWITCH: {
        wchar_t testMode[8]{};
        if (GetEnvironmentVariableW(L"ATTENDANCEAPP_TEST_MODE", testMode, 8) > 0
            && wParam <= static_cast<WPARAM>(UiLanguage::ChineseTraditionalHongKong)) {
            Settings().language = static_cast<UiLanguage>(wParam);
            ApplyLanguageAtomically(Ui().settingsWindow);
            return 1;
        }
        return 0;
    }
    case WM_APP_ANIMATION_DONE:
        return 0;
    case WM_CLOSE:
        if (Document().dirty && AutoSaveNow() == AutosaveResult::Failed) {
            std::wstring warning = Tr(
                L"Autosave failed. Closing now may lose unsaved attendance data. Close anyway?",
                L"\u81ea\u52a8\u4fdd\u5b58\u5931\u8d25\u3002\u73b0\u5728\u5173\u95ed\u53ef\u80fd\u4f1a\u4e22\u5931\u672a\u4fdd\u5b58\u7684\u8003\u52e4\u6570\u636e\u3002\u4ecd\u8981\u5173\u95ed\u5417\uff1f");
            if (ThemedMessageBox(hwnd, warning,
                    Tr(L"Autosave failed", L"\u81ea\u52a8\u4fdd\u5b58\u5931\u8d25"), true) != IDYES) {
                return 0;
            }
        }
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        StopReportJobs();
        KillTimer(hwnd, 1);
        KillTimer(hwnd, 2);
        RemoveAnimationState(hwnd);
        StopFontEnumerationThread();
        StopAnimationThread();
        if (Ui().font) DeleteObject(Ui().font);
        if (Ui().titleFont) DeleteObject(Ui().titleFont);
        if (Ui().smallFont) DeleteObject(Ui().smallFont);
        if (Ui().bgBrush) DeleteObject(Ui().bgBrush);
        if (Ui().inputBrush) DeleteObject(Ui().inputBrush);
        if (Ui().panelBrush) DeleteObject(Ui().panelBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace attendance
