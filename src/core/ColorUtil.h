#pragma once

#include <string>
#include <optional>

#include "geometry/linalg.h"

namespace OpenSCAD {

using ColorMap = std::unordered_map<std::string, Color4f>;
using ColorMaps = std::unordered_map<std::string, ColorMap>;

constexpr char COLOR_MAP_NAME_WEB_COLORS[] = "Web Colors";
constexpr char COLOR_MAP_NAME_XKCD_COLORS[] = "XKCD Colors";

inline Color4f CORNFIELD_FACE_COLOR{0xf9, 0xd7, 0x2c, 0xff};

std::optional<Color4f> parse_color(const std::string& col);

Color4f getColor(const std::string& col, const Color4f& defaultcolor);

Color4f getContrastColor(const Color4f& col);

Vector4f getColorHSV(const Color4f& col);

ColorMaps getColorMaps();

}  // namespace OpenSCAD
