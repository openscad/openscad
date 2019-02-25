#pragma once

#include <string>

std::string lookup_file(const std::string &filename,
                        const std::string &path, const std::string &fallbackpath);
