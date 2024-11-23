#pragma once

#include <string>
#include <filesystem>

class PartCAD {
    public:
        static std::string getPart(const std::string& partSpec, const std::filesystem::path& configPath);
};