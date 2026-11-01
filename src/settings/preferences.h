#pragma once

#include "core/types.h"

namespace attendance {

int ClampSettingInt(const std::string& value, int fallback, int minValue, int maxValue);
bool ParseBoolSetting(const std::string& value, bool fallback);
void LoadSettings();
SettingsSnapshot CaptureSettings();
void ApplySettingsSnapshot(const SettingsSnapshot& settings);
bool WriteSettingsSnapshot(const SettingsSnapshot& settings);
bool ResetSettings();
bool SaveSettingsWithFeedback(HWND owner = nullptr);

} // namespace attendance
