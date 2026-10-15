#pragma once

#include "core/types.h"

namespace attendance {

int ThemedMessageBox(HWND owner, const std::wstring& message, const std::wstring& title, bool yesNo);
bool ConfirmDiscardUnsaved(const std::wstring& actionText);
bool PromptText(const std::wstring& title, const std::wstring& prompt, std::wstring& value);
void ShowMessage(const std::wstring& text, const std::wstring& title = L"AttendanceApp");

} // namespace attendance
