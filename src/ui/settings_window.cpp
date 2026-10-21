#include <memory>
#include "ui/layout.h"
#include "ui/redraw.h"
#include "ui/settings_window.h"
#include "settings/toggles.h"
#include "ui/common.h"
#include "animation/scheduler.h"
#include "ui/title_bar.h"
#include "core/text.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "settings/fonts.h"
#include "ui/combobox.h"
#include "settings/preferences.h"
#include "ui/table.h"
#include "ui/theme.h"
#include "ui/appearance.h"
#include "ui/roll_call.h"
#include "ui/controls.h"
#include "ui/drawing.h"
#include "settings/state.h"
#include "settings/font_state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

void FillSettingsCombos(HWND hwnd, bool refillLanguageAndFont = true);
void ApplySettingsLanguage(HWND hwnd);
int NormalizeScrollFrameRateInput(HWND hwnd);
void ApplySettingsFromWindow(HWND hwnd);
void ResizeSettingsLayout(HWND hwnd);
LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
HWND EnsureSettingsWindowCreated();
void SyncSettingsWindowState(HWND hwnd);
void HideSettingsWindow();

void ApplyFontListToSettings(HWND hwnd){
    if (!hwnd || !IsWindow(hwnd)) return;
    HWND font = GetDlgItem(hwnd, IDC_SETTINGS_FONT);
    if (!font) return;
    if (SendMessageW(font, CB_GETDROPPEDSTATE, 0, 0)) {
        Fonts().fontComboRefreshPending = true;
        return;
    }

    std::vector<std::wstring> fonts;
    uint64_t generation = 0;
    {
        std::lock_guard<std::mutex> lock(Fonts().fontEnumerationMutex);
        generation = Fonts().fontPublishedGeneration;
        if (generation == Fonts().fontAppliedGeneration && SendMessageW(font, CB_GETCOUNT, 0, 0) > 0) {
            LRESULT selected = SendMessageW(font, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
                reinterpret_cast<LPARAM>(Settings().fontFamily.c_str()));
            if (selected != CB_ERR) {
                SendMessageW(font, CB_SETCURSEL, selected, 0);
                Fonts().fontComboRefreshPending = false;
                return;
            }
        }
        if (Fonts().fontCacheReady) fonts = Fonts().availableFonts;
    }
    if (std::find(fonts.begin(), fonts.end(), Settings().fontFamily) == fonts.end()) {
        fonts.push_back(Settings().fontFamily);
        std::sort(fonts.begin(), fonts.end());
    }

    RedrawScope redraw(font);
    SendMessageW(font, CB_RESETCONTENT, 0, 0);
    int selected = 0;
    for (int i = 0; i < static_cast<int>(fonts.size()); ++i) {
        SendMessageW(font, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(fonts[i].c_str()));
        if (Settings().fontFamily == fonts[i]) selected = i;
    }
    SendMessageW(font, CB_SETCURSEL, selected, 0);
    InvalidateRect(font, nullptr, FALSE);
    Fonts().fontComboRefreshPending = false;
    Fonts().fontAppliedGeneration = generation;
}

