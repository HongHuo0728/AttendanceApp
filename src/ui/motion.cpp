#include "ui/motion.h"
#include "animation/scheduler.h"
#include "core/constants.h"
#include "settings/state.h"
#include "ui/common.h"
#include "ui/redraw.h"
#include "ui/state.h"

namespace attendance {
double ItemEntrance(double progress, int visibleIndex) {
  if (progress >= 1.0 || Settings().animationLevel != AnimationLevel::Advanced)
    return progress;
  return CubicBezierEase(
      std::clamp((progress * 360 - std::min(visibleIndex, 6) * 20) / 240, 0.0,
                 1.0),
      .22, 1, .36, 1);
}
RECT PaperViewport(int width, int height, double progress, bool upward) {
  int visible = std::clamp(
      (int)std::lround(height * std::clamp(progress, 0.0, 1.0)), 0, height);
  return RECT{0, upward ? height - visible : 0, width,
              upward ? height : visible};
}
void ClipPaperWindow(HWND window, double progress, bool upward) {
  RECT bounds{};
  GetWindowRect(window, &bounds);
  int width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
  RECT clip = PaperViewport(width, height, progress, upward);
  auto signature =
      (UINT_PTR)(clip.top + clip.bottom + 1) | (upward ? (UINT_PTR)1 << 30 : 0);
  auto dimensions = (UINT_PTR)(unsigned short)width |
                    ((UINT_PTR)(unsigned short)height << 16);
  if ((UINT_PTR)GetPropW(window, L"Attendance.PaperClip") == signature &&
      (UINT_PTR)GetPropW(window, L"Attendance.PaperSize") == dimensions)
    return;
  SetPropW(window, L"Attendance.PaperClip", (HANDLE)signature);
  SetPropW(window, L"Attendance.PaperSize", (HANDLE)dimensions);
  HRGN shape =
      CreateRoundRectRgn(0, 0, width + 1, height + 1, Visual::PopupRadius * 2,
                         Visual::PopupRadius * 2);
  HRGN visible = CreateRectRgnIndirect(&clip);
  if (shape && visible)
    CombineRgn(shape, shape, visible, RGN_AND);
  if (visible)
    DeleteObject(visible);
  if (shape && !SetWindowRgn(window, shape, FALSE))
    DeleteObject(shape);
}
void PaintSurface(HDC dc, RECT rect, COLORREF fill, int radius) {
  HGDIOBJ pen = SelectObject(dc, GetStockObject(DC_PEN));
  HGDIOBJ brush = SelectObject(dc, GetStockObject(DC_BRUSH));
  for (int inset = 0; inset < 3; ++inset) {
    SetDCBrushColor(
        dc, BlendColor(Ui().COLOR_BG, Ui().COLOR_BORDER, .05 + inset * .04));
    SetDCPenColor(dc, GetDCBrushColor(dc));
    RoundRect(dc, rect.left + inset, rect.top + inset + 1, rect.right - inset,
              rect.bottom - inset, radius * 2, radius * 2);
  }
  InflateRect(&rect, -3, -3);
  SetDCBrushColor(dc, fill);
  SetDCPenColor(dc, fill);
  RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom,
            std::max(2, radius - 3) * 2, std::max(2, radius - 3) * 2);
  SelectObject(dc, brush);
  SelectObject(dc, pen);
}
void PaintRoundedControl(HDC dc, RECT rect, COLORREF fill, COLORREF border,
                         int radius) {
  int width = rect.right - rect.left, height = rect.bottom - rect.top;
  if (width <= 0 || height <= 0)
    return;
  // Only the shape is supersampled. Native text remains at the display's
  // resolution.
  HDC smooth = CreateCompatibleDC(dc);
  HBITMAP bitmap =
      smooth ? CreateCompatibleBitmap(dc, width * 2, height * 2) : nullptr;
  if (smooth && bitmap) {
    HGDIOBJ oldBitmap = SelectObject(smooth, bitmap);
    RECT area{0, 0, width * 2, height * 2};
    FillRect(smooth, &area, Ui().bgBrush);
    HGDIOBJ oldBrush = SelectObject(smooth, GetStockObject(DC_BRUSH));
    HPEN outline = CreatePen(PS_SOLID, 2, border);
    HGDIOBJ oldPen = SelectObject(smooth, outline);
    SetDCBrushColor(smooth, fill);
    RoundRect(smooth, 1, 1, width * 2 - 1, height * 2 - 1, radius * 4,
              radius * 4);
    int saved = SaveDC(dc);
    SetStretchBltMode(dc, HALFTONE);
    SetBrushOrgEx(dc, 0, 0, nullptr);
    StretchBlt(dc, rect.left, rect.top, width, height, smooth, 0, 0, width * 2,
               height * 2, SRCCOPY);
    RestoreDC(dc, saved);
    SelectObject(smooth, oldPen);
    SelectObject(smooth, oldBrush);
    SelectObject(smooth, oldBitmap);
    DeleteObject(outline);
    DeleteObject(bitmap);
    DeleteDC(smooth);
    return;
  }
  if (smooth)
    DeleteDC(smooth);
  HGDIOBJ oldPen = SelectObject(dc, GetStockObject(DC_PEN)),
          oldBrush = SelectObject(dc, GetStockObject(DC_BRUSH));
  SetDCPenColor(dc, border);
  SetDCBrushColor(dc, fill);
  RoundRect(dc, rect.left, rect.top, rect.right, rect.bottom, radius * 2,
            radius * 2);
  SelectObject(dc, oldBrush);
  SelectObject(dc, oldPen);
}
int MeasuredTextHeight(HWND control) {
  HDC dc = GetDC(control);
  if (!dc)
    return 24;
  HFONT font = (HFONT)SendMessageW(control, WM_GETFONT, 0, 0);
  HGDIOBJ old = font ? SelectObject(dc, font) : nullptr;
  TEXTMETRICW metrics{};
  GetTextMetricsW(dc, &metrics);
  if (old)
    SelectObject(dc, old);
  ReleaseDC(control, dc);
  return std::max(18, (int)(metrics.tmHeight + metrics.tmExternalLeading));
}
int MeasuredTextWidth(HWND control) {
  HDC dc = GetDC(control);
  if (!dc)
    return 160;
  HFONT font = (HFONT)SendMessageW(control, WM_GETFONT, 0, 0);
  HGDIOBJ old = font ? SelectObject(dc, font) : nullptr;
  auto text = GetText(control);
  SIZE size{};
  GetTextExtentPoint32W(dc, text.c_str(), (int)text.size(), &size);
  if (old)
    SelectObject(dc, old);
  ReleaseDC(control, dc);
  return size.cx;
}
int WrappedTextHeight(HWND control, int width) {
  HDC dc = GetDC(control);
  if (!dc)
    return 24;
  HGDIOBJ old =
      SelectObject(dc, (HFONT)SendMessageW(control, WM_GETFONT, 0, 0));
  RECT rect{0, 0, std::max(16, width), 0};
  auto text = GetText(control);
  DrawTextW(dc, text.c_str(), -1, &rect,
            DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
  SelectObject(dc, old);
  ReleaseDC(control, dc);
  return std::max(18, (int)rect.bottom);
}
SIZE BoundedWindowSize(HWND owner, int clientWidth, int clientHeight,
                       DWORD style, DWORD exStyle) {
  RECT bounds{0, 0, clientWidth, clientHeight};
  AdjustWindowRectEx(&bounds, style, FALSE, exStyle);
  MONITORINFO monitor{sizeof(monitor)};
  GetMonitorInfoW(MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST), &monitor);
  return SIZE{
      std::min((LONG)(bounds.right - bounds.left),
               std::max(1L, monitor.rcWork.right - monitor.rcWork.left - 24)),
      std::min((LONG)(bounds.bottom - bounds.top),
               std::max(1L, monitor.rcWork.bottom - monitor.rcWork.top - 24))};
}
void DrawStableDigits(HDC dc, const std::wstring &text, RECT rect,
                      UINT alignment) {
  SIZE size{};
  int digitWidth = 0;
  for (wchar_t c = L'0'; c <= L'9'; ++c) {
    GetTextExtentPoint32W(dc, &c, 1, &size);
    digitWidth = std::max(digitWidth, (int)size.cx);
  }
  TEXTMETRICW metrics{};
  GetTextMetricsW(dc, &metrics);
  int lineH = std::max(1, (int)(metrics.tmHeight + metrics.tmExternalLeading));
  int lines = 1 + (int)std::count(text.begin(), text.end(), L'\n');
  int y = rect.top +
          std::max(0, ((int)(rect.bottom - rect.top) - lines * lineH) / 2);
  int saved = SaveDC(dc);
  IntersectClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
  size_t start = 0;
  while (start <= text.size()) {
    size_t end = text.find(L'\n', start);
    if (end == std::wstring::npos)
      end = text.size();
    struct Run {
      size_t start, length;
      int width;
      bool digit;
    };
    std::vector<Run> runs;
    int total = 0;
    for (size_t i = start; i < end;) {
      if (text[i] == L'\r') {
        ++i;
        continue;
      }
      bool digit = text[i] >= L'0' && text[i] <= L'9';
      size_t next = i + 1;
      if (!digit)
        while (next < end && text[next] != L'\r' &&
               !(text[next] >= L'0' && text[next] <= L'9'))
          ++next;
      GetTextExtentPoint32W(dc, text.data() + i, (int)(next - i), &size);
      int width = digit ? digitWidth : size.cx;
      runs.push_back({i, next - i, width, digit});
      total += width;
      i = next;
    }
    int x = alignment & DT_RIGHT    ? rect.right - total
            : alignment & DT_CENTER ? (rect.left + rect.right - total) / 2
                                    : rect.left;
    for (const auto &run : runs) {
      RECT cell{x, y, x + run.width, y + lineH};
      DrawTextW(dc, text.data() + run.start, (int)run.length, &cell,
                (run.digit ? DT_CENTER : DT_LEFT) | DT_VCENTER | DT_SINGLELINE |
                    DT_NOPREFIX);
      x += run.width;
    }
    if (end == text.size())
      break;
    start = end + 1;
    y += lineH;
  }
  RestoreDC(dc, saved);
}
namespace {
LRESULT CALLBACK StableDigitsProc(HWND window, UINT message, WPARAM w, LPARAM l,
                                  UINT_PTR, DWORD_PTR) {
  if (message == WM_NCDESTROY)
    RemoveWindowSubclass(window, StableDigitsProc, 31);
  if (message != WM_PAINT)
    return DefSubclassProc(window, message, w, l);
  PAINTSTRUCT ps{};
  HDC dc = BeginPaint(window, &ps);
  RECT rect{};
  GetClientRect(window, &rect);
  BufferedPaint buffer(dc, rect);
  FillRect(buffer.DC(), &rect, Ui().bgBrush);
  HGDIOBJ old =
      SelectObject(buffer.DC(), (HFONT)SendMessageW(window, WM_GETFONT, 0, 0));
  SetBkMode(buffer.DC(), TRANSPARENT);
  SetTextColor(buffer.DC(), Ui().COLOR_TEXT);
  DrawStableDigits(buffer.DC(), GetText(window), rect, DT_RIGHT);
  SelectObject(buffer.DC(), old);
  buffer.Commit();
  EndPaint(window, &ps);
  return 0;
}
LRESULT CALLBACK BalancedTitleProc(HWND window, UINT message, WPARAM w,
                                   LPARAM l, UINT_PTR, DWORD_PTR) {
  if (message == WM_NCDESTROY)
    RemoveWindowSubclass(window, BalancedTitleProc, 32);
  if (message != WM_PAINT)
    return DefSubclassProc(window, message, w, l);
  PAINTSTRUCT ps{};
  HDC dc = BeginPaint(window, &ps);
  RECT rect{};
  GetClientRect(window, &rect);
  BufferedPaint buffer(dc, rect);
  HDC paint = buffer.DC();
  FillRect(paint, &rect, Ui().bgBrush);
  HGDIOBJ old =
      SelectObject(paint, (HFONT)SendMessageW(window, WM_GETFONT, 0, 0));
  auto text = GetText(window);
  SIZE full{};
  GetTextExtentPoint32W(paint, text.c_str(), (int)text.size(), &full);
  if (full.cx > rect.right && text.find(L' ') != std::wstring::npos) {
    int best = INT_MAX;
    size_t split = std::wstring::npos;
    for (size_t index = 0; index < text.size(); ++index)
      if (text[index] == L' ') {
        SIZE left{}, right{};
        GetTextExtentPoint32W(paint, text.data(), (int)index, &left);
        GetTextExtentPoint32W(paint, text.data() + index + 1,
                              (int)(text.size() - index - 1), &right);
        if (left.cx > rect.right || right.cx > rect.right)
          continue;
        int score =
            std::abs(left.cx - right.cx) +
            (text.find(L' ', index + 1) == std::wstring::npos ? 10000 : 0);
        if (score < best) {
          best = score;
          split = index;
        }
      }
    if (split != std::wstring::npos)
      text[split] = L'\n';
  }
  SetBkMode(paint, TRANSPARENT);
  SetTextColor(paint, GetDlgCtrlID(window) == IDC_SUBTITLE ? Ui().COLOR_MUTED
                                                           : Ui().COLOR_TEXT);
  DrawTextW(paint, text.c_str(), -1, &rect,
            DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
  SelectObject(paint, old);
  buffer.Commit();
  EndPaint(window, &ps);
  return 0;
}
} // namespace
void EnableStableDigits(HWND control) {
  if (control)
    SetWindowSubclass(control, StableDigitsProc, 31, 0);
}
void EnableBalancedTitle(HWND control) {
  if (control)
    SetWindowSubclass(control, BalancedTitleProc, 32, 0);
}
} // namespace attendance
