#pragma once

#include "core/types.h"

namespace attendance {

HFONT CreateUiFont(int height, int weight);
int ComboItemHeight();
void RequestFontEnumeration();
void StopFontEnumerationThread();

} // namespace attendance