void FillSettingsCombos(HWND hwnd, bool refillLanguageAndFont){
    RedrawScope redraw(hwnd);
    HWND language = GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE);
    HWND font = GetDlgItem(hwnd, IDC_SETTINGS_FONT);
    HWND animation = GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LEVEL);
    HWND particle = GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL);
    HWND report = GetDlgItem(hwnd, IDC_SETTINGS_REPORT_TEMPLATE);
    HWND theme = GetDlgItem(hwnd, IDC_SETTINGS_THEME);
    HWND accent = GetDlgItem(hwnd, IDC_SETTINGS_ACCENT);
    HWND textScale = GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE);
    HWND density = GetDlgItem(hwnd, IDC_SETTINGS_DENSITY);
    HWND autosave = GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_INTERVAL);

    if (refillLanguageAndFont) {
        SendMessageW(language, CB_RESETCONTENT, 0, 0);
        for (int i = 0; i <= (int)UiLanguage::ChineseTraditionalHongKong; ++i) {
            SendMessageW(language, CB_ADDSTRING, 0, (LPARAM)LanguageName((UiLanguage)i));
        }
    }
    SendMessageW(language, CB_SETCURSEL, (WPARAM)Settings().language, 0);

    if (refillLanguageAndFont) {
        ApplyFontListToSettings(hwnd);
    }

    SendMessageW(animation, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= 2; ++i) {
        std::wstring label = AnimationLevelName((AnimationLevel)i);
        SendMessageW(animation, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(animation, CB_SETCURSEL, (WPARAM)Settings().animationLevel, 0);

    SendMessageW(particle, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= 2; ++i) {
        std::wstring label = ParticleLevelName((ParticleLevel)i);
        SendMessageW(particle, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(particle, CB_SETCURSEL, (WPARAM)Settings().particleLevel, 0);

    SendMessageW(report, CB_RESETCONTENT, 0, 0);
    for (int i = 0; i <= 3; ++i) {
        std::wstring label = ReportTemplateName((ReportTemplate)i);
        SendMessageW(report, CB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
    SendMessageW(report, CB_SETCURSEL, (WPARAM)Settings().reportTemplate, 0);

    SendMessageW(theme, CB_RESETCONTENT, 0, 0);
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)Tr(L"Pure black", L"\u7eaf\u9ed1").c_str());
    SendMessageW(theme, CB_ADDSTRING, 0, (LPARAM)Tr(L"Pure white", L"\u7eaf\u767d").c_str());
    SendMessageW(theme, CB_SETCURSEL, (WPARAM)Settings().theme, 0);

    SendMessageW(accent, CB_RESETCONTENT, 0, 0);
    SendMessageW(accent, CB_ADDSTRING, 0, (LPARAM)Tr(L"Neutral", L"\u4e2d\u6027\u7070").c_str());
    SendMessageW(accent, CB_ADDSTRING, 0, (LPARAM)Tr(L"Muted sage", L"\u4f4e\u9971\u548c\u9f20\u5c3e\u8349").c_str());
    SendMessageW(accent, CB_ADDSTRING, 0, (LPARAM)Tr(L"Muted cyan", L"\u4f4e\u9971\u548c\u9752\u8272").c_str());
    SendMessageW(accent, CB_SETCURSEL, (WPARAM)Settings().accentTone, 0);

    SendMessageW(textScale, CB_RESETCONTENT, 0, 0);
    SendMessageW(textScale, CB_ADDSTRING, 0, (LPARAM)Tr(L"Small", L"\u5c0f").c_str());
    SendMessageW(textScale, CB_ADDSTRING, 0, (LPARAM)Tr(L"Standard", L"\u6807\u51c6").c_str());
    SendMessageW(textScale, CB_ADDSTRING, 0, (LPARAM)Tr(L"Large", L"\u5927").c_str());
    SendMessageW(textScale, CB_SETCURSEL, (WPARAM)Settings().textScale, 0);

    SendMessageW(density, CB_RESETCONTENT, 0, 0);
    SendMessageW(density, CB_ADDSTRING, 0, (LPARAM)Tr(L"Compact", L"\u7d27\u51d1").c_str());
    SendMessageW(density, CB_ADDSTRING, 0, (LPARAM)Tr(L"Comfortable", L"\u8212\u9002").c_str());
    SendMessageW(density, CB_ADDSTRING, 0, (LPARAM)Tr(L"Spacious", L"\u5bbd\u677e").c_str());
    SendMessageW(density, CB_SETCURSEL, (WPARAM)Settings().layoutDensity, 0);

    SendMessageW(autosave, CB_RESETCONTENT, 0, 0);
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"Off", L"\u5173\u95ed").c_str());
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"30 seconds", L"30 \u79d2").c_str());
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"1 minute", L"1 \u5206\u949f").c_str());
    SendMessageW(autosave, CB_ADDSTRING, 0, (LPARAM)Tr(L"5 minutes", L"5 \u5206\u949f").c_str());
    int autosaveIndex = Settings().autosaveIntervalSeconds == 0 ? 0 : Settings().autosaveIntervalSeconds == 60 ? 2 : Settings().autosaveIntervalSeconds == 300 ? 3 : 1;
    SendMessageW(autosave, CB_SETCURSEL, autosaveIndex, 0);

    if (!Ui().languageUpdateActive) {
        const HWND combos[] = {language, font, animation, particle, report, theme, accent, textScale, density, autosave};
        for (HWND combo : combos) {
            RedrawWindow(combo, nullptr, nullptr, RDW_INVALIDATE | RDW_NOERASE);
        }
    }
    EnableWindow(particle, Settings().particlesEnabled ? TRUE : FALSE);
}

