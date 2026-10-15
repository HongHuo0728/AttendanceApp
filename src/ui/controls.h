#pragma once

#include "core/types.h"

namespace attendance {

void EnableMouseWheelForward(HWND hwnd);
void RedrawAnimatedControl(HWND hwnd);
HWND MakeControl(const wchar_t* cls, const wchar_t* text, DWORD style, int id);
HWND MakeButton(const wchar_t* text, int id);
HWND MakeSettingsControl(HWND parent, const wchar_t* cls, const wchar_t* text, DWORD style, int id);

} // namespace attendance
