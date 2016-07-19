#pragma once

#include <string>

class PolySet *import_stl(const std::string &filename);
PolySet *import_off(const std::string &filename);
const class Polygon2d &import_dxf(const std::string &filename);
