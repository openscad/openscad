#pragma once

#include <string>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

extern int parser_error_pos;

/**
 * Initialize library path.
 */
void parser_init();

fs::path search_libs(const fs::path &localpath);
fs::path find_valid_path(const fs::path &sourcepath,
                         const fs::path &localpath,
                         const std::vector<std::string> *openfilenames = nullptr);
