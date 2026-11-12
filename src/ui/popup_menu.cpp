#include "ui/popup_menu.h"
#include "animation/scheduler.h"
#include "core/constants.h"
#include "settings/state.h"
#include "ui/drawing.h"
#include "ui/layout.h"
#include "ui/motion.h"
#include "ui/state.h"

namespace attendance {

int ThemedMenuItemTop(const ThemedMenuState *state, int index);
int ThemedMenuTotalHeight(const ThemedMenuState *state);
int ThemedMenuContentTop(const ThemedMenuState *state);
int ThemedMenuContentBottom(const ThemedMenuState *state);
int ThemedMenuHitTest(ThemedMenuState *state, int y);
void ScrollThemedMenu(HWND hwnd, ThemedMenuState *state, int delta);
void QueueThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState *state,
                                 int wheelDelta);
void UpdateThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState *state);
int FindThemedMenuItem(const ThemedMenuState *state, int start, int direction);
int FirstThemedMenuItem(const ThemedMenuState *state);
int LastThemedMenuItem(const ThemedMenuState *state);
void EnsureThemedMenuItemVisible(HWND hwnd, ThemedMenuState *state, int index);
void SelectThemedMenuItem(HWND hwnd, ThemedMenuState *state, int index);
RECT ThemedMenuItemRect(HWND hwnd, const ThemedMenuState *state, int index);
void ApplyThemedMenuReveal(HWND hwnd, ThemedMenuState *state);
void PaintThemedPopupMenuContent(HWND hwnd, HDC hdc, ThemedMenuState *state);
void ReleaseThemedMenuBuffer(ThemedMenuState *state);
bool EnsureThemedMenuBuffer(HDC target, ThemedMenuState *state, int width,
                            int height);
void PaintThemedPopupMenu(HWND hwnd, HDC hdc, ThemedMenuState *state);
void CloseThemedPopupMenu(HWND hwnd, ThemedMenuState *state, int selected);
LRESULT CALLBACK ThemedMenuProc(HWND hwnd, UINT msg, WPARAM wParam,
                                LPARAM lParam);
void RegisterThemedMenuClass();
int MeasureThemedMenuWidth(const std::vector<ThemedMenuItem> &items);

int MenuRowHeight(const ThemedMenuState *state, int index) {
  if (state->items[index].separator)
    return THEMED_MENU_SEPARATOR_H;
  return index < (int)state->itemHeights.size() ? state->itemHeights[index]
                                                : THEMED_MENU_ITEM_H;
}
RECT ThemedMenuScrollThumb(const ThemedMenuState *state) {
  int trackH = std::max(1, state->height - 16);
  int height = std::clamp(
      (int)std::lround(
          trackH * (state->height / (double)std::max(1, state->contentHeight))),
      std::min(34, trackH), trackH);
  int top =
      8 + (state->maxScrollY > 0
               ? (int)std::lround((trackH - height) *
                                  (state->scrollY / (double)state->maxScrollY))
               : 0);
  return RECT{state->width - 9, top, state->width - 5, top + height};
}
int ThemedMenuItemTop(const ThemedMenuState *state, int index) {
  if (index >= 0 && index < (int)state->itemTops.size())
    return state->itemTops[index];
  int y = THEMED_MENU_PAD_Y;
  for (int i = 0; i < index; ++i) {
    y += MenuRowHeight(state, i);
  }
  return y;
}

int ThemedMenuTotalHeight(const ThemedMenuState *state) {
  int height = THEMED_MENU_PAD_Y * 2;
  for (int i = 0; i < (int)state->items.size(); ++i)
    height += MenuRowHeight(state, i);
  return height;
}

int ThemedMenuContentTop(const ThemedMenuState *state) {
  (void)state;
  return 0;
}

int ThemedMenuContentBottom(const ThemedMenuState *state) {
  if (!state)
    return 0;
  return state->height;
}

int ThemedMenuHitTest(ThemedMenuState *state, int y) {
  if (!state || y < ThemedMenuContentTop(state) ||
      y >= ThemedMenuContentBottom(state))
    return -1;
  int contentY = y - ThemedMenuContentTop(state) + state->scrollY;
  for (int i = 0; i < (int)state->items.size(); ++i) {
    int top = ThemedMenuItemTop(state, i);
    int height = MenuRowHeight(state, i);
    if (contentY >= top && contentY < top + height) {
      return state->items[i].separator ? -1 : i;
    }
  }
  return -1;
}

