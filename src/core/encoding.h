#pragma once

#include "core/types.h"

namespace attendance {

std::string WideToUtf8(const std::wstring& input);
std::wstring Utf8ToWide(std::string_view input);
std::string Escape(const std::string& input);
bool DecodeUtf8Field(std::string_view decoded, size_t maximumBytes, std::wstring& output);
bool DecodeEscapedUtf8Field(std::string_view encoded, size_t maximumBytes, std::wstring& output);
bool SplitRecordFields(std::string_view line, std::array<std::string_view, 4>& parts);
std::string HtmlCell(const std::wstring& value);
std::string XmlCell(const std::wstring& value);

} // namespace attendance
