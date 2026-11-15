#include "handle_dep.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
#ifndef _WIN32
#include <sys/wait.h>
#endif

#include <boost/regex.hpp>

#include "platform/PlatformUtils.h"
#include "utils/printutils.h"

namespace fs = std::filesystem;

const char *make_command = nullptr;

namespace {

std::unordered_set<fs::path> dependencies;

}  // namespace

void handle_dep(const fs::path& filepath)
{
  if (dependencies.find(filepath) != dependencies.end()) {
    return;  // included and used files are very likely to be added many times by the parser
  }
  dependencies.insert(filepath);

  if (make_command && !fs::exists(filepath)) {
    // This should only happen from command-line execution.
    // If changed, add an alternate error-reporting process.
    auto cmd = STR(make_command, " '",
                   boost::regex_replace(filepath.u8string(), boost::regex("'"), "'\\''"), "'");
    errno = 0;
    int res = system(cmd.c_str());

    // Could not launch system() correctly
#ifdef _WIN32
    if ((res == 0 || res == -1) && errno != 0) {
#else   // NOT _WIN32
    if (res == -1 && errno != 0) {
#endif  // _WIN32 / NOT _WIN32
      perror("ERROR: system(make_cmd) failed");
    }
#ifndef _WIN32  // NOT _WIN32
    // Abnormal process failure (e.g., segfault, killed, etc)
    else if (!WIFEXITED(res)) {
      std::cerr << "ERROR: " << cmd.c_str() << ": Process terminated abnormally!" << std::endl;
    }
#endif  // NOT _WIN32

    // Error code from process.
#ifdef _WIN32
    else if (0 != res) {
#else   // NOT _WIN32
    else if (0 != (res = WEXITSTATUS(res))) {
#endif  // _WIN32 / NOT _WIN32
      std::cerr << "ERROR: " << cmd.c_str() << ": Exit status " << res << std::endl;
    }

    // Otherwise, success!
  }
}

bool write_deps(const fs::path& filename, const std::vector<fs::path>& output_files)
{
  FILE *fp = PlatformUtils::fopen(filename, "wt");
  if (!fp) {
    fprintf(stderr, "Can't open dependencies file %s for writing!\n", quotePath(filename).c_str());
    return false;
  }
  for (const auto& output_file : output_files) {
    fprintf(fp, "%s ", output_file.u8string().c_str());
  }
  fseek(fp, -1, SEEK_CUR);
  fprintf(fp, ":");

  for (const auto& path : dependencies) {
    // NEEDSWORK maybe need quoting here.
    fprintf(fp, " \\\n\t%s", path.u8string().c_str());
  }
  fprintf(fp, "\n");
  fclose(fp);
  return true;
}
