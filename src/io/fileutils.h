#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

std::string lookup_file(const std::string& filename,
                        const std::string& path, const std::string& fallbackpath);


fs::path fs_uncomplete(fs::path const& p, fs::path const& base);
int64_t fs_timestamp(fs::path const& path);