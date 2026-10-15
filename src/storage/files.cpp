#include "storage/files.h"
#include "core/constants.h"

namespace attendance {

std::filesystem::path ApplicationDataDirectory();
bool EnsureParentDirectory(const std::filesystem::path& path);
std::filesystem::path TemporarySiblingPath(const std::filesystem::path& path);

std::filesystem::path ApplicationDataDirectory(){
    try {
        constexpr DWORD kMaximumEnvironmentPathLength = 32767;
        DWORD overrideLength = GetEnvironmentVariableW(L"ATTENDANCEAPP_DATA_DIR", nullptr, 0);
        if (overrideLength > 1 && overrideLength <= kMaximumEnvironmentPathLength) {
            std::vector<wchar_t> overridePath(overrideLength, L'\0');
            DWORD copied = GetEnvironmentVariableW(
                L"ATTENDANCEAPP_DATA_DIR", overridePath.data(), overrideLength);
            if (copied > 0 && copied < overrideLength) {
                return std::filesystem::path(overridePath.data());
            }
        }

        PWSTR roamingPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(
                FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &roamingPath))) {
            std::filesystem::path result;
            try {
                result = std::filesystem::path(roamingPath) / L"AttendanceApp";
            } catch (...) {
                CoTaskMemFree(roamingPath);
                throw;
            }
            CoTaskMemFree(roamingPath);
            return result;
        }
    } catch (const std::filesystem::filesystem_error&) {
        return {};
    }
    return {};
}

std::filesystem::path SettingsFilePath(){
    auto directory = ApplicationDataDirectory();
    return directory.empty() ? std::filesystem::path{} : directory / L"settings.ini";
}

bool EnsureParentDirectory(const std::filesystem::path& path){
    try {
        const auto parent = path.parent_path();
        if (parent.empty()) return true;
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        return !ec;
    } catch (const std::filesystem::filesystem_error&) {
        return false;
    }
}

std::filesystem::path TemporarySiblingPath(const std::filesystem::path& path){
    static std::atomic_uint64_t sequence{0};
    std::wstring suffix = L".tmp-" + std::to_wstring(GetCurrentProcessId()) + L"-"
        + std::to_wstring(GetTickCount64()) + L"-" + std::to_wstring(sequence.fetch_add(1));
    return path.parent_path() / (path.filename().wstring() + suffix);
}

bool WriteStreamFileAtomically(
    const std::filesystem::path& path,
    const std::function<bool(std::ofstream&)>& writer){
    struct TemporaryFile {
        std::filesystem::path path;
        ~TemporaryFile() {
            if (!path.empty()) { std::error_code ec; std::filesystem::remove(path, ec); }
        }
    } cleanup;
    try {
        if (path.empty() || !EnsureParentDirectory(path)) return false;
        const auto temporary = TemporarySiblingPath(path);
        cleanup.path = temporary;
        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            if (!file) return false;
            bool writerSucceeded = writer(file);
            file.flush();
            if (!writerSucceeded || !file) {
                file.close();
                std::error_code ec;
                std::filesystem::remove(temporary, ec);
                return false;
            }
            file.close();
            if (file.fail()) {
                std::error_code ec;
                std::filesystem::remove(temporary, ec);
                return false;
            }
        }

        // Keep replacement commits ordered even when writers prepare files concurrently.
        static std::mutex commitMutex;
        std::lock_guard<std::mutex> commitLock(commitMutex);
        if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            std::error_code ec;
            std::filesystem::remove(temporary, ec);
            return false;
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool WriteBinaryFileAtomically(const std::filesystem::path& path, const std::string& data){
    return WriteStreamFileAtomically(path, [&](std::ofstream& file) {
        file.write(data.data(), static_cast<std::streamsize>(data.size()));
        return file.good();
    });
}

bool ReadBinaryFileLimited(const std::filesystem::path& path, std::string& output, uintmax_t maximumBytes){
    output.clear();
    try {
        std::error_code ec;
        const uintmax_t size = std::filesystem::file_size(path, ec);
        if (ec || size > maximumBytes || size > SIZE_MAX
            || size > static_cast<uintmax_t>((std::numeric_limits<std::streamsize>::max)())) return false;
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;
        output.resize(static_cast<size_t>(size));
        if (size) file.read(output.data(), static_cast<std::streamsize>(size));
        if (!file || file.peek() != std::char_traits<char>::eof() || file.bad()) {
            output.clear();
            return false;
        }
        return true;
    } catch (const std::exception&) {
        output.clear();
        return false;
    }
}

bool FileExistsNoThrow(const std::filesystem::path& path){
    if (path.empty()) return false;
    std::error_code ec;
    return std::filesystem::exists(path, ec) && !ec;
}

std::filesystem::path AppDataFilePath(const wchar_t* fileName){
    try {
        auto directory = ApplicationDataDirectory();
        if (directory.empty()) return {};
        std::filesystem::path result = directory / fileName;
        std::error_code ec;
        std::filesystem::create_directories(result.parent_path(), ec);
        if (ec) result.clear();
        return result;
    } catch (const std::filesystem::filesystem_error&) {
        return {};
    }
}

} // namespace attendance
