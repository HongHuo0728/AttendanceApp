#include "ui/redraw.h"
#include "ui/combobox.h"
#include "animation/scheduler.h"
#include "ui/controls.h"
#include "settings/fonts.h"
#include "ui/popup_menu.h"
#include "ui/theme.h"
#include "ui/drawing.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

void ReleaseComboListBuffer(ComboListScrollState& state);
bool EnsureComboListBuffer(
    HDC target, ComboListScrollState& state, int width, int height);
void RefreshComboListItems(ComboListScrollState& state);
int ComboListItemHeight(HWND list);
double ComboListMaxScrollPx(HWND list, HWND combo);
void SyncComboListScrollState(HWND list, ComboListScrollState& state);
void AdvanceComboListScroll(HWND list, ComboListScrollState& state, uint64_t now);
int ComboListIndexAtPoint(HWND list, const ComboListScrollState& state, int y);
void PaintSmoothComboList(HWND list, HDC target, ComboListScrollState& state);
void NotifyComboSelection(HWND combo, int index);
LRESULT CALLBACK ComboListScrollProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);
void EnableComboListSmoothScroll(HWND combo, HWND list);
LRESULT CALLBACK ComboPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);

std::wstring GetComboSelectedText(HWND combo){
    int index = (int)SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (index < 0) return {};
    int len = (int)SendMessageW(combo, CB_GETLBTEXTLEN, index, 0);
    if (len < 0) return {};
    std::wstring text(len + 1, L'\0');
    SendMessageW(combo, CB_GETLBTEXT, index, (LPARAM)text.data());
    text.resize(len);
    return text;
}

void CommitComboSelectionNow(HWND combo){
    if (!combo) return;
    InvalidateRect(combo, nullptr, FALSE);
    COMBOBOXINFO info{};
    info.cbSize = sizeof(info);
    if (GetComboBoxInfo(combo, &info) && info.hwndList) {
        InvalidateRect(info.hwndList, nullptr, FALSE);
    }
}

void ReleaseComboListBuffer(ComboListScrollState& state){
    if (state.bufferDc && state.bufferOldBitmap) {
        SelectObject(state.bufferDc, state.bufferOldBitmap);
    }
    if (state.bufferBitmap) DeleteObject(state.bufferBitmap);
    if (state.bufferDc) DeleteDC(state.bufferDc);
    state.bufferDc = nullptr;
    state.bufferBitmap = nullptr;
    state.bufferOldBitmap = nullptr;
    state.bufferWidth = 0;
    state.bufferHeight = 0;
}

bool EnsureComboListBuffer(
    HDC target, ComboListScrollState& state, int width, int height){
    if (!target || width <= 0 || height <= 0) return false;
    if (state.bufferDc && state.bufferBitmap
        && state.bufferWidth == width && state.bufferHeight == height) {
        return true;
    }
    ReleaseComboListBuffer(state);
    state.bufferDc = CreateCompatibleDC(target);
    if (!state.bufferDc) return false;
    state.bufferBitmap = CreateCompatibleBitmap(target, width, height);
    if (!state.bufferBitmap) {
        ReleaseComboListBuffer(state);
        return false;
    }
    state.bufferOldBitmap = SelectObject(state.bufferDc, state.bufferBitmap);
    state.bufferWidth = width;
    state.bufferHeight = height;
    return true;
}

void RefreshComboListItems(ComboListScrollState& state){
    state.cachedItems.clear();
    if (!state.combo || !IsWindow(state.combo)) return;
    int count = (int)SendMessageW(state.combo, CB_GETCOUNT, 0, 0);
    if (count <= 0) return;
    state.cachedItems.reserve((size_t)count);
    for (int index = 0; index < count; ++index) {
        int length = (int)SendMessageW(state.combo, CB_GETLBTEXTLEN, index, 0);
        if (length < 0) {
            state.cachedItems.emplace_back();
            continue;
        }
        std::wstring text((size_t)length + 1, L'\0');
        SendMessageW(state.combo, CB_GETLBTEXT, index, (LPARAM)text.data());
        text.resize((size_t)length);
        state.cachedItems.push_back(std::move(text));
    }
}

