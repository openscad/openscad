#ifndef CGALUTILS_H_
#define CGALUTILS_H_

#include <cgalfwd.h>

class PolySet *createPolySetFromPolyhedron(const CGAL_Polyhedron &p);
CGAL_Polyhedron *createPolyhedronFromPolySet(const class PolySet &ps);
std::string dump_cgal_nef_polyhedron2( const CGAL_Nef_polyhedron2 &N );
std::string dump_cgal_nef_polyhedron3( const CGAL_Nef_polyhedron3 &N );
std::string dump_cgal_nef_polyhedron2_svg( const CGAL_Nef_polyhedron2 &N );
std::string dump_cgal_nef_polyhedron3_svg( const CGAL_Nef_polyhedron3 &N );
static int svg_counter = 0;

#endif
