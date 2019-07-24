#pragma once

#include <string>
#include "AST.h"

class PolySet *import_stl(const std::string &filename, const Location &loc);
class PolySet *import_off(const std::string &filename, const Location &loc);
class Polygon2d *import_svg(const std::string &filename, const double dpi, const bool center, const Location &loc);
class Polygon2d *import_dxf(double fn, double fs, double fa, const std::string &filename, const std::string &layername, 
						    double xorigin, double yorigin, double scale);
#ifdef ENABLE_CGAL
class CGAL_Nef_polyhedron *import_nef3(const std::string &filename, const Location &loc);
#endif
