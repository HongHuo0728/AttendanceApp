#pragma once

#include "core/types.h"

namespace attendance {

bool IsCoreParticleButton(int id);
void TriggerButtonFeedback(HWND hwnd, POINT origin);
void DrawButtonEffects(const DRAWITEMSTRUCT* draw, const RECT& buttonRc, COLORREF baseFill);

} // namespace attendance
