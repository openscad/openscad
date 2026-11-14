#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

// MinGW's std::filesystem doesn't recognize \\server\share as a path root:
// fs::absolute() puts the current drive in front of it, generic_string() drops
// one of the leading slashes and fs::canonical() fails. These keep UNC paths
// intact (#5425); for other paths they're the same as the std functions.
bool fs_is_unc(const fs::path& p);
fs::path fs_absolute(const fs::path& p);
fs::path fs_canonical(const fs::path& p);
std::string fs_generic_string(const fs::path& p);

std::string lookup_file(const std::string& path, const std::string& parent);

fs::path fs_uncomplete(fs::path const& p, fs::path const& base);
int64_t fs_timestamp(fs::path const& path);
