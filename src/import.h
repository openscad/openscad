#pragma once

#include <string>

class PolySet *import_stl(const std::string &filename);
PolySet *import_off(const std::string &filename);
class Polygon2d *import_svg(const std::string &filename);
