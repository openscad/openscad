#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

extern int parser_error_pos;

/**
 * Initialize library path.
 */
void parser_init();

fs::path search_libs(const fs::path& localpath);
fs::path find_valid_path(const fs::path& sourcepath,
                         const fs::path& localpath,
                         const std::vector<std::string> *openfilenames = nullptr);
fs::path get_library_for_path(const fs::path& localpath);

const std::vector<std::string>& get_library_path();