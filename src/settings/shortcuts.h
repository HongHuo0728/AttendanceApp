#pragma once

#include "core/types.h"

namespace attendance {

std::wstring NormalizeShortcut(const std::wstring& value, const std::wstring& fallback);
bool ShortcutMatches(const std::wstring& shortcut, WPARAM key);
std::wstring ShortcutConfigText();
bool ApplyShortcutConfigText(const std::wstring& config);
bool ExecuteMainShortcut(WPARAM key);
bool HandleMainShortcutMessage(const MSG& msg);

} // namespace attendance
