#pragma once
#include "core/types.h"

namespace attendance {
// Closing native popups commit immediately; this non-interactive surface is decoration only.
void ShowPopupExit(HWND source, HWND owner, HDC frame, int width, int height, bool upward);
void StopPopupExit();
} // namespace attendance