int ComboListItemHeight(HWND list){
    int itemHeight = (int)SendMessageW(list, LB_GETITEMHEIGHT, 0, 0);
    return itemHeight > 0 ? itemHeight : ComboItemHeight();
}

double ComboListMaxScrollPx(HWND list, HWND combo){
    int count = combo ? (int)SendMessageW(combo, CB_GETCOUNT, 0, 0) : 0;
    if (count <= 0) return 0.0;
    RECT rc{};
    GetClientRect(list, &rc);
    return std::max(0.0,
        count * (double)ComboListItemHeight(list) - (double)(rc.bottom - rc.top));
}

void SyncComboListScrollState(HWND list, ComboListScrollState& state){
    int top = (int)SendMessageW(list, LB_GETTOPINDEX, 0, 0);
    if (top == LB_ERR) top = 0;
    double position = std::clamp(
        top * (double)ComboListItemHeight(list), 0.0, ComboListMaxScrollPx(list, state.combo));
    state.startPx = position;
    state.currentPx = position;
    state.targetPx = position;
    state.startMs = 0;
    state.active = false;
}

void AdvanceComboListScroll(HWND list, ComboListScrollState& state, uint64_t now){
    if (!state.active) return;
    double progress = state.durationMs == 0
        ? 1.0
        : (double)(now - state.startMs) / state.durationMs;
    double eased = CubicBezierEase(progress, 0.22, 1.0, 0.36, 1.0);
    state.currentPx = state.startPx + (state.targetPx - state.startPx) * eased;
    state.currentPx = std::clamp(
        state.currentPx, 0.0, ComboListMaxScrollPx(list, state.combo));
    int itemHeight = std::max(1, ComboListItemHeight(list));
    SendMessageW(list, LB_SETTOPINDEX, (int)std::floor(state.currentPx / itemHeight), 0);
    if (progress >= 1.0) {
        state.currentPx = state.targetPx;
        state.active = false;
        state.startMs = 0;
    }
}

int ComboListIndexAtPoint(HWND list, const ComboListScrollState& state, int y){
    if (!state.combo) return -1;
    int count = (int)SendMessageW(state.combo, CB_GETCOUNT, 0, 0);
    int index = (int)std::floor(
        (y + state.currentPx) / std::max(1, ComboListItemHeight(list)));
    return index >= 0 && index < count ? index : -1;
}

