#include "settings/preferences.h"
#include "animation/scheduler.h"
#include "settings/shortcuts.h"
#include "ui/dialogs.h"
#include "localization/strings.h"
#include "storage/files.h"
#include "core/encoding.h"
#include "settings/state.h"
#include "ui/state.h"
#include "ui/settings_window.h"
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

SettingsSnapshot CaptureSettings(){
    SettingsSnapshot result;
    result.language = Settings().language;
    result.theme = Settings().theme;
    result.accentTone = Settings().accentTone;
    result.textScale = Settings().textScale;
    result.layoutDensity = Settings().layoutDensity;
    result.animationLevel = Settings().animationLevel;
    result.particleLevel = Settings().particleLevel;
    result.reportTemplate = Settings().reportTemplate;
    result.particlesEnabled = Settings().particlesEnabled;
    result.riskAlertsEnabled = Settings().riskAlertsEnabled;
    result.autosavePromptEnabled = Settings().autosavePromptEnabled;
    result.commandPaletteEnabled = Settings().commandPaletteEnabled;
    result.advancedFilterEnabled = Settings().advancedFilterEnabled;
    result.smoothScrollingEnabled = Settings().smoothScrollingEnabled;
    result.showStatsCards = Settings().showStatsCards;
    result.showFooterHint = Settings().showFooterHint;
    result.showTableGrid = Settings().showTableGrid;
    result.autosaveIntervalSeconds = Settings().autosaveIntervalSeconds;
    result.scrollAnimationFps = Settings().scrollAnimationFps.load();
    return result;
}

bool WriteSettingsSnapshot(const SettingsSnapshot& settings){
    auto path = SettingsFilePath();
    if (path.empty()) return false;
    std::ostringstream data;
    data << "language=" << LanguageToString(settings.language) << "\n";
    data << "advanced_personalization=1\n";
    data << "theme=" << (int)settings.theme << "\n";
    data << "accent_tone=" << (int)settings.accentTone << "\n";
    data << "text_scale=" << (int)settings.textScale << "\n";
    data << "layout_density=" << (int)settings.layoutDensity << "\n";
    data << "show_stats_cards=" << BoolSetting(settings.showStatsCards) << "\n";
    data << "show_footer_hint=" << BoolSetting(settings.showFooterHint) << "\n";
    data << "show_table_grid=" << BoolSetting(settings.showTableGrid) << "\n";
    data << "autosave_interval=" << settings.autosaveIntervalSeconds << "\n";
    data << "font=" << WideToUtf8(settings.fontFamily) << "\n";
    data << "default_save_dir=" << WideToUtf8(settings.defaultSaveDir) << "\n";
    data << "particles=" << (settings.particlesEnabled ? "1" : "0") << "\n";
    data << "animation_level=" << (int)settings.animationLevel << "\n";
    data << "particle_level=" << (int)settings.particleLevel << "\n";
    data << "report_template=" << (int)settings.reportTemplate << "\n";
    data << "risk_alerts=" << BoolSetting(settings.riskAlertsEnabled) << "\n";
    data << "autosave_prompt=" << BoolSetting(settings.autosavePromptEnabled) << "\n";
    data << "command_palette=" << BoolSetting(settings.commandPaletteEnabled) << "\n";
    data << "advanced_filter=" << BoolSetting(settings.advancedFilterEnabled) << "\n";
    data << "smooth_scrolling=" << BoolSetting(settings.smoothScrollingEnabled) << "\n";
    data << "scroll_animation_fps=" << settings.scrollAnimationFps << "\n";
    data << "shortcut_save=" << WideToUtf8(settings.shortcutSave) << "\n";
    data << "shortcut_import=" << WideToUtf8(settings.shortcutImport) << "\n";
    data << "shortcut_undo=" << WideToUtf8(settings.shortcutUndo) << "\n";
    data << "shortcut_redo=" << WideToUtf8(settings.shortcutRedo) << "\n";
    data << "shortcut_command=" << WideToUtf8(settings.shortcutCommand) << "\n";
    data << "shortcut_fullscreen=" << WideToUtf8(settings.shortcutFullscreen) << "\n";
    return WriteBinaryFileAtomically(path, data.str());
}

bool SaveSettings(){ return WriteSettingsSnapshot(CaptureSettings()); }

bool ParseBoolSetting(const std::string& value, bool fallback){
    if (value == "1") return true;
    if (value == "0") return false;
    return fallback;
}

bool DecodeSettingText(const std::string& value, std::wstring& output){
    return DecodeUtf8Field(value, MAX_SETTINGS_FILE_BYTES, output);
}

void ApplySettingsSnapshot(const SettingsSnapshot& settings){
    Settings().language = settings.language;
    Settings().theme = settings.theme;
    Settings().accentTone = settings.accentTone;
    Settings().textScale = settings.textScale;
    Settings().layoutDensity = settings.layoutDensity;
    Settings().fontFamily = (settings.fontFamily);
    Settings().defaultSaveDir = (settings.defaultSaveDir);
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
    Settings().shortcutSave = (settings.shortcutSave);
    Settings().shortcutImport = (settings.shortcutImport);
    Settings().shortcutUndo = (settings.shortcutUndo);
    Settings().shortcutRedo = (settings.shortcutRedo);
    Settings().shortcutCommand = (settings.shortcutCommand);
    Settings().shortcutFullscreen = (settings.shortcutFullscreen);
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

    ApplySettingsSnapshot(settings);
}

bool ResetSettings(){
    SettingsSnapshot defaults;
    if (!WriteSettingsSnapshot(defaults)) return false;
    ApplySettingsSnapshot(defaults);
    SignalAnimationScheduler();
    return true;
}

bool SaveSettingsWithFeedback(HWND owner){
    bool saved = SaveSettings();
    if (owner && owner == Ui().settingsWindow) { SetSettingsSaveResult(saved); return saved; }
    if (saved) return true;
    ThemedMessageBox(owner ? owner : Ui().hwnd,
        Tr(L"Could not save the settings file. Your changes will remain active only for this session.",
            L"\u65e0\u6cd5\u4fdd\u5b58\u8bbe\u7f6e\u6587\u4ef6\u3002\u672c\u6b21\u66f4\u6539\u53ea\u4f1a\u5728\u5f53\u524d\u4f1a\u8bdd\u4e2d\u751f\u6548\u3002"),
        Tr(L"Settings could not be saved", L"\u8bbe\u7f6e\u65e0\u6cd5\u4fdd\u5b58"), false);
    return false;
}

} // namespace attendance
