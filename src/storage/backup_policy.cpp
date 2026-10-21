#include "storage/backup_policy.h"
namespace attendance {
bool ReserveBackupSpace(const std::filesystem::path &dir, uintmax_t incoming,
                        bool ordinary, uintmax_t budget, size_t maximum) {
  if (incoming > budget || maximum == 0)
    return false;
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec)
    return false;
  struct Item {
    std::filesystem::path path;
    uintmax_t size;
    std::filesystem::file_time_type time;
  };
  std::vector<Item> removable;
  uintmax_t total = 0;
  std::filesystem::directory_iterator it(dir, ec), end;
  while (!ec && it != end) {
    auto entry = *it;
    auto status = entry.symlink_status(ec);
    if (ec)
      return false;
    if (std::filesystem::is_symlink(status))
      return false;
    if (std::filesystem::is_regular_file(status) &&
        entry.path().extension() == L".attd") {
      auto size = entry.file_size(ec);
      if (ec || total > (std::numeric_limits<uintmax_t>::max)() - size)
        return false;
      total += size;
      if (entry.path().filename().wstring().rfind(L"backup-", 0) == 0) {
        auto time = entry.last_write_time(ec);
        if (ec)
          return false;
        removable.push_back({entry.path(), size, time});
      }
    }
    it.increment(ec);
  }
  if (ec)
    return false;
  std::sort(removable.begin(), removable.end(),
            [](const auto &a, const auto &b) { return a.time < b.time; });
  size_t remaining = removable.size();
  for (const auto &item : removable) {
    if (total <= budget - incoming && remaining + (ordinary ? 1 : 0) <= maximum)
      break;
    if (!std::filesystem::remove(item.path, ec) || ec)
      return false;
    total -= item.size;
    --remaining;
  }
  return total <= budget - incoming &&
         remaining + (ordinary ? 1 : 0) <= maximum;
}
} // namespace attendance
