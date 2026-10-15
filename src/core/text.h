#pragma once

#include "core/types.h"

namespace attendance {

std::wstring LowerText(std::wstring value);
bool ContainsText(const std::wstring& haystack, const std::wstring& needle);
std::wstring TrimWide(std::wstring value);
std::wstring UpperText(std::wstring value);
std::wstring NormalizeSingleLineText(const std::wstring& text);

} // namespace attendance
