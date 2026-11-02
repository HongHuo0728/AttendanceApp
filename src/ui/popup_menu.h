#pragma once

#include "core/types.h"

namespace attendance {

int WheelScrollLineCount();
struct ThemedMenuOptions {
  bool comboChoice = false;
  int selectedIndex = -1;
};
int ShowThemedPopupMenu(HWND button, const std::vector<ThemedMenuItem> &items,
                        const ThemedMenuOptions &options = {});

} // namespace attendance
