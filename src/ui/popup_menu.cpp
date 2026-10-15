#include "ui/popup_menu.h"
#include "animation/scheduler.h"
#include "ui/drawing.h"
#include "ui/layout.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

int ThemedMenuItemTop(const ThemedMenuState* state, int index);
int ThemedMenuTotalHeight(const ThemedMenuState* state);
int ThemedMenuContentTop(const ThemedMenuState* state);
int ThemedMenuContentBottom(const ThemedMenuState* state);
int ThemedMenuHitTest(ThemedMenuState* state, int y);
void ScrollThemedMenu(HWND hwnd, ThemedMenuState* state, int delta);
void QueueThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState* state, int wheelDelta);
void UpdateThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState* state);
int FindThemedMenuItem(const ThemedMenuState* state, int start, int direction);
int FirstThemedMenuItem(const ThemedMenuState* state);
int LastThemedMenuItem(const ThemedMenuState* state);
void EnsureThemedMenuItemVisible(HWND hwnd, ThemedMenuState* state, int index);
void SelectThemedMenuItem(HWND hwnd, ThemedMenuState* state, int index);
RECT ThemedMenuItemRect(HWND hwnd, const ThemedMenuState* state, int index);
void ApplyThemedMenuReveal(HWND hwnd, ThemedMenuState* state);
void PaintThemedPopupMenuContent(HWND hwnd, HDC hdc, ThemedMenuState* state);
void ReleaseThemedMenuBuffer(ThemedMenuState* state);
bool EnsureThemedMenuBuffer(HDC target, ThemedMenuState* state, int width, int height);
void PaintThemedPopupMenu(HWND hwnd, HDC hdc, ThemedMenuState* state);
void CloseThemedPopupMenu(HWND hwnd, ThemedMenuState* state, int selected);
LRESULT CALLBACK ThemedMenuProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void RegisterThemedMenuClass();
int MeasureThemedMenuWidth(const std::vector<ThemedMenuItem>& items);

int ThemedMenuItemTop(const ThemedMenuState* state, int index){
    int y = THEMED_MENU_PAD_Y;
    for (int i = 0; i < index; ++i) {
        y += state->items[i].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
    }
    return y;
}

int ThemedMenuTotalHeight(const ThemedMenuState* state){
    int height = THEMED_MENU_PAD_Y * 2;
    for (const auto& item : state->items) {
        height += item.separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
    }
    return height;
}

int ThemedMenuContentTop(const ThemedMenuState* state){
    (void)state;
    return 0;
}

int ThemedMenuContentBottom(const ThemedMenuState* state){
    if (!state) return 0;
    return state->height;
}

int ThemedMenuHitTest(ThemedMenuState* state, int y){
    if (!state || y < ThemedMenuContentTop(state) || y >= ThemedMenuContentBottom(state)) return -1;
    int contentY = y - ThemedMenuContentTop(state) + state->scrollY;
    for (int i = 0; i < (int)state->items.size(); ++i) {
        int top = ThemedMenuItemTop(state, i);
        int height = state->items[i].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
        if (contentY >= top && contentY < top + height) {
            return state->items[i].separator ? -1 : i;
        }
    }
    return -1;
}

void ScrollThemedMenu(HWND hwnd, ThemedMenuState* state, int delta){
    if (!hwnd || !state || !state->scrollable || delta == 0) return;
    int next = std::clamp(state->scrollY + delta, 0, state->maxScrollY);
    if (next == state->scrollY) return;
    state->scrollY = next;
    state->smoothScrollStartY = next;
    state->smoothScrollCurrentY = next;
    state->smoothScrollTargetY = next;
    state->smoothScrollStartMs = 0;
    if (!state->keyboardMode) state->hover = -1;
    StartAnimation(hwnd, AnimChannel::Hover, 0.0, 80);
    InvalidateRect(hwnd, nullptr, FALSE);
}

int WheelScrollLineCount(){
    UINT lines = 3;
    if (!SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines, 0)) return 3;
    if (lines == WHEEL_PAGESCROLL) return 6;
    return std::clamp((int)lines, 1, 12);
}

void QueueThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState* state, int wheelDelta){
    if (!hwnd || !state || !state->scrollable || wheelDelta == 0) return;
    double pixelDelta = -(double)wheelDelta / WHEEL_DELTA
        * WheelScrollLineCount() * THEMED_MENU_ITEM_H;
    if (!Settings().smoothScrollingEnabled) {
        int immediateDelta = (int)std::lround(pixelDelta);
        if (immediateDelta == 0) immediateDelta = wheelDelta < 0 ? 1 : -1;
        ScrollThemedMenu(hwnd, state, immediateDelta);
        return;
    }

    uint64_t now = AnimationNowMs();
    if (state->smoothScrollStartMs != 0 && state->smoothScrollDurationMs > 0) {
        double elapsed = (double)(now - state->smoothScrollStartMs) / state->smoothScrollDurationMs;
        double eased = CubicBezierEase(elapsed, 0.22, 1.0, 0.36, 1.0);
        state->smoothScrollCurrentY = state->smoothScrollStartY
            + (state->smoothScrollTargetY - state->smoothScrollStartY) * eased;
    } else {
        state->smoothScrollCurrentY = state->scrollY;
    }
    state->smoothScrollStartY = state->smoothScrollCurrentY;
    state->smoothScrollTargetY = std::clamp(
        state->smoothScrollTargetY + pixelDelta, 0.0, (double)state->maxScrollY);
    if (std::abs(state->smoothScrollTargetY - state->smoothScrollStartY) < 0.5) return;
    double distance = std::abs(state->smoothScrollTargetY - state->smoothScrollStartY);
    bool precisionInput = std::abs(wheelDelta) < WHEEL_DELTA;
    state->smoothScrollDurationMs = precisionInput
        ? (uint32_t)std::clamp(80.0 + distance * 0.18, 90.0, 150.0)
        : (uint32_t)std::clamp(170.0 + distance * 0.16, 190.0, 260.0);
    state->smoothScrollStartMs = now;
    RestartAnimation(hwnd, AnimChannel::SmoothScroll, 1.0, state->smoothScrollDurationMs);
    if (!state->keyboardMode) state->hover = -1;
    StartAnimation(hwnd, AnimChannel::Hover, 0.0, 80);
}

