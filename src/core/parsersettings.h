#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

extern int parser_error_pos;

/**
 * Initialize library path.
 */
void parser_init();
void add_librarydir(const fs::path& libdir);

fs::path search_libs(const fs::path& localpath);
fs::path find_valid_path(const fs::path& sourcepath, const fs::path& localpath,
                         const std::vector<fs::path> *openfilenames = nullptr);
fs::path get_library_for_path(const fs::path& localpath);

const std::vector<fs::path>& get_library_path();