void ScrollThemedMenu(HWND hwnd, ThemedMenuState *state, int delta) {
  if (!hwnd || !state || !state->scrollable || delta == 0)
    return;
  int next = std::clamp(state->scrollY + delta, 0, state->maxScrollY);
  if (next == state->scrollY)
    return;
  state->scrollY = next;
  state->smoothScrollStartY = next;
  state->smoothScrollCurrentY = next;
  state->smoothScrollTargetY = next;
  state->smoothScrollStartMs = 0;
  if (!state->keyboardMode)
    state->hover = -1;
  StartAnimation(hwnd, AnimChannel::Hover, 0.0, 80);
  InvalidateRect(hwnd, nullptr, FALSE);
}

int WheelScrollLineCount() {
  UINT lines = 3;
  if (!SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0))
    return 3;
  if (lines == WHEEL_PAGESCROLL)
    return 6;
  return std::clamp((int)lines, 1, 12);
}

void QueueThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState *state,
                                 int wheelDelta) {
  if (!hwnd || !state || !state->scrollable || wheelDelta == 0)
    return;
  double pixelDelta = -(double)wheelDelta / WHEEL_DELTA *
                      WheelScrollLineCount() * THEMED_MENU_ITEM_H;
  if (!Settings().smoothScrollingEnabled ||
      Settings().animationLevel == AnimationLevel::Off) {
    int immediateDelta = (int)std::lround(pixelDelta);
    if (immediateDelta == 0)
      immediateDelta = wheelDelta < 0 ? 1 : -1;
    ScrollThemedMenu(hwnd, state, immediateDelta);
    return;
  }

  uint64_t now = AnimationNowMs();
  if (state->smoothScrollStartMs != 0 && state->smoothScrollDurationMs > 0) {
    double elapsed = (double)(now - state->smoothScrollStartMs) /
                     state->smoothScrollDurationMs;
    double eased = CubicBezierEase(elapsed, 0.22, 1.0, 0.36, 1.0);
    state->smoothScrollCurrentY =
        state->smoothScrollStartY +
        (state->smoothScrollTargetY - state->smoothScrollStartY) * eased;
  } else {
    state->smoothScrollCurrentY = state->scrollY;
  }
  state->smoothScrollStartY = state->smoothScrollCurrentY;
  state->smoothScrollTargetY = std::clamp(
      state->smoothScrollTargetY + pixelDelta, 0.0, (double)state->maxScrollY);
  if (std::abs(state->smoothScrollTargetY - state->smoothScrollStartY) < 0.5)
    return;
  double distance =
      std::abs(state->smoothScrollTargetY - state->smoothScrollStartY);
  bool precisionInput = std::abs(wheelDelta) < WHEEL_DELTA;
  state->smoothScrollDurationMs =
      precisionInput
          ? (uint32_t)std::clamp(80.0 + distance * 0.18, 90.0, 150.0)
          : (uint32_t)std::clamp(170.0 + distance * 0.16, 190.0, 260.0);
  state->smoothScrollStartMs = now;
  RestartAnimation(hwnd, AnimChannel::SmoothScroll, 1.0,
                   state->smoothScrollDurationMs);
  if (!state->keyboardMode)
    state->hover = -1;
  StartAnimation(hwnd, AnimChannel::Hover, 0.0, 80);
}

void UpdateThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState *state) {
  if (!hwnd || !state || !Settings().smoothScrollingEnabled)
    return;
  uint64_t now = AnimationNowMs();
  double progress = state->smoothScrollDurationMs == 0
                        ? 1.0
                        : (double)(now - state->smoothScrollStartMs) /
                              state->smoothScrollDurationMs;
  double eased = CubicBezierEase(progress, 0.22, 1.0, 0.36, 1.0);
  state->smoothScrollCurrentY =
      state->smoothScrollStartY +
      (state->smoothScrollTargetY - state->smoothScrollStartY) * eased;
  int next = (int)std::lround(state->smoothScrollCurrentY);
  next = std::clamp(next, 0, state->maxScrollY);
  if (next != state->scrollY) {
    state->scrollY = next;
    InvalidateRect(hwnd, nullptr, FALSE);
  }
  if (progress >= 1.0) {
    state->smoothScrollCurrentY = state->smoothScrollTargetY;
    state->scrollY = (int)std::lround(state->smoothScrollTargetY);
    state->smoothScrollStartMs = 0;
  }
}

int FindThemedMenuItem(const ThemedMenuState *state, int start, int direction) {
  if (!state || state->items.empty() || direction == 0)
    return -1;
  int index = start;
  for (int tries = 0; tries < (int)state->items.size(); ++tries) {
    index += direction;
    if (index < 0 || index >= (int)state->items.size())
      return -1;
    if (!state->items[index].separator)
      return index;
  }
  return -1;
}

