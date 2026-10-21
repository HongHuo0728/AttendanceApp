#pragma once
#include "core/types.h"

namespace attendance {
// UI-thread scope. Nested callers never re-enable an outer redraw transaction.
// A hidden window must never receive WM_SETREDRAW(TRUE): it makes it visible.
class RedrawScope {
public:
  explicit RedrawScope(HWND window);
  ~RedrawScope();
  RedrawScope(const RedrawScope &) = delete;
  RedrawScope &operator=(const RedrawScope &) = delete;

private:
  HWND window_ = nullptr;
};

// Paint background, border, effects and text before presenting a control frame.
class BufferedPaint {
public:
  BufferedPaint(HDC target, const RECT &rect);
  ~BufferedPaint();
  HDC DC() const { return dc_; }
  void Commit();
  BufferedPaint(const BufferedPaint &) = delete;
  BufferedPaint &operator=(const BufferedPaint &) = delete;

private:
  HPAINTBUFFER buffer_ = nullptr;
  HDC dc_ = nullptr;
};
} // namespace attendance
