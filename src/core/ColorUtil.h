#pragma once

#include <string>
#include <optional>
#include <unordered_map>

#include "geometry/linalg.h"

template<> struct std::hash<Color4f> {
    std::size_t operator()(Color4f const& c) const noexcept {
      std::size_t hash = 0;
      for (int idx = 0;idx < 4;idx++) {
        std::size_t h = std::hash<float>{}(c[idx]);
        hash = h ^ (hash << 1);
      }
      return hash;
    }
};

namespace OpenSCAD {

inline Color4f CORNFIELD_FACE_COLOR{ 0xf9, 0xd7, 0x2c, 255 };

void rgbtohsv(float r, float g, float b, float& h, float& s, float& v);

std::optional<Color4f> parse_hex_color(const std::string& hex);

std::optional<Color4f> parse_web_color(const std::string& col);

std::optional<Color4f> parse_color(const std::string& col);

Color4f getColor(const std::string& col, const Color4f& defaultcolor = Color4f(0.0f, 0.0f, 0.0f));

Color4f getContrastColor(const Color4f& col);

Color4f getColorHSV(const Color4f& col);

extern std::unordered_map<std::string, Color4f> webcolors;

} // namespace
