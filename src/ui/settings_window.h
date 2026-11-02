#pragma once

#include "core/types.h"

namespace attendance {

HWND SettingsControl(HWND window, int id);
bool IsSettingsControl(HWND control);
void ApplyFontListToSettings(HWND hwnd);
void EnsureSettingsControlVisible(HWND control);
void ApplyLanguageAtomically(HWND settingsWindow);
void ResetAppearanceValues();
void PrewarmSettingsWindow();
void ShowSettingsWindow();
void HideSettingsWindow();
void SetSettingsSaveResult(bool saved);

} // namespace attendance