int FirstThemedMenuItem(const ThemedMenuState *state) {
  return FindThemedMenuItem(state, -1, 1);
}

int LastThemedMenuItem(const ThemedMenuState *state) {
  return state ? FindThemedMenuItem(state, (int)state->items.size(), -1) : -1;
}

int UpdateThemedMenuTypeAhead(ThemedMenuState& state, wchar_t character,
                             uint64_t now) {
  if (state.items.empty()) return -1;
  character = (wchar_t)towupper(character);
  bool fresh = now - state.typeAheadMs > 1000 || state.typeAhead.size() >= 64;
  bool cycle = state.typeAhead.size() == 1 && state.typeAhead.front() == character;
  if (fresh || cycle) state.typeAhead.clear();
  state.typeAheadMs = now;
  state.typeAhead.push_back(character);
  auto find = [&](bool next) {
    int count = (int)state.items.size();
    int start = std::max(0, state.hover + (next ? 1 : 0));
    for (int attempt = 0; attempt < count; ++attempt) {
      int index = (start + attempt) % count;
      if (state.items[index].separator) continue;
      auto text = state.items[index].text;
      std::transform(text.begin(), text.end(), text.begin(),
                     [](wchar_t ch) { return (wchar_t)towupper(ch); });
      if (text.rfind(state.typeAhead, 0) == 0) return index;
    }
    return -1;
  };
  int match = find(state.typeAhead.size() == 1);
  if (match < 0 && state.typeAhead.size() > 1) {
    state.typeAhead.assign(1, character);
    match = find(true);
  }
  return match;
}

void EnsureThemedMenuItemVisible(HWND hwnd, ThemedMenuState *state, int index) {
  if (!hwnd || !state || index < 0 || index >= (int)state->items.size() ||
      !state->scrollable)
    return;
  state->smoothScrollStartY = state->scrollY;
  state->smoothScrollCurrentY = state->scrollY;
  state->smoothScrollTargetY = state->scrollY;
  state->smoothScrollStartMs = 0;
  int top = ThemedMenuItemTop(state, index);
  int bottom = top + (MenuRowHeight(state, index));
  int viewportHeight =
      std::max(1, ThemedMenuContentBottom(state) - ThemedMenuContentTop(state));
  int next = state->scrollY;
  if (top < next)
    next = top;
  else if (bottom > next + viewportHeight)
    next = bottom - viewportHeight;
  next = std::clamp(next, 0, state->maxScrollY);
  if (next != state->scrollY) {
    state->scrollY = next;
    state->smoothScrollStartY = next;
    state->smoothScrollCurrentY = next;
    state->smoothScrollTargetY = next;
    state->smoothScrollStartMs = 0;
    InvalidateRect(hwnd, nullptr, FALSE);
  }
}

void SelectThemedMenuItem(HWND hwnd, ThemedMenuState *state, int index) {
  if (!hwnd || !state || index < 0 || index >= (int)state->items.size() ||
      state->items[index].separator)
    return;
  state->keyboardMode = true;
  state->hover = index;
  EnsureThemedMenuItemVisible(hwnd, state, index);
  StartAnimation(hwnd, AnimChannel::Hover, 1.0, 90);
  InvalidateRect(hwnd, nullptr, FALSE);
}

RECT ThemedMenuItemRect(HWND hwnd, const ThemedMenuState *state, int index) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  if (!state || index < 0 || index >= (int)state->items.size() ||
      state->items[index].separator) {
    return rc;
  }
  int slideOffset = 0;
  int top = ThemedMenuItemTop(state, index) - state->scrollY +
            ThemedMenuContentTop(state) + slideOffset;
  return RECT{6, top, rc.right - 6, top + MenuRowHeight(state, index)};
}

void ApplyThemedMenuReveal(HWND hwnd, ThemedMenuState *state) {
  if (!hwnd || !state)
    return;
  double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1) *
                  (1 - GetAnimationValue(hwnd, AnimChannel::Close, 0));
  ClipPaperWindow(hwnd, reveal,
                  GetPropW(hwnd, L"Attendance.PopupUp") != nullptr);
  InvalidateRect(hwnd, nullptr, FALSE);
}

