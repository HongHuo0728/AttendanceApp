#include "ui/title_bar.h"
#include "ui/theme.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

TitleBarVisualState CurrentTitleBarVisualState();
void ApplyTitleBarThemeAttributes(HWND hwnd);
bool ApplyLegacyAcrylicFallback(HWND hwnd);
BOOL CALLBACK RefreshAppTitleBarForWindow(HWND window, LPARAM configureBackdropValue);

TitleBarVisualState CurrentTitleBarVisualState(){
    TitleBarVisualState state;
    state.caption = Ui().COLOR_PANEL;
    state.border = Ui().COLOR_BORDER;
    state.dark = IsRenderedPaletteDark();
    state.text = state.dark ? RGB(255, 255, 255) : RGB(0, 0, 0);
    return state;
}

void ApplyTitleBarThemeAttributes(HWND hwnd){
    if (!hwnd || !IsWindow(hwnd)) return;
    TitleBarVisualState state = CurrentTitleBarVisualState();
    BOOL dark = state.dark ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR,
        &state.caption, sizeof(state.caption));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR,
        &state.text, sizeof(state.text));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR,
        &state.border, sizeof(state.border));
}

bool ApplyLegacyAcrylicFallback(HWND hwnd){
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (!user32) return false;
    auto setComposition = reinterpret_cast<SetWindowCompositionAttributeFn>(
        GetProcAddress(user32, "SetWindowCompositionAttribute"));
    if (!setComposition) return false;

    bool dark = IsRenderedPaletteDark();
    AccentPolicy policy;
    policy.state = AccentState::AcrylicBlurBehind;
    policy.flags = 2;
    policy.gradientColor = dark ? 0xCC202020 : 0xCCF4F4F4;
    WindowCompositionAttributeData data;
    data.attribute = 19; // WCA_ACCENT_POLICY
    data.data = &policy;
    data.size = sizeof(policy);
    return setComposition(hwnd, &data) != FALSE;
}

void ApplyGlassTitleBar(HWND hwnd){
    if (!hwnd || !IsWindow(hwnd)) return;
    ApplyTitleBarThemeAttributes(hwnd);

    int backdrop = 3;
    HRESULT backdropResult = DwmSetWindowAttribute(
        hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
    int cornerPreference = 2; // DWMWCP_ROUND
    DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
        &cornerPreference, sizeof(cornerPreference));

    MARGINS margins{0, 0, 1, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
    if (FAILED(backdropResult) && !ApplyLegacyAcrylicFallback(hwnd)) {
        DWM_BLURBEHIND blur{};
        blur.dwFlags = DWM_BB_ENABLE;
        blur.fEnable = TRUE;
        DwmEnableBlurBehindWindow(hwnd, &blur);
    }

}

BOOL CALLBACK RefreshAppTitleBarForWindow(HWND window, LPARAM configureBackdropValue){
    LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    if ((style & WS_CAPTION) == 0) return TRUE;
    if (configureBackdropValue != 0) {
        ApplyGlassTitleBar(window);
    } else {
        ApplyTitleBarThemeAttributes(window);

    }
    return TRUE;
}

void RefreshOpenAppTitleBars(bool configureBackdrop){
    if (!Ui().hwnd || !IsWindow(Ui().hwnd)) return;
    DWORD threadId = GetWindowThreadProcessId(Ui().hwnd, nullptr);
    if (threadId != 0) {
        EnumThreadWindows(threadId, RefreshAppTitleBarForWindow,
            configureBackdrop ? 1 : 0);
    }
}

void CenterOwnedWindow(HWND window, HWND owner){
    if (!window) return;
    RECT windowRc{};
    if (!GetWindowRect(window, &windowRc)) return;

    HMONITOR monitor = MonitorFromWindow(owner && IsWindow(owner) ? owner : window, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) return;

    RECT anchor = mi.rcWork;
    if (owner && IsWindow(owner)) GetWindowRect(owner, &anchor);
    int width = windowRc.right - windowRc.left;
    int height = windowRc.bottom - windowRc.top;
    int x = anchor.left + ((anchor.right - anchor.left) - width) / 2;
    int y = anchor.top + ((anchor.bottom - anchor.top) - height) / 2;
    x = std::clamp(x, (int)mi.rcWork.left, std::max((int)mi.rcWork.left, (int)mi.rcWork.right - width));
    y = std::clamp(y, (int)mi.rcWork.top, std::max((int)mi.rcWork.top, (int)mi.rcWork.bottom - height));
    SetWindowPos(window, nullptr, x, y, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
}

} // namespace attendance
