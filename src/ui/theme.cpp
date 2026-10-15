#include "ui/theme.h"
#include "animation/scheduler.h"
#include "ui/title_bar.h"
#include "settings/fonts.h"
#include "ui/appearance.h"
#include "ui/combobox.h"
#include "settings/state.h"
#include "settings/font_state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

void ResetBrushes();
ThemePalette BuildThemePalette(UiTheme theme, AccentTone accentTone);
ThemePalette CurrentThemePalette();
void SetThemePalette(const ThemePalette& palette);
ThemePalette BlendThemePalette(const ThemePalette& from, const ThemePalette& to, double amount);
bool ThemePalettesEqual(const ThemePalette& left, const ThemePalette& right);
BOOL CALLBACK ApplyThemeToChild(HWND child, LPARAM);
void RedrawThemeRoots(bool updateNow);
BOOL CALLBACK RedrawThemePopup(HWND window, LPARAM updateNowValue);
void RedrawThemePopups(bool updateNow);
void FinishThemeTransition();

void ResetBrushes(){
    if (Ui().bgBrush) DeleteObject(Ui().bgBrush);
    if (Ui().inputBrush) DeleteObject(Ui().inputBrush);
    if (Ui().panelBrush) DeleteObject(Ui().panelBrush);
    Ui().bgBrush = CreateSolidBrush(Ui().COLOR_BG);
    Ui().inputBrush = CreateSolidBrush(Ui().COLOR_INPUT);
    Ui().panelBrush = CreateSolidBrush(Ui().COLOR_PANEL);
}

ThemePalette BuildThemePalette(UiTheme theme, AccentTone accentTone){
    ThemePalette palette;
    if (theme == UiTheme::Light) {
        palette.background = RGB(255, 255, 255);
        palette.panel = RGB(246, 246, 246);
        palette.input = RGB(250, 250, 250);
        palette.text = RGB(0, 0, 0);
        palette.muted = RGB(58, 58, 58);
        palette.danger = RGB(150, 74, 74);
        palette.border = RGB(194, 194, 194);
        palette.hover = RGB(232, 232, 232);
        palette.card = RGB(250, 250, 250);
        palette.rowAlt = RGB(240, 240, 240);
        palette.selected = RGB(220, 228, 230);
        palette.checkFill = RGB(255, 255, 255);
        if (accentTone == AccentTone::Sage) palette.accent = RGB(76, 112, 96);
        else if (accentTone == AccentTone::Cyan) palette.accent = RGB(52, 112, 128);
        else palette.accent = RGB(78, 78, 78);
    } else {
        palette.background = RGB(0, 0, 0);
        palette.panel = RGB(18, 18, 18);
        palette.input = RGB(26, 26, 26);
        palette.text = RGB(255, 255, 255);
        palette.muted = RGB(224, 224, 224);
        palette.danger = RGB(176, 112, 112);
        palette.border = RGB(64, 64, 64);
        palette.hover = RGB(38, 38, 38);
        palette.card = RGB(16, 16, 16);
        palette.rowAlt = RGB(14, 14, 14);
        palette.selected = RGB(42, 42, 42);
        palette.checkFill = RGB(0, 0, 0);
        if (accentTone == AccentTone::Sage) palette.accent = RGB(142, 166, 154);
        else if (accentTone == AccentTone::Cyan) palette.accent = RGB(112, 174, 188);
        else palette.accent = RGB(188, 188, 188);
    }
    return palette;
}

ThemePalette CurrentThemePalette(){
    ThemePalette palette;
    palette.background = Ui().COLOR_BG;
    palette.panel = Ui().COLOR_PANEL;
    palette.input = Ui().COLOR_INPUT;
    palette.text = Ui().COLOR_TEXT;
    palette.muted = Ui().COLOR_MUTED;
    palette.accent = Ui().COLOR_ACCENT;
    palette.danger = Ui().COLOR_DANGER;
    palette.border = Ui().COLOR_BORDER;
    palette.hover = Ui().COLOR_HOVER;
    palette.card = Ui().COLOR_CARD;
    palette.rowAlt = Ui().COLOR_ROW_ALT;
    palette.selected = Ui().COLOR_SELECTED;
    palette.checkFill = Ui().COLOR_CHECK_FILL;
    return palette;
}

