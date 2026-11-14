#include "io/fileutils.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

#include "utils/printutils.h"

namespace fs = std::filesystem;

bool fs_is_unc(const fs::path& p)
{
#ifdef _WIN32
  const auto& s = p.native();
  const auto is_sep = [](auto c) { return c == '\\' || c == '/'; };
  return s.size() > 2 && is_sep(s[0]) && is_sep(s[1]) && !is_sep(s[2]);
#else
  (void)p;
  return false;
#endif
}

fs::path fs_absolute(const fs::path& p)
{
  return fs_is_unc(p) ? p : fs::absolute(p);
}

fs::path fs_canonical(const fs::path& p)
{
  return fs_is_unc(p) ? p : fs::canonical(p);
}

std::string fs_generic_string(const fs::path& p)
{
  std::string s = p.generic_string();
  if (fs_is_unc(p) && s.compare(0, 2, "//") != 0) s.insert(0, "/");
  return s;
}

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

  if (fs::path(path).is_absolute() || fs_is_unc(path)) {
    return path;
  }

  if (parent.empty()) {
    return "";
  }

  return fs_absolute(fs::path(parent) / path).string();
}

fs::path fs_uncomplete(fs::path const& p, fs::path const& base)
{
  if (p == fs::path{}) return p;
#ifndef __EMSCRIPTEN__
  // There isn't always a relative path (e.g. to another drive or a UNC path on
  // Windows), and fs::relative() can fail; return the path unchanged then.
  std::error_code ec;
  const fs::path rel = fs::relative(p, base == fs::path{} ? fs::path{"."} : base, ec);
  return ec || rel.empty() ? p : rel;
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
