#include "ui/statistics.h"
#include "animation/scheduler.h"
#include "core/constants.h"
#include "core/status.h"
#include "localization/strings.h"
#include "model/query.h"
#include "model/state.h"
#include "settings/state.h"
#include "ui/motion.h"
#include "ui/redraw.h"
#include "ui/state.h"
#include "ui/title_bar.h"

namespace attendance {

void PaintStatsChart(HWND hwnd, HDC hdc);
LRESULT CALLBACK ChartProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace {
std::wstring ChartTitle() {
  auto title = Tr(L"Statistics Chart", L"统计图表");
  if (!Document().sheets.empty())
    title += L" - " + Document().sheets[Document().activeSheet].name;
  return title;
}
void UpdateChartScroll(HWND hwnd) {
  RECT client{};
  GetClientRect(hwnd, &client);
  auto layout = MeasureStatsChartLayout(hwnd, client.right);
  Ui().chartScrollY =
      std::clamp(Ui().chartScrollY, 0,
                 std::max(0, layout.contentHeight - (int)client.bottom));
  SCROLLINFO info{sizeof(info)};
  info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
  info.nMax = layout.contentHeight - 1;
  info.nPage = client.bottom;
  info.nPos = Ui().chartScrollY;
  SetScrollInfo(hwnd, SB_VERT, &info, TRUE);
  InvalidateRect(hwnd, nullptr, FALSE);
}
} // namespace

StatsChartLayout MeasureStatsChartLayout(HWND hwnd, int width) {
  StatsChartLayout layout;
  int pad = 24, inner = std::max(1, width - pad * 2);
  HDC dc = GetDC(hwnd);
  HGDIOBJ old = SelectObject(dc, Ui().titleFont);
  layout.title = {pad, 20, pad + inner, 20};
  auto title = ChartTitle();
  DrawTextW(dc, title.c_str(), -1, &layout.title,
            DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
  SelectObject(dc, Ui().font);
  int labelW = 0;
  for (auto key : StatusKeys) {
    auto text = StatusDisplayText(key);
    SIZE size{};
    GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &size);
    labelW = std::max(labelW, (int)size.cx + 8);
  }
  labelW = std::clamp(labelW, 72, std::max(72, inner / 3));
  auto stats = CountRecords(Document().Records());
  int values[] = {stats.present, stats.absent, stats.late,
                  stats.other,   stats.leave,  stats.pending},
      countW = 32;
  for (int value : values) {
    auto text = std::to_wstring(value);
    SIZE size{};
    GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &size);
    countW = std::max(countW, (int)size.cx + 8);
  }
  int barX = pad + labelW + 16,
      barRight = std::max(barX + 1, width - pad - countW - 16),
      y = layout.title.bottom + 24;
  for (size_t i = 0; i < StatusKeys.size(); ++i) {
    auto text = StatusDisplayText(StatusKeys[i]);
    RECT measured{0, 0, labelW, 0};
    DrawTextW(dc, text.c_str(), -1, &measured,
              DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    int rowH = std::max(36, (int)measured.bottom + 8);
    layout.labels[i] = {pad, y, pad + labelW, y + rowH};
    layout.bars[i] = {barX, y, barRight, y + rowH};
    layout.counts[i] = {barRight + 16, y, width - pad, y + rowH};
    y += rowH + 16;
  }
  layout.contentHeight = y + 8;
  SelectObject(dc, old);
  ReleaseDC(hwnd, dc);
  return layout;
}

void PaintStatsChart(HWND hwnd, HDC hdc) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  FillRect(hdc, &rc, Ui().bgBrush);

  auto stats = CountRecords(Document().Records());
  int values[] = {stats.present, stats.absent, stats.late,
                  stats.other,   stats.leave,  stats.pending};
  COLORREF colors[] = {RGB(126, 154, 142), RGB(170, 112, 112),
                       RGB(164, 146, 104), RGB(126, 126, 126),
                       RGB(110, 140, 180), RGB(100, 100, 120)};
  int maxValue =
      std::max(1, *std::max_element(std::begin(values), std::end(values)));
  double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);

  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, Ui().COLOR_TEXT);
  auto layout = MeasureStatsChartLayout(hwnd, rc.right);
  auto title = ChartTitle();
  RECT titleRect = layout.title;
  OffsetRect(&titleRect, 0, -Ui().chartScrollY);
  HGDIOBJ oldFont = SelectObject(hdc, Ui().titleFont);
  DrawTextW(hdc, title.c_str(), -1, &titleRect, DT_WORDBREAK | DT_NOPREFIX);

  SelectObject(hdc, Ui().font);
  for (int i = 0; i < 6; ++i) {
    std::wstring label = StatusDisplayText(StatusKeys[i]);
    RECT labelRect = layout.labels[i];
    OffsetRect(&labelRect, 0, -Ui().chartScrollY);
    DrawTextW(hdc, label.c_str(), -1, &labelRect,
              DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
    RECT barBg = layout.bars[i];
    OffsetRect(&barBg, 0, -Ui().chartScrollY);
    int barMaxW = barBg.right - barBg.left;
    HBRUSH bg = CreateSolidBrush(Ui().COLOR_PANEL);
    FillRect(hdc, &barBg, bg);
    DeleteObject(bg);

    double rowProgress = ItemEntrance(reveal, i);
    int width = (int)(barMaxW * (values[i] / (double)maxValue) * rowProgress);
    RECT bar = barBg;
    bar.right = bar.left + width;
    HBRUSH fill = CreateSolidBrush(colors[i]);
    FillRect(hdc, &bar, fill);
    DeleteObject(fill);

    std::wstring count = std::to_wstring(values[i]);
    RECT countRect = layout.counts[i];
    OffsetRect(&countRect, 0, -Ui().chartScrollY);
    DrawStableDigits(hdc, count, countRect, DT_RIGHT);
  }
  SelectObject(hdc, oldFont);
}

