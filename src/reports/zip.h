#pragma once

#include "core/types.h"

namespace attendance {

bool WriteZipStore(const std::filesystem::path& path, std::vector<ZipEntry> entries);

} // namespace attendance
