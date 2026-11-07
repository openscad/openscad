#include "core/parsersettings.h"

#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <system_error>

#include "utils/scope_guard.hpp"

#ifndef _WIN32

// A file that can't be inspected because a parent directory has no permissions must be
// reported as not found (empty path), not throw std::filesystem::filesystem_error.
TEST_CASE("find_valid_path treats inaccessible files as not found", "[core][parsersettings]")
{
  const fs::path dir =
    fs::temp_directory_path() / ("openscad-find-valid-path-" + std::to_string(std::random_device{}()));
  const fs::path locked = dir / "locked";
  fs::create_directories(locked);
  std::ofstream(locked / "x.scad") << "cube(1);\n";
  std::ofstream(dir / "ok.scad") << "cube(1);\n";
  fs::permissions(locked, fs::perms::none);

  auto cleanup = sg::make_scope_guard([&]() {
    std::error_code ec;
    fs::permissions(locked, fs::perms::owner_all, ec);
    fs::remove_all(dir, ec);
  });

  // Permissions aren't enforced e.g. when running as root.
  std::error_code ec;
  (void)fs::exists(locked / "x.scad", ec);
  if (!ec) SKIP("directory permissions are not enforced for this user");

  SECTION("an accessible file is still found")
  {
    CHECK(find_valid_path(dir, "ok.scad") == fs::canonical(dir / "ok.scad").generic_string());
  }

  SECTION("relative path")
  {
    fs::path result;
    CHECK_NOTHROW(result = find_valid_path(dir, "locked/x.scad"));
    CHECK(result.empty());
  }

  SECTION("absolute path")
  {
    fs::path result;
    CHECK_NOTHROW(result = find_valid_path(dir, locked / "x.scad"));
    CHECK(result.empty());
  }
}

#endif  // _WIN32
