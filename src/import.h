#pragma once

#include <string>
#include "AST.h"

class PolySet *import_stl(const std::string &filename, const Location &loc);
PolySet *import_off(const std::string &filename, const Location &loc);
class Polygon2d *import_svg(const std::string &filename, const bool center, const Location &loc);
#ifdef ENABLE_CGAL
class CGAL_Nef_polyhedron *import_nef3(const std::string &filename, const Location &loc);
#endif
