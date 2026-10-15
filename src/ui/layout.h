#pragma once

#include "core/types.h"

namespace attendance {

void ToggleFullscreen(HWND hwnd);
void ResizeLayout(HWND hwnd);
void ScrollMainWindow(HWND hwnd, int bar, int code, int wheelDelta = 0);

} // namespace attendance
