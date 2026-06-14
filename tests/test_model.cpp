#include <catch2/catch_all.hpp>
#include "model.hpp"
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static const std::string TEMP = "/tmp/vilefile-test";
static FileSystemModel model;

struct TempDir {
  TempDir() {
    fs::remove_all(TEMP);
    fs::create_directories(TEMP);
  }
  ~TempDir() { fs::remove_all(TEMP); }
};

static void touch(const std::string &path, size_t size = 0) {
  std::ofstream f(path, std::ios::binary);
  if (size > 0) {
    f.seekp(size - 1);
    f.put('\0');
  }
}

TEST_CASE("permsToString") {
  using namespace std::filesystem;
  using p = perms;

  CHECK(permsToString(p::none) == "---------");

  CHECK(permsToString(p::owner_read | p::owner_write) == "rw-------");
  CHECK(permsToString(p::owner_read | p::owner_exec) == "r-x------");
  CHECK(permsToString(p::owner_all) == "rwx------");

  CHECK(permsToString(p::group_read | p::group_exec) == "---r-x---");
  CHECK(permsToString(p::group_all) == "---rwx---");

  CHECK(permsToString(p::others_read) == "------r--");
  CHECK(permsToString(p::others_all) == "------rwx");

  CHECK(permsToString(p::owner_all | p::group_all | p::others_all) ==
        "rwxrwxrwx");
  CHECK(permsToString(p::owner_read | p::owner_write | p::group_read |
                      p::others_read) == "rw-r--r--");
}

TEST_CASE("getTimeString") {
  auto now = fs::file_time_type::clock::now();
  auto result = getTimeString(now);
  CHECK(!result.empty());
  CHECK(result.size() == 16);
  CHECK(result[4] == '-');
  CHECK(result[7] == '-');
  CHECK(result[10] == ' ');
}

TEST_CASE("getDirectoryContents") {
  TempDir guard;

  SECTION("empty directory") {
    auto contents = model.getDirectoryContents(TEMP);
    CHECK(contents.empty());
  }

  SECTION("files only") {
    touch(TEMP + "/a.txt");
    touch(TEMP + "/b.txt");
    auto contents = model.getDirectoryContents(TEMP);
    CHECK(contents.size() == 2);
    CHECK(contents[0].isDirectory == false);
    CHECK(contents[1].isDirectory == false);
    CHECK(contents[0].name != contents[1].name);
  }

  SECTION("directories only") {
    fs::create_directory(TEMP + "/sub1");
    fs::create_directory(TEMP + "/sub2");
    auto contents = model.getDirectoryContents(TEMP);
    CHECK(contents.size() == 2);
    CHECK(contents[0].isDirectory == true);
    CHECK(contents[1].isDirectory == true);
  }

  SECTION("mixed files and dirs") {
    touch(TEMP + "/file.txt");
    fs::create_directory(TEMP + "/subdir");
    auto contents = model.getDirectoryContents(TEMP);
    CHECK(contents.size() == 2);
    int dirs = 0, files = 0;
    for (const auto &fd : contents) {
      fd.isDirectory ? ++dirs : ++files;
    }
    CHECK(dirs == 1);
    CHECK(files == 1);
  }

  SECTION("with file sizes") {
    touch(TEMP + "/bigfile.bin", 4096);
    auto contents = model.getDirectoryContents(TEMP);
    REQUIRE(contents.size() == 1);
    CHECK(contents[0].size == 4096);
  }

  SECTION("non-existent path returns empty") {
    auto contents = model.getDirectoryContents("/nonexistent/xyz");
    CHECK(contents.empty());
  }

  SECTION("entries have permissions") {
    touch(TEMP + "/permtest.txt");
    auto contents = model.getDirectoryContents(TEMP);
    REQUIRE(contents.size() == 1);
    CHECK(contents[0].permissions.size() == 9);
    CHECK(!contents[0].dateModified.empty());
  }

  SECTION("symlinks are skipped") {
    touch(TEMP + "/real.txt");
    fs::create_symlink(TEMP + "/real.txt", TEMP + "/link.txt");
    auto contents = model.getDirectoryContents(TEMP);
    int symlinks = 0;
    for (const auto &fd : contents)
      if (fd.name == "link.txt")
        ++symlinks;
    CHECK(symlinks == 0);
  }
}

TEST_CASE("getSubDirectories") {
  TempDir guard;

  SECTION("empty directory") {
    auto dirs = model.getSubDirectories(TEMP);
    CHECK(dirs.empty());
  }

  SECTION("only subdirs returned") {
    touch(TEMP + "/file.txt");
    fs::create_directory(TEMP + "/sub1");
    fs::create_directory(TEMP + "/sub2");
    auto dirs = model.getSubDirectories(TEMP);
    CHECK(dirs.size() == 2);
    for (const auto &d : dirs)
      CHECK(fs::is_directory(d));
  }

  SECTION("files only returns empty") {
    touch(TEMP + "/a.txt");
    touch(TEMP + "/b.txt");
    auto dirs = model.getSubDirectories(TEMP);
    CHECK(dirs.empty());
  }
}

TEST_CASE("FileData aggregate init") {
  FileData fd{"test.txt", "/tmp/test.txt", false, 42, "2026-01-15 12:00",
              "rw-r--r--"};
  CHECK(fd.name == "test.txt");
  CHECK(fd.fullPath == "/tmp/test.txt");
  CHECK(fd.isDirectory == false);
  CHECK(fd.size == 42);
  CHECK(fd.dateModified == "2026-01-15 12:00");
  CHECK(fd.permissions == "rw-r--r--");
}