void PaintSmoothComboList(HWND list, HDC target, ComboListScrollState& state){
    RECT rc{};
    GetClientRect(list, &rc);
    if (!state.combo || !IsWindow(state.combo)) {
        SetDCBrushColor(target, Ui().COLOR_INPUT);
        FillRect(target, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
        return;
    }
    int width = std::max(1, (int)(rc.right - rc.left));
    int height = std::max(1, (int)(rc.bottom - rc.top));
    if (!EnsureComboListBuffer(target, state, width, height)) {
        SetDCBrushColor(target, Ui().COLOR_INPUT);
        FillRect(target, &rc, (HBRUSH)GetStockObject(DC_BRUSH));
        return;
    }
    HDC buffer = state.bufferDc;

    HBRUSH dcBrush = (HBRUSH)GetStockObject(DC_BRUSH);
    SetDCBrushColor(buffer, Ui().COLOR_INPUT);
    FillRect(buffer, &rc, dcBrush);

    int itemHeight = std::max(1, ComboListItemHeight(list));
    int count = state.combo ? (int)SendMessageW(state.combo, CB_GETCOUNT, 0, 0) : 0;
    if ((int)state.cachedItems.size() != count) RefreshComboListItems(state);
    int selected = state.combo ? (int)SendMessageW(state.combo, CB_GETCURSEL, 0, 0) : -1;
    int first = std::max(0, (int)std::floor(state.currentPx / itemHeight));
    int last = std::min(count - 1,
        (int)std::ceil((state.currentPx + height) / itemHeight));
    double open = state.combo
        ? GetAnimationValue(state.combo, AnimChannel::ComboOpen, 1.0) : 1.0;

    HGDIOBJ oldFont = Ui().font ? SelectObject(buffer, Ui().font) : nullptr;
    SetBkMode(buffer, TRANSPARENT);
    for (int index = first; index <= last; ++index) {
        int top = (int)std::lround(index * (double)itemHeight - state.currentPx);
        RECT itemRc{0, top, width, top + itemHeight};
        double itemProgress = ElasticOut(std::clamp(
            (open - (double)std::min(index - first, 10) * 0.075) / 0.55, 0.0, 1.0));
        COLORREF base = index == selected ? Ui().COLOR_SELECTED
            : index == state.hoverIndex ? Ui().COLOR_HOVER : Ui().COLOR_INPUT;
        SetDCBrushColor(buffer, BlendColor(Ui().COLOR_BG, base, itemProgress));
        FillRect(buffer, &itemRc, dcBrush);

        const std::wstring& text = state.cachedItems[(size_t)index];
        RECT textRc = itemRc;
        textRc.left += 12;
        textRc.right -= 10;
        SetTextColor(buffer, BlendColor(Ui().COLOR_MUTED, Ui().COLOR_TEXT, itemProgress));
        DrawTextW(buffer, text.c_str(), -1, &textRc,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }
    if (oldFont) SelectObject(buffer, oldFont);

    HGDIOBJ oldPen = SelectObject(buffer, GetStockObject(DC_PEN));
    HGDIOBJ oldBrush = SelectObject(buffer, GetStockObject(HOLLOW_BRUSH));
    SetDCPenColor(buffer, Ui().COLOR_BORDER);
    Rectangle(buffer, 0, 0, width, height);
    SelectObject(buffer, oldBrush);
    SelectObject(buffer, oldPen);

    BitBlt(target, 0, 0, width, height, buffer, 0, 0, SRCCOPY);
}

void NotifyComboSelection(HWND combo, int index){
    if (!combo || index < 0) return;
    if ((int)SendMessageW(combo, CB_GETCURSEL, 0, 0) != index) {
        SendMessageW(combo, CB_SETCURSEL, index, 0);
        HWND parent = GetParent(combo);
        if (parent) {
            SendMessageW(parent, WM_COMMAND,
                MAKEWPARAM(GetDlgCtrlID(combo), CBN_SELCHANGE), (LPARAM)combo);
        }
    }
    InvalidateRect(combo, nullptr, FALSE);
}

LRESULT CALLBACK ComboListScrollProc(
    HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR){
    auto& state = Ui().comboListScrollStates[hwnd];
    switch (msg) {
    case WM_NCDESTROY:
        RemoveAnimationState(hwnd);
        ReleaseComboListBuffer(state);
        Ui().comboListScrollStates.erase(hwnd);
        RemoveWindowSubclass(hwnd, ComboListScrollProc, 6);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        ReleaseComboListBuffer(state);
        InvalidateRect(hwnd, nullptr, FALSE);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        AdvanceComboListScroll(hwnd, state, AnimationNowMs());
        PaintSmoothComboList(hwnd, hdc, state);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEWHEEL: {
        double maximum = ComboListMaxScrollPx(hwnd, state.combo);
        if (maximum <= 0.0) return 0;
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        double pixelDelta = -(double)delta / WHEEL_DELTA
            * WheelScrollLineCount() * ComboListItemHeight(hwnd);
        AdvanceComboListScroll(hwnd, state, AnimationNowMs());
        if (!Settings().smoothScrollingEnabled) {
            state.currentPx = std::clamp(state.currentPx + pixelDelta, 0.0, maximum);
            state.startPx = state.targetPx = state.currentPx;
            state.active = false;
            SendMessageW(hwnd, LB_SETTOPINDEX,
                (int)std::floor(state.currentPx / ComboListItemHeight(hwnd)), 0);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        state.startPx = state.currentPx;
        state.targetPx = std::clamp(state.targetPx + pixelDelta, 0.0, maximum);
        double distance = std::abs(state.targetPx - state.startPx);
        if (distance < 0.5) return 0;
        bool precisionInput = std::abs(delta) < WHEEL_DELTA;
        state.durationMs = precisionInput
            ? (uint32_t)std::clamp(80.0 + distance * 0.18, 90.0, 150.0)
            : (uint32_t)std::clamp(170.0 + distance * 0.16, 190.0, 260.0);
        state.startMs = AnimationNowMs();
        state.active = true;
        RestartAnimation(hwnd, AnimChannel::SmoothScroll, 1.0, state.durationMs);
        return 0;
    }
    case WM_MOUSEMOVE: {
        int hover = ComboListIndexAtPoint(hwnd, state, GET_Y_LPARAM(lParam));
        if (hover != state.hoverIndex) {
            state.hoverIndex = hover;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tracking);
        return 0;
    }
    case WM_MOUSELEAVE:
        state.hoverIndex = -1;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
        SetCapture(hwnd);
        return 0;
    case WM_LBUTTONUP: {
        int index = ComboListIndexAtPoint(hwnd, state, GET_Y_LPARAM(lParam));
        ReleaseCapture();
        if (index >= 0) {
            PostMessageW(state.combo, WM_APP_COMMIT_COMBO_SELECTION,
                static_cast<WPARAM>(index), 0);
        }
        return 0;
    }
    case WM_KEYDOWN: {
        if (wParam == VK_ESCAPE) {
            SendMessageW(state.combo, CB_SHOWDROPDOWN, FALSE, 0);
            return 0;
        }
        if (wParam == VK_RETURN || wParam == VK_SPACE) {
            SendMessageW(state.combo, CB_SHOWDROPDOWN, FALSE, 0);
            return 0;
        }
        int count = state.combo ? (int)SendMessageW(state.combo, CB_GETCOUNT, 0, 0) : 0;
        int selected = state.combo ? (int)SendMessageW(state.combo, CB_GETCURSEL, 0, 0) : -1;
        int next = selected;
        RECT rc{};
        GetClientRect(hwnd, &rc);
        int visible = std::max(1,
            (int)(rc.bottom - rc.top) / ComboListItemHeight(hwnd));
        if (wParam == VK_UP) next = std::max(0, selected - 1);
        else if (wParam == VK_DOWN) next = std::min(count - 1, selected + 1);
        else if (wParam == VK_HOME) next = 0;
        else if (wParam == VK_END) next = count - 1;
        else if (wParam == VK_PRIOR) next = std::max(0, selected - visible);
        else if (wParam == VK_NEXT) next = std::min(count - 1, selected + visible);
        else return DefSubclassProc(hwnd, msg, wParam, lParam);
        if (next >= 0 && next < count) {
            NotifyComboSelection(state.combo, next);
            int itemHeight = ComboListItemHeight(hwnd);
            double top = next * (double)itemHeight;
            double bottom = top + itemHeight;
            if (top < state.currentPx) state.targetPx = top;
            else if (bottom > state.currentPx + (rc.bottom - rc.top)) {
                state.targetPx = bottom - (rc.bottom - rc.top);
            } else {
                state.targetPx = state.currentPx;
            }
            state.targetPx = std::clamp(
                state.targetPx, 0.0, ComboListMaxScrollPx(hwnd, state.combo));
            state.startPx = state.currentPx;
            state.startMs = AnimationNowMs();
            state.durationMs = Settings().smoothScrollingEnabled ? 110 : 0;
            state.active = state.durationMs > 0
                && std::abs(state.targetPx - state.startPx) >= 0.5;
            if (state.active) {
                RestartAnimation(hwnd, AnimChannel::SmoothScroll, 1.0, state.durationMs);
            } else {
                state.currentPx = state.targetPx;
            }
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_APP_ANIMATION_TICK: {
        AcknowledgeAnimationTick(hwnd);
        AdvanceComboListScroll(hwnd, state, AnimationNowMs());
        RedrawWindow(hwnd, nullptr, nullptr,
            RDW_INVALIDATE | RDW_NOERASE);
        return 0;
    }
    case WM_VSCROLL: {
        state.active = false;
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        SyncComboListScrollState(hwnd, state);
        InvalidateRect(hwnd, nullptr, FALSE);
        return result;
    }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableComboListSmoothScroll(HWND combo, HWND list){
    if (!combo || !list) return;
    if (Ui().comboListScrollStates.find(list) == Ui().comboListScrollStates.end()) {
        ComboListScrollState state;
        state.combo = combo;
        RefreshComboListItems(state);
        SyncComboListScrollState(list, state);
        Ui().comboListScrollStates.emplace(list, std::move(state));
    } else {
        ComboListScrollState& state = Ui().comboListScrollStates[list];
        state.combo = combo;
        RefreshComboListItems(state);
    }
    SetWindowSubclass(list, ComboListScrollProc, 6, 0);
}

void StopSmoothScrollingNow(){
    CancelAnimationChannel(AnimChannel::SmoothScroll);
    for (auto& entry : Ui().comboListScrollStates) {
        HWND list = entry.first;
        ComboListScrollState& state = entry.second;
        state.active = false;
        state.startMs = 0;
        state.currentPx = std::clamp(
            state.targetPx, 0.0, ComboListMaxScrollPx(list, state.combo));
        state.startPx = state.targetPx = state.currentPx;
        if (IsWindow(list)) {
            int itemHeight = std::max(1, ComboListItemHeight(list));
            SendMessageW(list, LB_SETTOPINDEX,
                (int)std::floor(state.currentPx / itemHeight), 0);
            RedrawWindow(list, nullptr, nullptr,
                RDW_INVALIDATE | RDW_NOERASE);
        }
    }
}

LRESULT CALLBACK ComboPaintProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR){
    switch (msg) {
    case WM_APP_COMMIT_COMBO_SELECTION:
        SendMessageW(hwnd, CB_SHOWDROPDOWN, FALSE, 0);
        NotifyComboSelection(hwnd, static_cast<int>(wParam));
        return 0;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, ComboPaintProc, 5);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC target = BeginPaint(hwnd, &ps);
        RECT rect{}; GetClientRect(hwnd, &rect);
        BufferedPaint buffer(target, rect);
        PaintComboClosed(hwnd, buffer.DC());
        buffer.Commit();
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case CB_SETCURSEL: {
        if (SendMessageW(hwnd, CB_GETCURSEL, 0, 0) == static_cast<LRESULT>(wParam)) return wParam;
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        if (!Ui().languageUpdateActive) {
            RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
        }
        return result;
    }
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_ENABLE:
    case WM_THEMECHANGED: {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        InvalidateRect(hwnd, nullptr, FALSE);
        return result;
    }
    case WM_APP_ANIMATION_TICK: {
        AcknowledgeAnimationTick(hwnd);
        RedrawAnimatedControl(hwnd);
        COMBOBOXINFO info{};
        info.cbSize = sizeof(info);
        if (GetComboBoxInfo(hwnd, &info) && info.hwndList && IsWindowVisible(info.hwndList)) {
            RedrawWindow(info.hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
        }
        return 0;
    }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void ApplyComboDropDownTheme(HWND combo){
    if (!combo) return;
    COMBOBOXINFO info{};
    info.cbSize = sizeof(info);
    if (!GetComboBoxInfo(combo, &info)) return;

    ApplyNativeControlTheme(combo);
    if (info.hwndItem) ApplyNativeControlTheme(info.hwndItem);
    if (info.hwndList) {
        EnableComboListSmoothScroll(combo, info.hwndList);
        ApplyNativeControlTheme(info.hwndList);
        InvalidateRect(info.hwndList, nullptr, FALSE);
        RedrawWindow(info.hwndList, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME | RDW_NOERASE);
    }
}

void EnableComboPaint(HWND combo){
    if (!combo) return;
    SetWindowSubclass(combo, ComboPaintProc, 5, 0);
    ApplyComboDropDownTheme(combo);
}

} // namespace attendance