void PaintThemedPopupMenuContent(HWND hwnd, HDC hdc, ThemedMenuState *state) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);
  COLORREF fill = BlendColor(Ui().COLOR_BG, Ui().COLOR_ROW_ALT, reveal);
  COLORREF border = BlendColor(Ui().COLOR_PANEL, Ui().COLOR_BORDER, reveal);
  COLORREF hover = Ui().COLOR_HOVER;
  HBRUSH dcBrush = (HBRUSH)GetStockObject(DC_BRUSH);
  SetDCBrushColor(hdc, Ui().COLOR_BG);
  FillRect(hdc, &rc, dcBrush);
  PaintSurface(hdc, rc, fill);

  HGDIOBJ oldFont = SelectObject(hdc, Ui().font);
  SetBkMode(hdc, TRANSPARENT);

  const int contentTop = ThemedMenuContentTop(state);
  const int contentBottom = ThemedMenuContentBottom(state);
  const int slideOffset = 0;
  const double itemsProgress = GetAnimationValue(hwnd, AnimChannel::Items, 1);
  SaveDC(hdc);
  IntersectClipRect(hdc, 0, contentTop, rc.right, contentBottom);
  for (int i = 0; i < (int)state->items.size(); ++i) {
    int top =
        ThemedMenuItemTop(state, i) - state->scrollY + contentTop + slideOffset;
    int itemHeight = MenuRowHeight(state, i);
    if (top + itemHeight <= contentTop || top >= contentBottom)
      continue;
    double itemProgress = ItemEntrance(
        itemsProgress, std::max(0, i - state->scrollY / THEMED_MENU_ITEM_H));
    if (state->items[i].separator) {
      DrawSoftDivider(hdc, 12, rc.right - 12, top + 6);
      continue;
    }

    RECT itemRc{6, top, rc.right - 6, top + itemHeight};
    if (i == state->hover) {
      double hot = GetAnimationValue(hwnd, AnimChannel::Hover, 1.0);
      SetDCBrushColor(hdc, BlendColor(fill, hover, hot));
      HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(NULL_PEN));
      HGDIOBJ oldBrush = SelectObject(hdc, dcBrush);
      RoundRect(hdc, itemRc.left, itemRc.top, itemRc.right, itemRc.bottom, 12,
                12);
      SelectObject(hdc, oldBrush);
      SelectObject(hdc, oldPen);
    }

    RECT textRc = itemRc;
    textRc.left += 16;
    textRc.right -= state->scrollable ? 26 : 16;
    if (state->comboAnchor) {
      textRc.left += 20;
      if (i == state->checkedIndex) {
        HPEN tick = CreatePen(PS_SOLID, 2, Ui().COLOR_ACCENT);
        HGDIOBJ old = SelectObject(hdc, tick);
        int middle = top + itemHeight / 2;
        MoveToEx(hdc, 15, middle, nullptr);
        LineTo(hdc, 19, middle + 4);
        LineTo(hdc, 26, middle - 4);
        SelectObject(hdc, old);
        DeleteObject(tick);
      }
    }
    SetTextColor(hdc,
                 BlendColor(Ui().COLOR_MUTED, Ui().COLOR_TEXT, itemProgress));
    RECT measured{0, 0, textRc.right - textRc.left, 0};
    DrawTextW(hdc, state->items[i].text.c_str(), -1, &measured,
              DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    textRc.top += (itemHeight - measured.bottom) / 2;
    DrawTextW(hdc, state->items[i].text.c_str(), -1, &textRc,
              DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
  }
  RestoreDC(hdc, -1);

  if (state->scrollable && state->contentHeight > 0) {
    RECT track{rc.right - 9, rc.top + 8, rc.right - 5, rc.bottom - 8};
    SetDCBrushColor(hdc, Ui().COLOR_PANEL);
    FillRect(hdc, &track, dcBrush);

    RECT thumb = ThemedMenuScrollThumb(state);
    SetDCBrushColor(hdc, Ui().COLOR_ACCENT);
    FillRect(hdc, &thumb, dcBrush);
  }

  SelectObject(hdc, oldFont);

  HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));
  HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
  SetDCPenColor(hdc, border);
  // The rounded surface and shadow carry the popup boundary.
  SelectObject(hdc, oldBrush);
  SelectObject(hdc, oldPen);
}

void ReleaseThemedMenuBuffer(ThemedMenuState *state) {
  if (!state)
    return;
  if (state->bufferDc && state->bufferOldBitmap) {
    SelectObject(state->bufferDc, state->bufferOldBitmap);
  }
  if (state->bufferBitmap)
    DeleteObject(state->bufferBitmap);
  if (state->bufferDc)
    DeleteDC(state->bufferDc);
  state->bufferDc = nullptr;
  state->bufferBitmap = nullptr;
  state->bufferOldBitmap = nullptr;
  state->bufferWidth = 0;
  state->bufferHeight = 0;
}

