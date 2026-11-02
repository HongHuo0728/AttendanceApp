#include "ui/layout.h"
#include "core/constants.h"
#include "settings/fonts.h"
#include "settings/state.h"
#include "ui/common.h"
#include "ui/motion.h"
#include "ui/redraw.h"
#include "ui/state.h"

namespace attendance {

void ToggleFullscreen(HWND hwnd) {
  Ui().fullscreen = !Ui().fullscreen;
  if (Ui().fullscreen) {
    Ui().previousStyle = GetWindowLongW(hwnd, GWL_STYLE);
    GetWindowPlacement(hwnd, &Ui().previousPlacement);

    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
    SetWindowLongW(hwnd, GWL_STYLE, Ui().previousStyle & ~WS_OVERLAPPEDWINDOW);
    SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                 mi.rcMonitor.right - mi.rcMonitor.left,
                 mi.rcMonitor.bottom - mi.rcMonitor.top,
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  } else {
    SetWindowLongW(hwnd, GWL_STYLE, Ui().previousStyle);
    SetWindowPlacement(hwnd, &Ui().previousPlacement);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER |
                     SWP_FRAMECHANGED);
  }
}

void ResizeLayout(HWND hwnd) {
  static bool inLayout = false;
  if (inLayout)
    return;
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

  const bool compact = Settings().layoutDensity == LayoutDensity::Compact;
  const bool spacious = Settings().layoutDensity == LayoutDensity::Spacious;
  int pad = compact ? 20 : spacious ? 36 : 28;
  struct Placement {
    HWND child;
    int x, y, width, height;
  };
  std::vector<Placement> placements;
  auto moveHwnd = [&](HWND child, int x, int y, int width, int height) {
    if (child)
      placements.push_back({child, x, y, width, height});
  };
  auto move = [&](int id, int x, int y, int width, int height) {
    moveHwnd(GetDlgItem(hwnd, id), x, y, width, height);
  };

  int titleY = compact ? 14 : spacious ? 28 : 20;
  int courseW = 300;
  int courseBtnW = std::max(
      124, MeasuredTextWidth(GetDlgItem(hwnd, IDC_COURSE_OPTIONS)) + 40);
  int courseX = std::max(pad + 520, layoutW - pad - courseW - courseBtnW - 12);
  moveHwnd(Ui().courseCombo, courseX, titleY + 6, courseW, 430);
  move(IDC_COURSE_OPTIONS, courseX + courseW + 12, titleY + 6, courseBtnW,
       ComboItemHeight() + 6);

  int titleWidth = std::max(200, courseX - pad - 24);
  int titleHeight =
      WrappedTextHeight(GetDlgItem(hwnd, IDC_TITLE), titleWidth) + 4;
  int subtitleWidth = std::min(720, titleWidth);
  int subtitleHeight =
      WrappedTextHeight(GetDlgItem(hwnd, IDC_SUBTITLE), subtitleWidth) + 4;
  move(IDC_TITLE, pad, titleY, titleWidth, titleHeight);
  move(IDC_SUBTITLE, pad, titleY + titleHeight + 8, subtitleWidth,
       subtitleHeight);
  int statsY = std::max(compact    ? 90
                        : spacious ? 120
                                   : 104,
                        titleY + titleHeight + subtitleHeight + 32);
  int statsGap = compact ? 12 : spacious ? 22 : 16;
  int statsH = Settings().showStatsCards ? (compact    ? 78
                                            : spacious ? 108
                                                       : 92)
                                         : 0;
  int statW = std::max(180, (layoutW - pad * 2 - statsGap * 3) / 4);
  if (Settings().showStatsCards) {
    HDC dc = GetDC(hwnd);
    if (dc) {
      HGDIOBJ old =
          SelectObject(dc, Ui().smallFont ? Ui().smallFont : Ui().font);
      for (int id : {IDC_STAT_TOTAL, IDC_STAT_ATTENDANCE, IDC_STAT_ISSUES,
                     IDC_STAT_VISIBLE}) {
        auto text = GetText(GetDlgItem(hwnd, id));
        text = text.substr(0, text.find(L'\n'));
        RECT label{0, 0, std::max(1, statW - 36), 0};
        DrawTextW(dc, text.c_str(), -1, &label,
                  DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
        statsH = std::max(statsH, (int)label.bottom + 52);
      }
      SelectObject(dc, old);
      ReleaseDC(hwnd, dc);
    }
  }
  int statX = pad;
  move(IDC_STAT_TOTAL, statX, statsY, statW, statsH);
  move(IDC_STAT_ATTENDANCE, statX + (statW + statsGap), statsY, statW, statsH);
  move(IDC_STAT_ISSUES, statX + (statW + statsGap) * 2, statsY, statW, statsH);
  move(IDC_STAT_VISIBLE, statX + (statW + statsGap) * 3, statsY, statW, statsH);

  int sectionGap = compact ? 12 : spacious ? 24 : 18;
  int filterY =
      Settings().showStatsCards ? statsY + statsH + sectionGap : statsY;
  int listX = pad;
  int listW = std::max(720, layoutW - pad * 2);
  int clearW =
      std::max(136, MeasuredTextWidth(GetDlgItem(hwnd, IDC_CLEAR_FILTER)) + 32);
  int filterLabelW =
      std::max(74, MeasuredTextWidth(GetDlgItem(hwnd, IDC_FILTER_LABEL)) + 16);
  int filterW = std::min(460, std::max(240, listW / 3));
  int fieldH = std::max(compact    ? 32
                        : spacious ? 44
                                   : 36,
                        MeasuredTextHeight(Ui().filterEdit) + 16);
  int filterLabelH = MeasuredTextHeight(GetDlgItem(hwnd, IDC_FILTER_LABEL)) + 4;
  int fieldLabelOffset = std::max(0, (fieldH - filterLabelH) / 2);
  move(IDC_FILTER_LABEL, listX, filterY + fieldLabelOffset, filterLabelW,
       filterLabelH);
  moveHwnd(Ui().filterEdit, listX + filterLabelW, filterY, filterW, fieldH);
  move(IDC_CLEAR_FILTER, listX + filterLabelW + filterW + 12, filterY, clearW,
       fieldH);

  int dockPrimaryH = Settings().layoutDensity == LayoutDensity::Compact    ? 50
                     : Settings().layoutDensity == LayoutDensity::Spacious ? 64
                                                                           : 56;
  int drawerH = Settings().layoutDensity == LayoutDensity::Compact    ? 36
                : Settings().layoutDensity == LayoutDensity::Spacious ? 44
                                                                      : 40;
  int primaryCell = std::clamp((layoutW - pad * 2 -
                                (compact    ? 8
                                 : spacious ? 18
                                            : 12) *
                                    7) /
                                   8,
                               100, 148);
  int drawerCell = std::clamp((layoutW - pad * 2 -
                               (compact    ? 6
                                : spacious ? 14
                                           : 10) *
                                   7) /
                                  8,
                              112, 148);
  for (int id : {IDC_PRESENT, IDC_ABSENT, IDC_LATE, IDC_OTHER_STATUS, IDC_LEAVE,
                 IDC_PENDING, IDC_SAVE, IDC_EXPORT_CSV})
    dockPrimaryH = std::max(
        dockPrimaryH,
        WrappedTextHeight(GetDlgItem(hwnd, id), primaryCell - 24) + 16);
  for (int id : {IDC_ADD_UPDATE, IDC_EDIT_SELECTED, IDC_ALL_PRESENT, IDC_NEW,
                 IDC_IMPORT, IDC_DELETE, IDC_TOOLS, IDC_SETTINGS})
    drawerH = std::max(
        drawerH, WrappedTextHeight(GetDlgItem(hwnd, id), drawerCell - 24) + 16);
  int footerH = std::max(MeasuredTextHeight(GetDlgItem(hwnd, IDC_HINT)),
                         MeasuredTextHeight(GetDlgItem(hwnd, IDC_STATS))) +
                2;
  int bottomPad = std::max(compact ? 22 : spacious ? 36 : 28, footerH + 10);
  int dockGap = compact ? 10 : spacious ? 22 : 16;
  int editorH = std::max(compact    ? 34
                         : spacious ? 46
                                    : 38,
                         MeasuredTextHeight(Ui().dateEdit) + 16);
  int composerGap = std::max(compact ? 54 : spacious ? 82 : 66, editorH + 16);
  int listBottomGap = compact ? 14 : spacious ? 28 : 20;
  int composerW = std::min(1060, layoutW - pad * 2);
  int composerX = pad + (layoutW - pad * 2 - composerW) / 2;
  int dateW = std::max(210, composerW / 4);
  int nameW = std::max(240, composerW / 4);
  int otherW = composerW - dateW - nameW - 32;
  int labelH = std::max({WrappedTextHeight(GetDlgItem(hwnd, 2001), dateW),
                         WrappedTextHeight(GetDlgItem(hwnd, 2002), nameW),
                         WrappedTextHeight(GetDlgItem(hwnd, 2003), otherW)}) +
               4;
  int listY = filterY + fieldH + 16;
  // Reserve the entire composer, including its labels, before placing the list.
  // Long translations and large fonts increase the scrollable content height.
  int minimumH = listY + 260 + listBottomGap + labelH + 8 + composerGap +
                 dockPrimaryH + dockGap + drawerH + bottomPad;
  layoutH = std::max(layoutH, minimumH);
  int drawerY = layoutH - drawerH - bottomPad;
  int dockY = drawerY - dockPrimaryH - dockGap;
  int composerY = dockY - composerGap;
  int labelY = composerY - labelH - 8;
  int listH = labelY - listBottomGap - listY;
  moveHwnd(Ui().list, listX, listY, listW, listH);
  move(2001, composerX, labelY, dateW, labelH);
  moveHwnd(Ui().dateEdit, composerX, composerY, dateW, editorH);
  move(2002, composerX + dateW + 16, labelY, nameW, labelH);
  moveHwnd(Ui().nameEdit, composerX + dateW + 16, composerY, nameW, editorH);
  move(2003, composerX + dateW + nameW + 32, labelY, otherW, labelH);
  moveHwnd(Ui().otherEdit, composerX + dateW + nameW + 32, composerY, otherW,
           editorH);

  int primaryGap = compact ? 8 : spacious ? 18 : 12;
  int primaryW = std::clamp((layoutW - pad * 2 - primaryGap * 7) / 8, 100, 148);
  int primaryTotalW = primaryW * 8 + primaryGap * 7;
  int primaryX = pad + (layoutW - pad * 2 - primaryTotalW) / 2;
  move(IDC_PRESENT, primaryX, dockY, primaryW, dockPrimaryH);
  move(IDC_ABSENT, primaryX + (primaryW + primaryGap), dockY, primaryW,
       dockPrimaryH);
  move(IDC_LATE, primaryX + (primaryW + primaryGap) * 2, dockY, primaryW,
       dockPrimaryH);
  move(IDC_OTHER_STATUS, primaryX + (primaryW + primaryGap) * 3, dockY,
       primaryW, dockPrimaryH);
  move(IDC_LEAVE, primaryX + (primaryW + primaryGap) * 4, dockY, primaryW,
       dockPrimaryH);
  move(IDC_PENDING, primaryX + (primaryW + primaryGap) * 5, dockY, primaryW,
       dockPrimaryH);
  move(IDC_SAVE, primaryX + (primaryW + primaryGap) * 6, dockY, primaryW,
       dockPrimaryH);
  move(IDC_EXPORT_CSV, primaryX + (primaryW + primaryGap) * 7, dockY, primaryW,
       dockPrimaryH);

  int drawerGap = compact ? 6 : spacious ? 14 : 10;
  int drawerW = std::clamp((layoutW - pad * 2 - drawerGap * 7) / 8, 112, 148);
  int drawerTotalW = drawerW * 8 + drawerGap * 7;
  int drawerX = pad + (layoutW - pad * 2 - drawerTotalW) / 2;
  move(IDC_ADD_UPDATE, drawerX, drawerY, drawerW, drawerH);
  move(IDC_EDIT_SELECTED, drawerX + (drawerW + drawerGap), drawerY, drawerW,
       drawerH);
  move(IDC_ALL_PRESENT, drawerX + (drawerW + drawerGap) * 2, drawerY, drawerW,
       drawerH);
  move(IDC_NEW, drawerX + (drawerW + drawerGap) * 3, drawerY, drawerW, drawerH);
  move(IDC_IMPORT, drawerX + (drawerW + drawerGap) * 4, drawerY, drawerW,
       drawerH);
  move(IDC_DELETE, drawerX + (drawerW + drawerGap) * 5, drawerY, drawerW,
       drawerH);
  move(IDC_TOOLS, drawerX + (drawerW + drawerGap) * 6, drawerY, drawerW,
       drawerH);
  move(IDC_SETTINGS, drawerX + (drawerW + drawerGap) * 7, drawerY, drawerW,
       drawerH);

  int footerStatsW = std::min(720, std::max(420, layoutW - pad * 2));
  int footerStatsX = std::max(6, layoutW - footerStatsW - 6);
  int hintW = std::max(260, footerStatsX - 18);
  move(IDC_HINT, 6, layoutH - footerH - 2, hintW, footerH);
  move(IDC_STATS, footerStatsX, layoutH - footerH - 2, footerStatsW, footerH);

  int widths[4] = {
      std::max(210, listW / 5), std::max(220, listW / 4),
      std::max(150, listW / 7),
      std::max(260, listW - (listW / 5) - (listW / 4) - (listW / 7) - 24)};
  for (int i = 0; i < 4; ++i)
    if (ListView_GetColumnWidth(Ui().list, i) != widths[i])
      ListView_SetColumnWidth(Ui().list, i, widths[i]);

  Ui().contentW = layoutW;
  Ui().contentH = layoutH;
  Ui().scrollX = std::clamp(Ui().scrollX, 0, std::max(0, layoutW - w));
  Ui().scrollY = std::clamp(Ui().scrollY, 0, std::max(0, layoutH - h));

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

  // Scrollbar visibility can change the client area synchronously.
  GetClientRect(hwnd, &rc);
  Ui().scrollX =
      std::clamp(Ui().scrollX, 0, std::max(0, layoutW - (int)rc.right));
  Ui().scrollY =
      std::clamp(Ui().scrollY, 0, std::max(0, layoutH - (int)rc.bottom));
  HDWP batch = BeginDeferWindowPos((int)placements.size());
  for (const auto &p : placements) {
    if (batch)
      batch = DeferWindowPos(batch, p.child, nullptr, p.x - Ui().scrollX,
                             p.y - Ui().scrollY, p.width, p.height,
                             SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
  }
  if (batch)
    EndDeferWindowPos(batch);
  else
    for (const auto &p : placements)
      MoveWindow(p.child, p.x - Ui().scrollX, p.y - Ui().scrollY, p.width,
                 p.height, FALSE);

  inLayout = false;
}

void EnsureMainControlVisible(HWND control) {
  HWND hwnd = Ui().hwnd;
  if (!hwnd || !control || GetParent(control) != hwnd)
    return;
  RECT box{}, client{};
  GetWindowRect(control, &box);
  MapWindowPoints(nullptr, hwnd, (POINT *)&box, 2);
  GetClientRect(hwnd, &client);
  int x = Ui().scrollX, y = Ui().scrollY;
  if (box.left < 8)
    x += box.left - 8;
  else if (box.right > client.right - 8)
    x += box.right - client.right + 8;
  if (box.top < 8)
    y += box.top - 8;
  else if (box.bottom > client.bottom - 8)
    y += box.bottom - client.bottom + 8;
  x = std::clamp(x, 0, std::max(0, Ui().contentW - (int)client.right));
  y = std::clamp(y, 0, std::max(0, Ui().contentH - (int)client.bottom));
  if (x != Ui().scrollX || y != Ui().scrollY) {
    Ui().scrollX = x;
    Ui().scrollY = y;
    ResizeLayout(hwnd);
  }
}

void ScrollMainWindow(HWND hwnd, int bar, int code, int wheelDelta) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  int viewport = bar == SB_VERT ? rc.bottom - rc.top : rc.right - rc.left;
  int content = bar == SB_VERT ? Ui().contentH : Ui().contentW;
  int current = bar == SB_VERT ? Ui().scrollY : Ui().scrollX;
  int maxPos = std::max(0, content - viewport);
  int next = current;

  if (wheelDelta != 0) {
    int movement = (int)std::lround(wheelDelta * 90.0 / WHEEL_DELTA);
    next += bar == SB_HORZ ? movement : -movement;
  } else
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
      break;
    }

  next = std::clamp(next, 0, maxPos);
  if (next == current)
    return;

  if (bar == SB_VERT)
    Ui().scrollY = next;
  else
    Ui().scrollX = next;
  ResizeLayout(hwnd);
}

} // namespace attendance
