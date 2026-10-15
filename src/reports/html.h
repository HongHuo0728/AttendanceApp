#pragma once

#include "core/types.h"

namespace attendance {

void ExportPrintHtml();
std::function<bool()> PrepareHtmlExport(const std::wstring& path);

} // namespace attendance
