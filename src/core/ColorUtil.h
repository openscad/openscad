#pragma once

#include <string>
#include <optional>

#include "geometry/linalg.h"

namespace OpenSCAD {

inline Color4f CORNFIELD_FACE_COLOR{0xf9, 0xd7, 0x2c, 0xff};

std::optional<Color4f> parse_color(const std::string& col);

Color4f getColor(const std::string& col, const Color4f& defaultcolor);

Color4f getContrastColor(const Color4f& col);

Vector4f getColorHSV(const Color4f& col);

}  // namespace OpenSCAD
