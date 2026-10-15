#pragma once

#include "core/types.h"

namespace attendance {

bool IsRenderedPaletteDark();
void ApplyThemePalette();
void PaintAppBackground(HWND hwnd, HDC hdc);
void PaintAppBackgroundSlice(HWND child, HDC hdc);
BOOL CALLBACK ApplyFontToChild(HWND child, LPARAM);
void RecreateFonts();
void ApplyThemedControls(HWND root);
void BeginThemeTransition(UiTheme targetTheme);
void UpdateThemeTransitionFrame();
void ApplyDarkMode(HWND hwnd);

} // namespace attendance
