#ifndef SVG_H_
#define SVG_H_

#include "cgal.h"
#include <boost/algorithm/string.hpp>
#include <map>

namespace OpenSCAD {

// currently for debugging, not necessarily pretty or useful for users. (yet)

static int svg_cursor_py = 0;
static int svg_px_width = 480;
static int svg_px_height = 480;

std::string svg_header();
std::string svg_label( std::string s );
std::string svg_border();
std::string svg_axes();
std::string dump_svg( const CGAL_Nef_polyhedron2 &N );
std::string dump_svg( const CGAL_Nef_polyhedron3 &N );

} // namespace

#endif

