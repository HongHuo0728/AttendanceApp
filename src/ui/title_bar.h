#pragma once

#include "core/types.h"

namespace attendance {

void ApplyGlassTitleBar(HWND hwnd);
void RefreshOpenAppTitleBars(bool configureBackdrop = false);
void CenterOwnedWindow(HWND window, HWND owner);

} // namespace attendance
