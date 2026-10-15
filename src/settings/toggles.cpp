#include "settings/toggles.h"
#include "settings/state.h"
#include "core/constants.h"

namespace attendance {



const std::array<SettingToggleBinding, 9>& SettingToggleBindings() {
static const std::array<SettingToggleBinding, 9> bindings{{
    {IDC_SETTINGS_PARTICLES, &Settings().particlesEnabled},
    {IDC_SETTINGS_RISK_ALERTS, &Settings().riskAlertsEnabled},
    {IDC_SETTINGS_AUTOSAVE_PROMPT, &Settings().autosavePromptEnabled},
    {IDC_SETTINGS_COMMAND_PALETTE, &Settings().commandPaletteEnabled},
    {IDC_SETTINGS_ADVANCED_FILTER, &Settings().advancedFilterEnabled},
    {IDC_SETTINGS_SMOOTH_SCROLL, &Settings().smoothScrollingEnabled},
    {IDC_SETTINGS_SHOW_STATS, &Settings().showStatsCards},
    {IDC_SETTINGS_SHOW_HINT, &Settings().showFooterHint},
    {IDC_SETTINGS_TABLE_GRID, &Settings().showTableGrid},
}};
    return bindings;
}

bool* FindSettingToggleValue(int id){
    for (const auto& binding : SettingToggleBindings()) {
        if (binding.id == id) return binding.value;
    }
    return nullptr;
}

} // namespace attendance
