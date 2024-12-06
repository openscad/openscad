#include "core/parsersettings.h"

#include <algorithm>
#include <iterator>
#include <cassert>
#include <string>
#include <vector>

#include <filesystem>
#include <boost/algorithm/string.hpp>
#include "platform/PlatformUtils.h"

namespace fs = std::filesystem;

std::vector<std::string> librarypath;

static void add_librarydir(const std::string& libdir)
{
  librarypath.push_back(libdir);
}

const std::vector<std::string>& get_library_path()
{
  return librarypath;
}

/*!
   Searces for the given file in library paths and returns the full path if found.
   Returns an empty path if file cannot be found or filename is a directory.
 */
fs::path search_libs(const fs::path& localpath)
{
  for (const auto& dir : librarypath) {
    fs::path usepath = fs::path(dir) / localpath;
    if (fs::exists(usepath) && !fs::is_directory(usepath)) {
      return usepath.string();
    }
  }
  return {};
}

// files must be 'ordinary' - they must exist and be non-directories
// FIXME: We cannot print any output here since these function is called periodically
// from "Automatic reload and compile"
static bool check_valid(const fs::path& p, const std::vector<std::string> *openfilenames)
{
  if (p.empty()) {
    // LOG(message_group::Warning,,"File path is blank: %1$s",p);
    return false;
  }
  if (!p.has_parent_path()) {
    // LOG(message_group::Warning,,"No parent path: %1$s",p);
    return false;
  }
  if (!fs::exists(p)) {
    // LOG(message_group::Warning,,"File not found: %1$s",p);
    return false;
  }
  if (fs::is_directory(p)) {
    // LOG(message_group::Warning,,"%1$s invalid - points to a directory",p);
    return false;
  }
  const std::string& fullname = p.generic_string();
  // Detect circular includes
  if (openfilenames) {
    for (const auto& s : *openfilenames) {
      if (s == fullname) {
        // LOG(message_group::Warning,,"circular include file %1$s",fullname);
        return false;
      }
    }
  }
  return true;
}

/*!
   Check if the given filename is valid.

   If the given filename is absolute, do a simple check.
   If not, search the applicable paths for a valid file.

   Returns the absolute path to a valid file, or an empty path if no
   valid files could be found.
 */
inline fs::path find_valid_path_(const fs::path& sourcepath,
                          const fs::path& localpath,
                          const std::vector<std::string> *openfilenames)
{
  if (localpath.is_absolute()) {
    if (check_valid(localpath, openfilenames)) return fs::canonical(localpath);
  } else {
    fs::path fpath = sourcepath / localpath;
    if (fs::exists(fpath)) fpath = fs::canonical(fpath);
    if (check_valid(fpath, openfilenames)) return fpath;
    fpath = search_libs(localpath);
    if (!fpath.empty() && check_valid(fpath, openfilenames)) return fpath;
  }
  return {};
}

fs::path find_valid_path(const fs::path& sourcepath,
                         const fs::path& localpath,
                         const std::vector<std::string> *openfilenames)
{
  return {find_valid_path_(sourcepath, localpath, openfilenames).generic_string()};
}


static bool path_contains_file(fs::path dir, fs::path file)
{
  // from https://stackoverflow.com/a/15549954/1080604
  // If dir ends with "/" and isn't the root directory, then the final
  // component returned by iterators will include "." and will interfere
  // with the std::equal check below, so we strip it before proceeding.
  if (dir.filename() == ".") dir.remove_filename();
  // We're also not interested in the file's name.
  assert(file.has_filename());
  file.remove_filename();

  // If dir has more components than file, then file can't possibly
  // reside in dir.
  auto dir_len = std::distance(dir.begin(), dir.end());
  auto file_len = std::distance(file.begin(), file.end());
  if (dir_len > file_len) return false;

  // This stops checking when it reaches dir.end(), so it's OK if file
  // has more directory components afterward. They won't be checked.
  return std::equal(dir.begin(), dir.end(), file.begin());
}

fs::path get_library_for_path(const fs::path& localpath)
{
  for (const auto& libpath : librarypath) {
    if (path_contains_file(fs::path(libpath), localpath)) {
      return libpath;
    }
  }
  return {};
}


void parser_init()
{
  // Add paths from OPENSCADPATH before adding built-in paths
  const char *openscadpaths = getenv("OPENSCADPATH");
  if (openscadpaths) {
    std::string paths(openscadpaths);
    std::string sep = PlatformUtils::pathSeparatorChar();
    using string_split_iterator = boost::split_iterator<std::string::iterator>;
    for (string_split_iterator it = boost::make_split_iterator(paths, boost::first_finder(sep, boost::is_iequal())); it != string_split_iterator(); ++it) {
      auto str{boost::copy_range<std::string>(*it)};
      fs::path abspath = str.empty() ? fs::current_path() : fs::absolute(fs::path(str));
      add_librarydir(abspath.generic_string());
    }
  }

  add_librarydir(PlatformUtils::userLibraryPath());

  fs::path libpath = PlatformUtils::resourcePath("libraries");
  // std::filesystem::absolute() will throw if passed empty path
  if (libpath.empty()) {
    libpath = fs::current_path();
  }
  add_librarydir(fs::absolute(libpath).string());
}