void ApplySettingsLanguage(HWND hwnd){
    RedrawScope redraw(hwnd);
    std::wstring settingsTitle = Tr(L"Settings", L"\u8bbe\u7f6e");
    SetWindowTextW(hwnd, settingsTitle.c_str());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), Tr(L"Settings", L"\u8bbe\u7f6e"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_LANG_LABEL), Tr(L"Language", L"\u8bed\u8a00"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_FONT_LABEL), Tr(L"Interface Font", L"\u754c\u9762\u5b57\u4f53"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LABEL), Tr(L"Animation level", L"\u52a8\u753b\u5f3a\u5ea6"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LABEL), Tr(L"Particle density", L"\u7c92\u5b50\u5bc6\u5ea6"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_REPORT_LABEL), Tr(L"Report template", L"\u62a5\u544a\u6a21\u677f"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLES), ParticleEffectsLabel());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_RISK_ALERTS), Tr(L"Enable risk student reminders", L"\u542f\u7528\u98ce\u9669\u5b66\u751f\u63d0\u9192"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_PROMPT), Tr(L"Prompt for autosave recovery", L"\u542f\u52a8\u65f6\u63d0\u793a\u6062\u590d\u81ea\u52a8\u4fdd\u5b58"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_COMMAND_PALETTE), Tr(L"Enable Ctrl+K command palette", L"\u542f\u7528 Ctrl+K \u547d\u4ee4\u9762\u677f"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_ADVANCED_FILTER), Tr(L"Enable advanced filter syntax", L"\u542f\u7528\u9ad8\u7ea7\u7b5b\u9009\u8bed\u6cd5"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SMOOTH_SCROLL), SmoothDropdownScrollingLabel());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_THEME_LABEL), Tr(L"Theme", L"\u4e3b\u9898"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_ACCENT_LABEL), Tr(L"Accent tone", L"\u5f3a\u8c03\u8272\u8c03"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE_LABEL), Tr(L"Text size", L"\u6587\u5b57\u5927\u5c0f"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_DENSITY_LABEL), Tr(L"Interface density", L"\u754c\u9762\u5bc6\u5ea6"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_LABEL), Tr(L"Autosave interval", L"\u81ea\u52a8\u4fdd\u5b58\u95f4\u9694"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SCROLL_FPS_LABEL), ScrollFrameRateLabel());
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SCROLL_FPS_UNIT), L"FPS");
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SCROLL_FPS), std::to_wstring(Settings().scrollAnimationFps.load()));
    ConfigureEditMetrics(GetDlgItem(hwnd, IDC_SETTINGS_SCROLL_FPS));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_STATS), Tr(L"Show statistics cards", L"\u663e\u793a\u7edf\u8ba1\u5361\u7247"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SHOW_HINT), Tr(L"Show bottom usage hint", L"\u663e\u793a\u5e95\u90e8\u4f7f\u7528\u63d0\u793a"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_TABLE_GRID), Tr(L"Show table grid lines", L"\u663e\u793a\u8868\u683c\u7f51\u683c\u7ebf"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_APPLY), Tr(L"Apply", L"\u5e94\u7528"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_CLOSE), Tr(L"Close", L"\u5173\u95ed"));
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_RESET), Tr(L"Reset All Settings", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), Tr(L"Language", L"\u8bed\u8a00"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_FONT), Tr(L"Interface Font", L"\u754c\u9762\u5b57\u4f53"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LEVEL), Tr(L"Animation level", L"\u52a8\u753b\u5f3a\u5ea6"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL), Tr(L"Particle density", L"\u7c92\u5b50\u5bc6\u5ea6"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_REPORT_TEMPLATE), Tr(L"Report template", L"\u62a5\u544a\u6a21\u677f"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_THEME), Tr(L"Theme", L"\u4e3b\u9898"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_ACCENT), Tr(L"Accent tone", L"\u5f3a\u8c03\u8272\u8c03"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE), Tr(L"Text size", L"\u6587\u5b57\u5927\u5c0f"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_DENSITY), Tr(L"Interface density", L"\u754c\u9762\u5bc6\u5ea6"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_INTERVAL), Tr(L"Autosave interval", L"\u81ea\u52a8\u4fdd\u5b58\u95f4\u9694"));
    SetAccessibleName(GetDlgItem(hwnd, IDC_SETTINGS_SCROLL_FPS), ScrollFrameRateLabel());
    for (const auto& binding : SettingToggleBindings()) {
        SendMessageW(GetDlgItem(hwnd, binding.id), BM_SETCHECK,
            *binding.value ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    FillSettingsCombos(hwnd, !Ui().languageUpdateActive);
}

void ApplyLanguageAtomically(HWND settingsWindow){
    const HWND candidates[] = {Ui().hwnd, settingsWindow, Ui().chartWindow, Ui().quickRollCallWindow};
    std::vector<HWND> roots;
    for (HWND root : candidates) {
        if (root && IsWindow(root)
            && std::find(roots.begin(), roots.end(), root) == roots.end()) {
            roots.push_back(root);
        }
    }
    std::vector<HWND> redrawWindows = roots;
    for (HWND root : roots) {
        EnumChildWindows(root, [](HWND child, LPARAM data) -> BOOL {
            auto* windows = reinterpret_cast<std::vector<HWND>*>(data);
            if (std::find(windows->begin(), windows->end(), child) == windows->end()) {
                windows->push_back(child);
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&redrawWindows));
    }
    HWND previousFocus = GetFocus();
    Ui().languageUpdateActive = true;
    std::vector<std::unique_ptr<RedrawScope>> redrawScopes;
    for (HWND window : redrawWindows) redrawScopes.push_back(std::make_unique<RedrawScope>(window));

    ApplyMainLanguage();
    RefreshCourseCombo();
    if (settingsWindow && IsWindow(settingsWindow)) ApplySettingsLanguage(settingsWindow);
    if (Ui().chartWindow && IsWindow(Ui().chartWindow)) {
        SetWindowTextW(Ui().chartWindow, Tr(L"Statistics Chart", L"\u7edf\u8ba1\u56fe\u8868").c_str());
    }
    if (Ui().quickRollCallWindow && IsWindow(Ui().quickRollCallWindow)) {
        SetWindowTextW(Ui().quickRollCallWindow, Tr(L"Quick roll call", L"\u5feb\u901f\u70b9\u540d").c_str());
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_TITLE), Tr(L"Quick roll call", L"\u5feb\u901f\u70b9\u540d"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_NOTES_LABEL), Tr(L"Notes:", L"\u5907\u6ce8\uff1a"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_PRESENT), Tr(L"Present", L"\u51fa\u5e2d"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_ABSENT), Tr(L"Absent", L"\u7f3a\u5e2d"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_LATE), Tr(L"Late", L"\u8fdf\u5230"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_OTHER), Tr(L"Other", L"\u5176\u4ed6"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_PREVIOUS), Tr(L"Previous", L"\u4e0a\u4e00\u4f4d"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_SKIP), Tr(L"Skip", L"\u8df3\u8fc7"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_FINISH), Tr(L"Finish", L"\u5b8c\u6210"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_CANCEL), Tr(L"Cancel", L"\u53d6\u6d88"));
        SetText(GetDlgItem(Ui().quickRollCallWindow,IDC_QUICK_LEAVE),Tr(L"Leave",L"请假"));
        SetText(GetDlgItem(Ui().quickRollCallWindow,IDC_QUICK_PENDING),Tr(L"Pending",L"未点名"));
        SetText(GetDlgItem(Ui().quickRollCallWindow, IDC_QUICK_HINT),
            Tr(L"Keys 1-6: Present, Absent, Late, Other, Leave, Pending; arrows: Previous / Skip",L"数字键1至6选择六种状态，方向键上一位或跳过"));
        RefreshQuickRollCallWindow(Ui().quickRollCallWindow);
    }

    Ui().languageUpdateActive = false;
    redrawScopes.clear();
    if (settingsWindow) ResizeSettingsLayout(settingsWindow);
    if (previousFocus && IsWindow(previousFocus)) SetFocus(previousFocus);
}

