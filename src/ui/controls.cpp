#include "ui/settings_window.h"
#include "ui/controls.h"
#include "ui/common.h"
#include "animation/scheduler.h"
#include "core/text.h"
#include "settings/fonts.h"
#include "ui/layout.h"
#include "animation/particles.h"
#include "ui/combobox.h"
#include "animation/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

bool PasteSingleLineClipboardText(HWND hwnd);
LRESULT CALLBACK EditShortcutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);
void EnableEditShortcuts(HWND hwnd);
LRESULT CALLBACK WheelForwardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);
void DrawAnimatedEditFrame(HWND hwnd);
LRESULT CALLBACK InteractiveControlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR);
void EnableInteractiveAnimation(HWND hwnd);

bool PasteSingleLineClipboardText(HWND hwnd){
    if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(hwnd)) return false;
    bool handled = false;
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data) {
        const wchar_t* clipboardText = static_cast<const wchar_t*>(GlobalLock(data));
        if (clipboardText) {
            std::wstring normalized = NormalizeSingleLineText(clipboardText);
            GlobalUnlock(data);
            SendMessageW(hwnd, EM_REPLACESEL, TRUE,
                reinterpret_cast<LPARAM>(normalized.c_str()));
            handled = true;
        }
    }
    CloseClipboard();
    return handled;
}

