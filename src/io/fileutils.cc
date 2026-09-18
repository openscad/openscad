#include "io/fileutils.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

#include "utils/printutils.h"

namespace fs = std::filesystem;

/*!
   Canonicalizes the specified file path.  Returns empty on failure.
   - If the path is empty, return empty.
   - If the path is absolute, return it.
   - If the path is relative:
     - If the parent path is empty, return empty.
     - Else, return the path as an absolute path based on the parent path.
 */
std::string lookup_file(const std::string& path, const std::string& parent)
{
  std::string resultfile;

  if (path.empty()) {
    return "";
  }

  if (fs::path(path).is_absolute()) {
    return path;
  }

  if (parent.empty()) {
    return "";
  }

  return fs::absolute(fs::path(parent) / path).string();
}

fs::path fs_uncomplete(fs::path const& p, fs::path const& base)
{
  if (p == fs::path{}) return p;
#ifndef __EMSCRIPTEN__
  return fs::relative(p, base == fs::path{} ? fs::path{"."} : base);
#else
  return p;
#endif
}

int64_t fs_timestamp(fs::path const& path)
{
  int64_t seconds = 0;
  if (fs::exists(path)) {
    const auto t = fs::last_write_time(path);
    const auto duration = t.time_since_epoch();
    seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
  }
  return seconds;
}
