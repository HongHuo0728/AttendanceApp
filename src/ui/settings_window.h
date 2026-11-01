#pragma once

#include "core/types.h"

namespace attendance {

void ApplyFontListToSettings(HWND hwnd);
void EnsureSettingsControlVisible(HWND control);
void ApplyLanguageAtomically(HWND settingsWindow);
void ResetAppearanceValues();
void PrewarmSettingsWindow();
void ShowSettingsWindow();
void HideSettingsWindow();
void SetSettingsSaveResult(bool saved);

} // namespace attendance