void SetThemePalette(const ThemePalette& palette){
    Ui().COLOR_BG = palette.background;
    Ui().COLOR_PANEL = palette.panel;
    Ui().COLOR_INPUT = palette.input;
    Ui().COLOR_TEXT = palette.text;
    Ui().COLOR_MUTED = palette.muted;
    Ui().COLOR_ACCENT = palette.accent;
    Ui().COLOR_DANGER = palette.danger;
    Ui().COLOR_BORDER = palette.border;
    Ui().COLOR_HOVER = palette.hover;
    Ui().COLOR_CARD = palette.card;
    Ui().COLOR_ROW_ALT = palette.rowAlt;
    Ui().COLOR_SELECTED = palette.selected;
    Ui().COLOR_CHECK_FILL = palette.checkFill;
    ResetBrushes();
}

ThemePalette BlendThemePalette(const ThemePalette& from, const ThemePalette& to, double amount){
    ThemePalette palette;
    palette.background = BlendColor(from.background, to.background, amount);
    palette.panel = BlendColor(from.panel, to.panel, amount);
    palette.input = BlendColor(from.input, to.input, amount);
    palette.text = BlendColor(from.text, to.text, amount);
    palette.muted = BlendColor(from.muted, to.muted, amount);
    palette.accent = BlendColor(from.accent, to.accent, amount);
    palette.danger = BlendColor(from.danger, to.danger, amount);
    palette.border = BlendColor(from.border, to.border, amount);
    palette.hover = BlendColor(from.hover, to.hover, amount);
    palette.card = BlendColor(from.card, to.card, amount);
    palette.rowAlt = BlendColor(from.rowAlt, to.rowAlt, amount);
    palette.selected = BlendColor(from.selected, to.selected, amount);
    palette.checkFill = BlendColor(from.checkFill, to.checkFill, amount);
    return palette;
}

bool ThemePalettesEqual(const ThemePalette& left, const ThemePalette& right){
    return left.background == right.background
        && left.panel == right.panel
        && left.input == right.input
        && left.text == right.text
        && left.muted == right.muted
        && left.accent == right.accent
        && left.danger == right.danger
        && left.border == right.border
        && left.hover == right.hover
        && left.card == right.card
        && left.rowAlt == right.rowAlt
        && left.selected == right.selected
        && left.checkFill == right.checkFill;
}

bool IsRenderedPaletteDark(){
    int luminance = (GetRValue(Ui().COLOR_BG) * 299
        + GetGValue(Ui().COLOR_BG) * 587
        + GetBValue(Ui().COLOR_BG) * 114) / 1000;
    return luminance < 128;
}

void ApplyThemePalette(){
    SetThemePalette(BuildThemePalette(Settings().theme, Settings().accentTone));
}

void PaintAppBackground(HWND hwnd, HDC hdc){
    RECT rc{};
    GetClientRect(hwnd, &rc);
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    FillRect(hdc, &rc, Ui().bgBrush);
}

void PaintAppBackgroundSlice(HWND child, HDC hdc){
    RECT rc{};
    GetClientRect(child, &rc);
    FillRect(hdc, &rc, Ui().bgBrush);
}

BOOL CALLBACK ApplyFontToChild(HWND child, LPARAM){
    SendMessageW(child, WM_SETFONT, (WPARAM)Ui().font, TRUE);
    wchar_t className[32]{};
    GetClassNameW(child, className, 32);
    if (lstrcmpiW(className, L"ComboBox") == 0) {
        int itemHeight = ComboItemHeight();
        SendMessageW(child, CB_SETITEMHEIGHT, (WPARAM)-1, itemHeight);
        SendMessageW(child, CB_SETITEMHEIGHT, 0, itemHeight);
    }
    return TRUE;
}

