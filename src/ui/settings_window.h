#pragma once

#include "core/types.h"

namespace attendance {

void ApplyFontListToSettings(HWND hwnd);
void ApplyLanguageAtomically(HWND settingsWindow);
void ResetAdvancedPersonalizationValues();
void PrewarmSettingsWindow();
void ShowSettingsWindow();

} // namespace attendance
