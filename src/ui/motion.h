#pragma once
#include "core/types.h"

namespace attendance {
namespace Visual {
inline constexpr int ControlRadius = 8, PopupRadius = 12;
inline constexpr int Space[] = {4, 8, 12, 16, 24, 32};
inline constexpr uint32_t OpenMs = 240, CloseMs = 140;
} // namespace Visual
double ItemEntrance(double progress, int visibleIndex);
RECT PaperViewport(int width, int height, double progress, bool upward);
void ClipPaperWindow(HWND window, double progress, bool upward);
void PaintSurface(HDC dc, RECT rect, COLORREF fill,
                  int radius = Visual::PopupRadius);
void PaintRoundedControl(HDC dc, RECT rect, COLORREF fill, COLORREF border,
                         int radius = Visual::ControlRadius);
int MeasuredTextHeight(HWND control);
int MeasuredTextWidth(HWND control);
int WrappedTextHeight(HWND control, int width);
SIZE BoundedWindowSize(HWND owner, int clientWidth, int clientHeight,
                       DWORD style, DWORD exStyle = 0);
void DrawStableDigits(HDC dc, const std::wstring &text, RECT rect,
                      UINT alignment);
void EnableStableDigits(HWND control);
void EnableBalancedTitle(HWND control);
} // namespace attendance
