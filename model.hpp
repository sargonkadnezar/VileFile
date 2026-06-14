#ifndef VILEFILE_MODEL_H
#define VILEFILE_MODEL_H

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct FileData {
  std::string name;
  std::string fullPath;
  bool isDirectory;
  uintmax_t size;
  std::string dateModified;
  std::string permissions;
};

std::string permsToString(std::filesystem::perms p);
std::string getTimeString(std::filesystem::file_time_type ftime);

class FileSystemModel {
public:
  static std::string getRootPath() { return "/"; };
  std::vector<FileData> getDirectoryContents(const std::string &path);
  std::vector<std::string> getSubDirectories(const std::string &path);
};

#endif