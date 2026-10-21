#include "ui/redraw.h"
#include "ui/layout.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {



void ToggleFullscreen(HWND hwnd){
    Ui().fullscreen = !Ui().fullscreen;
    if (Ui().fullscreen) {
        Ui().previousStyle = GetWindowLongW(hwnd, GWL_STYLE);
        GetWindowPlacement(hwnd, &Ui().previousPlacement);

        MONITORINFO mi{sizeof(mi)};
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLongW(hwnd, GWL_STYLE, Ui().previousStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hwnd, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    } else {
        SetWindowLongW(hwnd, GWL_STYLE, Ui().previousStyle);
        SetWindowPlacement(hwnd, &Ui().previousPlacement);
        SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

void ResizeLayout(HWND hwnd){
    static bool inLayout = false;
    if (inLayout) return;
    inLayout = true;

    RECT rc{};
    GetClientRect(hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w <= 0 || h <= 0) {
        inLayout = false;
        return;
    }

    RedrawScope redraw(hwnd);

    int layoutW = std::max(w, MIN_LAYOUT_W);
    int layoutH = std::max(h, MIN_LAYOUT_H);
    Ui().contentW = layoutW;
    Ui().contentH = layoutH;

    Ui().scrollX = std::clamp(Ui().scrollX, 0, std::max(0, Ui().contentW - w));
    Ui().scrollY = std::clamp(Ui().scrollY, 0, std::max(0, Ui().contentH - h));

    const bool compact = Settings().layoutDensity == LayoutDensity::Compact;
    const bool spacious = Settings().layoutDensity == LayoutDensity::Spacious;
    int pad = compact ? 20 : spacious ? 36 : 28;
    auto move = [&](int id, int x, int y, int width, int height) {
        HWND child = GetDlgItem(hwnd, id);
        if (child) MoveWindow(child, x - Ui().scrollX, y - Ui().scrollY, width, height, FALSE);
    };
    auto moveHwnd = [&](HWND child, int x, int y, int width, int height) {
        if (child) MoveWindow(child, x - Ui().scrollX, y - Ui().scrollY, width, height, FALSE);
    };

    int titleY = compact ? 14 : spacious ? 28 : 20;
    move(IDC_TITLE, pad, titleY, 430, 38);
    move(IDC_SUBTITLE, pad, titleY + 42, std::min(720, layoutW - pad * 2), 26);

    int courseW = 300;
    int courseBtnW = 124;
    int courseX = std::max(pad + 520, layoutW - pad - courseW - courseBtnW - 12);
    moveHwnd(Ui().courseCombo, courseX, titleY + 6, courseW, 430);
    move(IDC_COURSE_OPTIONS, courseX + courseW + 12, titleY + 6, courseBtnW, 38);

    int statsY = compact ? 90 : spacious ? 120 : 104;
    int statsGap = compact ? 12 : spacious ? 22 : 16;
    int statsH = Settings().showStatsCards ? (compact ? 78 : spacious ? 108 : 92) : 0;
    int statW = std::max(180, (layoutW - pad * 2 - statsGap * 3) / 4);
    int statX = pad;
    move(IDC_STAT_TOTAL, statX, statsY, statW, statsH);
    move(IDC_STAT_ATTENDANCE, statX + (statW + statsGap), statsY, statW, statsH);
    move(IDC_STAT_ISSUES, statX + (statW + statsGap) * 2, statsY, statW, statsH);
    move(IDC_STAT_VISIBLE, statX + (statW + statsGap) * 3, statsY, statW, statsH);

    int sectionGap = compact ? 12 : spacious ? 24 : 18;
    int filterY = Settings().showStatsCards ? statsY + statsH + sectionGap : statsY;
    int listX = pad;
    int listW = std::max(720, layoutW - pad * 2);
    int clearW = 136;
    int filterLabelW = 74;
    int filterW = std::min(460, std::max(240, listW / 3));
    int fieldH = compact ? 32 : spacious ? 44 : 36;
    int fieldLabelOffset = std::max(4, (fieldH - 24) / 2);
    move(IDC_FILTER_LABEL, listX, filterY + fieldLabelOffset, filterLabelW, 24);
    moveHwnd(Ui().filterEdit, listX + filterLabelW, filterY, filterW, fieldH);
    move(IDC_CLEAR_FILTER, listX + filterLabelW + filterW + 12, filterY, clearW, fieldH);

    int dockPrimaryH = Settings().layoutDensity == LayoutDensity::Compact ? 50
        : Settings().layoutDensity == LayoutDensity::Spacious ? 64 : 56;
    int drawerH = Settings().layoutDensity == LayoutDensity::Compact ? 36
        : Settings().layoutDensity == LayoutDensity::Spacious ? 44 : 40;
    int bottomPad = compact ? 22 : spacious ? 36 : 28;
    int dockGap = compact ? 10 : spacious ? 22 : 16;
    int composerGap = compact ? 54 : spacious ? 82 : 66;
    int listGap = compact ? 42 : spacious ? 64 : 52;
    int listBottomGap = compact ? 14 : spacious ? 28 : 20;
    int drawerY = std::max(filterY + 430, layoutH - drawerH - bottomPad);
    int dockY = drawerY - dockPrimaryH - dockGap;
    int composerY = dockY - composerGap;
    int listY = filterY + listGap;
    int listH = std::max(260, composerY - listY - listBottomGap);
    moveHwnd(Ui().list, listX, listY, listW, listH);

    int composerW = std::min(1060, layoutW - pad * 2);
    int composerX = pad + (layoutW - pad * 2 - composerW) / 2;
    int labelY = composerY - (compact ? 16 : spacious ? 22 : 18);
    int dateW = std::max(210, composerW / 4);
    int nameW = std::max(240, composerW / 4);
    int otherW = composerW - dateW - nameW - 32;
    move(2001, composerX, labelY, dateW, 18);
    int editorH = compact ? 34 : spacious ? 46 : 38;
    moveHwnd(Ui().dateEdit, composerX, composerY, dateW, editorH);
    move(2002, composerX + dateW + 16, labelY, nameW, 18);
    moveHwnd(Ui().nameEdit, composerX + dateW + 16, composerY, nameW, editorH);
    move(2003, composerX + dateW + nameW + 32, labelY, otherW, 18);
    moveHwnd(Ui().otherEdit, composerX + dateW + nameW + 32, composerY, otherW, editorH);

    int primaryGap = compact ? 8 : spacious ? 18 : 12;
    int primaryW = std::clamp((layoutW - pad * 2 - primaryGap * 7) / 8, 100, 148);
    int primaryTotalW = primaryW * 8 + primaryGap * 7;
    int primaryX = pad + (layoutW - pad * 2 - primaryTotalW) / 2;
    move(IDC_PRESENT, primaryX, dockY, primaryW, dockPrimaryH);
    move(IDC_ABSENT, primaryX + (primaryW + primaryGap), dockY, primaryW, dockPrimaryH);
    move(IDC_LATE, primaryX + (primaryW + primaryGap) * 2, dockY, primaryW, dockPrimaryH);
    move(IDC_OTHER_STATUS, primaryX + (primaryW + primaryGap) * 3, dockY, primaryW, dockPrimaryH);
    move(IDC_LEAVE, primaryX + (primaryW + primaryGap) * 4, dockY, primaryW, dockPrimaryH);
    move(IDC_PENDING, primaryX + (primaryW + primaryGap) * 5, dockY, primaryW, dockPrimaryH);
    move(IDC_SAVE, primaryX + (primaryW + primaryGap) * 6, dockY, primaryW, dockPrimaryH);
    move(IDC_EXPORT_CSV, primaryX + (primaryW + primaryGap) * 7, dockY, primaryW, dockPrimaryH);

    int drawerGap = compact ? 6 : spacious ? 14 : 10;
    int drawerW = std::clamp((layoutW - pad * 2 - drawerGap * 7) / 8, 112, 148);
    int drawerTotalW = drawerW * 8 + drawerGap * 7;
    int drawerX = pad + (layoutW - pad * 2 - drawerTotalW) / 2;
    move(IDC_ADD_UPDATE, drawerX, drawerY, drawerW, drawerH);
    move(IDC_EDIT_SELECTED, drawerX + (drawerW + drawerGap), drawerY, drawerW, drawerH);
    move(IDC_ALL_PRESENT, drawerX + (drawerW + drawerGap) * 2, drawerY, drawerW, drawerH);
    move(IDC_NEW, drawerX + (drawerW + drawerGap) * 3, drawerY, drawerW, drawerH);
    move(IDC_IMPORT, drawerX + (drawerW + drawerGap) * 4, drawerY, drawerW, drawerH);
    move(IDC_DELETE, drawerX + (drawerW + drawerGap) * 5, drawerY, drawerW, drawerH);
    move(IDC_TOOLS, drawerX + (drawerW + drawerGap) * 6, drawerY, drawerW, drawerH);
    move(IDC_SETTINGS, drawerX + (drawerW + drawerGap) * 7, drawerY, drawerW, drawerH);

    int footerStatsW = std::min(720, std::max(420, layoutW - pad * 2));
    int footerStatsX = std::max(6, layoutW - footerStatsW - 6);
    int hintW = std::max(260, footerStatsX - 18);
    move(IDC_HINT, 6, layoutH - 20, hintW, 18);
    move(IDC_STATS, footerStatsX, layoutH - 20, footerStatsW, 18);

    int widths[4] = {
        std::max(210, listW / 5),
        std::max(220, listW / 4),
        std::max(150, listW / 7),
        std::max(260, listW - (listW / 5) - (listW / 4) - (listW / 7) - 24)
    };
    for (int i = 0; i < 4; ++i) ListView_SetColumnWidth(Ui().list, i, widths[i]);

    SCROLLINFO si{};
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = std::max(0, Ui().contentW - 1);
    si.nPage = std::max(1, w);
    si.nPos = Ui().scrollX;
    SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

    si.nMax = std::max(0, Ui().contentH - 1);
    si.nPage = std::max(1, h);
    si.nPos = Ui().scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

    inLayout = false;
}

void ScrollMainWindow(HWND hwnd, int bar, int code, int wheelDelta){
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int viewport = bar == SB_VERT ? rc.bottom - rc.top : rc.right - rc.left;
    int content = bar == SB_VERT ? Ui().contentH : Ui().contentW;
    int current = bar == SB_VERT ? Ui().scrollY : Ui().scrollX;
    int maxPos = std::max(0, content - viewport);
    int next = current;

    switch (code) {
    case SB_LINEUP:
        next -= 36;
        break;
    case SB_LINEDOWN:
        next += 36;
        break;
    case SB_PAGEUP:
        next -= std::max(120, viewport - 80);
        break;
    case SB_PAGEDOWN:
        next += std::max(120, viewport - 80);
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: {
        SCROLLINFO si{};
        si.cbSize = sizeof(si);
        si.fMask = SIF_TRACKPOS;
        GetScrollInfo(hwnd, bar, &si);
        next = si.nTrackPos;
        break;
    }
    case SB_TOP:
        next = 0;
        break;
    case SB_BOTTOM:
        next = maxPos;
        break;
    case SB_ENDSCROLL:
        return;
    default:
        if (wheelDelta != 0) {
            next += -wheelDelta / WHEEL_DELTA * 90;
            if (std::abs(wheelDelta) < WHEEL_DELTA) next += wheelDelta < 0 ? 90 : -90;
        }
        break;
    }

    next = std::clamp(next, 0, maxPos);
    if (next == current) return;

    if (bar == SB_VERT) Ui().scrollY = next;
    else Ui().scrollX = next;
    ResizeLayout(hwnd);
}

} // namespace attendance