void RecreateFonts(){
    static std::wstring appliedFamily;
    static TextScale appliedScale = TextScale::Standard;
    if (Ui().font && Ui().titleFont && Ui().smallFont
        && appliedFamily == Settings().fontFamily && appliedScale == Settings().textScale) return;
    HFONT oldFont = Ui().font;
    HFONT oldTitleFont = Ui().titleFont;
    HFONT oldSmallFont = Ui().smallFont;

    double scale = Settings().textScale == TextScale::Small ? 0.90
        : Settings().textScale == TextScale::Large ? 1.15 : 1.0;
    Ui().font = CreateUiFont((int)std::lround(20 * scale), FW_NORMAL);
    Ui().titleFont = CreateUiFont((int)std::lround(30 * scale), FW_SEMIBOLD);
    Ui().smallFont = CreateUiFont((int)std::lround(17 * scale), FW_NORMAL);
    if (!Ui().font || !Ui().titleFont || !Ui().smallFont) {
        if (Ui().font) DeleteObject(Ui().font);
        if (Ui().titleFont) DeleteObject(Ui().titleFont);
        if (Ui().smallFont) DeleteObject(Ui().smallFont);
        Ui().font = oldFont; Ui().titleFont = oldTitleFont; Ui().smallFont = oldSmallFont;
        return;
    }
    appliedFamily = Settings().fontFamily;
    appliedScale = Settings().textScale;

    const HWND roots[] = {Ui().hwnd, Ui().settingsWindow, Ui().chartWindow, Ui().quickRollCallWindow};
    for (HWND root : roots) {
        if (root && IsWindow(root)) EnumChildWindows(root, ApplyFontToChild, 0);
    }
    if (Ui().hwnd && IsWindow(Ui().hwnd)) {
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_TITLE), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_SUBTITLE), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_STATS), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_HINT), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_STAT_TOTAL), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_STAT_ATTENDANCE), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_STAT_ISSUES), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
        SendMessageW(GetDlgItem(Ui().hwnd, IDC_STAT_VISIBLE), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
    }
    if (Ui().settingsWindow && IsWindow(Ui().settingsWindow)) {
        SendMessageW(GetDlgItem(Ui().settingsWindow, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        SendMessageW(GetDlgItem(Ui().settingsWindow, IDC_SETTINGS_ADVANCED_TITLE), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
    }
    if (Ui().quickRollCallWindow && IsWindow(Ui().quickRollCallWindow)) {
        SendMessageW(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_TITLE), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        SendMessageW(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_STUDENT), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        SendMessageW(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_HINT), WM_SETFONT, (WPARAM)Ui().smallFont, TRUE);
    }

    if (oldFont) DeleteObject(oldFont);
    if (oldTitleFont) DeleteObject(oldTitleFont);
    if (oldSmallFont) DeleteObject(oldSmallFont);
}

BOOL CALLBACK ApplyThemeToChild(HWND child, LPARAM){
    SetWindowTheme(child, IsRenderedPaletteDark() ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    wchar_t className[32]{};
    GetClassNameW(child, className, 32);
    if (lstrcmpiW(className, L"ComboBox") == 0) ApplyComboDropDownTheme(child);
    InvalidateRect(child, nullptr, FALSE);
    return TRUE;
}

void ApplyThemedControls(HWND root){
    if (!root) return;
    SetWindowTheme(root, IsRenderedPaletteDark() ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    EnumChildWindows(root, ApplyThemeToChild, 0);
}

void RedrawThemeRoots(bool updateNow){
    const HWND roots[] = {Ui().hwnd, Ui().settingsWindow, Ui().chartWindow, Ui().quickRollCallWindow};
    UINT flags = RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE;
    if (updateNow) flags |= RDW_UPDATENOW;
    for (HWND root : roots) {
        if (!root || !IsWindow(root)) continue;
        RedrawWindow(root, nullptr, nullptr, flags);
    }
}

BOOL CALLBACK RedrawThemePopup(HWND window, LPARAM updateNowValue){
    wchar_t className[64]{};
    GetClassNameW(window, className, 64);
    if (lstrcmpiW(className, L"ComboLBox") != 0
        && lstrcmpiW(className, L"AttendanceThemedPopupMenu") != 0) {
        return TRUE;
    }
    UINT flags = RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE;
    if (updateNowValue != 0) flags |= RDW_UPDATENOW;
    RedrawWindow(window, nullptr, nullptr, flags);
    return TRUE;
}

void RedrawThemePopups(bool updateNow){
    if (!Ui().hwnd || !IsWindow(Ui().hwnd)) return;
    DWORD threadId = GetWindowThreadProcessId(Ui().hwnd, nullptr);
    if (threadId != 0) {
        EnumThreadWindows(threadId, RedrawThemePopup, updateNow ? 1 : 0);
    }
}

void FinishThemeTransition(){
    if (!Ui().themeTransition.active) return;
    Ui().themeTransition.active = false;
    Settings().theme = Ui().themeTransition.targetTheme;
    SetThemePalette(Ui().themeTransition.target);
    ApplyDarkMode(Ui().hwnd);
    ApplyGlassTitleBar(Ui().settingsWindow);
    ApplyGlassTitleBar(Ui().chartWindow);
    ApplyGlassTitleBar(Ui().quickRollCallWindow);
    ApplyThemedControls(Ui().settingsWindow);
    ApplyThemedControls(Ui().chartWindow);
    ApplyThemedControls(Ui().quickRollCallWindow);
    RedrawThemeRoots(true);
    RedrawThemePopups(true);
}

void BeginThemeTransition(UiTheme targetTheme){
    if (!Ui().hwnd || !IsWindow(Ui().hwnd)) {
        Settings().theme = targetTheme;
        ApplyVisualSettings();
        return;
    }
    ThemePalette startPalette = CurrentThemePalette();
    ThemePalette targetPalette = BuildThemePalette(targetTheme, Settings().accentTone);
    Ui().themeTransition.start = startPalette;
    Ui().themeTransition.target = targetPalette;
    Ui().themeTransition.targetTheme = targetTheme;
    Ui().themeTransition.active = true;
    Settings().theme = targetTheme;
    if (ThemePalettesEqual(startPalette, targetPalette)) {
        FinishThemeTransition();
        return;
    }
    RestartAnimation(Ui().hwnd, AnimChannel::ThemeTransition, 1.0, 340);
}

void UpdateThemeTransitionFrame(){
    if (!Ui().themeTransition.active) return;
    double progress = GetAnimationValue(Ui().hwnd, AnimChannel::ThemeTransition, 1.0);
    SetThemePalette(BlendThemePalette(Ui().themeTransition.start, Ui().themeTransition.target, progress));
    RefreshOpenAppTitleBars(false);
    if (Ui().list) {
        ListView_SetBkColor(Ui().list, Ui().COLOR_PANEL);
        ListView_SetTextBkColor(Ui().list, Ui().COLOR_PANEL);
        ListView_SetTextColor(Ui().list, Ui().COLOR_TEXT);
    }
    // WM_PAINT is a low-priority message. Synchronously drawing each frame
    // prevents Windows from coalescing the whole transition into its final frame.
    RedrawThemeRoots(true);
    RedrawThemePopups(true);
    if (progress >= 0.999) FinishThemeTransition();
}

void ApplyDarkMode(HWND hwnd){
    BOOL value = IsRenderedPaletteDark() ? TRUE : FALSE;
    if (hwnd) DwmSetWindowAttribute(hwnd, 20, &value, sizeof(value));
    if (hwnd == Ui().hwnd) ApplyGlassTitleBar(hwnd);
    ApplyThemedControls(hwnd);
    if (Ui().list) {
        SetWindowTheme(Ui().list, IsRenderedPaletteDark() ? L"DarkMode_Explorer" : L"Explorer", nullptr);
        HWND header = ListView_GetHeader(Ui().list);
        if (header) {
            SetWindowTheme(header, IsRenderedPaletteDark() ? L"DarkMode_Explorer" : L"Explorer", nullptr);
            InvalidateRect(header, nullptr, FALSE);
        }
        ListView_SetBkColor(Ui().list, Ui().COLOR_PANEL);
        ListView_SetTextBkColor(Ui().list, Ui().COLOR_PANEL);
        ListView_SetTextColor(Ui().list, Ui().COLOR_TEXT);
        DWORD listStyle = LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP;
        if (Settings().showTableGrid) listStyle |= LVS_EX_GRIDLINES;
        ListView_SetExtendedListViewStyle(Ui().list, listStyle);
    }
}

} // namespace attendance
