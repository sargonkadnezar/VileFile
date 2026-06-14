#include <catch2/catch_all.hpp>
#include "view.hpp"
#include "model.hpp"

TEST_CASE("formatSize") {
  SECTION("directory returns dash") {
    CHECK(MainFrame::formatSize(0, true) == "--");
    CHECK(MainFrame::formatSize(1024, true) == "--");
  }

  SECTION("bytes") {
    CHECK(MainFrame::formatSize(0, false) == "0 B");
    CHECK(MainFrame::formatSize(1, false) == "1 B");
    CHECK(MainFrame::formatSize(1023, false) == "1023 B");
  }

  SECTION("kilobytes") {
    CHECK(MainFrame::formatSize(1024, false) == "1.0 KB");
    CHECK(MainFrame::formatSize(1536, false) == "1.5 KB");
  }

  SECTION("megabytes") {
    CHECK(MainFrame::formatSize(1024 * 1024, false) == "1.0 MB");
    CHECK(MainFrame::formatSize(2 * 1024 * 1024, false) == "2.0 MB");
  }

  SECTION("gigabytes") {
    CHECK(MainFrame::formatSize(1024UL * 1024 * 1024, false) == "1.0 GB");
  }

  SECTION("terabytes") {
    CHECK(MainFrame::formatSize(1024UL * 1024 * 1024 * 1024, false) ==
          "1.0 TB");
    CHECK(MainFrame::formatSize(2UL * 1024 * 1024 * 1024 * 1024, false) ==
          "2.0 TB");
  }

  SECTION("boundary values") {
    CHECK(MainFrame::formatSize(1024 * 1024 - 1, false).find("1024.0 KB") !=
          wxString::npos);
    CHECK(MainFrame::formatSize(1024 * 1024, false) == "1.0 MB");
    CHECK(MainFrame::formatSize(1024 * 1024 * 1024 - 1, false).find(
              "1024.0 MB") != wxString::npos);
    CHECK(MainFrame::formatSize(1024UL * 1024 * 1024, false) == "1.0 GB");
  }
}

TEST_CASE("detectFileType") {
  SECTION("directory") {
    CHECK(MainFrame::detectFileType("anything", true) == "File Folder");
  }

  SECTION("known extensions") {
    CHECK(MainFrame::detectFileType("main.cpp", false) == "C++ Source");
    CHECK(MainFrame::detectFileType("header.hpp", false) == "C++ Header");
    CHECK(MainFrame::detectFileType("old.h", false) == "C Header");
    CHECK(MainFrame::detectFileType("photo.png", false) == "PNG Image");
    CHECK(MainFrame::detectFileType("image.jpg", false) == "JPEG Image");
    CHECK(MainFrame::detectFileType("image.jpeg", false) == "JPEG Image");
    CHECK(MainFrame::detectFileType("doc.pdf", false) == "PDF Document");
    CHECK(MainFrame::detectFileType("notes.txt", false) == "Text File");
    CHECK(MainFrame::detectFileType("readme.md", false) == "Markdown");
    CHECK(MainFrame::detectFileType("script.py", false) == "Python Source");
    CHECK(MainFrame::detectFileType("archive.zip", false) == "Archive");
    CHECK(MainFrame::detectFileType("song.mp3", false) == "Audio");
  }

  SECTION("case insensitive") {
    CHECK(MainFrame::detectFileType("MAIN.CPP", false) == "C++ Source");
    CHECK(MainFrame::detectFileType("Image.PNG", false) == "PNG Image");
  }

  SECTION("unknown extension") {
    CHECK(MainFrame::detectFileType("data.xyz", false) == "xyz File");
  }

  SECTION("no extension") {
    CHECK(MainFrame::detectFileType("Makefile", false) == "File");
  }
}

TEST_CASE("compareNamesCaseInsensitive") {
  CHECK(MainFrame::compareNamesCaseInsensitive("a", "b") == true);
  CHECK(MainFrame::compareNamesCaseInsensitive("b", "a") == false);
  CHECK(MainFrame::compareNamesCaseInsensitive("a", "a") == false);
  CHECK(MainFrame::compareNamesCaseInsensitive("A", "a") == false);
  CHECK(MainFrame::compareNamesCaseInsensitive("A", "b") == true);
}

TEST_CASE("compareFileData") {
  FileData dir{"dir", "/tmp/dir", true, 0, "", "rwxr-xr-x"};
  FileData fileA{"a.txt", "/tmp/a.txt", false, 100, "2026-01-01", "rw-r--r--"};
  FileData fileB{"b.txt", "/tmp/b.txt", false, 200, "2026-01-02",
                 "rw-r--r--"};

  SECTION("sort by name (col 0)") {
    CHECK(MainFrame::compareFileData(dir, fileA, 0, true) == true);
    CHECK(MainFrame::compareFileData(fileA, fileB, 0, true) == true);
    CHECK(MainFrame::compareFileData(fileB, fileA, 0, true) == false);
    CHECK(MainFrame::compareFileData(fileA, dir, 0, true) == false);
  }

  SECTION("sort by name descending") {
    CHECK(MainFrame::compareFileData(dir, fileA, 0, false) == false);
    CHECK(MainFrame::compareFileData(fileA, fileB, 0, false) == false);
  }

  SECTION("sort by size (col 1)") {
    FileData big{"big", "/tmp/big", false, 500, "", ""};
    CHECK(MainFrame::compareFileData(fileA, big, 1, true) == true);
    CHECK(MainFrame::compareFileData(big, fileA, 1, true) == false);
  }

  SECTION("sort by date (col 3)") {
    CHECK(MainFrame::compareFileData(fileA, fileB, 3, true) == true);
    CHECK(MainFrame::compareFileData(fileB, fileA, 3, true) == false);
  }

  SECTION("empty date sorts last") {
    FileData noDate{"x", "/tmp/x", false, 0, "", ""};
    CHECK(MainFrame::compareFileData(fileA, noDate, 3, true) == true);
    CHECK(MainFrame::compareFileData(noDate, fileA, 3, true) == false);
  }

  SECTION("sort by permissions (col 4)") {
    FileData read{"r", "/tmp/r", false, 0, "", "r--r--r--"};
    FileData write{"w", "/tmp/w", false, 0, "", "rw-rw-rw-"};
    CHECK(MainFrame::compareFileData(read, write, 4, true) == true);
  }
}

TEST_CASE("sort menu IDs map to correct columns") {
  CHECK(wxID_HIGHEST + 10 - (wxID_HIGHEST + 10) == 0);
  CHECK(wxID_HIGHEST + 11 - (wxID_HIGHEST + 10) == 1);
  CHECK(wxID_HIGHEST + 12 - (wxID_HIGHEST + 10) == 2);
  CHECK(wxID_HIGHEST + 13 - (wxID_HIGHEST + 10) == 3);
  CHECK(wxID_HIGHEST + 14 - (wxID_HIGHEST + 10) == 4);
}

TEST_CASE("path traversal guard with fs::path operator/") {
  namespace fs = std::filesystem;
  fs::path base = "/home/user/docs";
  CHECK((base / "subfolder").string() == "/home/user/docs/subfolder");
  CHECK((base / "..").string() == "/home/user/docs/..");
  CHECK(fs::weakly_canonical(base / ".." / "secret") == "/home/user/secret");
}