int NormalizeScrollFrameRateInput(HWND hwnd){
    HWND edit = GetDlgItem(hwnd, IDC_SETTINGS_SCROLL_FPS);
    int current = std::clamp(Settings().scrollAnimationFps.load(), 60, 360);
    int normalized = current;
    std::wstring value = TrimWide(GetText(edit));
    if (!value.empty()) {
        try {
            size_t parsed = 0;
            long long candidate = std::stoll(value, &parsed, 10);
            if (parsed == value.size()) {
                normalized = (int)std::clamp<long long>(candidate, 60, 360);
            }
        } catch (const std::exception&) {
            normalized = current;
        }
    }
    SetText(edit, std::to_wstring(normalized));
    ConfigureEditMetrics(edit);
    if (normalized != current) {
        Settings().scrollAnimationFps.store(normalized);
        SignalAnimationScheduler();
    }
    return normalized;
}

void ApplySettingsFromWindow(HWND hwnd){
    const auto previousLanguage = Settings().language;
    const auto previousFont = Settings().fontFamily;
    const auto previousScale = Settings().textScale;
    const auto previousDensity = Settings().layoutDensity;
    const int previousAutosaveInterval = Settings().autosaveIntervalSeconds;
    UiTheme previousTheme = Settings().theme;
    AccentTone previousAccent = Settings().accentTone;
    UiTheme requestedTheme = Settings().theme;
    int language = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_LANGUAGE), CB_GETCURSEL, 0, 0);
    int fontIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_FONT), CB_GETCURSEL, 0, 0);
    int animationIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_ANIMATION_LEVEL), CB_GETCURSEL, 0, 0);
    int particleIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL), CB_GETCURSEL, 0, 0);
    int reportIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_REPORT_TEMPLATE), CB_GETCURSEL, 0, 0);
    int themeIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_THEME), CB_GETCURSEL, 0, 0);
    int accentIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_ACCENT), CB_GETCURSEL, 0, 0);
    int textScaleIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TEXT_SCALE), CB_GETCURSEL, 0, 0);
    int densityIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_DENSITY), CB_GETCURSEL, 0, 0);
    int autosaveIndex = (int)SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_AUTOSAVE_INTERVAL), CB_GETCURSEL, 0, 0);

    if (language < 0 || language > (int)UiLanguage::ChineseTraditionalHongKong) language = 0;
    Settings().language = (UiLanguage)language;
    if (animationIndex >= 0 && animationIndex <= 2) Settings().animationLevel = (AnimationLevel)animationIndex;
    if (particleIndex >= 0 && particleIndex <= 2) Settings().particleLevel = (ParticleLevel)particleIndex;
    if (reportIndex >= 0 && reportIndex <= 3) Settings().reportTemplate = (ReportTemplate)reportIndex;
    {
        if (themeIndex >= 0 && themeIndex <= 1) requestedTheme = (UiTheme)themeIndex;
        if (accentIndex >= 0 && accentIndex <= 2) Settings().accentTone = (AccentTone)accentIndex;
        if (textScaleIndex >= 0 && textScaleIndex <= 2) Settings().textScale = (TextScale)textScaleIndex;
        if (densityIndex >= 0 && densityIndex <= 2) Settings().layoutDensity = (LayoutDensity)densityIndex;
        static const int autosaveSeconds[] = {0, 30, 60, 300};
        if (autosaveIndex >= 0 && autosaveIndex <= 3) Settings().autosaveIntervalSeconds = autosaveSeconds[autosaveIndex];
        NormalizeScrollFrameRateInput(hwnd);
    }

    if (fontIndex >= 0) {
        std::wstring fontName = GetComboSelectedText(GetDlgItem(hwnd, IDC_SETTINGS_FONT));
        if (!fontName.empty()) Settings().fontFamily = fontName;
    }
    bool paletteChanged = requestedTheme != previousTheme || Settings().accentTone != previousAccent;
    if (paletteChanged) {
        BeginThemeTransition(requestedTheme);
    }
    if (paletteChanged || previousFont != Settings().fontFamily || previousScale != Settings().textScale
        || previousDensity != Settings().layoutDensity || previousAutosaveInterval != Settings().autosaveIntervalSeconds) {
        // A palette transition changes colors only; it must not rebuild controls or layout.
        if (previousFont != Settings().fontFamily || previousScale != Settings().textScale
            || previousDensity != Settings().layoutDensity) {
            ApplyVisualSettings(Ui().themeTransition.active);
            ResizeSettingsLayout(hwnd);
        }
        if (previousAutosaveInterval != Settings().autosaveIntervalSeconds && Ui().hwnd) {
            KillTimer(Ui().hwnd, 1);
            if (Settings().autosaveIntervalSeconds > 0)
                SetTimer(Ui().hwnd, 1, Settings().autosaveIntervalSeconds * 1000, nullptr);
        }
    }
    if (previousLanguage != Settings().language) ApplyLanguageAtomically(hwnd);
    SaveSettingsWithFeedback(hwnd);
}

