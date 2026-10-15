#include "settings/preferences.h"
#include "animation/scheduler.h"
#include "settings/shortcuts.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "core/encoding.h"
#include "settings/state.h"
#include "ui/state.h"
#include "core/constants.h"

namespace attendance {

std::string BoolSetting(bool value);
bool SaveSettings();
bool DecodeSettingText(const std::string& value, std::wstring& output);

int ClampSettingInt(const std::string& value, int fallback, int minValue, int maxValue){
    try {
        size_t parsedLength = 0;
        long long parsed = std::stoll(value, &parsedLength, 10);
        if (parsedLength != value.size()) return fallback;
        return static_cast<int>(std::clamp<long long>(parsed, minValue, maxValue));
    } catch (...) {
        return fallback;
    }
}

std::string BoolSetting(bool value){
    return value ? "1" : "0";
}

bool SaveSettings(){
    auto path = SettingsFilePath();
    if (path.empty()) return false;
    std::ostringstream data;
    data << "language=" << LanguageToString(Settings().language) << "\n";
    data << "advanced_personalization=1\n";
    data << "theme=" << (int)Settings().theme << "\n";
    data << "accent_tone=" << (int)Settings().accentTone << "\n";
    data << "text_scale=" << (int)Settings().textScale << "\n";
    data << "layout_density=" << (int)Settings().layoutDensity << "\n";
    data << "show_stats_cards=" << BoolSetting(Settings().showStatsCards) << "\n";
    data << "show_footer_hint=" << BoolSetting(Settings().showFooterHint) << "\n";
    data << "show_table_grid=" << BoolSetting(Settings().showTableGrid) << "\n";
    data << "autosave_interval=" << Settings().autosaveIntervalSeconds << "\n";
    data << "font=" << WideToUtf8(Settings().fontFamily) << "\n";
    data << "default_save_dir=" << WideToUtf8(Settings().defaultSaveDir) << "\n";
    data << "particles=" << (Settings().particlesEnabled ? "1" : "0") << "\n";
    data << "animation_level=" << (int)Settings().animationLevel << "\n";
    data << "particle_level=" << (int)Settings().particleLevel << "\n";
    data << "report_template=" << (int)Settings().reportTemplate << "\n";
    data << "risk_alerts=" << BoolSetting(Settings().riskAlertsEnabled) << "\n";
    data << "autosave_prompt=" << BoolSetting(Settings().autosavePromptEnabled) << "\n";
    data << "command_palette=" << BoolSetting(Settings().commandPaletteEnabled) << "\n";
    data << "advanced_filter=" << BoolSetting(Settings().advancedFilterEnabled) << "\n";
    data << "smooth_scrolling=" << BoolSetting(Settings().smoothScrollingEnabled) << "\n";
    data << "scroll_animation_fps=" << Settings().scrollAnimationFps.load() << "\n";
    data << "shortcut_save=" << WideToUtf8(Settings().shortcutSave) << "\n";
    data << "shortcut_import=" << WideToUtf8(Settings().shortcutImport) << "\n";
    data << "shortcut_undo=" << WideToUtf8(Settings().shortcutUndo) << "\n";
    data << "shortcut_redo=" << WideToUtf8(Settings().shortcutRedo) << "\n";
    data << "shortcut_command=" << WideToUtf8(Settings().shortcutCommand) << "\n";
    data << "shortcut_fullscreen=" << WideToUtf8(Settings().shortcutFullscreen) << "\n";
    return WriteBinaryFileAtomically(path, data.str());
}

bool ParseBoolSetting(const std::string& value, bool fallback){
    if (value == "1") return true;
    if (value == "0") return false;
    return fallback;
}

bool DecodeSettingText(const std::string& value, std::wstring& output){
    return DecodeUtf8Field(value, MAX_SETTINGS_FILE_BYTES, output);
}

void LoadSettings(){
    auto path = SettingsFilePath();
    if (!FileExistsNoThrow(path)) return;

    std::string settingsData;
    if (!ReadBinaryFileLimited(path, settingsData, MAX_SETTINGS_FILE_BYTES)) return;
    std::istringstream file(settingsData);
    SettingsSnapshot settings;

    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        if (key == "language") settings.language = LanguageFromString(value);
        else if (key == "advanced_personalization") continue; // Legacy gate, all settings are now public.
        else if (key == "theme") settings.theme = (value == "light" || value == "1") ? UiTheme::Light : UiTheme::Dark;
        else if (key == "accent_tone") settings.accentTone = (AccentTone)ClampSettingInt(value, 0, 0, 2);
        else if (key == "text_scale") settings.textScale = (TextScale)ClampSettingInt(value, 1, 0, 2);
        else if (key == "layout_density") settings.layoutDensity = (LayoutDensity)ClampSettingInt(value, 1, 0, 2);
        else if (key == "show_stats_cards") settings.showStatsCards = ParseBoolSetting(value, true);
        else if (key == "show_footer_hint") settings.showFooterHint = ParseBoolSetting(value, true);
        else if (key == "show_table_grid") settings.showTableGrid = ParseBoolSetting(value, true);
        else if (key == "autosave_interval") {
            int seconds = ClampSettingInt(value, 30, 0, 300);
            settings.autosaveIntervalSeconds = seconds == 0 || seconds == 30 || seconds == 60 || seconds == 300 ? seconds : 30;
        }
        else if (key == "font" && !value.empty()) {
            std::wstring decoded;
            if (DecodeSettingText(value, decoded) && !decoded.empty()) settings.fontFamily = std::move(decoded);
        }
        else if (key == "default_save_dir") {
            std::wstring decoded;
            if (DecodeSettingText(value, decoded)) settings.defaultSaveDir = std::move(decoded);
        }
        else if (key == "particles") settings.particlesEnabled = ParseBoolSetting(value, true);
        else if (key == "animation_level") settings.animationLevel = (AnimationLevel)ClampSettingInt(value, (int)AnimationLevel::Advanced, 0, 2);
        else if (key == "particle_level") settings.particleLevel = (ParticleLevel)ClampSettingInt(value, (int)ParticleLevel::Medium, 0, 2);
        else if (key == "report_template") settings.reportTemplate = (ReportTemplate)ClampSettingInt(value, (int)ReportTemplate::Complete, 0, 3);
        else if (key == "risk_alerts") settings.riskAlertsEnabled = ParseBoolSetting(value, true);
        else if (key == "autosave_prompt") settings.autosavePromptEnabled = ParseBoolSetting(value, true);
        else if (key == "command_palette") settings.commandPaletteEnabled = ParseBoolSetting(value, true);
        else if (key == "advanced_filter") settings.advancedFilterEnabled = ParseBoolSetting(value, true);
        else if (key == "smooth_scrolling") settings.smoothScrollingEnabled = ParseBoolSetting(value, true);
        else if (key == "scroll_animation_fps") settings.scrollAnimationFps = ClampSettingInt(value, 60, 60, 360);
        else if (key.rfind("shortcut_", 0) == 0) {
            std::wstring decoded;
            if (!DecodeSettingText(value, decoded)) continue;
            if (key == "shortcut_save") settings.shortcutSave = NormalizeShortcut(decoded, L"Ctrl+S");
            else if (key == "shortcut_import") settings.shortcutImport = NormalizeShortcut(decoded, L"Ctrl+O");
            else if (key == "shortcut_undo") settings.shortcutUndo = NormalizeShortcut(decoded, L"Ctrl+Z");
            else if (key == "shortcut_redo") settings.shortcutRedo = NormalizeShortcut(decoded, L"Ctrl+Y");
            else if (key == "shortcut_command") settings.shortcutCommand = NormalizeShortcut(decoded, L"Ctrl+K");
            else if (key == "shortcut_fullscreen") settings.shortcutFullscreen = NormalizeShortcut(decoded, L"F11");
        }
    }