void UpdateThemedMenuSmoothScroll(HWND hwnd, ThemedMenuState* state){
    if (!hwnd || !state || !Settings().smoothScrollingEnabled) return;
    uint64_t now = AnimationNowMs();
    double progress = state->smoothScrollDurationMs == 0
        ? 1.0
        : (double)(now - state->smoothScrollStartMs) / state->smoothScrollDurationMs;
    double eased = CubicBezierEase(progress, 0.22, 1.0, 0.36, 1.0);
    state->smoothScrollCurrentY = state->smoothScrollStartY
        + (state->smoothScrollTargetY - state->smoothScrollStartY) * eased;
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

int FindThemedMenuItem(const ThemedMenuState* state, int start, int direction){
    if (!state || state->items.empty() || direction == 0) return -1;
    int index = start;
    for (int tries = 0; tries < (int)state->items.size(); ++tries) {
        index += direction;
        if (index < 0 || index >= (int)state->items.size()) return -1;
        if (!state->items[index].separator) return index;
    }
    return -1;
}

int FirstThemedMenuItem(const ThemedMenuState* state){
    return FindThemedMenuItem(state, -1, 1);
}

int LastThemedMenuItem(const ThemedMenuState* state){
    return state ? FindThemedMenuItem(state, (int)state->items.size(), -1) : -1;
}

void EnsureThemedMenuItemVisible(HWND hwnd, ThemedMenuState* state, int index){
    if (!hwnd || !state || index < 0 || index >= (int)state->items.size() || !state->scrollable) return;
    state->smoothScrollStartY = state->scrollY;
    state->smoothScrollCurrentY = state->scrollY;
    state->smoothScrollTargetY = state->scrollY;
    state->smoothScrollStartMs = 0;
    int top = ThemedMenuItemTop(state, index);
    int bottom = top + (state->items[index].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H);
    int viewportHeight = std::max(1, ThemedMenuContentBottom(state) - ThemedMenuContentTop(state));
    int next = state->scrollY;
    if (top < next) next = top;
    else if (bottom > next + viewportHeight) next = bottom - viewportHeight;
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

void SelectThemedMenuItem(HWND hwnd, ThemedMenuState* state, int index){
    if (!hwnd || !state || index < 0 || index >= (int)state->items.size() || state->items[index].separator) return;
    state->keyboardMode = true;
    state->hover = index;
    EnsureThemedMenuItemVisible(hwnd, state, index);
    StartAnimation(hwnd, AnimChannel::Hover, 1.0, 90);
    InvalidateRect(hwnd, nullptr, FALSE);
}

RECT ThemedMenuItemRect(HWND hwnd, const ThemedMenuState* state, int index){
    RECT rc{};
    GetClientRect(hwnd, &rc);
    if (!state || index < 0 || index >= (int)state->items.size() || state->items[index].separator) {
        return rc;
    }
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);
    int slideOffset = -(int)std::lround((1.0 - reveal) * THEMED_MENU_SLIDE_PX);
    int top = ThemedMenuItemTop(state, index) - state->scrollY + ThemedMenuContentTop(state) + slideOffset;
    return RECT{6, top, rc.right - 6, top + THEMED_MENU_ITEM_H};
}

void ApplyThemedMenuReveal(HWND hwnd, ThemedMenuState* state){
    if (!hwnd || !state) return;
    InvalidateRect(hwnd, nullptr, FALSE);
}

void PaintThemedPopupMenuContent(HWND hwnd, HDC hdc, ThemedMenuState* state){
    RECT rc{};
    GetClientRect(hwnd, &rc);
    double reveal = GetAnimationValue(hwnd, AnimChannel::Reveal, 1.0);
    COLORREF fill = BlendColor(Ui().COLOR_BG, Ui().COLOR_ROW_ALT, reveal);
    COLORREF border = BlendColor(Ui().COLOR_PANEL, Ui().COLOR_BORDER, reveal);
    COLORREF hover = Ui().COLOR_HOVER;
    int blurRadius = (int)std::lround(reveal * 10.0);

    HBRUSH dcBrush = (HBRUSH)GetStockObject(DC_BRUSH);
    SetDCBrushColor(hdc, fill);
    FillRect(hdc, &rc, dcBrush);

    HGDIOBJ oldGlowPen = SelectObject(hdc, GetStockObject(DC_PEN));
    HGDIOBJ oldGlowBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    for (int i = 0; i < blurRadius; i += 3) {
        RECT glow{rc.left + i, rc.top + i, rc.right - i, rc.bottom - i};
        SetDCPenColor(hdc,
            BlendColor(Ui().COLOR_BG, Ui().COLOR_SELECTED, reveal * (1.0 - i / 12.0)));
        Rectangle(hdc, glow.left, glow.top, glow.right, glow.bottom);
    }
    SelectObject(hdc, oldGlowBrush);
    SelectObject(hdc, oldGlowPen);

    HGDIOBJ oldFont = SelectObject(hdc, Ui().font);
    SetBkMode(hdc, TRANSPARENT);

    const int contentTop = ThemedMenuContentTop(state);
    const int contentBottom = ThemedMenuContentBottom(state);
    const int slideOffset = -(int)std::lround((1.0 - reveal) * THEMED_MENU_SLIDE_PX);
    SaveDC(hdc);
    IntersectClipRect(hdc, 0, contentTop, rc.right, contentBottom);
    for (int i = 0; i < (int)state->items.size(); ++i) {
        double itemProgress = EaseOut(reveal);
        int top = ThemedMenuItemTop(state, i) - state->scrollY + contentTop + slideOffset;
        int itemHeight = state->items[i].separator ? THEMED_MENU_SEPARATOR_H : THEMED_MENU_ITEM_H;
        if (top + itemHeight <= contentTop || top >= contentBottom) continue;
        if (state->items[i].separator) {
            DrawSoftDivider(hdc, 12, rc.right - 12, top + 6);
            continue;
        }

        RECT itemRc{0, top, rc.right, top + THEMED_MENU_ITEM_H};
        if (i == state->hover) {
            double hot = GetAnimationValue(hwnd, AnimChannel::Hover, 1.0);
            SetDCBrushColor(hdc, BlendColor(fill, hover, hot));
            FillRect(hdc, &itemRc, dcBrush);
        }

        RECT textRc = itemRc;
        textRc.left += 16;
        textRc.right -= state->scrollable ? 26 : 16;
        SetTextColor(hdc, BlendColor(Ui().COLOR_MUTED, Ui().COLOR_TEXT, itemProgress));
        DrawTextW(hdc, state->items[i].text.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    RestoreDC(hdc, -1);

    if (state->scrollable && state->contentHeight > 0) {
        RECT track{rc.right - 9, rc.top + 8, rc.right - 5, rc.bottom - 8};
        SetDCBrushColor(hdc, Ui().COLOR_PANEL);
        FillRect(hdc, &track, dcBrush);

        int trackHeight = std::max(1, (int)(track.bottom - track.top));
        int thumbHeight = std::clamp(
            (int)std::lround(trackHeight * (state->height / (double)state->contentHeight)),
            34,
            trackHeight);
        int thumbTravel = std::max(0, trackHeight - thumbHeight);
        int thumbTop = track.top + (state->maxScrollY > 0
            ? (int)std::lround(thumbTravel * (state->scrollY / (double)state->maxScrollY))
            : 0);
        RECT thumb{track.left, thumbTop, track.right, thumbTop + thumbHeight};
        SetDCBrushColor(hdc, Ui().COLOR_ACCENT);
        FillRect(hdc, &thumb, dcBrush);
    }

    SelectObject(hdc, oldFont);

    HGDIOBJ oldPen = SelectObject(hdc, GetStockObject(DC_PEN));
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    SetDCPenColor(hdc, border);
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
}

void ReleaseThemedMenuBuffer(ThemedMenuState* state){
    if (!state) return;
    if (state->bufferDc && state->bufferOldBitmap) {
        SelectObject(state->bufferDc, state->bufferOldBitmap);
    }
    if (state->bufferBitmap) DeleteObject(state->bufferBitmap);
    if (state->bufferDc) DeleteDC(state->bufferDc);
    state->bufferDc = nullptr;
    state->bufferBitmap = nullptr;
    state->bufferOldBitmap = nullptr;
    state->bufferWidth = 0;
    state->bufferHeight = 0;
}

bool EnsureThemedMenuBuffer(HDC target, ThemedMenuState* state, int width, int height){
    if (!state || !target || width <= 0 || height <= 0) return false;
    if (state->bufferDc && state->bufferBitmap
        && state->bufferWidth == width && state->bufferHeight == height) {
        return true;
    }
    ReleaseThemedMenuBuffer(state);
    state->bufferDc = CreateCompatibleDC(target);
    if (!state->bufferDc) return false;
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

void PaintThemedPopupMenu(HWND hwnd, HDC hdc, ThemedMenuState* state){
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

void CloseThemedPopupMenu(HWND hwnd, ThemedMenuState* state, int selected){
    if (!state || state->closing) return;
    state->selected = selected;
    state->closing = true;
    ReleaseCapture();
    BeginAnimatedClose(hwnd, 160);
}

LRESULT CALLBACK ThemedMenuProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    auto* state = reinterpret_cast<ThemedMenuState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_NCCREATE: {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        if (state) PaintThemedPopupMenu(hwnd, hdc, state);
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
            bool actuallyMoved = !state->hasMousePoint
                || mouse.x != state->lastMouse.x || mouse.y != state->lastMouse.y;
            state->lastMouse = mouse;
            state->hasMousePoint = true;
            if (state->keyboardMode && !actuallyMoved) return 0;
            state->keyboardMode = false;
            int next = ThemedMenuHitTest(state, mouse.y);
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
                state->keyboardMode = false;
                state->hover = -1;
                ScrollThemedMenu(hwnd, state, pt.y < rc.bottom / 2 ? -state->height : state->height);
                return 0;
            }
        }
        return 0;
    case WM_LBUTTONUP:
        if (state && !state->closing) {
            int hit = ThemedMenuHitTest(state, GET_Y_LPARAM(lParam));
            CloseThemedPopupMenu(hwnd, state, hit >= 0 ? state->items[hit].command : 0);
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE && state && !state->closing) {
            CloseThemedPopupMenu(hwnd, state, 0);
            return 0;
        }
        if (state && !state->closing) {
            if (wParam == VK_RETURN || wParam == VK_SPACE) {
                if (state->hover >= 0 && state->hover < (int)state->items.size() && !state->items[state->hover].separator) {
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
            if (wParam == VK_UP || wParam == VK_DOWN || wParam == VK_PRIOR || wParam == VK_NEXT) {
                int direction = (wParam == VK_UP || wParam == VK_PRIOR) ? -1 : 1;
                int steps = (wParam == VK_PRIOR || wParam == VK_NEXT) ? 6 : 1;
                int index = state->hover;
                if (index < 0) {
                    index = direction > 0 ? FirstThemedMenuItem(state) : LastThemedMenuItem(state);
                } else {
                    for (int i = 0; i < steps; ++i) {
                        int next = FindThemedMenuItem(state, index, direction);
                        if (next < 0) break;
                        index = next;
                    }
                }
                SelectThemedMenuItem(hwnd, state, index);
                return 0;
            }
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE && state && !state->done && !state->closing) {
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
        if (state && !state->closing) {
            UpdateThemedMenuSmoothScroll(hwnd, state);
            ApplyThemedMenuReveal(hwnd, state);
        }
        RedrawWindow(hwnd, nullptr, nullptr,
            RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOERASE);
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

void RegisterThemedMenuClass(){
    static bool registered = false;
    if (registered) return;

    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = ThemedMenuProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = L"AttendanceThemedPopupMenu";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    RegisterClassW(&wc);
    registered = true;
}

int MeasureThemedMenuWidth(const std::vector<ThemedMenuItem>& items){
    HDC hdc = GetDC(Ui().hwnd);
    HGDIOBJ oldFont = SelectObject(hdc, Ui().font);
    int width = 180;
    for (const auto& item : items) {
        if (item.separator) continue;
        SIZE size{};
        GetTextExtentPoint32W(hdc, item.text.c_str(), (int)item.text.size(), &size);
        width = std::max(width, (int)size.cx + 42);
    }
    SelectObject(hdc, oldFont);
    ReleaseDC(Ui().hwnd, hdc);
    return std::min(width, 460);
}

int ShowThemedPopupMenu(HWND button, const std::vector<ThemedMenuItem>& items){
    if (!button || items.empty()) return 0;
    ThemedMenuState state;
    state.items = items;
    state.mainScrollX = Ui().scrollX;
    state.mainScrollY = Ui().scrollY;
    ++Ui().openThemedPopupMenus;
    RegisterThemedMenuClass();

    RECT buttonRc{};
    GetWindowRect(button, &buttonRc);
    int width = MeasureThemedMenuWidth(items);
    state.contentHeight = ThemedMenuTotalHeight(&state);
    int height = state.contentHeight;
    int x = buttonRc.right - width;
    int y = buttonRc.bottom + 4;

    HMONITOR monitor = MonitorFromRect(&buttonRc, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (GetMonitorInfoW(monitor, &mi)) {
        int workHeight = (int)(mi.rcWork.bottom - mi.rcWork.top);
        int maxHeight = std::max(160, std::min(THEMED_MENU_MAX_H, workHeight - 16));
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
        if (y + height > (int)mi.rcWork.bottom) y = (int)buttonRc.top - height - 4;
        y = std::clamp(y, minY, maxY);
    }
    state.finalX = x;
    state.finalY = y;
    state.width = width;
    state.height = height;

    HWND popup = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        L"AttendanceThemedPopupMenu", L"",
        WS_POPUP,
        state.finalX, state.finalY, width, height,
        Ui().hwnd, nullptr, GetModuleHandleW(nullptr), &state
    );
    if (!popup) {
        --Ui().openThemedPopupMenus;
        return 0;
    }

    StartWindowReveal(popup, 180);
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
        if (messageResult == -1) break;
        if ((msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN || msg.message == WM_MOUSEWHEEL)
            && msg.hwnd != popup) {
            SendMessageW(popup, msg.message, msg.wParam, msg.lParam);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    --Ui().openThemedPopupMenus;
    if (IsWindow(popup)) DestroyWindow(popup);
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
