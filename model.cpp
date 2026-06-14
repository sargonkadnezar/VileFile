#include "model.hpp"
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

// this is really horrible, why is time like that
static std::string getTimeString(fs::file_time_type ftime) {
  using namespace std::chrono;
  auto sysTime = time_point_cast<system_clock::duration>(
      ftime - fs::file_time_type::clock::now() + system_clock::now());
  std::time_t tt = system_clock::to_time_t(sysTime);
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M");
  return oss.str();
}
// supercooler iterator
std::vector<FileData>
FileSystemModel::getDirectoryContents(const std::string &path) {
  std::vector<FileData> result;
  try {
    for (const auto &entry : fs::directory_iterator(
             path, fs::directory_options::skip_permission_denied)) {
      std::error_code ec;
      bool isDir = entry.is_directory(ec);
      if (ec || (!isDir && !entry.is_regular_file(ec)))
        continue;

      uintmax_t size = isDir ? 0 : entry.file_size(ec);
      if (ec)
        size = 0;

      std::string dateStr;
      if (auto ftime = entry.last_write_time(ec); !ec)
        dateStr = getTimeString(ftime);

      result.push_back({entry.path().filename().string(), entry.path().string(),
                        isDir, size, dateStr});
    }
  } catch (const fs::filesystem_error &) {
  }
  return result;
}
// noch cooleres iterieren
std::vector<std::string>
FileSystemModel::getSubDirectories(const std::string &path) {
  std::vector<std::string> out;
  for (const auto &fd : getDirectoryContents(path))
    if (fd.isDirectory)
      out.push_back(fd.fullPath);
  return out;
}