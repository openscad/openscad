#include "fileutils.h"
#include "printutils.h"

#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

/*!
   Returns the absolute path to the given filename, unless it's empty.
   If the file isn't found in the given path, the fallback path will be
   used to be backwards compatible with <= 2013.01 (see issue #217).
 */
std::string lookup_file(const std::string &filename,
                        const std::string &path, const std::string &fallbackpath)
{
  std::string resultfile;
  if (!filename.empty() && !fs::path(filename).is_absolute()) {
    fs::path absfile;
    if (!path.empty()) absfile = fs::absolute(fs::path(path) / filename);
    fs::path absfile_fallback;
    if (!fallbackpath.empty()) absfile_fallback = fs::absolute(fs::path(fallbackpath) / filename);

    if (!fs::exists(absfile) && fs::exists(absfile_fallback)) {
      resultfile = absfile_fallback.string();
      PRINT_DEPRECATION("Imported file (%s) found in document root instead of relative to the importing module. This behavior is deprecated", filename);
    }
    else {
      resultfile = absfile.string();
    }
  }
  else {
    resultfile = filename;
  }
  return resultfile;
}
