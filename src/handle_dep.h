#pragma once

#include <string>
#include <vector>

extern const char *make_command;
void handle_dep(const std::string& filename);
bool write_deps(const std::string& filename, const std::vector<std::string>& output_files);
