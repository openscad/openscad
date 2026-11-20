#include "io/fileutils.h"

#include <catch2/catch_all.hpp>
#include <filesystem>
#include <string>

TEST_CASE("fs_uncomplete makes a path relative to the base", "[io][fileutils]")
{
  const fs::path base = fs::temp_directory_path();
  CHECK(fs_uncomplete(base / "dir" / "file.scad", base) == fs::path("dir") / "file.scad");
}

#ifndef _WIN32
TEST_CASE("fs_uncomplete returns the path unchanged when fs::relative() fails", "[io][fileutils]")
{
  // A path component longer than NAME_MAX makes fs::relative() fail (ENAMETOOLONG).
  const fs::path p = fs::temp_directory_path() / std::string(300, 'a') / "file.scad";
  CHECK(fs_uncomplete(p, fs::temp_directory_path()) == p);
}
#endif