    Settings().language = settings.language;
    Settings().theme = settings.theme;
    Settings().accentTone = settings.accentTone;
    Settings().textScale = settings.textScale;
    Settings().layoutDensity = settings.layoutDensity;
    Settings().fontFamily = std::move(settings.fontFamily);
    Settings().defaultSaveDir = std::move(settings.defaultSaveDir);
    Settings().animationLevel = settings.animationLevel;
    Settings().particleLevel = settings.particleLevel;
    Settings().reportTemplate = settings.reportTemplate;
    Settings().particlesEnabled = settings.particlesEnabled;
    Settings().riskAlertsEnabled = settings.riskAlertsEnabled;
    Settings().autosavePromptEnabled = settings.autosavePromptEnabled;
    Settings().commandPaletteEnabled = settings.commandPaletteEnabled;
    Settings().advancedFilterEnabled = settings.advancedFilterEnabled;
    Settings().smoothScrollingEnabled = settings.smoothScrollingEnabled;
    Settings().showStatsCards = settings.showStatsCards;
    Settings().showFooterHint = settings.showFooterHint;
    Settings().showTableGrid = settings.showTableGrid;
    Settings().autosaveIntervalSeconds = settings.autosaveIntervalSeconds;
    Settings().scrollAnimationFps.store(settings.scrollAnimationFps);
    Settings().shortcutSave = std::move(settings.shortcutSave);
    Settings().shortcutImport = std::move(settings.shortcutImport);
    Settings().shortcutUndo = std::move(settings.shortcutUndo);
    Settings().shortcutRedo = std::move(settings.shortcutRedo);
    Settings().shortcutCommand = std::move(settings.shortcutCommand);
    Settings().shortcutFullscreen = std::move(settings.shortcutFullscreen);
}