LRESULT CALLBACK ChartProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_SIZE:
    UpdateChartScroll(hwnd);
    return 0;
  case WM_GETMINMAXINFO: {
    SIZE minimum = BoundedWindowSize(
        hwnd, 520, MeasureStatsChartLayout(hwnd, 520).contentHeight,
        WS_OVERLAPPEDWINDOW | WS_VSCROLL, WS_EX_LAYERED);
    auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
    info->ptMinTrackSize = {minimum.cx, minimum.cy};
    return 0;
  }
  case WM_MOUSEWHEEL:
    Ui().chartScrollY -=
        (int)std::lround(GET_WHEEL_DELTA_WPARAM(wParam) / 120.0 * 100);
    UpdateChartScroll(hwnd);
    return 0;
  case WM_VSCROLL: {
    SCROLLINFO info{sizeof(info)};
    info.fMask = SIF_ALL;
    GetScrollInfo(hwnd, SB_VERT, &info);
    switch (LOWORD(wParam)) {
    case SB_LINEUP:
      Ui().chartScrollY -= 40;
      break;
    case SB_LINEDOWN:
      Ui().chartScrollY += 40;
      break;
    case SB_PAGEUP:
      Ui().chartScrollY -= (int)info.nPage;
      break;
    case SB_PAGEDOWN:
      Ui().chartScrollY += (int)info.nPage;
      break;
    case SB_TOP:
      Ui().chartScrollY = 0;
      break;
    case SB_BOTTOM:
      Ui().chartScrollY = info.nMax;
      break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
      Ui().chartScrollY = info.nTrackPos;
      break;
    default:
      return 0;
    }
    UpdateChartScroll(hwnd);
    return 0;
  }
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE) {
      SendMessageW(hwnd, WM_CLOSE, 0, 0);
      return 0;
    }
    break;
  case WM_PAINT: {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rc{};
    GetClientRect(hwnd, &rc);
    int width = std::max(1, (int)(rc.right - rc.left));
    int height = std::max(1, (int)(rc.bottom - rc.top));
    HDC memDc = CreateCompatibleDC(hdc);
    HBITMAP bitmap = CreateCompatibleBitmap(hdc, width, height);
    HGDIOBJ oldBitmap = SelectObject(memDc, bitmap);
    PaintStatsChart(hwnd, memDc);
    BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);
    SelectObject(memDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDc);
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_APP_ANIMATION_TICK:
    AcknowledgeAnimationTick(hwnd);
    SetLayeredWindowAttributes(
        hwnd, 0,
        (BYTE)std::lround(255 *
                          GetAnimationValue(hwnd, AnimChannel::Reveal, 1) *
                          (1 - GetAnimationValue(hwnd, AnimChannel::Close, 0))),
        LWA_ALPHA);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
  case WM_APP_ANIMATION_DONE:
    DestroyWindow(hwnd);
    return 0;
  case WM_CLOSE:
    BeginAnimatedClose(hwnd);
    return 0;
  case WM_DESTROY:
    RemoveAnimationState(hwnd);
    Ui().chartWindow = nullptr;
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowStatsChart() {
  if (Ui().chartWindow && IsWindow(Ui().chartWindow)) {
    InvalidateRect(Ui().chartWindow, nullptr, FALSE);
    SetForegroundWindow(Ui().chartWindow);
    return;
  }

  HINSTANCE instance = GetModuleHandleW(nullptr);
  const wchar_t *className = L"AttendanceStatsChart";
  static bool registered = false;
  if (!registered) {
    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ChartProc;
    wc.hInstance = instance;
    wc.lpszClassName = className;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
    RegisterClassW(&wc);
    registered = true;
  }
  std::wstring chartTitle =
      Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868");
  Ui().chartScrollY = 0;
  SIZE size = BoundedWindowSize(
      Ui().hwnd, 800, MeasureStatsChartLayout(Ui().hwnd, 800).contentHeight,
      WS_OVERLAPPEDWINDOW | WS_VSCROLL, WS_EX_LAYERED);
  Ui().chartWindow = CreateWindowExW(
      WS_EX_LAYERED, className, chartTitle.c_str(),
      WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VSCROLL, CW_USEDEFAULT,
      CW_USEDEFAULT, size.cx, size.cy, Ui().hwnd, nullptr, instance, nullptr);
  ApplyGlassTitleBar(Ui().chartWindow);
  CenterOwnedWindow(Ui().chartWindow, Ui().hwnd);
  StartWindowReveal(Ui().chartWindow, 190);
  SetLayeredWindowAttributes(
      Ui().chartWindow, 0,
      Settings().animationLevel == AnimationLevel::Off ? 255 : 0, LWA_ALPHA);
  ShowWindow(Ui().chartWindow, SW_SHOW);
  UpdateWindow(Ui().chartWindow);
}

} // namespace attendance
