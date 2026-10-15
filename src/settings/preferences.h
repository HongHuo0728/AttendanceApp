#pragma once

#include "core/types.h"

namespace attendance {

int ClampSettingInt(const std::string& value, int fallback, int minValue, int maxValue);
bool ParseBoolSetting(const std::string& value, bool fallback);
void LoadSettings();
void ResetSettings();
bool SaveSettingsWithFeedback(HWND owner = nullptr);

} // namespace attendance
