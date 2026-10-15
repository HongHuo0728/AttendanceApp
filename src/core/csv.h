#pragma once

#include "core/types.h"

namespace attendance {

std::string CsvCell(const std::wstring& value);
std::string TsvCell(const std::wstring& value);
std::vector<std::string> ParseCsvLine(const std::string& line);
std::string TrimUtf8(std::string value);
bool LooksLikeNameHeader(const std::wstring& value);

} // namespace attendance
