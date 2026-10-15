#pragma once

#include "core/types.h"

namespace attendance {

int WheelScrollLineCount();
int ShowThemedPopupMenu(HWND button, const std::vector<ThemedMenuItem>& items);

} // namespace attendance
