#include "handle_dep.h"
#include "utils/printutils.h"
#include <iostream>
#include <string>
#include <cstdlib> // for system()
#include <unordered_set>
#include <vector>
#include <boost/regex.hpp>
#include <filesystem>
namespace fs = std::filesystem;

#ifndef _WIN32 // NOT _WIN32
#include <sys/wait.h>
#endif // NOT _WIN32

std::unordered_set<std::string> dependencies;
const char *make_command = nullptr;

void handle_dep(const std::string& filename)
{
  fs::path filepath(filename);
  std::string dep = boost::regex_replace(filepath.generic_string(), boost::regex("\\ "), "\\\\ ");
  if (dependencies.find(dep) != dependencies.end()) {
    return; // included and used files are very likely to be added many times by the parser
  }
  dependencies.insert(dep);

  if (make_command && !fs::exists(filepath)) {
    // This should only happen from command-line execution.
    // If changed, add an alternate error-reporting process.
    auto cmd = STR(make_command, " '", boost::regex_replace(filename, boost::regex("'"), "'\\''"), "'");
    errno = 0;
    int res = system(cmd.c_str());

    // Could not launch system() correctly
#ifdef _WIN32
    if ((res == 0 || res == -1) && errno != 0) {
#else // NOT _WIN32
    if (res == -1 && errno != 0) {
#endif // _WIN32 / NOT _WIN32
      perror("ERROR: system(make_cmd) failed");
    }

#ifndef _WIN32 // NOT _WIN32
    // Abnormal process failure (e.g., segfault, killed, etc)
    else if (!WIFEXITED(res)) {
      std::cerr << "ERROR: " << cmd.c_str()
                << ": Process terminated abnormally!" << std::endl;
    }
#endif // NOT _WIN32

    // Error code from process.
#ifdef _WIN32
    else if (0 != res) {
#else // NOT _WIN32
    else if (0 != (res = WEXITSTATUS(res))) {
#endif // _WIN32 / NOT _WIN32
      std::cerr << "ERROR: " << cmd.c_str() << ": Exit status "
                << res << std::endl;
    }

    // Otherwise, success!
  }
}

bool write_deps(const std::string& filename, const std::vector<std::string>& output_files)
{
  FILE *fp = fopen(filename.c_str(), "wt");
  if (!fp) {
    fprintf(stderr, "Can't open dependencies file `%s' for writing!\n", filename.c_str());
    return false;
  }
  for (const auto& output_file : output_files) {
    fprintf(fp, "%s ", output_file.c_str());
  }
  fseek(fp, -1, SEEK_CUR);
  fprintf(fp, ":");

  for (const auto& str : dependencies) {
    fprintf(fp, " \\\n\t%s", str.c_str());
  }
  fprintf(fp, "\n");
  fclose(fp);
  return true;
}
