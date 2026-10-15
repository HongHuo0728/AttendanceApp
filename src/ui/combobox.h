#pragma once

#include "core/types.h"

namespace attendance {

std::wstring GetComboSelectedText(HWND combo);
void CommitComboSelectionNow(HWND combo);
void StopSmoothScrollingNow();
void ApplyComboDropDownTheme(HWND combo);
void EnableComboPaint(HWND combo);

} // namespace attendance
