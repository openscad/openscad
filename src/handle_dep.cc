#include "handle_dep.h"
#include "printutils.h"
#include <string>
#include <sstream>
#include <stdlib.h> // for system()
#include <unordered_set>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

std::unordered_set<std::string> dependencies;
const char *make_command = nullptr;

void handle_dep(const std::string &filename)
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
        auto cmd = STR(make_command << " '" << boost::regex_replace(filename, boost::regex("'"), "'\\''") << "'");
        int res = system(cmd.c_str());
        if (res == -1) {
            perror("ERROR: system(make_cmd) failed");
        }
        else if (WIFEXITED(res)) {
            int cmd_res = WEXITSTATUS(res);
            if (cmd_res != 0) {
                std::cerr << "ERROR: " << cmd.c_str() << ": Exit status "
                    << cmd_res << std::endl;
            }
        }
        else {
            std::cerr << "ERROR: " << cmd.c_str()
                << ": Process terminated abnormally!" << std::endl;
        }
    }
}

bool write_deps(const std::string &filename, const std::string &output_file)
{
    FILE *fp = fopen(filename.c_str(), "wt");
    if (!fp) {
        fprintf(stderr, "Can't open dependencies file `%s' for writing!\n", filename.c_str());
        return false;
    }
    fprintf(fp, "%s:", output_file.c_str());

    for(const auto &str : dependencies) {
        fprintf(fp, " \\\n\t%s", str.c_str());
    }
    fprintf(fp, "\n");
    fclose(fp);
    return true;
}