LRESULT CALLBACK EditShortcutProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR){
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, EditShortcutProc, 1);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    if (msg == WM_KEYDOWN && wParam == 'A' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        SendMessageW(hwnd, EM_SETSEL, 0, -1);
        return 0;
    }
    if (msg == WM_CHAR && wParam == VK_RETURN) {
        return 0;
    }
    if (msg == WM_CHAR && wParam == 1) {
        return 0;
    }
    if (msg == WM_PASTE && PasteSingleLineClipboardText(hwnd)) {
        return 0;
    }
    if (msg == WM_SIZE || msg == WM_WINDOWPOSCHANGED) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        ConfigureEditMetrics(hwnd);
        return result;
    }
    if (msg == WM_SETFONT) {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        ConfigureEditMetrics(hwnd);
        return result;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableEditShortcuts(HWND hwnd){
    SetWindowSubclass(hwnd, EditShortcutProc, 1, 0);
    ConfigureEditMetrics(hwnd);
}

LRESULT CALLBACK WheelForwardProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR){
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, WheelForwardProc, 2);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    if (msg == WM_MOUSEWHEEL && Ui().hwnd) {
        if (Ui().openThemedPopupMenus > 0) return 0;
        ScrollMainWindow(Ui().hwnd, SB_VERT, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    if (msg == WM_MOUSEHWHEEL && Ui().hwnd) {
        if (Ui().openThemedPopupMenus > 0) return 0;
        ScrollMainWindow(Ui().hwnd, SB_HORZ, 0, GET_WHEEL_DELTA_WPARAM(wParam));
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableMouseWheelForward(HWND hwnd){
    SetWindowSubclass(hwnd, WheelForwardProc, 2, 0);
}

void RedrawAnimatedControl(HWND hwnd){
    wchar_t className[32]{};
    GetClassNameW(hwnd, className, 32);
    if (lstrcmpiW(className, L"Edit") == 0) {
        DrawAnimatedEditFrame(hwnd); // Animate the border without erasing native edit text.
    } else {
        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

void DrawAnimatedEditFrame(HWND hwnd){
    wchar_t className[32]{};
    GetClassNameW(hwnd, className, 32);
    if (lstrcmpiW(className, L"Edit") != 0) return;

    HDC hdc = GetWindowDC(hwnd);
    if (!hdc) return;
    RECT rc{};
    GetWindowRect(hwnd, &rc);
    OffsetRect(&rc, -rc.left, -rc.top);

    double hover = GetAnimationValue(hwnd, AnimChannel::Hover, GetFocus() == hwnd ? 1.0 : 0.0);
    COLORREF border = BlendColor(Ui().COLOR_BORDER, Ui().COLOR_ACCENT, hover);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
    ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK InteractiveControlProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR){
    switch (msg) {
    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, InteractiveControlProc, 7);
        RemoveAnimationState(hwnd);
        Animations().buttonEffects.erase(hwnd);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_MOUSEMOVE: {
        TRACKMOUSEEVENT track{};
        track.cbSize = sizeof(track);
        track.dwFlags = TME_LEAVE;
        track.hwndTrack = hwnd;
        TrackMouseEvent(&track);
        StartAnimation(hwnd, AnimChannel::Hover, 1.0, 140);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    case WM_MOUSELEAVE:
        StartAnimation(hwnd, AnimChannel::Hover, GetFocus() == hwnd ? 1.0 : 0.0, 180);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_LBUTTONDOWN: {
        POINT origin{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        TriggerButtonFeedback(hwnd, origin);
        StartAnimation(hwnd, AnimChannel::Press, 1.0, 70);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    }
    case WM_LBUTTONUP:
    case WM_CANCELMODE:
    case WM_CAPTURECHANGED:
        StartAnimation(hwnd, AnimChannel::Press, 0.0, 180);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_SETFOCUS:
        EnsureSettingsControlVisible(hwnd);
        StartAnimation(hwnd, AnimChannel::Hover, 1.0, 150);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_KILLFOCUS:
        StartAnimation(hwnd, AnimChannel::Hover, 0.0, 180);
        return DefSubclassProc(hwnd, msg, wParam, lParam);
    case WM_NCPAINT: {
        LRESULT result = DefSubclassProc(hwnd, msg, wParam, lParam);
        DrawAnimatedEditFrame(hwnd);
        return result;
    }
    case WM_APP_ANIMATION_TICK:
        AcknowledgeAnimationTick(hwnd);
        RedrawAnimatedControl(hwnd);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void EnableInteractiveAnimation(HWND hwnd){
    if (!hwnd) return;
    wchar_t className[32]{};
    GetClassNameW(hwnd, className, 32);
    if (lstrcmpiW(className, L"Button") != 0 && lstrcmpiW(className, L"Edit") != 0
        && lstrcmpiW(className, L"ComboBox") != 0) return;
    SetWindowSubclass(hwnd, InteractiveControlProc, 7, 0);
    if (lstrcmpiW(className, L"Button") == 0
        && (GetWindowLongPtrW(hwnd, GWL_STYLE) & BS_OWNERDRAW) != 0) {
        ButtonEffectState& state = Animations().buttonEffects[hwnd];
        if (IsCoreParticleButton(GetDlgCtrlID(hwnd))) state.particles.reserve(72);
    }
}

HWND MakeControl(const wchar_t* cls, const wchar_t* text, DWORD style, int id){
    DWORD finalStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | style;
    if (lstrcmpiW(cls, L"EDIT") == 0) finalStyle |= WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_NOHIDESEL;
    HWND hwnd = CreateWindowExW(
        0, cls, text,
        finalStyle,
        0, 0, 100, 30,
        Ui().hwnd, (HMENU)(intptr_t)id, GetModuleHandleW(nullptr), nullptr
    );
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)Ui().font, TRUE);
    if (lstrcmpiW(cls, L"EDIT") == 0) {
        SendMessageW(hwnd, EM_SETLIMITTEXT, MAX_RECORD_FIELD_BYTES / sizeof(wchar_t), 0);
    }
    if (lstrcmpiW(cls, L"STATIC") != 0) EnableInteractiveAnimation(hwnd);
    if (lstrcmpiW(cls, L"EDIT") == 0) EnableEditShortcuts(hwnd);
    if (lstrcmpiW(cls, L"COMBOBOX") == 0 && (style & CBS_OWNERDRAWFIXED)) {
        SendMessageW(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, ComboItemHeight());
        SendMessageW(hwnd, CB_SETITEMHEIGHT, 0, ComboItemHeight());
        SendMessageW(hwnd, CB_SETMINVISIBLE, 12, 0);
        EnableComboPaint(hwnd);
    }
    return hwnd;
}

HWND MakeButton(const wchar_t* text, int id){
    return MakeControl(L"BUTTON", text, BS_PUSHBUTTON | BS_OWNERDRAW, id);
}

HWND MakeSettingsControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id){
    DWORD finalStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | style;
    if (lstrcmpiW(cls, L"EDIT") == 0) {
        finalStyle |= WS_BORDER | ES_LEFT | ES_AUTOHSCROLL | ES_NOHIDESEL;
        if ((style & ES_NUMBER) == 0) finalStyle |= ES_MULTILINE;
    }
    HWND hwnd = CreateWindowExW(
        0, cls, text,
        finalStyle,
        0, 0, 100, 30,
        parent, (HMENU)(intptr_t)id, GetModuleHandleW(nullptr), nullptr
    );
    SendMessageW(hwnd, WM_SETFONT, (WPARAM)Ui().font, TRUE);
    if (lstrcmpiW(cls, L"EDIT") == 0) {
        SendMessageW(hwnd, EM_SETLIMITTEXT, MAX_RECORD_FIELD_BYTES / sizeof(wchar_t), 0);
    }
    if (lstrcmpiW(cls, L"STATIC") != 0) EnableInteractiveAnimation(hwnd);
    if (lstrcmpiW(cls, L"EDIT") == 0) EnableEditShortcuts(hwnd);
    if (lstrcmpiW(cls, L"COMBOBOX") == 0 && (style & CBS_OWNERDRAWFIXED)) {
        SendMessageW(hwnd, CB_SETITEMHEIGHT, (WPARAM)-1, ComboItemHeight());
        SendMessageW(hwnd, CB_SETITEMHEIGHT, 0, ComboItemHeight());
        SendMessageW(hwnd, CB_SETMINVISIBLE, 12, 0);
        EnableComboPaint(hwnd);
    }
    return hwnd;
}

} // namespace attendance
