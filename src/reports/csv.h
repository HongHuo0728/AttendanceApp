#pragma once

#include "core/types.h"

namespace attendance {

void ExportCsv();
// Capture on the UI thread; the returned task owns its data and localized labels.
std::function<bool()> PrepareCsvExport(const std::wstring& path);

} // namespace attendance
