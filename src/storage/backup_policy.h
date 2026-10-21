#pragma once
#include "core/types.h"
namespace attendance {
bool ReserveBackupSpace(const std::filesystem::path &directory,
                        uintmax_t incoming, bool ordinary,
                        uintmax_t budget = 128ull * 1024 * 1024,
                        size_t maximum = 10);
}
