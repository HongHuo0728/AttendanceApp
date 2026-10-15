#pragma once

#include "core/types.h"

namespace attendance {

struct SettingsState {
    bool particlesEnabled = true;
    UiLanguage language = UiLanguage::English;
    UiTheme theme = UiTheme::Dark;
    AccentTone accentTone = AccentTone::Neutral;
    TextScale textScale = TextScale::Standard;
    LayoutDensity layoutDensity = LayoutDensity::Comfortable;
    std::wstring fontFamily = L"Segoe UI";
    std::wstring defaultSaveDir;
    AnimationLevel animationLevel = AnimationLevel::Advanced;
    ParticleLevel particleLevel = ParticleLevel::Medium;
    ReportTemplate reportTemplate = ReportTemplate::Complete;
    bool riskAlertsEnabled = true;
    bool autosavePromptEnabled = true;
    bool commandPaletteEnabled = true;
    bool advancedFilterEnabled = true;
    bool smoothScrollingEnabled = true;
    std::atomic_int scrollAnimationFps{60};
    bool showStatsCards = true;
    bool showFooterHint = true;
    bool showTableGrid = true;
    int autosaveIntervalSeconds = 30;
    std::wstring shortcutSave = L"Ctrl+S";
    std::wstring shortcutImport = L"Ctrl+O";
    std::wstring shortcutUndo = L"Ctrl+Z";
    std::wstring shortcutRedo = L"Ctrl+Y";
    std::wstring shortcutCommand = L"Ctrl+K";
    std::wstring shortcutFullscreen = L"F11";
};

SettingsState& Settings();

} // namespace attendance
