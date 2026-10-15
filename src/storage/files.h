#pragma once

#include "core/types.h"

namespace attendance {

std::filesystem::path SettingsFilePath();
bool WriteStreamFileAtomically(
    const std::filesystem::path& path,
    const std::function<bool(std::ofstream&)>& writer);
bool WriteBinaryFileAtomically(const std::filesystem::path& path, const std::string& data);
bool ReadBinaryFileLimited(const std::filesystem::path& path, std::string& output, uintmax_t maximumBytes);
bool FileExistsNoThrow(const std::filesystem::path& path);
std::filesystem::path AppDataFilePath(const wchar_t* fileName);

} // namespace attendance
