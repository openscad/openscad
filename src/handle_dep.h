#pragma once

#include <string>

extern const char *make_command;
void handle_dep(const std::string &filename);
bool write_deps(const std::string &filename, const std::string &output_file);