bool EnsureThemedMenuBuffer(HDC target, ThemedMenuState *state, int width,
                            int height) {
  if (!state || !target || width <= 0 || height <= 0)
    return false;
  if (state->bufferDc && state->bufferBitmap && state->bufferWidth == width &&
      state->bufferHeight == height) {
    return true;
  }
  ReleaseThemedMenuBuffer(state);
  state->bufferDc = CreateCompatibleDC(target);
  if (!state->bufferDc)
    return false;
  state->bufferBitmap = CreateCompatibleBitmap(target, width, height);
  if (!state->bufferBitmap) {
    ReleaseThemedMenuBuffer(state);
    return false;
  }
  state->bufferOldBitmap = SelectObject(state->bufferDc, state->bufferBitmap);
  state->bufferWidth = width;
  state->bufferHeight = height;
  return true;
}

void PaintThemedPopupMenu(HWND hwnd, HDC hdc, ThemedMenuState *state) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  int width = std::max(1, (int)(rc.right - rc.left));
  int height = std::max(1, (int)(rc.bottom - rc.top));

  if (!EnsureThemedMenuBuffer(hdc, state, width, height)) {
    PaintThemedPopupMenuContent(hwnd, hdc, state);
    return;
  }
  PaintThemedPopupMenuContent(hwnd, state->bufferDc, state);
  BitBlt(hdc, 0, 0, width, height, state->bufferDc, 0, 0, SRCCOPY);
}

void CloseThemedPopupMenu(HWND hwnd, ThemedMenuState *state, int selected) {
  if (!state || state->closing)
    return;
  state->selected = selected;
  state->closing = true;
  if (state->comboAnchor && IsWindow(state->comboAnchor))
    StartAnimation(state->comboAnchor, AnimChannel::ComboOpen, 0,
                   Visual::CloseMs);
  ReleaseCapture();
  BeginAnimatedClose(hwnd, Visual::CloseMs);
}

