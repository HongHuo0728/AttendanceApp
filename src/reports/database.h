#pragma once

#include "core/types.h"

namespace attendance {

void ExportDatabaseMirror();
std::function<bool()> PrepareDatabaseExport(const std::filesystem::path& path);

} // namespace attendance
