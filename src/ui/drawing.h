#pragma once

#include "core/types.h"

namespace attendance {

void DrawSoftDivider(HDC hdc, int left, int right, int y);
bool DrawParticleToggleItem(const DRAWITEMSTRUCT* draw);
bool DrawButtonItem(const DRAWITEMSTRUCT* draw);
bool DrawComboItem(const DRAWITEMSTRUCT* draw);
void PaintComboClosed(HWND hwnd, HDC hdc);
void EnableHeaderPaint(HWND header);
void EnableStatsPaint(HWND stats);

} // namespace attendance
