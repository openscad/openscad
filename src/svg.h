#ifndef SVG_H_
#define SVG_H_

#include "cgal.h"
#include <boost/algorithm/string.hpp>
#include <map>

namespace OpenSCAD {

static int svg_cursor = 0;

std::string svg_header( int pixwidth = 480, int pixheight = 480 );
std::string svg_label(std::string s);
std::string svg_border();
std::string svg_axes();
CGAL_Point_2e project_svg_3to2( CGAL_Point_3 p, CGAL_Iso_cuboid_3 bbox );
CGAL_Point_2e project_svg_2to2( CGAL_Point_2e p, CGAL_Iso_rectangle_2e bbox );

std::string dump_cgal_nef_polyhedron2_face_svg(
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c1,
	CGAL_Nef_polyhedron2::Explorer::Halfedge_around_face_const_circulator c2,
	CGAL_Nef_polyhedron2::Explorer explorer,
	std::string color,
	bool mark,
	CGAL_Iso_rectangle_2e bbox );
std::string dump_svg( const CGAL_Nef_polyhedron2 &N );
class NefPoly3_dumper_svg;
std::string dump_svg( const CGAL_Nef_polyhedron3 &N );


} // namespace

#endif

