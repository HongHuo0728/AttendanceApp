#pragma once

#include "core/types.h"

namespace attendance {

std::wstring GetText(HWND hwnd);
void SetText(HWND hwnd, const std::wstring& text);
void SetAccessibleName(HWND hwnd, const std::wstring& name);
void ConfigureEditMetrics(HWND hwnd);

} // namespace attendance
