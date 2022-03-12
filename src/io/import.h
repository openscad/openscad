#pragma once

#include <string>
#include "AST.h"

class PolySet *import_stl(const std::string& filename, const Location& loc);
PolySet *import_off(const std::string& filename, const Location& loc);
class Polygon2d *import_svg(double fn, double fs, double fa, const std::string& filename, const double dpi, const bool center, const Location& loc);
#ifdef ENABLE_CGAL
class CGAL_Nef_polyhedron *import_nef3(const std::string& filename, const Location& loc);
#endif
class Value import_json(const std::string& filename, class EvaluationSession *session, const Location& loc);
