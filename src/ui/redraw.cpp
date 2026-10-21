#include "ui/redraw.h"

namespace attendance {
namespace {
constexpr wchar_t RedrawDepth[] = L"AttendanceApp.RedrawDepth";
struct PaintThread {
  HRESULT result = BufferedPaintInit();
  ~PaintThread() {
    if (SUCCEEDED(result))
      BufferedPaintUnInit();
  }
};
} // namespace

RedrawScope::RedrawScope(HWND window) {
  if (!window || !IsWindow(window))
    return;
  auto depth = reinterpret_cast<UINT_PTR>(GetPropW(window, RedrawDepth));
  if (!depth && !(GetWindowLongPtrW(window, GWL_STYLE) & WS_VISIBLE))
    return;
  if (!SetPropW(window, RedrawDepth, reinterpret_cast<HANDLE>(depth + 1)))
    return;
  window_ = window;
  if (!depth)
    SendMessageW(window_, WM_SETREDRAW, FALSE, 0);
}

RedrawScope::~RedrawScope() {
  if (!window_ || !IsWindow(window_))
    return;
  auto depth = reinterpret_cast<UINT_PTR>(GetPropW(window_, RedrawDepth));
  if (depth > 1) {
    SetPropW(window_, RedrawDepth, reinterpret_cast<HANDLE>(depth - 1));
    return;
  }
  if (depth == 1) {
    RemovePropW(window_, RedrawDepth);
    SendMessageW(window_, WM_SETREDRAW, TRUE, 0);
  }
  RedrawWindow(window_, nullptr, nullptr,
               RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE);
}

BufferedPaint::BufferedPaint(HDC target, const RECT &rect) : dc_(target) {
  static thread_local PaintThread thread;
  if (!target || FAILED(thread.result) || IsRectEmpty(&rect))
    return;
  HDC bufferDc = nullptr;
  buffer_ = BeginBufferedPaint(target, &rect, BPBF_COMPATIBLEBITMAP, nullptr,
                               &bufferDc);
  if (buffer_)
    dc_ = bufferDc;
}
BufferedPaint::~BufferedPaint() { Commit(); }
void BufferedPaint::Commit() {
  if (buffer_) {
    EndBufferedPaint(buffer_, TRUE);
    buffer_ = nullptr;
  }
}
} // namespace attendance