void ResizeSettingsLayout(HWND hwnd){
    if (!hwnd) return;
    RedrawScope redraw(hwnd);
    RECT client{}; GetClientRect(hwnd, &client);
    const int width = std::max(1, (int)client.right);
    const int height = std::max(1, (int)client.bottom);
    const int pad = 24, gap = 24;
    const int columns = width >= 800 ? 2 : 1;
    const int fieldWidth = std::max(1, (width - pad * 2 - gap * (columns - 1)) / columns);
    // Labels sit above values, allowing long translations and large fonts to use the full width.
    const std::pair<int,int> fields[] = {
        {IDC_SETTINGS_LANG_LABEL, IDC_SETTINGS_LANGUAGE},
        {IDC_SETTINGS_FONT_LABEL, IDC_SETTINGS_FONT},
        {IDC_SETTINGS_THEME_LABEL, IDC_SETTINGS_THEME},
        {IDC_SETTINGS_ACCENT_LABEL, IDC_SETTINGS_ACCENT},
        {IDC_SETTINGS_TEXT_SCALE_LABEL, IDC_SETTINGS_TEXT_SCALE},
        {IDC_SETTINGS_DENSITY_LABEL, IDC_SETTINGS_DENSITY},
        {IDC_SETTINGS_ANIMATION_LABEL, IDC_SETTINGS_ANIMATION_LEVEL},
        {IDC_SETTINGS_PARTICLE_LABEL, IDC_SETTINGS_PARTICLE_LEVEL},
        {IDC_SETTINGS_REPORT_LABEL, IDC_SETTINGS_REPORT_TEMPLATE},
        {IDC_SETTINGS_AUTOSAVE_LABEL, IDC_SETTINGS_AUTOSAVE_INTERVAL},
        {IDC_SETTINGS_SCROLL_FPS_LABEL, IDC_SETTINGS_SCROLL_FPS}
    };
    const int fieldRows = ((int)std::size(fields) + columns - 1) / columns;
    const int toggles[] = {IDC_SETTINGS_PARTICLES, IDC_SETTINGS_SMOOTH_SCROLL,
        IDC_SETTINGS_SHOW_STATS, IDC_SETTINGS_SHOW_HINT, IDC_SETTINGS_TABLE_GRID,
        IDC_SETTINGS_RISK_ALERTS, IDC_SETTINGS_AUTOSAVE_PROMPT,
        IDC_SETTINGS_COMMAND_PALETTE, IDC_SETTINGS_ADVANCED_FILTER};
    const int toggleY = 78 + fieldRows * 84 + 12;
    const int toggleRows = ((int)std::size(toggles) + columns - 1) / columns;
    const int actionsY = toggleY + toggleRows * 42 + 18;
    const int contentHeight = actionsY + 112;
    Ui().settingsScrollY = std::clamp(Ui().settingsScrollY, 0, std::max(0, contentHeight - height));
    struct Placement { HWND control; int x, y, width, height; };
    std::vector<Placement> placements;
    placements.reserve(48);
    auto move = [&](int id, int x, int y, int w, int h) {
        HWND control = GetDlgItem(hwnd, id);
        if (!control) return;
        placements.push_back({control, x, y - Ui().settingsScrollY, w, h});
    };
    move(IDC_SETTINGS_TITLE, pad, 22, width - pad * 2, 38);
    for (int i = 0; i < (int)std::size(fields); ++i) {
        int x = pad + (i % columns) * (fieldWidth + gap), y = 78 + (i / columns) * 84;
        move(fields[i].first, x, y, fieldWidth, 28);
        bool fps = fields[i].second == IDC_SETTINGS_SCROLL_FPS;
        move(fields[i].second, x, y + 30, fps ? 100 : fieldWidth, fps ? 34 : 430);
        if (fps) move(IDC_SETTINGS_SCROLL_FPS_UNIT, x + 112, y + 36, 52, 28);
    }
    for (int i = 0; i < (int)std::size(toggles); ++i)
        move(toggles[i], pad + (i % columns) * (fieldWidth + gap), toggleY + (i / columns) * 42, fieldWidth, 34);
    move(IDC_SETTINGS_RESET, pad, actionsY, width - pad * 2, 36);
    const int buttonWidth = std::max(1, (width - pad * 2 - 12) / 2);
    move(IDC_SETTINGS_APPLY, pad, actionsY + 48, buttonWidth, 38);
    move(IDC_SETTINGS_CLOSE, pad + buttonWidth + 12, actionsY + 48, buttonWidth, 38);
    HDWP positions = BeginDeferWindowPos((int)placements.size());
    const UINT flags = SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOREDRAW;
    for (const auto& p : placements) {
        if (!positions) break;
        positions = DeferWindowPos(positions, p.control, nullptr, p.x, p.y, p.width, p.height, flags);
    }
    if (!positions || !EndDeferWindowPos(positions)) {
        for (const auto& p : placements)
            SetWindowPos(p.control, nullptr, p.x, p.y, p.width, p.height, flags);
    }
    SCROLLINFO scroll{sizeof(scroll)};
    scroll.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll.nMax = contentHeight - 1; scroll.nPage = height; scroll.nPos = Ui().settingsScrollY;
    SetScrollInfo(hwnd, SB_VERT, &scroll, TRUE);
}

void EnsureSettingsControlVisible(HWND control){
    HWND hwnd = Ui().settingsWindow;
    if (!hwnd || !control || GetParent(control) != hwnd) return;
    RECT box{}, client{};
    GetWindowRect(control, &box); GetClientRect(hwnd, &client);
    MapWindowPoints(nullptr, hwnd, reinterpret_cast<POINT*>(&box), 2);
    int next = Ui().settingsScrollY;
    if (box.top < 32) next += box.top - 32;
    else if (box.bottom > client.bottom - 12) next += box.bottom - client.bottom + 12;
    SCROLLINFO info{sizeof(info)}; info.fMask = SIF_ALL; GetScrollInfo(hwnd, SB_VERT, &info);
    next = std::clamp(next, 0, std::max(0, info.nMax - (int)info.nPage + 1));
    if (next != Ui().settingsScrollY) { Ui().settingsScrollY = next; ResizeSettingsLayout(hwnd); }
}

void ResetAppearanceValues(){
    Settings().theme = UiTheme::Dark;
    Settings().accentTone = AccentTone::Neutral;
    Settings().textScale = TextScale::Standard;
    Settings().layoutDensity = LayoutDensity::Comfortable;
    Settings().showStatsCards = true;
    Settings().showFooterHint = true;
    Settings().showTableGrid = true;
    Settings().autosaveIntervalSeconds = 30;
    Settings().scrollAnimationFps.store(60);
    SignalAnimationScheduler();
}

