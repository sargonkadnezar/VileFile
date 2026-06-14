#ifndef VILEFILE_MODEL_H
#define VILEFILE_MODEL_H

#include <cstdint>
#include <string>
#include <vector>

struct FileData {
  std::string name;
  std::string fullPath;
  bool isDirectory;
  uintmax_t size;
  std::string dateModified;
};

class FileSystemModel {
public:
  static std::string getRootPath();
  std::vector<FileData> getDirectoryContents(const std::string &path);
  std::vector<std::string> getSubDirectories(const std::string &path);
};

#endif