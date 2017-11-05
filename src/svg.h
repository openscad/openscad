#pragma once

#include "cgal.h"
#include <boost/algorithm/string.hpp>
#include <map>

namespace OpenSCAD {

// currently for debugging, not necessarily pretty or useful for users. (yet)

#define SVG_PXW 480
#define SVG_PXH 480
extern int svg_cursor_py;
extern int svg_px_width;
extern int svg_px_height;

std::string svg_header(int widthpx = SVG_PXW, int heightpx = SVG_PXH);
std::string svg_label(std::string s);
std::string svg_border();
std::string svg_axes();
std::string dump_svg(const CGAL_Nef_polyhedron2 &N);
std::string dump_svg(const CGAL_Nef_polyhedron3 &N);

} // namespace