LRESULT CALLBACK SettingsProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch (msg) {
    case WM_CREATE: {
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_TITLE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_LANG_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_LANGUAGE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_FONT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_FONT);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_ANIMATION_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_ANIMATION_LEVEL);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_PARTICLE_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_PARTICLE_LEVEL);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_REPORT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_REPORT_TEMPLATE);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_PARTICLES);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_RISK_ALERTS);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_AUTOSAVE_PROMPT);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_COMMAND_PALETTE);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_ADVANCED_FILTER);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_SMOOTH_SCROLL);

        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_THEME_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_THEME);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_ACCENT_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_ACCENT);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_TEXT_SCALE_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_TEXT_SCALE);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_DENSITY_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_DENSITY);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_AUTOSAVE_LABEL);
        MakeSettingsControl(hwnd, L"COMBOBOX", L"", CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP, IDC_SETTINGS_AUTOSAVE_INTERVAL);
        MakeSettingsControl(hwnd, L"STATIC", L"", 0, IDC_SETTINGS_SCROLL_FPS_LABEL);
        HWND scrollFps = MakeSettingsControl(hwnd, L"EDIT", L"60",
            ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP, IDC_SETTINGS_SCROLL_FPS);
        MakeSettingsControl(hwnd, L"STATIC", L"FPS", 0, IDC_SETTINGS_SCROLL_FPS_UNIT);
        SendMessageW(scrollFps, EM_SETLIMITTEXT, 3, 0);
        ConfigureEditMetrics(scrollFps);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_SHOW_STATS);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_SHOW_HINT);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_TABLE_GRID);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_RESET);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_APPLY);
        MakeSettingsControl(hwnd, L"BUTTON", L"", BS_PUSHBUTTON | BS_OWNERDRAW | WS_TABSTOP, IDC_SETTINGS_CLOSE);
        // Tab follows the same row-major order as the unified form.
        HWND after = HWND_TOP;
        for (int id : {IDC_SETTINGS_LANGUAGE, IDC_SETTINGS_FONT, IDC_SETTINGS_THEME,
            IDC_SETTINGS_ACCENT, IDC_SETTINGS_TEXT_SCALE, IDC_SETTINGS_DENSITY,
            IDC_SETTINGS_ANIMATION_LEVEL, IDC_SETTINGS_PARTICLE_LEVEL,
            IDC_SETTINGS_REPORT_TEMPLATE, IDC_SETTINGS_AUTOSAVE_INTERVAL,
            IDC_SETTINGS_SCROLL_FPS, IDC_SETTINGS_PARTICLES, IDC_SETTINGS_SMOOTH_SCROLL,
            IDC_SETTINGS_SHOW_STATS, IDC_SETTINGS_SHOW_HINT, IDC_SETTINGS_TABLE_GRID,
            IDC_SETTINGS_RISK_ALERTS, IDC_SETTINGS_AUTOSAVE_PROMPT,
            IDC_SETTINGS_COMMAND_PALETTE, IDC_SETTINGS_ADVANCED_FILTER,
            IDC_SETTINGS_RESET, IDC_SETTINGS_APPLY, IDC_SETTINGS_CLOSE}) {
            HWND control = GetDlgItem(hwnd, id);
            SetWindowPos(control, after, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW);
            after = control;
        }
        SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
        ApplySettingsLanguage(hwnd);
        ResizeSettingsLayout(hwnd);
        return 0;
    }
    case WM_SIZE:
        ResizeSettingsLayout(hwnd);
        return 0;
    case WM_VSCROLL: {
        SCROLLINFO info{sizeof(info)};
        info.fMask = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &info);
        int next = Ui().settingsScrollY;
        switch (LOWORD(wParam)) {
        case SB_LINEUP: next -= 36; break;
        case SB_LINEDOWN: next += 36; break;
        case SB_PAGEUP: next -= (int)info.nPage; break;
        case SB_PAGEDOWN: next += (int)info.nPage; break;
        case SB_THUMBTRACK: next = info.nTrackPos; break;
        case SB_TOP: next = info.nMin; break;
        case SB_BOTTOM: next = info.nMax; break;
        default: return 0;
        }
        int maximum = std::max(0, info.nMax - (int)info.nPage + 1);
        next = std::clamp(next, 0, maximum);
        if (next != Ui().settingsScrollY) {
            Ui().settingsScrollY = next;
            ResizeSettingsLayout(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
        SendMessageW(hwnd, WM_VSCROLL,
            MAKEWPARAM(GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? SB_LINEUP : SB_LINEDOWN, 0), 0);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_SETTINGS_SCROLL_FPS
            && HIWORD(wParam) == EN_KILLFOCUS) {
            NormalizeScrollFrameRateInput(hwnd);
            SaveSettingsWithFeedback(hwnd);
            return 0;
        }
        if (HIWORD(wParam) == CBN_DROPDOWN) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 1.0, 340);
            ApplyComboDropDownTheme((HWND)lParam);
            return 0;
        }
        if (HIWORD(wParam) == CBN_CLOSEUP) {
            StartAnimation((HWND)lParam, AnimChannel::ComboOpen, 0.0, 220);
            if (LOWORD(wParam) == IDC_SETTINGS_FONT && Fonts().fontComboRefreshPending) {
                ApplyFontListToSettings(hwnd);
            }
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_LANGUAGE && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            int language = (int)SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
            if (language >= 0 && language <= (int)UiLanguage::ChineseTraditionalHongKong
                && Settings().language != (UiLanguage)language) {
                Settings().language = (UiLanguage)language;
                ApplyLanguageAtomically(hwnd);
                SaveSettingsWithFeedback(hwnd);
            }
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_FONT && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if ((LOWORD(wParam) == IDC_SETTINGS_ANIMATION_LEVEL
            || LOWORD(wParam) == IDC_SETTINGS_PARTICLE_LEVEL
            || LOWORD(wParam) == IDC_SETTINGS_REPORT_TEMPLATE
            || LOWORD(wParam) == IDC_SETTINGS_THEME
            || LOWORD(wParam) == IDC_SETTINGS_ACCENT
            || LOWORD(wParam) == IDC_SETTINGS_TEXT_SCALE
            || LOWORD(wParam) == IDC_SETTINGS_DENSITY
            || LOWORD(wParam) == IDC_SETTINGS_AUTOSAVE_INTERVAL) && HIWORD(wParam) == CBN_SELCHANGE) {
            CommitComboSelectionNow((HWND)lParam);
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if ((LOWORD(wParam) == IDC_SETTINGS_PARTICLES
            || LOWORD(wParam) == IDC_SETTINGS_RISK_ALERTS
            || LOWORD(wParam) == IDC_SETTINGS_AUTOSAVE_PROMPT
            || LOWORD(wParam) == IDC_SETTINGS_COMMAND_PALETTE
            || LOWORD(wParam) == IDC_SETTINGS_ADVANCED_FILTER
            || LOWORD(wParam) == IDC_SETTINGS_SMOOTH_SCROLL) && HIWORD(wParam) == BN_CLICKED) {
            int id = LOWORD(wParam);
            HWND toggle = GetDlgItem(hwnd, id);
            bool* value = FindSettingToggleValue(id);
            if (!value) return 0;
            *value = !*value;
            bool checked = *value;
            SendMessageW(toggle, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
            if (id == IDC_SETTINGS_PARTICLES) {
                EnableWindow(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL), checked ? TRUE : FALSE);
            }
            if (id == IDC_SETTINGS_SMOOTH_SCROLL && !checked) {
                StopSmoothScrollingNow();
            }
            SaveSettingsWithFeedback(hwnd);
            InvalidateRect(toggle, nullptr, FALSE);
            return 0;
        }
        if ((LOWORD(wParam) == IDC_SETTINGS_SHOW_STATS
            || LOWORD(wParam) == IDC_SETTINGS_SHOW_HINT
            || LOWORD(wParam) == IDC_SETTINGS_TABLE_GRID) && HIWORD(wParam) == BN_CLICKED) {
            int id = LOWORD(wParam);
            bool* value = FindSettingToggleValue(id);
            if (!value) return 0;
            *value = !*value;
            bool checked = *value;
            SendMessageW((HWND)lParam, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
            if (id == IDC_SETTINGS_TABLE_GRID && Ui().list) {
                ListView_SetExtendedListViewStyleEx(Ui().list, LVS_EX_GRIDLINES,
                    checked ? LVS_EX_GRIDLINES : 0);
                InvalidateRect(Ui().list, nullptr, FALSE);
            } else if (Ui().hwnd) {
                RedrawScope redraw(Ui().hwnd);
                if (id == IDC_SETTINGS_SHOW_STATS) {
                    for (int card : {IDC_STAT_TOTAL, IDC_STAT_ATTENDANCE, IDC_STAT_ISSUES, IDC_STAT_VISIBLE})
                        ShowWindow(GetDlgItem(Ui().hwnd, card), checked ? SW_SHOW : SW_HIDE);
                } else ShowWindow(GetDlgItem(Ui().hwnd, IDC_HINT), checked ? SW_SHOW : SW_HIDE);
                ResizeLayout(Ui().hwnd);
            }
            SaveSettingsWithFeedback(hwnd);
            InvalidateRect((HWND)lParam, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_APPLY) {
            ApplySettingsFromWindow(hwnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_CLOSE) {
            HideSettingsWindow();
            return 0;
        }
        if (LOWORD(wParam) == IDC_SETTINGS_RESET) {
            std::wstring resetMsg = Tr(L"Reset all settings and delete the AppData configuration file?", L"\u91cd\u7f6e\u6240\u6709\u8bbe\u7f6e\u5e76\u5220\u9664 AppData \u914d\u7f6e\u6587\u4ef6\uff1f");
            std::wstring resetTitle = Tr(L"Reset Settings", L"\u91cd\u7f6e\u8bbe\u7f6e");
            if (ThemedMessageBox(hwnd, resetMsg, resetTitle, true) == IDYES) {
                UiTheme previousTheme = Settings().theme;
                ResetSettings();
                if (previousTheme != UiTheme::Dark || Ui().themeTransition.active) {
                    BeginThemeTransition(UiTheme::Dark);
                    ApplyVisualSettings(true);
                } else {
                    ApplyVisualSettings();
                }
                EnumChildWindows(hwnd, ApplyFontToChild, 0);
                SendMessageW(GetDlgItem(hwnd, IDC_SETTINGS_TITLE), WM_SETFONT, (WPARAM)Ui().titleFont, TRUE);
                ApplySettingsLanguage(hwnd);
                ResizeSettingsLayout(hwnd);
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        break;
    case WM_MEASUREITEM: {
        auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
        if (measure->CtlType == ODT_COMBOBOX) {
            measure->itemHeight = ComboItemHeight();
            return TRUE;
        }
        break;
    }
    case WM_DRAWITEM: {
        auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (DrawComboItem(draw)) return TRUE;
        if (DrawParticleToggleItem(draw)) return TRUE;
        if (DrawButtonItem(draw)) return TRUE;
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintAppBackground(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND control = (HWND)lParam;
        int id = GetDlgCtrlID(control);
        SetTextColor(hdc, id == IDC_SETTINGS_TITLE ? Ui().COLOR_TEXT : Ui().COLOR_MUTED);
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, Ui().COLOR_BG);
        return (LRESULT)Ui().bgBrush;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, Ui().COLOR_TEXT);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)Ui().bgBrush;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, Ui().COLOR_TEXT);
        SetBkColor(hdc, Ui().COLOR_INPUT);
        return (LRESULT)Ui().inputBrush;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            const int comboIds[] = {
                IDC_SETTINGS_LANGUAGE, IDC_SETTINGS_FONT, IDC_SETTINGS_ANIMATION_LEVEL,
                IDC_SETTINGS_PARTICLE_LEVEL, IDC_SETTINGS_REPORT_TEMPLATE, IDC_SETTINGS_THEME,
                IDC_SETTINGS_ACCENT, IDC_SETTINGS_TEXT_SCALE, IDC_SETTINGS_DENSITY,
                IDC_SETTINGS_AUTOSAVE_INTERVAL
            };
            for (int id : comboIds) {
                HWND combo = GetDlgItem(hwnd, id);
                if (combo && SendMessageW(combo, CB_GETDROPPEDSTATE, 0, 0)) {
                    SendMessageW(combo, CB_SHOWDROPDOWN, FALSE, 0);
                    return 0;
                }
            }
            HideSettingsWindow();
            return 0;
        }
        break;
    case WM_APP_ANIMATION_TICK:
        AcknowledgeAnimationTick(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_APP_ANIMATION_DONE:
        return 0;
    case WM_CLOSE:
        HideSettingsWindow();
        return 0;
    case WM_DESTROY:
        RemoveAnimationState(hwnd);
        Ui().settingsWindow = nullptr;
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HWND EnsureSettingsWindowCreated(){
    if (Ui().settingsWindow && IsWindow(Ui().settingsWindow)) return Ui().settingsWindow;
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const wchar_t* className = L"AttendanceSettingsWindow";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc{};
        wc.style = 0;
        wc.lpfnWndProc = SettingsProc;
        wc.hInstance = instance;
        wc.lpszClassName = className;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP_ICON));
        RegisterClassW(&wc);
        registered = true;
    }

    std::wstring settingsTitle = Tr(L"Settings", L"\u8bbe\u7f6e");
    Ui().settingsWindow = CreateWindowExW(
        WS_EX_CONTROLPARENT | WS_EX_COMPOSITED,
        className,
        settingsTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 560, 730,
        Ui().hwnd, nullptr, instance, nullptr
    );
    RECT work{};
    MONITORINFO monitor{sizeof(monitor)};
    GetMonitorInfoW(MonitorFromWindow(Ui().hwnd, MONITOR_DEFAULTTONEAREST), &monitor);
    work = monitor.rcWork;
    SetWindowPos(Ui().settingsWindow, nullptr, 0, 0,
        std::min(1040, std::max(320, (int)(work.right - work.left) - 16)),
        std::min(860, std::max(320, (int)(work.bottom - work.top) - 16)),
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    ApplyGlassTitleBar(Ui().settingsWindow);
    ApplyThemedControls(Ui().settingsWindow);
    CenterOwnedWindow(Ui().settingsWindow, Ui().hwnd);
    return Ui().settingsWindow;
}

void SyncSettingsWindowState(HWND hwnd){
    if (!hwnd || !IsWindow(hwnd)) return;
    RedrawScope redraw(hwnd);
    for (const auto& binding : SettingToggleBindings()) {
        SendMessageW(GetDlgItem(hwnd, binding.id), BM_SETCHECK,
            *binding.value ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    FillSettingsCombos(hwnd, false);
    SetText(GetDlgItem(hwnd, IDC_SETTINGS_SCROLL_FPS), std::to_wstring(Settings().scrollAnimationFps.load()));
    EnableWindow(GetDlgItem(hwnd, IDC_SETTINGS_PARTICLE_LEVEL), Settings().particlesEnabled ? TRUE : FALSE);
    ResizeSettingsLayout(hwnd);
    ApplyFontListToSettings(hwnd);
}

void PrewarmSettingsWindow(){
    EnsureSettingsWindowCreated();
}

void HideSettingsWindow(){
    if (!Ui().settingsWindow || !IsWindow(Ui().settingsWindow)) return;
    const int comboIds[] = {
        IDC_SETTINGS_LANGUAGE, IDC_SETTINGS_FONT, IDC_SETTINGS_ANIMATION_LEVEL,
        IDC_SETTINGS_PARTICLE_LEVEL, IDC_SETTINGS_REPORT_TEMPLATE, IDC_SETTINGS_THEME,
        IDC_SETTINGS_ACCENT, IDC_SETTINGS_TEXT_SCALE, IDC_SETTINGS_DENSITY,
        IDC_SETTINGS_AUTOSAVE_INTERVAL
    };
    for (int id : comboIds) {
        HWND combo = GetDlgItem(Ui().settingsWindow, id);
        if (combo) SendMessageW(combo, CB_SHOWDROPDOWN, FALSE, 0);
    }
    ShowWindow(Ui().settingsWindow, SW_HIDE);
    Ui().settingsScrollY = 0;
    if (Ui().hwnd && IsWindow(Ui().hwnd)) SetFocus(Ui().hwnd);
}

void ShowSettingsWindow(){
    HWND settings = EnsureSettingsWindowCreated();
    if (!settings) return;
    if (IsWindowVisible(settings)) { SetForegroundWindow(settings); return; }
    Ui().settingsScrollY = 0;
    SyncSettingsWindowState(settings);
    ShowWindow(Ui().settingsWindow, SW_SHOW);
    RedrawWindow(Ui().settingsWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_NOERASE | RDW_UPDATENOW);
    SetForegroundWindow(Ui().settingsWindow);
}

} // namespace attendance
