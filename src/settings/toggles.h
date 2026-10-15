#pragma once

#include "core/types.h"

namespace attendance {

bool* FindSettingToggleValue(int id);
const std::array<SettingToggleBinding, 9>& SettingToggleBindings();

} // namespace attendance
