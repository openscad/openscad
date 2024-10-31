#include "io/fileutils.h"
#include "utils/printutils.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

/*!
   Returns the absolute path to the given filename, unless it's empty.
   If the file isn't found in the given path, the fallback path will be
   used to be backwards compatible with <= 2013.01 (see issue #217).
 */
std::string lookup_file(const std::string& filename,
                        const std::string& path, const std::string& fallbackpath)
{
  std::string resultfile;
  if (!filename.empty() && !fs::path(filename).is_absolute()) {
    fs::path absfile;
    if (!path.empty()) absfile = fs::absolute(fs::path(path) / filename);
    fs::path absfile_fallback;
    if (!fallbackpath.empty()) absfile_fallback = fs::absolute(fs::path(fallbackpath) / filename);

    if (!fs::exists(absfile) && fs::exists(absfile_fallback)) {
      resultfile = absfile_fallback.string();
      LOG(message_group::Deprecated, "Imported file (%1$s) found in document root instead of relative to the importing module. This behavior is deprecated", std::string(filename));
    } else {
      resultfile = absfile.string();
    }
  } else {
    resultfile = filename;
  }
  return resultfile;
}

fs::path fs_uncomplete(fs::path const& p, fs::path const& base)
{
  if (p == fs::path{}) return p;
  return fs::relative(p, base == fs::path{} ? fs::path{"."} : base);
}

int64_t fs_timestamp(fs::path const& path) {
  int64_t seconds = 0;
  if (fs::exists(path)) {
    const auto t = fs::last_write_time(path);
    const auto duration = t.time_since_epoch();
    seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
  }
  return seconds;
}