LRESULT CALLBACK ThemedMenuProc(HWND hwnd, UINT msg, WPARAM wParam,
                                LPARAM lParam) {
  auto *state = reinterpret_cast<ThemedMenuState *>(
      GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  switch (msg) {
  case WM_NCCREATE: {
    auto *create = reinterpret_cast<CREATESTRUCTW *>(lParam);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    return TRUE;
  }
  case WM_PAINT: {
    PAINTSTRUCT ps{};
    HDC hdc = BeginPaint(hwnd, &ps);
    if (state)
      PaintThemedPopupMenu(hwnd, hdc, state);
    EndPaint(hwnd, &ps);
    return 0;
  }
  case WM_ERASEBKGND:
    return 1;
  case WM_SIZE:
    ReleaseThemedMenuBuffer(state);
    InvalidateRect(hwnd, nullptr, FALSE);
    return 0;
  case WM_MOUSEMOVE:
    if (state && !state->closing) {
      POINT mouse{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      auto drag = (INT_PTR)GetPropW(hwnd, L"Attendance.MenuScrollDrag");
      if (drag) {
        RECT thumb = ThemedMenuScrollThumb(state);
        int travel =
            std::max(1, state->height - 16 - (int)(thumb.bottom - thumb.top));
        double fraction =
            std::clamp((mouse.y - 8 - (drag - 1)) / (double)travel, 0.0, 1.0);
        int next = (int)std::lround(fraction * state->maxScrollY);
        ScrollThemedMenu(hwnd, state, next - state->scrollY);
        return 0;
      }
      bool actuallyMoved = !state->hasMousePoint ||
                           mouse.x != state->lastMouse.x ||
                           mouse.y != state->lastMouse.y;
      state->lastMouse = mouse;
      state->hasMousePoint = true;
      if (state->keyboardMode && !actuallyMoved)
        return 0;
      state->keyboardMode = false;
      RECT viewport =
          PaperViewport(state->width, state->height,
                        GetAnimationValue(hwnd, AnimChannel::Reveal, 1),
                        GetPropW(hwnd, L"Attendance.PopupUp") != nullptr);
      int next = PtInRect(&viewport, mouse) &&
                         (!state->scrollable || mouse.x < state->width - 16)
                     ? ThemedMenuHitTest(state, mouse.y)
                     : -1;
      if (next != state->hover) {
        RECT oldRc = ThemedMenuItemRect(hwnd, state, state->hover);
        state->hover = next;
        StartAnimation(hwnd, AnimChannel::Hover, next >= 0 ? 1.0 : 0.0, 110);
        RECT newRc = ThemedMenuItemRect(hwnd, state, state->hover);
        InvalidateRect(hwnd, &oldRc, FALSE);
        InvalidateRect(hwnd, &newRc, FALSE);
      }
    }
    return 0;
  case WM_MOUSEWHEEL:
    if (state && !state->closing && state->scrollable) {
      state->keyboardMode = false;
      state->hover = -1;
      QueueThemedMenuSmoothScroll(hwnd, state, GET_WHEEL_DELTA_WPARAM(wParam));
      return 0;
    }
    break;
  case WM_LBUTTONDOWN:
    if (state && !state->closing) {
      SetPropW(hwnd, L"Attendance.MenuPressed", (HANDLE)1);
      state->smoothScrollStartY = state->scrollY;
      state->smoothScrollCurrentY = state->scrollY;
      state->smoothScrollTargetY = state->scrollY;
      state->smoothScrollStartMs = 0;
      POINT pt{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      RECT rc{};
      GetClientRect(hwnd, &rc);
      if (!PtInRect(&rc, pt)) {
        CloseThemedPopupMenu(hwnd, state, 0);
        return 0;
      }
      if (state->scrollable && pt.x >= rc.right - 16) {
        SetPropW(hwnd, L"Attendance.MenuScrollPressed", (HANDLE)1);
        state->keyboardMode = false;
        state->hover = -1;
        RECT thumb = ThemedMenuScrollThumb(state);
        if (pt.y >= thumb.top && pt.y < thumb.bottom)
          SetPropW(hwnd, L"Attendance.MenuScrollDrag",
                   (HANDLE)(INT_PTR)(pt.y - thumb.top + 1));
        else
          ScrollThemedMenu(hwnd, state,
                           pt.y < thumb.top ? -state->height : state->height);
        return 0;
      }
    }
    return 0;
  case WM_LBUTTONUP:
    if (state && !state->closing) {
      if (RemovePropW(hwnd, L"Attendance.MenuScrollPressed")) {
        RemovePropW(hwnd, L"Attendance.MenuScrollDrag");
        RemovePropW(hwnd, L"Attendance.MenuPressed");
        return 0;
      }
      if (!RemovePropW(hwnd, L"Attendance.MenuPressed"))
        return 0;
      POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      if (state->scrollable && point.x >= state->width - 16)
        return 0;
      RECT viewport =
          PaperViewport(state->width, state->height,
                        GetAnimationValue(hwnd, AnimChannel::Reveal, 1),
                        GetPropW(hwnd, L"Attendance.PopupUp") != nullptr);
      int hit =
          PtInRect(&viewport, point) ? ThemedMenuHitTest(state, point.y) : -1;
      CloseThemedPopupMenu(hwnd, state,
                           hit >= 0 ? state->items[hit].command : 0);
    }
    return 0;
  case WM_KEYDOWN:
    if (wParam == VK_ESCAPE && state && !state->closing) {
      CloseThemedPopupMenu(hwnd, state, 0);
      return 0;
    }
    if (state && !state->closing) {
      if (state->comboAnchor && (wParam == VK_F4 || wParam == VK_TAB)) {
        CloseThemedPopupMenu(hwnd, state, 0);
        if (wParam == VK_TAB) {
          HWND owner = GetAncestor(state->comboAnchor, GA_ROOT);
          HWND next = GetNextDlgTabItem(owner, state->comboAnchor,
                                        (GetKeyState(VK_SHIFT) & 0x8000) != 0);
          if (next) {
            SetPropW(state->comboAnchor, L"Attendance.ComboFocusMoved",
                     (HANDLE)1);
            SetFocus(next);
          }
        }
        return 0;
      }
      if (wParam == VK_RETURN || (wParam == VK_SPACE && !state->comboAnchor)) {
        if (state->hover >= 0 && state->hover < (int)state->items.size() &&
            !state->items[state->hover].separator) {
          CloseThemedPopupMenu(hwnd, state, state->items[state->hover].command);
        }
        return 0;
      }
      if (wParam == VK_HOME) {
        SelectThemedMenuItem(hwnd, state, FirstThemedMenuItem(state));
        return 0;
      }
      if (wParam == VK_END) {
        SelectThemedMenuItem(hwnd, state, LastThemedMenuItem(state));
        return 0;
      }
      if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_PRIOR ||
          wParam == VK_NEXT) {
        int direction = (wParam == VK_UP || wParam == VK_PRIOR) ? -1 : 1;
        int steps = (wParam == VK_PRIOR || wParam == VK_NEXT) ? 6 : 1;
        int index = state->hover;
        if (index < 0) {
          index = direction > 0 ? FirstThemedMenuItem(state)
                                : LastThemedMenuItem(state);
        } else {
          for (int i = 0; i < steps; ++i) {
            int next = FindThemedMenuItem(state, index, direction);
            if (next < 0)
              break;
            index = next;
          }
        }
        SelectThemedMenuItem(hwnd, state, index);
        return 0;
      }
    }
    break;
  case WM_SYSKEYDOWN:
    if (state && state->comboAnchor &&
        (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_F4)) {
      CloseThemedPopupMenu(hwnd, state, 0);
      return 0;
    }
    break;
  case WM_CHAR:
    if (state && state->comboAnchor && !state->closing && wParam >= 32) {
      int match = UpdateThemedMenuTypeAhead(*state, (wchar_t)wParam, AnimationNowMs());
      if (match >= 0) SelectThemedMenuItem(hwnd, state, match);
      return 0;
    }
    break;
  case WM_CANCELMODE:
    CloseThemedPopupMenu(hwnd, state, 0);
    return 0;
  case WM_ACTIVATE:
    if (LOWORD(wParam) == WA_INACTIVE && state && !state->done &&
        !state->closing) {
      CloseThemedPopupMenu(hwnd, state, 0);
      return 0;
    }
    break;
  case WM_CAPTURECHANGED:
    if (state && !state->done && !state->closing && (HWND)lParam != hwnd) {
      CloseThemedPopupMenu(hwnd, state, 0);
      return 0;
    }
    break;
  case WM_APP_ANIMATION_TICK:
    AcknowledgeAnimationTick(hwnd);
    if (state) {
      if (!state->closing)
        UpdateThemedMenuSmoothScroll(hwnd, state);
      ApplyThemedMenuReveal(hwnd, state);
    }
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
    return 0;
  case WM_APP_ANIMATION_DONE:
    DestroyWindow(hwnd);
    return 0;
  case WM_DESTROY:
    ReleaseCapture();
    if (state) {
      ReleaseThemedMenuBuffer(state);
      state->done = true;
    }
    RemoveAnimationState(hwnd);
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void RegisterThemedMenuClass() {
  static bool registered = false;
  if (registered)
    return;

  WNDCLASSW wc{};
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DROPSHADOW;
  wc.lpfnWndProc = ThemedMenuProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.lpszClassName = L"AttendanceThemedPopupMenu";
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  RegisterClassW(&wc);
  registered = true;
}

int MeasureThemedMenuWidth(const std::vector<ThemedMenuItem> &items) {
  HDC hdc = GetDC(Ui().hwnd);
  HGDIOBJ oldFont = SelectObject(hdc, Ui().font);
  int width = 180;
  for (const auto &item : items) {
    if (item.separator)
      continue;
    SIZE size{};
    GetTextExtentPoint32W(hdc, item.text.c_str(), (int)item.text.size(), &size);
    width = std::max(width, (int)size.cx + 42);
  }
  SelectObject(hdc, oldFont);
  ReleaseDC(Ui().hwnd, hdc);
  return width;
}

int ShowThemedPopupMenu(HWND button, const std::vector<ThemedMenuItem> &items,
                        const ThemedMenuOptions &options) {
  if (!button || items.empty())
    return 0;
  ThemedMenuState state;
  state.items = items;
  state.comboAnchor = options.comboChoice ? button : nullptr;
  state.checkedIndex = options.selectedIndex;
  state.hover = options.selectedIndex;
  state.keyboardMode = options.selectedIndex >= 0;
  state.mainScrollX = Ui().scrollX;
  state.mainScrollY = Ui().scrollY;
  ++Ui().openThemedPopupMenus;
  RegisterThemedMenuClass();

  RECT buttonRc{};
  GetWindowRect(button, &buttonRc);
  MONITORINFO mi{sizeof(mi)};
  bool haveWorkArea = GetMonitorInfoW(
      MonitorFromRect(&buttonRc, MONITOR_DEFAULTTONEAREST), &mi) != FALSE;
  int width = MeasureThemedMenuWidth(items);
  if (options.comboChoice)
    width = std::max(width + 20, (int)(buttonRc.right - buttonRc.left));
  if (haveWorkArea)
    width = std::min(width, std::max(1, (int)(mi.rcWork.right - mi.rcWork.left) - 8));
  HDC measureDc = GetDC(button);
  HGDIOBJ oldFont = SelectObject(measureDc, Ui().font);
  for (const auto &item : items) {
    // Reserve the checkmark, both margins and the scrollbar before measuring.
    RECT text{0, 0, std::max(1, width - (options.comboChoice ? 76 : 56)), 0};
    DrawTextW(measureDc, item.text.c_str(), -1, &text,
              DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
    state.itemHeights.push_back(
        std::max(Settings().layoutDensity == LayoutDensity::Compact ? 36 : 40,
                 (int)text.bottom + 16));
  }
  SelectObject(measureDc, oldFont);
  ReleaseDC(button, measureDc);
  int rowTop = THEMED_MENU_PAD_Y;
  for (int index = 0; index < (int)items.size(); ++index) {
    state.itemTops.push_back(rowTop);
    rowTop += MenuRowHeight(&state, index);
  }
  state.contentHeight = ThemedMenuTotalHeight(&state);
  int height = state.contentHeight;
  int x = buttonRc.right - width;
  if (options.comboChoice)
    x = buttonRc.left;
  int y = buttonRc.bottom + 4;

  if (haveWorkArea) {
    int workHeight = (int)(mi.rcWork.bottom - mi.rcWork.top);
    int maxHeight = std::max(1, std::min(THEMED_MENU_MAX_H, workHeight - 8));
    if (height > maxHeight) {
      state.scrollable = true;
      height = maxHeight;
      state.maxScrollY = std::max(0, state.contentHeight - height);
    }
    int minX = (int)mi.rcWork.left + 4;
    int maxX = std::max(minX, (int)mi.rcWork.right - width - 4);
    int minY = (int)mi.rcWork.top + 4;
    int maxY = std::max(minY, (int)mi.rcWork.bottom - height - 4);
    x = std::clamp(x, minX, maxX);
    if (y + height > (int)mi.rcWork.bottom)
      y = (int)buttonRc.top - height - 4;
    y = std::clamp(y, minY, maxY);
  }
  state.finalX = x;
  state.finalY = y;
  state.width = width;
  state.height = height;

  HWND popup =
      CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                      L"AttendanceThemedPopupMenu", L"", WS_POPUP, state.finalX,
                      state.finalY, width, height, GetAncestor(button, GA_ROOT),
                      nullptr, GetModuleHandleW(nullptr), &state);
  if (!popup) {
    --Ui().openThemedPopupMenus;
    return 0;
  }

  if (y < buttonRc.top)
    SetPropW(popup, L"Attendance.PopupUp", (HANDLE)1);
  StartWindowReveal(popup, Visual::OpenMs);
  if (options.comboChoice) {
    SetPropW(button, L"Attendance.ComboPopup", popup);
    EnsureThemedMenuItemVisible(popup, &state, options.selectedIndex);
  }
  ShowWindow(popup, SW_SHOWNOACTIVATE);
  ApplyThemedMenuReveal(popup, &state);
  SetCapture(popup);
  SetFocus(popup);

  MSG msg{};
  while (!state.done && IsWindow(popup)) {
    int messageResult = GetMessageW(&msg, nullptr, 0, 0);
    if (messageResult == 0) {
      PostQuitMessage((int)msg.wParam);
      break;
    }
    if (messageResult == -1)
      break;
    if ((msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN ||
         msg.message == WM_MOUSEWHEEL || msg.message == WM_CHAR) &&
        msg.hwnd != popup) {
      if (state.comboAnchor && msg.message == WM_KEYDOWN &&
          msg.wParam != VK_RETURN)
        TranslateMessage(&msg);
      SendMessageW(popup, msg.message, msg.wParam, msg.lParam);
      continue;
    }
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  --Ui().openThemedPopupMenus;
  if (IsWindow(popup))
    DestroyWindow(popup);
  if (options.comboChoice && IsWindow(button)) {
    RemovePropW(button, L"Attendance.ComboPopup");
    StartAnimation(button, AnimChannel::ComboOpen, 0, Visual::CloseMs);
  }
  if (!options.comboChoice && IsWindow(button) &&
      GetActiveWindow() == GetAncestor(button, GA_ROOT)) SetFocus(button);
  InvalidateRect(button, nullptr, FALSE);
  if (Ui().scrollX != state.mainScrollX || Ui().scrollY != state.mainScrollY) {
    Ui().scrollX = state.mainScrollX;
    Ui().scrollY = state.mainScrollY;
    ResizeLayout(Ui().hwnd);
    InvalidateRect(Ui().hwnd, nullptr, FALSE);
  }
  return state.selected;
}

} // namespace attendance