void ResetSettings(){
    auto path = SettingsFilePath();
    if (!path.empty()) {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    Settings().language = UiLanguage::English;
    Settings().theme = UiTheme::Dark;
    Settings().accentTone = AccentTone::Neutral;
    Settings().textScale = TextScale::Standard;
    Settings().layoutDensity = LayoutDensity::Comfortable;
    Settings().fontFamily = L"Segoe UI";
    Settings().defaultSaveDir.clear();
    Settings().particlesEnabled = true;
    Settings().animationLevel = AnimationLevel::Advanced;
    Settings().particleLevel = ParticleLevel::Medium;
    Settings().reportTemplate = ReportTemplate::Complete;
    Settings().riskAlertsEnabled = true;
    Settings().autosavePromptEnabled = true;
    Settings().commandPaletteEnabled = true;
    Settings().advancedFilterEnabled = true;
    Settings().smoothScrollingEnabled = true;
    Settings().showStatsCards = true;
    Settings().showFooterHint = true;
    Settings().showTableGrid = true;
    Settings().autosaveIntervalSeconds = 30;
    Settings().scrollAnimationFps.store(60);
    SignalAnimationScheduler();
    Settings().shortcutSave = L"Ctrl+S";
    Settings().shortcutImport = L"Ctrl+O";
    Settings().shortcutUndo = L"Ctrl+Z";
    Settings().shortcutRedo = L"Ctrl+Y";
    Settings().shortcutCommand = L"Ctrl+K";
    Settings().shortcutFullscreen = L"F11";
}

bool SaveSettingsWithFeedback(HWND owner){
    if (SaveSettings()) return true;
    ThemedMessageBox(owner ? owner : Ui().hwnd,
        Tr(L"Could not save the settings file. Your changes will remain active only for this session.",
            L"\u65e0\u6cd5\u4fdd\u5b58\u8bbe\u7f6e\u6587\u4ef6\u3002\u672c\u6b21\u66f4\u6539\u53ea\u4f1a\u5728\u5f53\u524d\u4f1a\u8bdd\u4e2d\u751f\u6548\u3002"),
        Tr(L"Settings could not be saved", L"\u8bbe\u7f6e\u65e0\u6cd5\u4fdd\u5b58"), false);
    return false;
}

} // namespace attendance
