#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

extern const char *make_command;
void handle_dep(const fs::path& filename);
bool write_deps(const fs::path& filename, const std::vector<fs::path>& output_files);
